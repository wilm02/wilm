#define CATF 0   // Ergänzungen für eine Katzenklappe
#define RTCM 0   // Ergänzungen für einen nicht immer vorhandenen reset-resistenten Speicher
#define TEMP 1   // Ergänzungen für Ausgabe von Temperaturmessungen
#define DUMP 1   // hex-Dump der Kommunikation über Port 2002

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>               // für UDP
#include <ESP8266HTTPUpdateServer.h>        // für OTA-Update
#include <ESP8266mDNS.h>                    // für MDNS
/*rtcm*/#if RTCM == 1
/*rtcm*/ extern "C" { 
/*rtcm*/    #include "user_interface.h"         // für system_rtc_mem_read/write
/*rtcm*/ }
/*rtcm*/ #define MAGIC_RTC 0x78e403b5           // zur Erkennung, ob rtc gültig ist
/*rtcm*/ #define RTC_MAGIC      64
/*rtcm*/ #define RTC_AFTERBOOT  65
/*rtcm*/ #define RTC_BEFOREBOOT 66
/*rtcm*/ #define RTC_INDEX      67
/*rtcm*/ #define RTC_STARTMEM   68
/*rtcm*/ #define RTC_ENDMEM    191
/*rtcm*/#endif


#define MYSSID "myssid"                     // Eintrag der SSID (Name des WLAN-Netzwerks)
#define MYPASS "mypass"                     // Eintrag des WLAN-Passworts
#define DEVNAME "sim8266"
#define MY2SSID "myssid2"                   // Eintrag der SSID (Name des WLAN-Netzwerks)
#define MY2PASS "mypass2"                   // Eintrag des WLAN-Passworts
#define PROGRAMVERSION "20191231_000000"
#define LED_ANT   2                         // IO der LED neben Antenne (Pin_D4)
#define BUTTON    0                         // IO der Taste (Pin_D3)---Pressbutton---Gnd
/*catf*/#if CATF == 1
/*catf*/#define RELAIS1 12                  // IO des Relais (Pin_D6) => red button
/*catf*/#endif

#define swap16(x) ((((x)&0xff00)>>8) | ((((x)&0xff)<<8)))
#define swap32(x) ((((x)&0xff000000)>>24) | (((x)&0xff00)<<8) | (((x)&0xff0000)>>8) | (((x)&0xff)<<24))

#define LOGLEN 2000                         // Länge der logging-Daten
char gLog[LOGLEN];                          // Ringpuffer für logging

#define PACKETLEN 1518                   +20// +20 für Sicherheit
char gPacket[PACKETLEN+1];                  // Puffer für Pakete
IPAddress gOwnIP(0,0,0,0);
IPAddress gFritzIP(0,0,0,0);
uint8_t gMac[6];                            // MAC vom ESP8266
bool gSim=false;                            // true: nur Simulation beachten, false: echte Box und Simulation beachten
int gOnoff=1;                               // 0:LED off, 1:LED on, erster Schaltzustand nach Booten
/*temp*/#if TEMP == 1
/*temp*/int gTemperat=170;
/*temp*/#endif
int gFritzMode=0;
ESP8266WebServer gServer80(80);
ESP8266HTTPUpdateServer gHttpUpdater;
/*rctm*/#if RTCM == 1
/*rctm*/uint32_t gMillisBB = 0;                // millis before boot (offset)
/*rctm*/#endif



void logging(const char *fmt,...)
{
#define LINELEN 80
    static bool init=true;
    if (init) {
       init=false;
       Serial.begin(115200);
       delay(10);
    }
    char sTmp[LINELEN];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sTmp, LINELEN, fmt, ap);      // vprintf gibt auf stdout aus, vsnprintf in string der Länge n
    va_end(ap);
    Serial.print(sTmp);                     // zusätzlich seriell ausgeben
    // Ausgabe in gLog
    if (strlen(gLog)+strlen(sTmp)>LOGLEN-2) // curl -s esp8266.local/log
        strcpy(gLog, sTmp);                 // Ring voll, vorne wieder beginnen
    else
        strcat(gLog, sTmp);                 // Daten anhängen
}


char* watch(int sec=0) {
    static char temp[20];
    if (sec==0) {
        sec = millis() / 1000;
    }
    int min = sec / 60;
    int hr = min / 60;
    int day = hr / 24;
    snprintf(temp, 20, "%02d_%02d:%02d:%02d", day, hr % 24, min % 60, sec % 60);
    return temp;
}


