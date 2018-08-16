# makefile for rtkweb

RTKLIB_SRCS := ./RTKLIB/src

VPATH := ./RTKLIB/src:./RTKLIB/src/rcv:.


# for beagleboard
#CTARGET= -mfpu=neon -mfloat-abi=softfp -ffast-math
CTARGET := -DENAGLO -DENAGAL -DENAQZS -DENACMP -DNFREQ=3

CFLAGS := -fPIC -std=c99 -Wall -O3 -ansi -pedantic -Wno-unused-but-set-variable -DTRACE $(CTARGET) -g -I$(RTKLIB_SRCS)
LDLIBS := -lm -lrt -lpthread

OBJS := rtkcmn.o rtksvr.o rtkpos.o geoid.o solution.o lambda.o \
        sbas.o stream.o rcvraw.o rtcm.o preceph.o options.o pntpos.o ppp.o ppp_ar.o \
        novatel.o ublox.o ss2.o crescent.o skytraq.o gw10.o javad.o nvs.o binex.o \
        rt17.o ephemeris.o rinex.o ionex.o rtcm2.o rtcm3.o rtcm3e.o qzslex.o rtkweb.o

OBJS := $(addprefix build/, $(OBJS))

TARGET1 := build/test_rtkweb
TARGET2 := plugins/libprotocol_rtkweb.so

$(shell mkdir -p build)

.PHONY: all clean

all : $(TARGET1) $(TARGET2)

clean:
	rm -rf build $(TARGET1) $(TARGET2) *.o rtkweb.nav *.out *.trace

$(TARGET1)  : test_rtkweb.c $(OBJS)
	$(CC) -o $@ $< $(OBJS) $(CFLAGS) $(LDLIBS)

$(TARGET2) : protocol_rtkweb.c $(OBJS)
	$(CC) -o $@ $< $(OBJS) -fPIC -Wall -O3 -pthread -shared -Wl,-soname,$@ $(LDLIBS) -lwebsockets -I$(RTKLIB_SRCS)

build/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

build/rtkweb.o   : rtklib.h rtkweb.h
build/rtkcmn.o   : rtklib.h
build/rtksvr.o   : rtklib.h
build/rtkpos.o   : rtklib.h
build/geoid.o    : rtklib.h
build/solution.o : rtklib.h
build/lambda.o   : rtklib.h
build/sbas.o     : rtklib.h
build/rcvraw.o   : rtklib.h
build/rtcm.o     : rtklib.h
build/rtcm2.o    : rtklib.h
build/rtcm3.o    : rtklib.h
build/rtcm3e.o   : rtklib.h
build/preceph.o  : rtklib.h
build/options.o  : rtklib.h
build/pntpos.o   : rtklib.h
build/ppp.o      : rtklib.h
build/novatel.o  : rtklib.h
build/ublox.o    : rtklib.h
build/ss2.o      : rtklib.h
build/crescent.o : rtklib.h
build/skytraq.o  : rtklib.h
build/gw10.o     : rtklib.h
build/javad.o    : rtklib.h
build/nvs.o      : rtklib.h
build/binex.o    : rtklib.h
build/rt17.o     : rtklib.h
build/ephemeris.o: rtklib.h
build/rinex.o    : rtklib.h
build/ionex.o    : rtklib.h
build/qzslex.o   : rtklib.h
build/stream.o   : rtklib.h
build/ppp_ar.o   : rtklib.h
