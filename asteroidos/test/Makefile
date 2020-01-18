# glestest makefile
# Global definitions

CC        ?= gcc

OBJS      = gles_video.o test.o

BIN       = glestest

# Platform specific definitions 

CFLAGS+=-D_LINUX -fpermissive

#LIBS +=-L$(SDKSTAGE)/opt/vc/lib/ -lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt 
LIBS +=-lpthread
LIBS +=`pkg-config libsync --libs`
LIBS +=`pkg-config android-headers --libs`
LIBS +=`pkg-config libhardware --libs`
LIBS +=`pkg-config hybris-egl-platform --libs`
LIBS +=`pkg-config hwcomposer-egl --libs`
LIBS += -lGLESv2 -lEGL
#LIBS +=-Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed


#INCLUDES+=INC_STAGE
INCLUDES+=-Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed
INCLUDES+=`pkg-config android-headers --cflags`
INCLUDES+=`pkg-config libhardware --cflags`
INCLUDES+=`pkg-config hybris-egl-platform --cflags`
INCLUDES+=`pkg-config hwcomposer-egl --cflags`
#INCLUDES+=INC_STAGE/hybris/hwcomposerwindow
#INCLUDES+=-I$(SDKSTAGE)/opt/vc/include -I$(SDKSTAGE)/opt/vc/include/interface/vcos/pthreads

# Compilation:

all:	$(BIN)

%.o: %.c
	$(CXX) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(BIN): $(OBJS)
	$(CXX) $(OBJS) $(LIBS) -o $(BIN)

clean:
	rm -f *.o $(BIN)

