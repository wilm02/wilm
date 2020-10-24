// Quickhack
// =========
// Absolut unvollständige Simulation der Smart-Home-Funktionalität einer Fritzbox.
// Nur die Komponenten, die wilm benötigt, werden simuliert.
//

#ifdef __unix__
  #include <stdio.h>		// for fflush
  #include <unistd.h>		// for
  #include <stdlib.h>		// for exit
  #include <errno.h>		// for errno
  #include <stdarg.h>		// for va_list
  #include <string.h>		// for strcpy
  #include <arpa/inet.h>	// for SOCKET
  #include <netdb.h>        // for gethostbyname
  #include <ifaddrs.h>      // for getifaddrs

  #define SOCKET          int
  #define SOCKADDR_IN     struct sockaddr_in
  #define SOCKADDR        struct sockaddr
  #define closesocket     close

  #define WSADATA         int
  #define WSAGetLastError(x) /*-13*/errno
  #define MAKEWORD(x,y)   0
  #define WSAStartup(x,y) 0
  #define WSACleanup()
  #define msleep(x)       usleep((x)*1000)
#else
  #include <unistd.h>	// for uint32_t
  #include <stdio.h>
  #define _WIN32_WINNT 0x0501 	// for getaddrinfo
  #include <WinSock2.h>	// for getaddrinfo
  #include <ws2tcpip.h>
  #include <windows.h>            // Sleep
  #define msleep(x)    Sleep(x)

#endif


#define BUFFERSIZE 1024

#define swap16(x) ((((x)&0xff00)>>8) | ((((x)&0xff)<<8)))
#define swap32(x) ((((x)&0xff000000)>>24) | (((x)&0xff00)<<8) | (((x)&0xff0000)>>8) | (((x)&0xff)<<24))

void ErrorExit(const char *fmt,...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);    // vprintf gibt auf stdout aus, vsprintf in string
    va_end(ap);
    exit(-1);
}


unsigned int myIP()
{
#ifdef __unix__
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
	unsigned int addr;

    getifaddrs (&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (!(ifa->ifa_addr)) continue;
		if (ifa->ifa_addr->sa_family!=AF_INET) continue;
		sa = (struct sockaddr_in *) ifa->ifa_addr;
		if(!strncmp(inet_ntoa(sa->sin_addr),"127.0.",6)) continue;		// Loopback-Adapter ignorieren
		addr=sa->sin_addr.s_addr;
		//printf("Interface: %s %s\n", ifa->ifa_name, inet_ntoa(sa->sin_addr));    //...inet_ntoa((*sa).sin_addr));
    }
    freeifaddrs(ifap);
	return addr;
#else
    char hostname[80];
    struct hostent *hostinfo;
    unsigned long addr;
    if (gethostname(hostname, sizeof(hostname)) != 0) ErrorExit("hostname");
    //printf("hostname:%s\n", hostname);
    hostinfo = gethostbyname(hostname);
    if (hostinfo == 0) ErrorExit("gethostbyname");
    //printf("hostinfo->h_name:%s\n", hostinfo->h_name);
    for (int i=0; hostinfo->h_addr_list[i] != 0; i++) {
        if(!strncmp(inet_ntoa(*(struct in_addr*)hostinfo->h_addr_list[i]),"169.254.",8)) continue;		// nicht verbundene Adapter ignorieren
        if(!strncmp(inet_ntoa(*(struct in_addr*)hostinfo->h_addr_list[i]),"192.168.56.",11)) continue;  // Adapter von Virtual Box ignorieren
        addr=*(unsigned long *)hostinfo->h_addr_list[i];
        //printf("Adapter #%d: %s\n", i, inet_ntoa(*(struct in_addr *)&addr));
    }
    return addr;
#endif
}

char sBuf[BUFFERSIZE];