/*catf*/#if CATF == 1
/*catf*/// Programmiertes Drücken der roten Taste der Katzenklappe
/*catf*/void door(char *input=(char*)"")               // "r03w02" entspricht 3sec rot, 2sec warten
/*catf*/{
/*catf*/    static unsigned long timer=0;
/*catf*/    static char code[32]="";            // auszuführender Code
/*catf*/    if (input[0]!=0) {                  // neuer Code eingegeben
/*catf*/        if (timer==0)                   // nur wenn kein Code mehr läuft,
/*catf*/            strcpy(code, input);        // neuen Code sichern, sonst ignorieren
/*catf*/        else
/*catf*/            logging("ERROR: busy, ignore code\n");
/*catf*/    }
/*catf*/    if (timer!=0){                      // Endzeit ist eingetragen
/*catf*/        if (millis()<timer) return;     // Endzeit noch nicht erreicht => fertig
/*catf*/        if (code[0]=='r') {logging("%s red toggled\n", watch());  digitalWrite(RELAIS1, 1);}
/*catf*/        if (code[0]=='w') {logging("%s wait end\n", watch());}
/*catf*/        timer=0;
/*catf*/        strcpy(code, &code[3]);         // nächsten Code holen
/*catf*/    }
/*catf*/    if (code[0]==0) return;             // kein Code mehr vorhanden => fertig
/*catf*/    if (timer==0) {                     // keine Endzeit eingetragen
/*catf*/        int duration=(code[1]-'0')*10+code[2]-'0';
/*catf*/        if ((duration<0) || (duration>99)) {// bei unplausibler Dauer
/*catf*/            logging("ERROR: ignore wrong duration\n");
/*catf*/            timer=0;                    // alles löschen
/*catf*/            code[0]=0;
/*catf*/            return;
/*catf*/        }
/*catf*/        timer=millis()+duration*1000;   // neue Endzeit eintragen
/*catf*/        if      (code[0]=='r') {/*logging("%s red start\n", watch());*/  digitalWrite(RELAIS1, 0);}
/*catf*/        else if (code[0]=='w') {/*logging("%s wait start\n", watch())*/;}
/*catf*/        else {                          // falsche Codes ignorieren
/*catf*/            logging("ERROR: ignore wrong code\n");
/*catf*/            timer=0;                    // alles löschen
/*catf*/            code[0]=0;
/*catf*/            return;
/*catf*/        }
/*catf*/    }
/*catf*/    return;
/*catf*/}
/*catf*/#endif


void handleRoot() {
    char htmlcode[600];

    strcpy(htmlcode,
        "<html>"
        "  <head>"
        "    <meta http-equiv='refresh' content='5'/>"
        "    <title>" DEVNAME "</title>"
        "  </head>"
        "  <body>"
        "  Version: " PROGRAMVERSION "<br>"
        "  Device: " DEVNAME "<br>"
        "  Uptime: "
    );

    strcat(htmlcode, watch());
    strcat(htmlcode, "<br>");

    if      (gFritzMode==4) strcat(htmlcode, "Fritz: connected<br><br>");
    else                    strcat(htmlcode, "Fritz: searching<br><br>");

    if (gOnoff==0) strcat(htmlcode, "<br>LED: off<br><br>");
    if (gOnoff==1) strcat(htmlcode, "<br>LED: on<br><br>");

    strcat(htmlcode,
        "    <form action='toggel'>"
        "      <input type='submit' value='Toggel'>"
        "    </form><br>"
        "    <form action='log'>"
        "      <input type='submit' value='Logging'>"
        "    </form>"
        "  </body>"
        "</html>"
    );
    gServer80.send(200, "text/html", htmlcode);
}

/*rtcm*/#if RTCM == 1
/*rtcm*/void add_rtc(uint32_t data) {
/*rtcm*/    uint32_t index;
/*rtcm*/    system_rtc_mem_read(RTC_INDEX, &index, 4);      // get first empty index
/*rtcm*/    system_rtc_mem_write((uint8_t)index, &data, 4); // write data
/*rtcm*/    if (++index>RTC_ENDMEM) index=RTC_STARTMEM;
/*rtcm*/    system_rtc_mem_write(RTC_INDEX, &index, 4);     // write new index
/*rtcm*/}
/*rtcm*/#endif


