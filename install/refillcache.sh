mkdir -p cache
rm -rf cache
mkdir -p cache
(
cd cache
ID=3d128e5c785cbe2096a0def394554d1d8091601d
wget https://github.com/esp8266/Arduino/archive/${ID}.zip
ID=de30f21a222ec62f5a023dd955439b4f57702768
wget https://github.com/espressif/esptool/archive/${ID}.zip
ID=354887a25f83064dc0c795e11704190845812713
wget https://github.com/d-a-v/esp82xx-nonos-linklayer/archive/${ID}.zip
ID=89454af34e3e61ddfc9837f3da5a0bc8ed44c3aa
wget https://github.com/earlephilhower/bearssl-esp8266/archive/${ID}.zip
ID=9da4d3729a57a181307bf9acf73473d052a38874
wget https://github.com/plerup/espsoftwareserial/archive/${ID}.zip
ID=6b65737715039ef92d348014316b575b52547019
wget https://github.com/ARMmbed/littlefs/archive/${ID}.zip
ID=b240d2231a117bbd89b79902eb54cae948ee2f42
wget https://github.com/earlephilhower/ESP8266SdFat/archive/${ID}.zip
ID=1c6fe4669bb615f7040aef866db6db8a45dc098a
wget https://github.com/plerup/makeEspArduino/archive/${ID}.zip
ID=x86_64-linux-gnu.xtensa-lx106-elf-b40a506.1563313032
wget https://github.com/earlephilhower/esp-quick-toolchain/releases/download/2.5.0-4/${ID}.tar.gz
)