#!/bin/bash
. ../src/lib.sh
echo $(XORCRYPT "{|+ \"'$'") | sudo -S whoami 2>&1 > /dev/null # für ein paar Sekunden läuft sudo jetzt ohne Passwort
sudo pkill -f wilm

rm -f rtc button
. ../src/make_sim.sh
. ../src/run_sim.sh > /dev/null 2> /dev/null &
(cd fbsim; make)


#warte boot
sleep 6
#fbsim
./fbsim/fbsim u_x #u_x_c_s3_v
sleep 3
#log
echo "log-answer:"
curl -s sim8266.local/log