void setup() {
    millis();                               // für richtige Zeit in Simulation

    pinMode(LED_ANT, OUTPUT);
/*catf*/#if CATF == 1
/*catf*/    pinMode(RELAIS1, OUTPUT); digitalWrite(RELAIS1, 1);              // Pin_D6 hat 3,3V, RELAIS1 ist aus (negative Logik)
/*catf*/#endif
    ESP8266WiFiMulti wifiMulti;             // oder: WiFi.mode(WIFI_STA);
    wifiMulti.addAP(MYSSID, MYPASS);        // oder: WiFi.begin(MYSSID, MYPASS);
    wifiMulti.addAP(MY2SSID, MY2PASS);      //
    while(wifiMulti.run()!=WL_CONNECTED) {  // oder: while (WiFi.status() != WL_CONNECTED) {
       delay(500);
       logging(".");
    }
    logging(". got wifi\n");
    while ((int)(gFritzIP=WiFi.gatewayIP())==0) {   // im Fritz-Netzwerk gibt WiFi.gatewayIP() die Adresse der Fritz-Box
        delay(500);
        logging(".");
    }
    logging(". got gateway\n");
    delay(1000);
    gOwnIP=WiFi.localIP();
    logging("OwnIP: %d.%d.%d.%d\n", gOwnIP[0],gOwnIP[1],gOwnIP[2],gOwnIP[3]);
   
    strcpy(gPacket, const_cast<char*>(WiFi.macAddress().c_str()));
    gPacket[2]=0;  gMac[0]=strtol(&gPacket[0],  NULL, 16);
    gPacket[5]=0;  gMac[1]=strtol(&gPacket[3],  NULL, 16);
    gPacket[8]=0;  gMac[2]=strtol(&gPacket[6],  NULL, 16);
    gPacket[11]=0; gMac[3]=strtol(&gPacket[9],  NULL, 16);
    gPacket[14]=0; gMac[4]=strtol(&gPacket[12], NULL, 16);
    gPacket[17]=0; gMac[5]=strtol(&gPacket[15], NULL, 16);
    logging("gMac: %02X:%02X:%02X:%02X:%02X:%02X\n",
        gMac[0],gMac[1],gMac[2],gMac[3],gMac[4],gMac[5]);
    if (MDNS.begin(DEVNAME)) logging("MDNS: %s.local\n", DEVNAME);
    gHttpUpdater.setup(&gServer80, "/firmware");    // Firmware-Update
    gServer80.on("/", handleRoot);
    gServer80.onNotFound([](){
        gServer80.send(404, "text/plain", "404: Not found");
    });
    gServer80.on("/log", []() { // gLog als Ring-Puffer auslesen mit http://<ip address>/log
        String tmp;
        tmp=String(&gLog[strlen(gLog)+1]);
        tmp+=String(gLog);
/*rtcm*/#if RTCM == 1
/*rtcm*/        uint32_t i;
/*rtcm*/        uint32_t rtcStore;
/*rtcm*/        uint32_t maxindex;
/*rtcm*/        tmp+=String("---------------\n");
/*rtcm*/        tmp+=watch();
/*rtcm*/        tmp+=String(" <==== actual time\n---------------\n");
/*rtcm*/        system_rtc_mem_read(RTC_INDEX, &maxindex, 4);       // first empty index
/*rtcm*/        for(i=RTC_STARTMEM; i<maxindex; i++) {
/*rtcm*/            system_rtc_mem_read((uint8_t)i, &rtcStore, 4);  // first empty index
/*rtcm*/            if (rtcStore==0xFFFFFFFF) {
/*rtcm*/                tmp+="####Reboot\n";
/*rtcm*/            } else if (rtcStore==0xFFFFFFFE) {
/*rtcm*/                tmp+="####Fritzboot\n";
/*rtcm*/            } else {
/*rtcm*/                tmp+=String(watch(rtcStore / 1000));
/*rtcm*/                if ((rtcStore&1)==1) tmp+=" new value: on  \n";
/*rtcm*/                if ((rtcStore&1)==0) tmp+=" new value: off \n";
/*rtcm*/            }
/*rtcm*/        }
/*rtcm*/#endif

        gServer80.send(200, "text/plain", tmp);
    });
    gServer80.on("/toggel", []() {
        gOnoff=gOnoff^1;                    // Led toggeln
        gServer80.sendHeader("Location","/"); // neue Location "/"
        gServer80.send(303);                // HTTP status 303 für redirect mit neuer Location
    });

/*catf*/#if CATF == 1
/*catf*/        gServer80.on("/door", []() {
/*catf*/            String argument=gServer80.arg("argument");  // http://<ip address>/door?argument=default
/*catf*/            logging("door argument: ");
/*catf*/            logging(const_cast<char*>(argument.c_str()));
/*catf*/            logging("\n");
/*catf*/            door(const_cast<char*>(argument.c_str()));
/*catf*/            gServer80.send(200, "text/plain", "door argument: "+ argument);
/*catf*/        });
/*catf*/#endif

    gServer80.begin();
	
/*rtcm*/#if RTCM == 1
/*rtcm*/    uint32_t rtcStore;
/*rtcm*/    system_rtc_mem_read(RTC_MAGIC, &rtcStore, 4);
/*rtcm*/    if (rtcStore!=MAGIC_RTC) {   // erstes Booten nach Poweroff
/*rtcm*/        rtcStore=MAGIC_RTC;
/*rtcm*/        system_rtc_mem_write(RTC_MAGIC, &rtcStore, 4);      // MAGIC_RTC
/*rtcm*/        rtcStore=millis();
/*rtcm*/        system_rtc_mem_write(RTC_AFTERBOOT, &rtcStore, 4);      // millis after last boot
/*rtcm*/        gMillisBB=0;
/*rtcm*/        system_rtc_mem_write(RTC_BEFOREBOOT, &gMillisBB, 4);    // millis before last boot
/*rtcm*/        rtcStore=RTC_STARTMEM;
/*rtcm*/        system_rtc_mem_write(RTC_INDEX, &rtcStore, 4);      // first empty index
/*rtcm*/        logging("very first boot after poweroff\n");
/*rtcm*/    } else {
/*rtcm*/        system_rtc_mem_read(RTC_AFTERBOOT, &rtcStore, 4);       //
/*rtcm*/        system_rtc_mem_read(RTC_BEFOREBOOT, &gMillisBB, 4);     // get old value
/*rtcm*/        gMillisBB+=rtcStore;
/*rtcm*/        system_rtc_mem_write(RTC_BEFOREBOOT, &gMillisBB, 4);    // millis before last boot
/*rtcm*/        system_rtc_mem_read(RTC_INDEX, &rtcStore, 4);       // first empty index
/*rtcm*/        if (--rtcStore<RTC_STARTMEM) rtcStore=RTC_ENDMEM;
/*rtcm*/        system_rtc_mem_read((uint8_t)rtcStore, &rtcStore, 4);   // get last data
/*rtcm*/        gOnoff=(rtcStore&0x01)^1;
/*rtcm*/        logging("time before last boot: ");
/*rtcm*/        logging(watch());
/*rtcm*/        logging("\n");
/*rtcm*/        add_rtc(0xFFFFFFFF);
/*rtcm*/    }
/*rtcm*/#endif
}


