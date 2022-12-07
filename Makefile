
CPP = g++

VPATH=.:./Config


BASEFLAGS = -Dstricmp=strcasecmp -Dstrcmpi=strcasecmp

# Marine Bot source code is compatible with original HL SDK v2.3
# so if you're using the new Half-Life SDK from Allied Modders then
# use following line
#BASEFLAGS += -DNEWSDKAM

# if you're using the new Valve Half-Life SDK released in year 2013
# then use following line
#BASEFLAGS += -DNEWSDKVALVE

# if you're using Metamod patched HL SDK v2.3 then
# use following line
#BASEFLAGS += -DNEWSDKMM


# if you wish to compile debugging version of Marine Bot
# (it's not meant for general use on a public server) then
# use the following line
#BASEFLAGS += -D_DEBUG

CPPFLAGS = ${BASEFLAGS} -mtune=generic -march=i686 -mmmx -msse -msse2 -mfpmath=387 -m32 -O1 \
	-std=gnu++11 -Wno-write-strings -Wno-attributes \
	-I. -I./Config -I./engine -I./common -I./pm_shared -I./dlls -I./public

OBJ = bot.o bot_client.o bot_combat.o bot_manager.o bot_navigate.o \
	bot_start.o client_commands.o dll.o engine.o h_export.o linkfunc.o \
	mb_hud.o namefunc.o util.o waypoint.o \
	Config.o lex.yy.o config.tab.o

LFLAGS = -fPIC -shared

# if the server crashes right at the start with errors such as
# "Game DLL version mismatch" or
# "libstdc++.so.6: version `CXXABI_1.3.9' not found" then
# use the following line to link the libstdc++ statically
#LFLAGS += -static-libstdc++

marine_bot.so: ${OBJ}
	${CPP} ${LFLAGS} -o $@ ${OBJ} -ldl -m32

clean:
	-rm -f *.o
	-rm -f *.so	

%.o:	%.cpp
	${CPP} ${CPPFLAGS} -c $< -o $@

%.o:	%.c
	${CPP} ${CPPFLAGS} -c $< -o $@

