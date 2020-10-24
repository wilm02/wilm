ROOTDIR=$(pwd)/..
TMPDIR=/tmp

mkdir -p ${TMPDIR}/install/cache

touch ${TMPDIR}/devel
rm -rf ${TMPDIR}/devel
mkdir -p ${TMPDIR}/devel

ID=3d128e5c785cbe2096a0def394554d1d8091601d
##wget https://github.com/esp8266/Arduino/archive/${ID}.zip
(cd ${TMPDIR}
  unzip ${ROOTDIR}/install/cache/${ID}.zip
  mv Arduino-${ID} esp8266
  mv esp8266 ${TMPDIR}/devel
  sleep 1
  cp  ${TMPDIR}/devel/esp8266/libraries/ESP8266WiFi/src/WiFiClient.cpp      ${TMPDIR}/devel/esp8266/tests/host/common/MockWiFiClient.cpp # Original holen zum Ändern
  patch ${TMPDIR}/devel/esp8266/tests/host/common/MockWiFiClient.cpp        ${ROOTDIR}/install/MockWiFiClient.patch
  patch ${TMPDIR}/devel/esp8266/tests/host/Makefile                         ${ROOTDIR}/install/Makefile.patch
  patch ${TMPDIR}/devel/esp8266/tests/host/common/Arduino.cpp               ${ROOTDIR}/install/Arduino.patch          
  patch ${TMPDIR}/devel/esp8266/tests/host/common/HostWiring.cpp            ${ROOTDIR}/install/HostWiring.patch       
  patch ${TMPDIR}/devel/esp8266/tests/host/common/MockWiFiServerSocket.cpp  ${ROOTDIR}/install/MockWiFiServerSocket.patch
  patch ${TMPDIR}/devel/esp8266/tests/host/common/user_interface.cpp        ${ROOTDIR}/install/user_interface.patch
)

ID=de30f21a222ec62f5a023dd955439b4f57702768
##wget https://github.com/espressif/esptool/archive/${ID}.zip
(cd ${TMPDIR}
  unzip ${ROOTDIR}/install/cache/${ID}.zip
  mv esptool-${ID} esptool
  rmdir ${TMPDIR}/devel/esp8266/tools/esptool
  mv esptool ${TMPDIR}/devel/esp8266/tools
)

ID=354887a25f83064dc0c795e11704190845812713
##wget https://github.com/d-a-v/esp82xx-nonos-linklayer/archive/${ID}.zip
(cd ${TMPDIR}
  unzip ${ROOTDIR}/install/cache/${ID}.zip
  mv esp82xx-nonos-linklayer-${ID} builder
  rmdir ${TMPDIR}/devel/esp8266/tools/sdk/lwip2/builder
  mv builder ${TMPDIR}/devel/esp8266/tools/sdk/lwip2
)

ID=89454af34e3e61ddfc9837f3da5a0bc8ed44c3aa
##wget https://github.com/earlephilhower/bearssl-esp8266/archive/${ID}.zip
(cd ${TMPDIR}
  unzip ${ROOTDIR}/install/cache/${ID}.zip
  mv bearssl-esp8266-${ID} bearssl
  rmdir ${TMPDIR}/devel/esp8266/tools/sdk/ssl/bearssl
  mv bearssl ${TMPDIR}/devel/esp8266/tools/sdk/ssl
)

ID=9da4d3729a57a181307bf9acf73473d052a38874
##wget https://github.com/plerup/espsoftwareserial/archive/${ID}.zip
(cd ${TMPDIR}
  unzip ${ROOTDIR}/install/cache/${ID}.zip
  mv espsoftwareserial-${ID} SoftwareSerial
  rmdir ${TMPDIR}/devel/esp8266/libraries/SoftwareSerial
  mv SoftwareSerial ${TMPDIR}/devel/esp8266/libraries
)

ID=6b65737715039ef92d348014316b575b52547019
##wget https://github.com/ARMmbed/littlefs/archive/${ID}.zip
(cd ${TMPDIR}
  unzip ${ROOTDIR}/install/cache/${ID}.zip
  mv littlefs-${ID} littlefs
  rmdir ${TMPDIR}/devel/esp8266/libraries/LittleFS/lib/littlefs
  mv littlefs ${TMPDIR}/devel/esp8266/libraries/LittleFS/lib
)

ID=b240d2231a117bbd89b79902eb54cae948ee2f42
##wget https://github.com/earlephilhower/ESP8266SdFat/archive/${ID}.zip
(cd ${TMPDIR}
  unzip ${ROOTDIR}/install/cache/${ID}.zip
  mv ESP8266SdFat-${ID} ESP8266SdFat
  rmdir ${TMPDIR}/devel/esp8266/libraries/ESP8266SdFat
  mv ESP8266SdFat ${TMPDIR}/devel/esp8266/libraries
)

ID=1c6fe4669bb615f7040aef866db6db8a45dc098a
##wget https://github.com/plerup/makeEspArduino/archive/${ID}.zip
(cd ${TMPDIR}
  unzip ${ROOTDIR}/install/cache/${ID}.zip
  mv makeEspArduino-${ID} makeEspArduino
  rmdir ${TMPDIR}/devel/makeEspArduino
  mv makeEspArduino ${TMPDIR}/devel
)

ID=x86_64-linux-gnu.xtensa-lx106-elf-b40a506.1563313032
##wget https://github.com/earlephilhower/esp-quick-toolchain/releases/download/2.5.0-4/${ID}.tar.gz
(cd ${TMPDIR}
  tar xvzf ${ROOTDIR}/install/cache/${ID}.tar.gz
  mv xtensa-lx106-elf ${TMPDIR}/devel/esp8266/tools
)

touch ${ROOTDIR}/devel
rm -rf ${ROOTDIR}/devel
cp -PrL ${TMPDIR}/devel ${ROOTDIR}/devel  # mit -L werden symbolische Links als eigene Datei kopiert