// mode 0: weitersuchen
// mode 1: beim nächsten Mal udp.beginMulticast notwendig
// return 0: weitersuchen
// return 1: Paket wurde erkannt
int ssdp() {
    static uint8_t mode=1;
    static WiFiUDP udp;
    if (mode==1) {
        mode=0;
        udp.beginMulticast(gOwnIP, IPAddress(239,255,255,250), 1900);
    }
    if (!udp.parsePacket()) return 0;       // kein Paket ist da
    int len=udp.read(gPacket, PACKETLEN);   // lese Daten und untersuche die letzten 13 Zeichen auf "avm-aha:1"
    if (len<=13) return 0;                  // keine 13 Zeichen gelesen

    if (strncmp((&gPacket[len-13]), "sim-aha:1", 9)!=0) {
        if (gSim) return 0;
        if (strncmp((&gPacket[len-13]), "avm-aha:1", 9)!=0) return 0;
        if ((uint32_t)udp.remoteIP()!=(uint32_t)gFritzIP) return 0; // Paket kam nicht von Fritzbox
    } else {
        gSim=true;          // einmal sim gefunden: ab jetzt nur noch Simulation beachten
        gFritzIP=udp.remoteIP();
    }
    udp.beginPacket(gFritzIP, udp.remotePort());// Antwort zusammenstellen
    sprintf(gPacket,
        "HTTP/1.1 200 OK\r\n"
        "LOCATION: http://%d.%d.%d.%d:49000/aha.xml\r\n"                        // gOwnIP
        "SERVER: " DEVNAME " UPnP/1.0 AVM FRITZ!Powerline 546E 118.06.50\r\n"   // FriendlyName
        "CACHE-CONTROL: max-age=1800\r\n"
        "EXT:\r\n"
        "ST: urn:schemas-upnp-org:device:avm-aha:1\r\n"
        "USN: uuid:4796e12e-ab96-4769-b0d0-%02X%02X%02X%02X%02X%02X::urn:schemas-upnp-org:device:avm-aha:1\r\n" // gMac[]
        "\r\n",
        gOwnIP[0],gOwnIP[1],gOwnIP[2],gOwnIP[3],
        gMac[0],gMac[1],gMac[2],gMac[3],gMac[4],gMac[5]);
    udp.print(gPacket);
    udp.endPacket();
    udp.stop();
    mode=1;                                 // neue Initialisierung notwendig
    return 1;
}


void notify(int mode) { // 0:byebye, 1:alive
    WiFiUDP udp;
    udp.beginPacketMulticast(IPAddress(239,255,255,250), 1900, gOwnIP);// Antwort zusammenstellen
    sprintf(gPacket,
        "NOTIFY * HTTP/1.1\r\n"
        "HOST: 239.255.255.250:1900\r\n"
        "LOCATION: http://%d.%d.%d.%d:49000/aha.xml\r\n"                        // gOwnIP
        "SERVER: " DEVNAME " UPnP/1.0 AVM FRITZ!Powerline 546E 118.06.50\r\n"   // FriendlyName
        "CACHE-CONTROL: max-age=1800\r\n"
        "NT: urn:schemas-upnp-org:device:avm-aha:1\r\n"
        "NTS: ssdp:%s\r\n"
        "USN: uuid:4796e12e-ab96-4769-b0d0-%02X%02X%02X%02X%02X%02X::urn:schemas-upnp-org:device:avm-aha:1\r\n" // gMac[]
        "\r\n",
		gOwnIP[0],gOwnIP[1],gOwnIP[2],gOwnIP[3],
		(mode?"alive":"byebye"),
        gMac[0],gMac[1],gMac[2],gMac[3],gMac[4],gMac[5]);
    udp.write(gPacket);
    udp.endPacket();
    udp.stop();
}


