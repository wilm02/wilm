#!/bin/bash
. ../src/lib.sh
echo $(XORCRYPT "{|+ \"'$'") | sudo -S whoami 2>&1 > /dev/null # für ein paar Sekunden läuft sudo jetzt ohne Passwort
#ö# 
#ö# sudo pkill -f wilm
#ö# echo Browser starten mit http://sim8266.local
#ö# 
#ö# INO=wilm
#ö# sudo rm -f ../devel/esp8266/tests/host/bin/${INO}/${INO}-*
#ö# sudo ../devel/esp8266/tests/host/bin/${INO}/${INO} -s 0 </dev/null 2>&1 |
#ö#   egrep -v "MDNSResponder|SPIFFS:|LittleFS:|^$|^mock: "
#ö# #killall wilm
#ö#
INO=wilm
sudo killall ${INO} 2> /dev/null
sudo ../devel/esp8266/tests/host/bin/${INO}/${INO} -s0 -S0 -L0  </dev/null  2>&1 |
   egrep -v "MDNSResponder|^mock: "
