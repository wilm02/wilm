#!/bin/bash
. lib.sh

## #https://github.com/plerup/makeEspArduino
mkdir -p /tmp/mkESP/pmt_generic
rm -f /tmp/mkESP/pmt.ino
cp -n wilm.ino /tmp/mkESP/pmt.ino
DATE=$(date +%Y%m%d_%H%M%S)
DEVNAME=catflap
SSID=WLAN-son2  # $(XORCRYPT "q{waudty+%!")
SSID2=adb006    # $(XORCRYPT "\"\$\$ %#\$*'%*%\"++(# % ") 
sed -i "s/^#define PROGRAMVERSION.*/#define PROGRAMVERSION \"${DATE}\"/" /tmp/mkESP/pmt.ino 
sed -i "s/^#define MYSSID.*/#define MYSSID \"${SSID}\"/" /tmp/mkESP/pmt.ino 
sed -i "s/^#define MYPASS.*/#define MYPASS \""$(XORCRYPT "q{waudty+%!")"\"/" /tmp/mkESP/pmt.ino
sed -i "s/^#define MY2SSID.*/#define MY2SSID \"${SSID2}\"/" /tmp/mkESP/pmt.ino 
sed -i "s/^#define MY2PASS.*/#define MY2PASS \""$(XORCRYPT "\"\$\$ %#\$*'%*%\"++(# % ")"\"/" /tmp/mkESP/pmt.ino
sed -i "s/^#define DEVNAME.*/#define DEVNAME \"${DEVNAME}\"/" /tmp/mkESP/pmt.ino
echo patched wilm.ino

rm -f /tmp/mkESP/wilm_generic/pmt.bin
make -f ../devel/makeEspArduino/makeEspArduino.mk ESP_ROOT=../devel/esp8266 SKETCH=/tmp/mkESP/pmt.ino #flash
if [ -f /tmp/mkESP/pmt_generic/pmt.bin ]; then
  mkdir -p bin
  SHORT=../devel/esp8266/tests/host/common
  tar czf bin/${DATE}wilm.tgz \
    wilm.ino \
	make_clean.sh \
	make_sim.sh \
	run_sim.sh \
	make_bin.sh
  cp /tmp/mkESP/pmt_generic/pmt.bin bin/${DATE}wilm.bin
  echo install with: curl -s ${DEVNAME}.local/firmware -F image=@bin/${DATE}wilm.bin
fi
