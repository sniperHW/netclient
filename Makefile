CFLAGS   = -g -fno-strict-aliasing -Wall -std=c++0x
LDFLAGS  = -llua 
INCLUDE  = -I./ -I./deps

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(uname_S),Linux)
	LDFLAGS += -ldl -lpthread
	DEFINE  += -D_LINUX
endif

ifeq ($(uname_S),FreeBSD)
	LDFLAGS += -lexecinfo -lpthread
	DEFINE  += -D_BSD
endif

ifeq ($(findstring MINGW32_NT,$(uname_S)),MINGW32_NT)
	LDFLAGS += -lws2_32
	DEFINE  += -D_WIN
endif

source   =\
main.cpp\
SysTime.cpp\
LuaPacket.cpp\
NetLua.cpp\
Reactor.cpp\
RPacket.cpp\
Socket.cpp


all:$(source)
	g++ $(SHARED) $(CFLAGS) -c $(source) $(DEFINE) $(INCLUDE)
	g++ $(SHARED) $(CFLAGS) -o LuaNet *.o $(LDFLAGS) ./deps/http-parser/libhttp_parser.a 
	rm *.o

testmysql:example/testmysql.c
	gcc -g -o testmysql example/testmysql.c ./deps/mysql/lib/libmysql.lib  -I./deps 
