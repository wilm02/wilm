# wilm
 
Wilm ist eine Aktuator-Software für einen ESP8266, der in einem Fritz-Netzwerk als kompatibler SmartHome-Schalter dargestellt wird und analog zu DECT 200/210 oder Powerline 546E verwendet werden kann. Die LED des Microcontrollers kann genauso wie andere Fritz-Aktuatoren geschaltet werden.
Wilm ist zwar für einen ESP8266 geschrieben, darüber hinaus läuft Wilm aber auch komplett ohne eigene Hardware in der ESP8266-Simulation unter Linux.
 
Auch wenn heute (Sommer 2020) SmartHome der Fritzboxen komplett auf DECT aufsetzt, ist auch ohne DECT-Hardware nur über LAN ein SmartHome-Schalter auf einer Fritzbox möglich. Der mittlerweile abgekündigte, schaltbare Fritz-Powerline 546E ist ein Beispiel dafür. Vor einiger Zeit habe ich den Datenverkehr eines 546e zur Fritzbox näher analysiert, ihn auf das unbedingt Notwendige beschränkt und danach auf einem ESP8266 nachprogrammiert:
 
1. Die Fritzbox sendet regelmäßig einen Multicast auf Port 1900 mit der Kennung "device:avm-aha", den der SmartHome-Schalter einmalig mit seiner IP-Adresse beantwortet.
 
2. Die Fritzbox erfragt danach von dieser IP-Adresse zwei xml-Konfigurations-Dateien über Port 49000.
 
3. Schließlich öffnet die Fritzbox den Port 2002, über den dann die gesamte weitere (binäre) Kommunikation mit dem SmartHome-Schalter läuft.
 
4. In der SmartHome-Übersicht der Fritzbox wird automatisch ein neuer Schalter ergänzt.
 
Wer diese Kommunikation mit Wireshark nachvollziehen möchte, kann NICHT die Fritzbox selber nutzen, da das Fritz-OS offensichtlich ausgehende Pakete über Port 2002 NICHT mitprotokolliert! "Natürlich" gibt es keine Doku zu der binären Schnittstelle und immer wieder traf ich auf unerwartete Schwierigkeiten. Inzwischen läuft der Wilm-Aktuator bei mir aber stabil mehrere Tage durch.
 
Die Onboard-LED des ESP8266 kann mit der Onboard-Taste und allen anderen "üblichen" Bedienelementen der Fritz-Umgebung geschaltet werden: Web-Gui, Android-App, Fritz-Fon... Zusätzlich gibt es noch ein kleinen Webserver auf dem ESP8266 selber. Verbrauchs- oder Spannungs-Statistiken werden bisher nicht unterstützt, genau wie ein Temperatursensor, der an der Powerline 546E leider fehlt und nicht nachvollzogen werden kann.
 
Ich hatte angefangen mit der Arduino-IDE unter Windows zu programmieren. Inzwischen nutze ich ausschließlich den Linux-Zweig, der in der IDE unter "zusätzliche Boardverwalter-URLs" heruntergeladen wird, verbunden mit einem Makefile. In diesem Zweig enthalten ist eine Tests-Host-Umgebung für Linux, die ich ausgiebig nutze. Damit laufen die identischen Quellen sowohl direkt auf dem ESP8266 als auch in Simulation unter Linux! Kleine Patches für die unvollständige Tests-Host-Umgebung liefere ich hier mit. Meine gesamte aktuelle Installation kann bei Bedarf mit install/install.sh nachvollzogen werden und ist unter doku/install.txt etwas erklärt.
 
Nebenbei ist eine Simulation (test/fbsim) entstanden, die rudimentär die SmartHome-Kommunikation der Fritzbox mit Wilm nachstellt.

Unter https://www.ip-phone-forum.de/threads/wilm-selbstprogrammierter-fritz-aktuator.308314 wird eine Diskussion zu wilm geführt.
