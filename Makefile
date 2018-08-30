# the following specifies the path to your MOD...

CPP = gcc-4.8

VPATH=.:./Config

BASEFLAGS = -Dstricmp=strcasecmp -Dstrcmpi=strcasecmp

#-D_DEBUG

SDKDIR = ./hlsdk

#Doesn't support any optimisations for -O1 nor -O2?

CPPFLAGS = ${BASEFLAGS} -mtune=generic -march=i686 -msse -msse2 -m32 -mfpmath=sse -s -pipe -funsafe-math-optimizations \
		-Wall -I. -I./Config -I./dlls -I$(SDKDIR)/engine -I$(SDKDIR)/common -I$(SDKDIR)/pm_shared -I$(SDKDIR)/dlls

OBJ = bot.o bot_client.o bot_combat.o bot_navigate.o bot_start.o \
	dll.o engine.o h_export.o linkfunc.o util.o waypoint.o \
	Config.o lex.yy.o config.tab.o mb_hud.o PathMap.o

marine_bot.so: ${OBJ}
	${CPP} -fPIC -shared -o $@ ${OBJ} -ldl

clean:
	-rm -f *.o
	-rm -f *.so	

%.o:	%.cpp
	${CPP} ${CPPFLAGS} -c $< -o $@

%.o:	%.c
	${CPP} ${CPPFLAGS} -c $< -o $@
