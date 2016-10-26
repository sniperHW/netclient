CFLAGS   = -g -fno-strict-aliasing -Wall -std=c++0x
LDFLAGS  = -llua 
INCLUDE  = -I./ -I./deps -I./deps/lua-5.3.0/src
LIBRARY  = -L./deps/lua-5.3.0/src
HTTP_PARSER = ./deps/http-parser/libhttp_parser.a
MAKE    =

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(uname_S),Linux)
	LDFLAGS += -ldl -lpthread
	DEFINE  += -D_LINUX
endif

ifeq ($(uname_S),Darwin)
	MAKE += make
	CC += clang
	DEFINE += -D_MACH
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

all:$(source) $(HTTP_PARSER)
	g++ $(SHARED) $(CFLAGS) -c $(source) $(DEFINE) $(INCLUDE)
	g++ $(SHARED) $(CFLAGS) -o LuaNet *.o $(LDFLAGS) $(HTTP_PARSER) $(LIBRARY) 
	rm *.o

$(HTTP_PARSER):
	cd ./deps/http-parser/;$(MAKE) package

testmysql:example/testmysql.c
	gcc -g -o testmysql example/testmysql.c ./deps/mysql/lib/libmysql.lib  -I./deps 