char* func_udp()
{
    int rc;
	SOCKET sock;
    SOCKADDR_IN addr_u, reAddr;
    int reAddrLen=sizeof(SOCKADDR_IN);

    // UDP Socket erstellen
    if((sock=socket(AF_INET,SOCK_DGRAM,0))<0)
        ErrorExit("Fehler: Der Socket konnte nicht erstellt werden, fehler code: %d\n",WSAGetLastError());

    // addr_u vorbereiten
    addr_u.sin_family=AF_INET;
    addr_u.sin_port=htons(1900);
    addr_u.sin_addr.s_addr=inet_addr("239.255.255.250");

    // richtige myIP eintragen, damit richtiger Adapter gewählt wird
    struct ip_mreq mreq;
    mreq.imr_interface.s_addr = myIP();
//    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char*)&mreq.imr_interface, sizeof(struct in_addr)) < 0) ErrorExit("setsockopt");
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char*)&mreq.imr_interface, sizeof(struct in_addr));  // if() läuft unter windows nicht

// SSDP senden ////////////////
    //rcpy(sBuf, "M-Search * HTTP/1.1\r\n...skip...\r\nST: urn:schemas-upnp-org:device:avm-aha:1\r\n\r\n");
    strcpy(sBuf, "M-Search * HTTP/1.1\r\n...skip...\r\nST: urn:schemas-upnp-org:device:sim-aha:1\r\n\r\n");
    if((rc=sendto (sock,sBuf,strlen(sBuf),0,(SOCKADDR*)&addr_u,sizeof(SOCKADDR_IN)))<0)
        ErrorExit("Fehler: sendto, fehler code: %d\n",WSAGetLastError());
//    else printf("### M-Search mit %d Bytes gesendet\n", rc);

    rc=recvfrom(sock,sBuf,BUFFERSIZE,0,(SOCKADDR*)&reAddr,(socklen_t*)&reAddrLen);
    char *ptr;
    if(rc<0) ErrorExit("Fehler: recvfrom, fehler code: %d\n",WSAGetLastError());
    else {
//        printf("### %d Bytes empfangen!\n", rc);
        sBuf[rc]='\0';
//        printf("%s",sBuf);
        if (ptr=strstr(sBuf, "\r\nLOCATION: http://")){
            int i;
            for (i=19; i<36; i++)
                if(ptr[i]==':')
                    ptr[i]=0;
            //printf("### Extrahierte IP: %s\n", (ptr+19));
        } else ErrorExit("Fehler: LOC\n");
    }
	return ptr+19;
}

void func_xml(char *sIPaddr)
{
    int rc;
	SOCKET s;
    SOCKADDR_IN addr;
    char buf[4096];

    int iPort=49000;

    // Socket erstellen
    s=socket(AF_INET,SOCK_STREAM,0);
    if(s<0) ErrorExit("Fehler: Der Socket konnte nicht erstellt werden, fehler code: %d\n",WSAGetLastError());
    //else printf("### Socket erstellt!\n");

    // Verbinden
    memset(&addr,0,sizeof(SOCKADDR_IN)); // zuerst alles auf 0 setzten
    addr.sin_family=AF_INET;
    addr.sin_port=htons(iPort);
    addr.sin_addr.s_addr=inet_addr(sIPaddr);

msleep(500);
    rc=connect(s,(SOCKADDR*)&addr,sizeof(SOCKADDR));
    if(rc<0) ErrorExit("Fehler: connect gescheitert, fehler code: %d\n",WSAGetLastError());
    //else printf("### Verbunden mit %s an %d ..\n", sIPaddr,iPort);

    // 1. xml-Daten austauschen
    //printf("### send GET 1\n");
    strcpy(buf, "GET /aha.xml HTTP/1.1\r\n...skip...\r\n");
    send(s,buf,strlen(buf),0);

    rc=recv(s,buf,4096,0);
    if(rc==0) ErrorExit("Server hat die Verbindung getrennt1..\n");
    if(rc<0)  ErrorExit("Fehler: recv, fehler code: %d\n",WSAGetLastError());
    buf[rc]='\0';
    //printf("### Server antwortet mit Header:\n");

    rc=recv(s,buf,4096,0);
    if(rc==0) ErrorExit("Server hat die Verbindung getrennt2..\n");
    if(rc<0)  ErrorExit("Fehler: recv, fehler code: %d\n",WSAGetLastError());
    buf[rc]='\0';
    //printf("### Server antwortet mit Daten:\n");

    closesocket(s);

    // Socket erstellen
    s=socket(AF_INET,SOCK_STREAM,0);
    if(s<0) ErrorExit("Fehler: Der Socket konnte nicht erstellt werden, fehler code: %d\n",WSAGetLastError());
    //else printf("### Socket erstellt!\n");

    // Verbinden
    memset(&addr,0,sizeof(SOCKADDR_IN)); // zuerst alles auf 0 setzten
    addr.sin_family=AF_INET;
    addr.sin_port=htons(iPort);
    addr.sin_addr.s_addr=inet_addr(sIPaddr);

    rc=connect(s,(SOCKADDR*)&addr,sizeof(SOCKADDR));
    if(rc<0) ErrorExit("Fehler: connect gescheitert, fehler code: %d\n",WSAGetLastError());
    //else printf("### Verbunden mit %s an %d ..\n", sIPaddr,iPort);

    // 2. xml-Daten austauschen
    //printf("### send GET 2\n");
    strcpy(buf, "GET /avm-ahaSCPD.xml HTTP/1.1\r\n...skip...\r\n");
    send(s,buf,strlen(buf),0);

    rc=recv(s,buf,4096,0);
    if(rc==0) ErrorExit("Server hat die Verbindung getrennt3..\n");
    if(rc<0)  ErrorExit("Fehler: recv, fehler code: %d\n",WSAGetLastError());
    buf[rc]='\0';
    //printf("### Server antwortet mit Header:\n");

    rc=recv(s,buf,4096,0);
    if(rc==0) ErrorExit("Server hat die Verbindung getrennt4..\n");
    if(rc<0)  ErrorExit("Fehler: recv, fehler code: %d\n",WSAGetLastError());
    buf[rc]='\0';
    //printf("### Server antwortet mit Daten:\n");

    closesocket(s);
	msleep(300); //0.3 sec
}

