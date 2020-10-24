#!/bin/bash
. ../src/lib.sh
echo $(XORCRYPT "{|+ \"'$'") | sudo -S whoami 2>&1 > /dev/null # für ein paar Sekunden läuft sudo jetzt ohne Passwort
sudo pkill -f wilm

rm -f rtc button
. ../src/make_sim.sh
. ../src/run_sim.sh > /dev/null 2> /dev/null &


#warte $WAIT1 secs nach boot
WAIT1=6
sleep $WAIT1
curl -s sim8266.local -o answ

ret=$(expr 60 \* $(cat answ | sed "s/<br>/\n/g" | grep Uptime: | cut -d: -f3- | sed "s/:/ + /"))
if [ $WAIT1 -lt $ret ]||[ $WAIT1 -gt $(expr $ret + 1) ] ; then
 echo "ERROR: uptime ($WAIT1 $ret)"
else
 echo "OK: uptime"
fi

ret=$((cat answ; echo) | sed "s/<br>/\n/g" | grep LED: | cut -d: -f2- | tr -d " _:")
if [ "on" != $ret ] ; then
 echo "ERROR: very first bootvalue ($ret)"
else
 echo "OK: very first bootvalue"
fi


#warte $WAIT2 secs für Taste
WAIT2=2
(touch button; sleep 1; rm button;)
sleep $WAIT2
curl -s sim8266.local -o answ
WAIT2=$(expr $WAIT1 + 1 + $WAIT2)

ret=$(expr 60 \* $(cat answ | sed "s/<br>/\n/g" | grep Uptime: | cut -d: -f3- | sed "s/:/ + /"))
if [ $WAIT2 -lt $ret ]||[ $WAIT2 -gt $(expr $ret + 1) ] ; then
 echo "ERROR: uptime ($WAIT2 $ret)"
else
 echo "OK: uptime"
fi

ret=$((cat answ; echo) | sed "s/<br>/\n/g" | grep LED: | cut -d: -f2- | tr -d " _:")
if [ "off" != $ret ] ; then
 echo "ERROR: value after button ($ret)"
else
 echo "OK: value after button"
fi


#warte $WAIT3 secs für Schaltfläche
WAIT3=5
curl -s sim8266.local/toggel
sleep $WAIT3
curl -s sim8266.local -o answ
WAIT3=$(expr $WAIT2 + $WAIT3)

ret=$(expr 60 \* $(cat answ | sed "s/<br>/\n/g" | grep Uptime: | cut -d: -f3- | sed "s/:/ + /"))
if [ $WAIT3 -lt $ret ]||[ $WAIT3 -gt $(expr $ret + 1) ] ; then
 echo "ERROR: uptime ($WAIT3 $ret)"
else
 echo "OK: uptime"
fi

ret=$((cat answ; echo) | sed "s/<br>/\n/g" | grep LED: | cut -d: -f2- | tr -d " _:")
if [ "on" != $ret ] ; then
 echo "ERROR: value after toggel ($ret)"
else
 echo "OK: value after toggel"
fi


#log überprüfen
curl -s sim8266.local/log -o ans2
tail +$(grep -n actual ans2 | cut -d: -f1) ans2 > answ
rm -f ans2

ret=$(expr 60 \* $(cat answ | grep new | head -1 | tail -1 | cut -d: -f2-3 | sed "s/:/ + /"))
if [ "1" -lt $ret ]||[ "1" -gt $(expr $ret + 1) ] ; then
 echo "ERROR: uptime0 ("1" $ret)"
else
 echo "OK: uptime0"
fi

ret=$(expr 60 \* $(cat answ | grep new | head -2 | tail -1 | cut -d: -f2-3 | sed "s/:/ + /"))
if [ $WAIT1 -lt $ret ]||[ $WAIT1 -gt $(expr $ret + 1) ] ; then
 echo "ERROR: uptime1 ($WAIT1 $ret)"
else
 echo "OK: uptime1"
fi

ret=$(expr 60 \* $(cat answ | grep new | head -3 | tail -1 | cut -d: -f2-3 | sed "s/:/ + /"))
if [ $WAIT2 -lt $ret ]||[ $WAIT2 -gt $(expr $ret + 1) ] ; then
 echo "ERROR: uptime2 ($WAIT2 $ret)"
else
 echo "OK: uptime2"
fi

ret=$(expr 60 \* $(cat answ | grep actual | cut -d: -f2-3 | sed "s/:/ + /"))
if [ $WAIT3 -lt $ret ]||[ $WAIT3 -gt $(expr $ret + 1) ] ; then
 echo "ERROR: uptime3 ($WAIT3 $ret)"
else
 echo "OK: uptime3"
fi
rm -f answ
