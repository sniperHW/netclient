#ifndef _BYTEBUFFER_H
#define _BYTEBUFFER_H

#include <iostream>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include "chk_order.h"

#ifdef _WIN
#include <Windows.h>
#endif

namespace net{


class ByteBuffer{

public:

	ByteBuffer(size_t size):buffer(size),refCount(1){}

	ByteBuffer(const ByteBuffer& o):buffer(o.buffer.capacity()),refCount(1){
		memcpy((void*)&buffer[0],(void*)&o.buffer[0],o.buffer.capacity());
	}

	size_t Cap() const{
		return buffer.capacity();
	}

	std::vector<char>& Buf(){
		return buffer;
	}

	ByteBuffer* IncRef(){
#ifdef _WIN
		InterlockedIncrement(&refCount);
#else
		__sync_add_and_fetch(&refCount,1);
#endif
		return this;
	}

	void DecRef(){
#ifdef _WIN
		if(InterlockedDecrement(&refCount) <= 0)
#else
		if(__sync_sub_and_fetch(&refCount,1) <=0 )
#endif
			delete this;		
	}

	void WriteUint8(size_t pos,unsigned char v){
		write<unsigned char>(pos,v);
	}

	void WriteUint16(size_t pos,unsigned short v){

		write<unsigned short>(pos,chk_hton16(v));
	}

	void WriteUint32(size_t pos,unsigned int v){
		write<unsigned int>(pos,chk_hton32(v));
	}

	void WriteUint64(size_t pos,unsigned long long v){
		write<unsigned long long>(pos,chk_hton64(v));
	}

	void WriteDouble(size_t pos,double v){
		write<double>(pos,v);
	}

	void WriteFloat(size_t pos,float v){
		write<float>(pos,v);
	}

	void WriteBin(size_t pos,void *v,size_t size){
		size_t cap = buffer.capacity();
		if (cap < pos + size){
			if(cap >= size)	
				buffer.resize(cap*2);
			else
				buffer.resize(cap+size);
		}
		memcpy((void*)&buffer[pos],v,size);
	}

	void WriteString(size_t pos,const char *v){
		WriteBin(pos,(void*)v,strlen(v)+1);
	}

	unsigned char ReadUint8(size_t pos) const{
		return read<unsigned char>(pos);
	}

	unsigned short ReadUint16(size_t pos) const{
		return chk_ntoh16(read<unsigned short>(pos));
	}

	unsigned int ReadUint32(size_t pos) const{
		return chk_ntoh32(read<unsigned int>(pos));
	}

	char ReadInt8(size_t pos) const{
		return read<char>(pos);
	}

	short ReadInt16(size_t pos) const{
		return (short)chk_ntoh16((uint16_t)read<short>(pos));
	}	

	int ReadInt32(size_t pos) const{
		return (int)chk_ntoh32((uint32_t)(read<int>(pos)));
	}

	unsigned long long ReadUint64(size_t pos) const{
		return (unsigned long long)chk_ntoh64((uint64_t)(read<unsigned long long>(pos)));
	}

	float ReadFloat(size_t pos) const{
		return read<float>(pos);
	}

	double ReadDouble(size_t pos) const {
		return read<double>(pos);
	}

	void *ReadBin(size_t pos) const {
		if(pos < buffer.capacity())
			return (void*)&buffer[pos];
		return NULL;
	}

	const char *ReadString(size_t pos) const{
		return (const char *)ReadBin(pos);
	}

private:

	template<typename T>
	void write(size_t pos,const T &v){
		size_t cap = buffer.capacity();
		if (cap <= pos + sizeof(T)){
			if(cap >= sizeof(T))	
				buffer.resize(cap*2);
			else
				buffer.resize(cap+sizeof(T));
		}
		(*((T*)&buffer[pos])) = v;
	}

	template<typename T>
	T read(size_t pos) const{
		if(pos+sizeof(T) > buffer.capacity())
			return T();
		return (*((T*)&buffer[pos]));
	}

	ByteBuffer& operator = (const ByteBuffer&);
	~ByteBuffer(){}
	std::vector<char> buffer;
	volatile long refCount;
};

}


#endif