int func_conf(char *sIPaddr)
{
    int rc;
	SOCKET s;
    SOCKADDR_IN addr;
    char buf[4096];
    int iPort=2002;

    // Socket erstellen
    s=socket(AF_INET,SOCK_STREAM,0);
    if(s<0) ErrorExit("Fehler: Der Socket konnte nicht erstellt werden, fehler code: %d\n",WSAGetLastError());
    else printf("### Socket erstellt!\n");

    // Verbinden
    memset(&addr,0,sizeof(SOCKADDR_IN)); // zuerst alles auf 0 setzten
    addr.sin_family=AF_INET;
    addr.sin_port=htons(iPort);
    addr.sin_addr.s_addr=inet_addr(sIPaddr);

    rc=connect(s,(SOCKADDR*)&addr,sizeof(SOCKADDR));
    if(rc<0) ErrorExit("Fehler: connect gescheitert, fehler code: %d\n",WSAGetLastError());
    else printf("### Verbunden mit %s an %d ..\n", sIPaddr,iPort);

#define PAYLOADLEN 400
    char payload[PAYLOADLEN];
	uint8_t onoff=1;
	int paylen;
	paylen=10;
    memset(&payload,0,PAYLOADLEN);          // zuerst alles auf 0 setzten
    payload[ 0]=(char)5;                    // Token: conf
    *(uint16_t*)&payload[2]=swap16(paylen);	// Länge eintragen
    send(s,payload,paylen,0);
	
    rc=recv(s,buf,4096,0);
    if(rc==0) ErrorExit("Server hat die Verbindung getrennt..\n");
    if(rc<0)  ErrorExit("Fehler: recv, fehler code: %d\n",WSAGetLastError());
    if(rc!=328) ErrorExit("Fehler: Aktuator sendet falsche Länge\n");
	onoff=buf[179];
	printf("conf %d\n", onoff);
    closesocket(s);
	return onoff;
}

