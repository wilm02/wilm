# Unterscheidung, je nach Betriebssystem
ifeq ($(shell uname),Linux) # Variablen für Linux
  CFLAGS = -g -O
  LIBS =                    #-lpcap -lpthread
  EXTENSION=				#.elf
else                        # Variablen für Windows
  CFLAGS = -g -O            #-I ../../WpdPack_4_1_2/include  
  LIBS = -lws2_32           #-lwpcap -L ../../WpdPack_4_1_2/lib
  EXTENSION=.exe
endif

RUMPF = fbsim
OBJS = ${RUMPF}.o
EXE = ${RUMPF}${EXTENSION}
CC = g++

all: ${OBJS}
	${CC} ${CFLAGS} -o ${EXE} ${OBJS} ${LIBS}
	@rm ${OBJS}             # OBJS sind je nach Betriebssystem unterschiedlich

clean:
	rm -f ${OBJS} ${EXE}

.c.o:
	${CC} ${CFLAGS} -c -o $*.o $<
