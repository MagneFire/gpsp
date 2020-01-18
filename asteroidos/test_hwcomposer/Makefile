# glestest makefile
# Global definitions

CC        ?= gcc

OBJS      = test_hwcomposer.o

BIN       = glestest

# Platform specific definitions 

CFLAGS+=-D_LINUX

LIBS +=-L$(SDKSTAGE)/opt/vc/lib/ -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt 


#INCLUDES+=INC_STAGE
INCLUDES+=`pkg-config android-headers --cflags`
INCLUDES+=`pkg-config libhardware --cflags`
INCLUDES+=`pkg-config hybris-egl-platform --cflags`
INCLUDES+=`pkg-config hwcomposer-egl --cflags`
#INCLUDES+=INC_STAGE/hybris/hwcomposerwindow
#INCLUDES+=-I$(SDKSTAGE)/opt/vc/include -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads

# Compilation:

all:	$(BIN)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BIN): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(BIN)

clean:
	rm -f *.o $(BIN)