void func_value(char *sIPaddr, int onoff)
{
    int rc;
	SOCKET s;
    SOCKADDR_IN addr;
    char buf[4096];
    int iPort=2002;

    // Socket erstellen
    s=socket(AF_INET,SOCK_STREAM,0);
    if(s<0) ErrorExit("Fehler: Der Socket konnte nicht erstellt werden, fehler code: %d\n",WSAGetLastError());
    else printf("### Socket erstellt!\n");

    // Verbinden
    memset(&addr,0,sizeof(SOCKADDR_IN)); // zuerst alles auf 0 setzten
    addr.sin_family=AF_INET;
    addr.sin_port=htons(iPort);
    addr.sin_addr.s_addr=inet_addr(sIPaddr);

    rc=connect(s,(SOCKADDR*)&addr,sizeof(SOCKADDR));
    if(rc<0) ErrorExit("Fehler: connect gescheitert, fehler code: %d\n",WSAGetLastError());
    else printf("### Verbunden mit %s an %d ..\n", sIPaddr,iPort);

#define PAYLOADLEN 400
    char payload[PAYLOADLEN];
	int paylen;
	paylen=30;
    memset(&payload,0,PAYLOADLEN);          // zuerst alles auf 0 setzten
    payload[ 0]=(char)7;                    // Token: values (einzelne Werte)
    payload[19]=(char)0x0f;                 // Schalterstellung
    *(uint32_t*)&payload[24]=swap32(onoff);     // 1=An
    *(uint16_t*)&payload[2]=swap16(paylen);	// Länge eintragen
    send(s,payload,paylen,0);
    closesocket(s);
}

char gsOwnIP[20];

int main(int argc, char**argv)
{
    setbuf(stdout, NULL);                           // direkte Ausgabe ohne Pufferung
	
    char sIPaddr[256]="";			// hier die Adresse vom 8266 eintragen => bei Simulation die eigene des Linux-Systems

#if 0
	struct sockaddr_in saSample;
    *(uint32_t*)&(saSample.sin_addr) = myIP();
	strcpy(sIPaddr, inet_ntoa(saSample.sin_addr));	// Linux at home
#else
    //strcpy(sIPaddr, "192.168.100.217");	// Hardware at home
    strcpy(sIPaddr, "192.168.100.229");	// Linux at home
#endif
	
	
	int onoff=1;
//ö	FILE*fp;
//ö	fp=fopen(".fbsim.ini","r");
//ö    if (fp != NULL) {
//ö		printf("found .fbsim.ini\n");
//ö        if(fscanf(fp, "%s", sIPaddr) != 1) ErrorExit ("Error no data in ini.file");
//ö        if(fscanf(fp, "%d", &onoff) != 1) ErrorExit ("Error no data in ini.file");
//ö		fclose(fp);
//ö	} else
//ö		printf("NOT found .fbsim.ini\n");
	printf("sIPaddr:%s       onoff:%d\n", sIPaddr, onoff);

    WSADATA wsa;
    int rc;
    if ((rc = WSAStartup(MAKEWORD(2, 2), &wsa))!=0)
        ErrorExit("ERROR: startWinsock failed: %d\n", rc);

	char arg[256];

	char *pointer=arg;
//ö	if (argc==1) strcpy(arg, "u_x_c_v" /* "u_x_c_s3_v" */);
/*ö*/if (argc==1) strcpy(arg, "u_x_c_s3_v");
	else if (argc==2) strcpy(arg, argv[1]);
	else ErrorExit("ERROR: wrong argument\n");
	printf("arg: %s\n", arg);

	int nr;
	while (strlen(arg)>=(pointer-arg)) {
		if      (*pointer==0) {}
		else if (*pointer=='_') {}
		else if (*pointer=='s') {
			pointer++;
			nr=*pointer-'0';
			printf("sleep %d sec\n", nr);
			msleep(nr*1000);
		}
		else if (*pointer=='u') {
			printf("sende udp ... ");
			strcpy(sIPaddr, func_udp());
			printf("empfange %s\n", sIPaddr);
		}
		else if (*pointer=='x') {
			printf("sende xml ... ");
			func_xml(sIPaddr);
			printf("empfange ok\n");
		}
		else if (*pointer=='c') {
			printf("conf\n");
			onoff=func_conf(sIPaddr);
			printf("onoff: %d\n", onoff);
		}
		else if (*pointer=='v') {
			printf("value\n");
			onoff=onoff^1;
			func_value(sIPaddr, onoff);
		}
		else {
			printf("unknown: %c\n", *pointer);
		}
		pointer++;
	}

//ö	fp=fopen(".fbsim.ini","w");
//ö    fprintf(fp, "%s\n", sIPaddr);
//ö    fprintf(fp, "%d\n", onoff);
//ö	fclose(fp);


    return 0;
}