// mode 0: weitersuchen
// mode 1: beim nächsten Mal server.begin() notwendig
// return 0: weitersuchen
// return 1: Paket wurde beantwortet
// return -1: Fehler
int xml() {
    static uint8_t mode=1;
    static unsigned long timer=-1;
    static WiFiServer server(49000);
    static WiFiClient client;               // client wird zu NULL initialisiert
    if (mode==1) {
        mode=0;
        server.begin();
        timer=millis()+1000*   2;           // Sekunden Reaktionszeit
        ////logging("Server 49000 started\n"); 
    }
    if (millis()>timer) {
        ////logging("no xml within time\n");
        if (server.available()) client.stop();
        server.close();
        mode=1;
        return -1;                          // Timeout, keine rechtzeitige Antwort
    }
    if (!client) client=server.available(); // client.stop() setzt client wieder auf NULL
    if (!client) return 0;
    if (!client.connected()) return 0;
    if (!client.available()) return 0;          // Verbindung ist da, aber (noch) keine Daten
    String req = client.readStringUntil('\r');  // erste Zeile lesen
    while (client.available()) client.read();   // kompletten Rest lesen
    if (req == "GET /aha.xml HTTP/1.1") {
        ////logging("xml1\n");
        sprintf(gPacket,
            "<?xml version=\"1.0\"?>\r\n"
            "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\r\n"
            " <specVersion>\r\n"
            "  <major>1</major>\r\n"
            "  <minor>0</minor>\r\n"
            " </specVersion>\r\n"
            " <device>\r\n"
            "  <deviceType>urn:schemas-upnp-org:device:avm-aha:1</deviceType>\r\n"
            "  <friendlyName>%s</friendlyName>\r\n"                                     // FriendlyName
            "  <manufacturer>AVM</manufacturer>\r\n"
            "  <manufacturerURL>http://www.avm.de</manufacturerURL>\r\n"
            "  <modelDescription>AVM AHA</modelDescription>\r\n"
            "  <modelName>AVM AHA</modelName>\r\n"
            "  <modelNumber>AVM AHA</modelNumber>\r\n"
            "  <modelURL>http://www.avm.de</modelURL>\r\n"
            "  <UDN>uuid:4796e12e-ab96-4769-b0d0-%02X%02X%02X%02X%02X%02X</UDN>\r\n"    // gMac[]
            "  <iconList>\r\n"
            "   <icon>\r\n"
            "    <mimetype>image/gif</mimetype>\r\n"
            "    <width>118</width>\r\n"
            "    <height>119</height>\r\n"
            "    <depth>8</depth>\r\n"
            "    <url>/ligd.gif</url>\r\n"
            "   </icon>\r\n"
            "  </iconList>\r\n"
            "  <serviceList>\r\n"
            "   <service>\r\n"
            "    <serviceType>urn:schemas-any-com:service:avm-aha:1</serviceType>\r\n"
            "    <serviceId>urn:any-com:serviceId:avm-aha1</serviceId>\r\n"
            "    <controlURL>/upnp/control/avm-aha</controlURL>\r\n"
            "    <eventSubURL>/upnp/control/avm-aha</eventSubURL>\r\n"
            "    <SCPDURL>/avm-ahaSCPD.xml</SCPDURL>\r\n"
            "   </service>\r\n"
            "  </serviceList>\r\n"
            "  <presentationURL>http://%d.%d.%d.%d</presentationURL>\r\n"               // gOwnIP
            " </device>\r\n"
            "</root>\r\n",
            DEVNAME,
            gMac[0],gMac[1],gMac[2],gMac[3],gMac[4],gMac[5],
            gOwnIP[0],gOwnIP[1],gOwnIP[2],gOwnIP[3]);
        client.print(
            "HTTP/1.1 200 OK\r\n"
            "Cache-Control: max-age=120\r\n"
            "Connection: close\r\n"
            "Content-Length: "+String(strlen(gPacket))+"\r\n"
            "Content-Type: text/xml\r\n"	// ohne "ETag: \"0B2CBD318DC142AE8\"\r\n"
            "Last-Modified: Thu, 01 Jan 1970 00:00:10 GMT\r\n"
            "Mime-Version: 1.0\r\n"
            "\r\n");
        client.print(gPacket);
        client.stop();     // unbedingt notwendig, um mehrfach Client zu bedienen
        return 0;
    }
    if (req == "GET /avm-ahaSCPD.xml HTTP/1.1") {
        ////logging("xml2\n");
        sprintf(gPacket,
            "<?xml version=\"1.0\"?>\r\n"
            "<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">\r\n"
            " <specVersion>\r\n"
            "  <major>1</major>\r\n"
            "  <minor>0</minor>\r\n"
            " </specVersion>\r\n"
            " <serviceStateTable>\r\n"
            "  <stateVariable sendEvents=\"no\">\r\n"
            "  <name>Dummy</name>\r\n"
            "  <dataType>string</dataType>\r\n"
            "  </stateVariable>\r\n"
            " </serviceStateTable>\r\n"
            " <actionList />\r\n"
            "</scpd>\r\n");
        client.print(
            "HTTP/1.1 200 OK\r\n"
            "Cache-Control: max-age=120\r\n"
            "Connection: close\r\n"
            "Content-Length: "+String(strlen(gPacket))+"\r\n"
            "Content-Type: text/xml\r\n"	// ohne "ETag: \"7F081B5FA3B015293\"\r\n"
            "Last-Modified: Thu, 01 Jan 1970 00:00:20 GMT\r\n"
            "Mime-Version: 1.0\r\n"
            "\r\n");
        client.print(gPacket);
        client.stop();     // unbedingt notwendig, um mehrfach Client zu bedienen
        server.stop();
        mode=1;
        return 1;
    }
    logging("ERROR unknown GET\n");
    if (server.available()) client.stop();
    server.stop();
    mode=1;
    return -1;            // Timeout, keine rechtzeitige Antwort
}


// einzelne Variablen oder ganze Variablen-Felder mit Werten versorgen
void append_var(uint8_t *feld, uint16_t *pidx, uint16_t var, uint32_t val)
{
    static uint16_t last=0;
    uint16_t len=4;
    if (var) {          // falls var==0, dann anhängen an letzten Wert
        *(uint16_t*)&feld[*pidx]=0;             *pidx+=2;
        *(uint16_t*)&feld[*pidx]=swap16(var);   *pidx+=2;  // var eintragen
        last=*pidx;
        *(uint16_t*)&feld[*pidx]=swap16(len);   *pidx+=2;  // len eintragen
        *(uint16_t*)&feld[*pidx]=0;             *pidx+=2;
    } else {
        len=swap16(*(uint16_t*)&feld[last])+4;
        *(uint16_t*)&feld[last]=swap16(len);
    }
    *(uint32_t*)&feld[*pidx]=swap32(val);       *pidx+=4;  // val eintragen
}


void dump(char *inout, uint8_t *start, uint32_t len)
{
	uint32_t i  =len+*start+*inout;i++; // for compiler messages
/*dump*/#if DUMP == 1
/*dump*/	logging("%s%s:", &watch()[7], inout);
/*dump*/	for(i=0; i<len; i++)
/*dump*/		logging("%02x",	start[i]);
/*dump*/	logging("\n");
/*dump*/#endif
}


