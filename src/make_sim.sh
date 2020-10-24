(
  pwd=$(pwd)
  cd ../devel/esp8266/tests/host/;
  make -f Makefile MKFLAGS=-Wextra INODIR=${pwd} INO=wilm bin/wilm/wilm
)
rm -f rtc



