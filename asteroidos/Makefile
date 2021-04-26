# gpSP makefile
# Gilead Kutnick - Exophase
# pandora port - notaz
# respberry pi - DPR

# Global definitions

#CC        = gcc

OBJS      = asteroid.o main.o cpu.o memory.o video.o input.o sound.o gui.o \
            cheats.o zip.o  arm_stub.o warm.o cpu_threaded.o\
	    hwcomposer.o gles_video.o video_blend.o

BIN       = gpsp

# Platform specific definitions 

VPATH      += .. ../arm
CFLAGS     += -DARM_ARCH -DASTEROID_BUILD -DQCOM_BSP -DHWC_PLUGIN_HAVE_HWCOMPOSER1_API  -Wall
CFLAGS     += -O3 -marm -mcpu=cortex-a7
CFLAGS     += `pkg-config sdl2 --cflags`
CFLAGS+=-Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed
CFLAGS+=`pkg-config android-headers --cflags`
CFLAGS+=`pkg-config libhardware --cflags`
CFLAGS+=`pkg-config hybris-egl-platform --cflags`
CFLAGS+=`pkg-config hwcomposer-egl --cflags`
CXXFLAGS += $(CFLAGS) -fpermissive

# expecting to have PATH set up to get correct sdl-config first

LIBS       += `pkg-config sdl2 --libs`
LIBS       += -ldl -lz
LIBS +=-lpthread
LIBS +=`pkg-config libsync --libs`
LIBS +=`pkg-config android-headers --libs`
LIBS +=`pkg-config libhardware --libs`
LIBS +=`pkg-config hybris-egl-platform --libs`
LIBS +=`pkg-config hwcomposer-egl --libs`
LIBS += -lGLESv2 -lEGL

# Compilation:

all: $(BIN)



%.o: %.S
	$(CXX) $(CFLAGS) -c -o $@ $<


cpu.o cpu_threaded.o: CFLAGS += -Wno-unused-variable -Wno-unused-label

$(BIN): $(OBJS)
	$(CXX) $(OBJS) $(LIBS) -o $(BIN)

clean:
	rm -f *.o $(BIN)