// mode 0: weitersuchen
// mode 1: beim nächsten Mal server.begin() notwendig
// return 0: keine Verbindung
// return 1: Verbindung steht
// return -1: Fehler
int p2002()
{
    static uint8_t mode=1;
    static unsigned long timer=-1;          // Timer für Timeout
    static unsigned long cyclical=-1;       // nächstes zyklischen Schreiben
    static WiFiServer server(2002);
    static WiFiClient client;               // client wird zu NULL initialisiert
    static int localOnoff=gOnoff;
    if (mode==1) {
        mode=0;
        server.begin();
        timer=millis()+1000*   2;           // Sekunden Reaktionszeit auf erste Reaktion 
        ////logging("Server 2002 started\n\n");
/*rtcm*/#if RTCM == 1
/*rtcm*/        add_rtc(0xFFFFFFFE);
/*rtcm*/#endif
    }
    if (millis()>timer) {
        logging("p2002-timeout\n");
        if (server.available()) client.stop();
        server.close();
        mode=1;
        return -1;                          // Timeout, keine rechtzeitige Antwort
    }

    if (!client) client=server.available(); // client.stop() setzt client wieder auf NULL
    if (!client) return 0;
    if (!client.connected()) return 0;

    int len;
    char* pPacket;
#define PAYLOADLEN 340  // <-------------- Anpassen!!!!!!!!!!!
    uint8_t payload[PAYLOADLEN];
    uint32_t i, paylen;
    pPacket=gPacket;
    if ((millis()>cyclical)||(localOnoff!=gOnoff)) {
        localOnoff=gOnoff;
        cyclical=millis()+1000*   30;       // zyklisch alle 30 Sekunden Daten senden
        // Erzeugen eines fake-Pakets, damit Daten zyklisch oder wegen Tastendruck gesendet werden
/*temp*/#if TEMP == 1
/*temp*/len=28*2;
/*temp*/#else
        len=28;
/*temp*/#endif
        memset(gPacket, 0, len);
                     gPacket[ 0]=7;                    // values
        *(uint16_t*)&gPacket[ 2]=swap16(28);           // Länge
                     gPacket[16]=1;                    // Kennung für fake-Paket, sonst immer 0
                     gPacket[19]=0x0f;                 // 0x0f=Schaltzustand
        *(uint32_t*)&gPacket[24]=swap32(gOnoff);       // Wert des Schaltzustands
/*temp*/#if TEMP == 1
/*temp*/             gPacket[28+ 0]=7;                 // values
/*temp*/*(uint16_t*)&gPacket[28+ 2]=swap16(28);        // Länge
/*temp*/             gPacket[28+16]=1;                 // Kennung für fake-Paket, sonst immer 0
/*temp*/             gPacket[28+19]=0x17;              // 0x17=Temperatur
/*temp*/*(uint32_t*)&gPacket[28+24]=swap32(gTemperat); // Wert der Temperatur
/*temp*/#endif
    } else {
        if (!client.available()) return 1;  // Verbindung ist da, aber (noch) keine Daten
        timer=millis()+1000*   50;          // mindestens alle 22 sec Datenverkehr (ping) (timeout von 25 oder 33 war zu wenig)
        // Einlesen der empfangenen Pakete
        len=client.read((uint8_t*)gPacket, PACKETLEN);
        if (len==0) return 1;
    }
    while(1) {
		dump((char*)"i", (uint8_t*)pPacket, swap16(*(uint16_t*)&pPacket[2]));
        if (pPacket[0]==0) {// init (zu Beginn der Kommunikation, spezifische Antwort)
            //logging("init\n");
            paylen=16;
            memset(payload, 0, paylen);                   // vorher alles löschen
            payload[0]=1;                                 // init_re
            payload[1]=3;                                 // Version 3?
            *(uint16_t*)&payload[2]=swap16(paylen);       // Länge
            payload[7]=1;                                 // 1=??
            payload[11]=1;                                // 1=??
            *(uint32_t*)&payload[12]=swap32(0x22013);     // 0x22013=??
            client.write((const uint8_t*)payload, paylen);
			dump((char*)"o", (uint8_t*)payload, paylen);
        } else if (pPacket[0]==3) { // drei (unklare Funktion, keine Antwort)
            //logging("drei\n");
            cyclical=millis()+1000*   30;  // erstes Starten des zyklischen Sendens von Daten
        } else if (pPacket[0]==5) { // conf (Konfiguration austauschen, spezifische Antwort)
            //logging("conf\n");
            memset(payload, 0, PAYLOADLEN);               // vorher alles löschen
            payload[0]=6;                                 // conf_re
            payload[1]=3;                                 // Version 3?
            payload[7]=1;                                 // 1=??
            *(uint16_t*)&payload[16]=swap16(1000);        // id:1000
            *(uint16_t*)&payload[18]=swap16(512);         // 512=??
            *(uint16_t*)&payload[22]=swap16(2);           // Modell:2=Powerline546E
/*temp*/#if TEMP == 1
/*temp*/    *(uint16_t*)&payload[26]=swap16(0x1380);      // functionbitmask
/*temp*/#else
            *(uint16_t*)&payload[26]=swap16(0x1280);      // functionbitmask
/*temp*/#endif
            strcpy((char*)&payload[28], DEVNAME);         // FriendlyName
            *(uint32_t*)&payload[108]=swap32(2932);       // manufacturer:0b74=AVM
            sprintf((char*)&payload[112], "%02X:%02X:%02X:%02X:%02X:%02X",
                gMac[0], gMac[1], gMac[2], gMac[3], gMac[4], gMac[5]); // AIN
            strcpy((char*)&payload[132], PROGRAMVERSION); // fwversion
            *(uint16_t*)&payload[158]=swap16(2);          // 2=??
            *(uint16_t*)&payload[160]=swap16(1000);       // id:1000
            uint16_t idx=168;
            append_var(payload, &idx, 0x0f, gOnoff);      // 0x0f=15 Schaltzustand 0/1
            append_var(payload, &idx, 0x23, 0xffff);      // 0x23=35
            append_var(payload, &idx, 0   , 0);           // 0x24=36
            append_var(payload, &idx, 0x25, 1000);        // 0x25=37
            append_var(payload, &idx, 0   , 2);           // 0x26=38
            append_var(payload, &idx, 0   , 1);           // 0x27=39
            append_var(payload, &idx, 0   , 20);          // 0x28=40
            append_var(payload, &idx, 0   , 5);           // 0x29=41
            append_var(payload, &idx, 0   , 0);           // 0x2a=42
            append_var(payload, &idx, 0   , 0);           // 0x2b=43
            append_var(payload, &idx, 0   , 15);          // 0x2c=44
            append_var(payload, &idx, 0   , 0);           // 0x2d=45
            append_var(payload, &idx, 0   , 2);           // 0x2e=46
            append_var(payload, &idx, 0   , 1);           // 0x2f=47
            append_var(payload, &idx, 0   , 3);           // 0x30=48
            append_var(payload, &idx, 0   , 0);           // 0x31=49
            append_var(payload, &idx, 0   , 0);           // 0x32=50
            append_var(payload, &idx, 0   , 15);          // 0x33=51
            append_var(payload, &idx, 0   , 0);           // 0x34=52
            append_var(payload, &idx, 0x12, 20);          // 0x12=18 Strom A [/10000]
            append_var(payload, &idx, 0x13, 236159);      // 0x13=19 Spannung V [/1000]
            append_var(payload, &idx, 0x14, 0);           // 0x14=20 Leistung W [/100]
            append_var(payload, &idx, 0x15, 132794);      // 0x15=21 Energie kWh [/1000]
            append_var(payload, &idx, 0x16, 0xfffffeec);  // 0x16=22 CosPhi [/1000] (-1...+1)
/*temp*/#if TEMP == 1
/*temp*/    append_var(payload, &idx, 0x17, gTemperat);   // 0x17=23 Temperatur [/10]
/*temp*/#endif
			if (idx-PAYLOADLEN>0) logging("ERROR: PAYLOADLEN %d too small\n", idx-PAYLOADLEN);
            *(uint16_t*)&payload[166]=swap16(idx-168);    // Länge dieses Datenfeldes
            *(uint16_t*)&payload[2]=swap16(idx);          // Länge
            paylen=idx;
            client.write((const uint8_t*)payload, paylen);
			dump((char*)"o", (uint8_t*)payload, paylen);
        } else if (pPacket[0]==7) { // values (einzelne Werte)
            //if (pPacket[16]==0) logging("read\n");
            if (pPacket[19]==0x0f) {
                gOnoff=swap32(*(uint32_t*)&pPacket[24]);  // Daten aus Paket auslesen
                memset(payload, 0, 36);                   // vorher alles löschen
                payload[0]=7;                             // value
                payload[1]=3;                             // Version 3?
                *(uint32_t*)&payload[ 4]=swap32(1);       // 1=??
                *(uint16_t*)&payload[ 8]=swap16(1000);    // 1000=??
                *(uint32_t*)&payload[10]=swap32(0);       //
                *(uint16_t*)&payload[14]=swap16(0x0c);    // Länge der Daten
                uint16_t idx=16;
                append_var(payload, &idx, 0x0f, gOnoff);  // 0x0f=Schaltzustand
                *(uint16_t*)&payload[ 2]=swap16(idx);     // Länge
                paylen=idx;
                //if (pPacket[16]==0) logging("writeOnOff: %d\n", gOnoff);
				//else                logging("cyclicOnOff: %d\n", gOnoff);
                client.write((const uint8_t*)payload, paylen);
				dump((char*)"o", (uint8_t*)payload, paylen);
            }
/*temp*/#if TEMP == 1
/*temp*/    if (pPacket[19]==0x17) {
/*temp*/        memset(payload, 0, 36);                   // vorher alles löschen
/*temp*/        payload[0]=7;                             // value
/*temp*/        payload[1]=3;                             // Version 3?
/*temp*/        *(uint32_t*)&payload[ 4]=swap32(1);       // 1=??
/*temp*/        *(uint16_t*)&payload[ 8]=swap16(1000);    // 1000=??
/*temp*/        *(uint32_t*)&payload[10]=swap32(0);       //
/*temp*/        *(uint16_t*)&payload[14]=swap16(0x0c);    // Länge der Daten
/*temp*/        uint16_t idx=16;
/*temp*/        append_var(payload, &idx, 0x17, gTemperat);// 0x17=Temperatur
/*temp*/        *(uint16_t*)&payload[ 2]=swap16(idx);     // Länge
/*temp*/        paylen=idx;
/*temp*/        //if (pPacket[16]==0) logging("writeTemp: %d\n", gTemperat);
/*temp*/        //else                logging("cyclicTemp: %d\n", gTemperat);
/*temp*/        client.write((const uint8_t*)payload, paylen);
/*temp*/		dump((char*)"o", (uint8_t*)payload, paylen);
/*temp*/    }
/*temp*/#endif
        } else if (pPacket[0]==8) { // ping (regelmäßig alle 22sec, nahezu identische Antwort)
            //logging("ping %d\n", swap32(*(uint32_t*)&pPacket[8]));
            for(i=0; i<12; i++)
                payload[i]=pPacket[i];
            payload[6]=0;
            payload[7]=0;
            paylen=12;
            client.write((const uint8_t*)payload, paylen);
			dump((char*)"o", (uint8_t*)payload, paylen);
/*temp*/#if TEMP == 1
/*temp*/gTemperat++; ///////////////////////////////FAKE!!!!!!!!!!!!!!!!!!!!!!!!!!!
/*temp*/#endif
        } else if (pPacket[0]==9) { // start (einmal zum Start des Devices, keine Antwort)
            //logging("start\n");
        } else {
            logging("ERROR: Unknown Command\n");
        }
        // Überprüfung der Länge
        if ((swap16(*(uint16_t*)&pPacket[2])==0) || (swap16(*(uint16_t*)&pPacket[2])>len)) {
            logging("ERROR: Invalid length\n");           // fehlerhafte Länge im Paket
            if (server.available()) client.stop();
            server.close();
            mode=1;
            return -1;
        }
        len-=swap16(*(uint16_t*)&pPacket[2]);
        if (len<=0) break;                                // Puffer komplett abgearbeitet
        pPacket+=swap16(*(uint16_t*)&pPacket[2]);         // Zeiger auf nächsten Befehl schieben
        yield();
    }
    return 1;
}


