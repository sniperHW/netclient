#ifndef _REACTOR_H
#define _REACTOR_H

#include <map>
#include "Socket.h"
namespace net{

class Reactor{

public:
	Reactor(){}
	void LoopOnce(unsigned int ms = 0);
	bool Add(Socket*,int event);
	bool Remove(Socket*,int event);
private:
	Reactor(const Reactor&);
	Reactor& operator = (const Reactor&);
	dlist sockets;
};
}



#endif
