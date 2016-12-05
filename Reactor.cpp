#include "Reactor.h"
#include "SysTime.h"
namespace net{

bool Reactor::Add(Socket *s,int event)
{
	if(s->reactor && s->reactor != this)
		return false;
	s->event |= event;
	s->reactor = this;
	sockets.Push(s);
	return true;
}

bool Reactor::Remove(Socket *s,int event)
{
	if(!s->reactor || s->reactor != this)
		return false;

	s->event &= (~event);
	if(s->event == 0)
		sockets.Remove(s);
	return true;
}

void Reactor::LoopOnce(unsigned int ms){
	fd_set r_set;
	fd_set w_set;
	fd_set e_set;
	int maxfd = 0;
	dnode *node = sockets.Begin();
	dnode *end  = sockets.End();		
	FD_ZERO(&r_set);
	FD_ZERO(&w_set);
	FD_ZERO(&e_set);
	for(; node != end;) 
	{
		Socket *s = (Socket*)node;
		if(s->Event() & EV_READ)
			FD_SET(s->Fd(),&r_set);
		if(s->Event() & EV_WRITE)
			FD_SET(s->Fd(),&w_set);
		FD_SET(s->Fd(),&e_set);	
		if((int)s->Fd() > maxfd)
			maxfd = s->Fd();
		node = node->next;
	}
	struct timeval timeout;
	timeout.tv_sec = ms/1000;
	timeout.tv_usec = (ms%1000)*1000;
	int n;
#ifdef _WIN
	if((n= TEMP_FAILURE_RETRY(::select(0,&r_set,&w_set,&e_set, &timeout))) > 0)
#else
	int fd_setsize = (maxfd + 1) < FD_SETSIZE ? (maxfd + 1) : FD_SETSIZE;
	if((n= TEMP_FAILURE_RETRY(::select(fd_setsize,&r_set,&w_set,&e_set, &timeout))) > 0)
#endif
	{
		size_t size = sockets.Size();
		while(size){
			Socket *s = (Socket*)sockets.Pop();
			s->IncRef();
			if(FD_ISSET(s->Fd(), &e_set) || ((s->Event() & EV_READ) && FD_ISSET(s->Fd(), &r_set)))
				s->onReadAct();
			if((s->Event() & EV_WRITE) && FD_ISSET(s->Fd(), &w_set))
				s->onWriteAct();
			if(s->state != closeing && s->state != 0)
				sockets.Push(s);
			s->DecRef();
			--size;
		}
	}
}
}