// gFritzMode 0: Einmalig byebye
// gFritzMode 1: nach Wartezeit einmalig alive
// gFritzMode 2: Abarbeitung xml
// gFritzMode 3: einmalig beim Start von p2002 wegen Meldung und alive
// gFritzMode 4: Dauerschleife bei p2002
void actor() {
    static uint32_t timer=0;
    int ret;
    if (gFritzMode==0) {
		notify(0);
        //logging("%s ssdp byebye\n", watch());
        gFritzMode=1;
		timer=millis()+2700;
		return;
    }
    if (gFritzMode==1) {
        if (millis()>=timer) {
 		    notify(1);
            //logging("%s ssdp alive\n", watch());
			gFritzMode=2;
			return;
		}
    }
    if (gFritzMode==2) {
        ret=xml();
        if (ret==0) return;
        if (ret==-1) {
            logging("ERROR xml\n");
            gFritzMode=0;
            return;
        }
        //logging("%s xml ok\n", watch());
        gFritzMode=3;
		return;
    }
		
    if ((gFritzMode==3)||(gFritzMode==4)) {
        ret=p2002();
        if (ret==0) return;
        if (ret==-1) {
            logging("ERROR p2002\n");
            gFritzMode=0;
            return;
        }
        if (gFritzMode==3) {
			notify(1);
			//logging("%s ssdp alive\n", watch());
			logging("%s FRITZ connected\n", watch());
			gFritzMode=4;
			return;
		}
		// gFritzMode==4
		if (ssdp()==1) {
			//logging("%s ssdp ok\n", watch());
		}
    }		
}


void loop() {
    gServer80.handleClient();
    MDNS.update();

    // Entprelltes Toggeln der Taste
    static uint8_t buttonprell=1;                   // Status des entprellten Buttons
    static uint32_t timerprell=0;                   // zur Überwachung der Prellzeit
    if (digitalRead(BUTTON)==buttonprell) timerprell=millis()+100;
    else {
        if (millis()>=timerprell) {                 // lange genug gedrückt
            timerprell=millis()+100;
            buttonprell=buttonprell^1;              // Status des entprellten Buttons ändern
            if (buttonprell) gOnoff=gOnoff^1;       // wenn gedrückt dann Led toggeln
        }
    }

    // Ausgeben des Schaltzustandes
    static int lastOnoff=99;
    if (lastOnoff!=gOnoff) {
        lastOnoff=gOnoff;
        digitalWrite(LED_ANT, gOnoff^1);    // LED aktualisieren (Schreiben mit negativer Logik)
        logging("%s new value: %s\n", watch(), (gOnoff==0?"off":"on "));
/*rtcm*/#if RTCM == 1
/*rtcm*/    add_rtc(((millis()+gMillisBB)&0xFFFFFF00)|gOnoff);
/*rtcm*/#endif        
    }

    // gOnoff im Fritz-Actor darstellen
    actor();
   
/*catf*/#if CATF == 1   
/*catf*/    static int catflapOnoff=99;
/*catf*/    door((char*)"");
/*catf*/    if ((gOnoff==1) && (catflapOnoff==0)) door((char*)"r05"); // Veterinärmodus ein
/*catf*/    if ((gOnoff==0) && (catflapOnoff==1)) door((char*)"r05"); // Veterinärmodus aus
/*catf*/    catflapOnoff=gOnoff;
/*catf*/#endif
    
/*rtcm*/#if RTCM == 1
/*rtcm*/    static uint32_t timer=0;
/*rtcm*/    if (millis()>=timer) {                          // schreibe alle 10 Sekunden die Zeit in rtc
/*rtcm*/        system_rtc_mem_write(RTC_AFTERBOOT, &timer, 4);
/*rtcm*/        timer=millis()+10*1000;
/*rtcm*/    }
/*rtcm*/#endif
}
