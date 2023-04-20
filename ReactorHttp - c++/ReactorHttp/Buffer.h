#pragma once
#include <string>
using namespace std;
struct Buffer {
public:
	Buffer(int size);
	~Buffer();
	//扩容
	void extendRoom(int size);

	//得到剩余的可写的内存容量
	inline int WriteableSize() {
		return m_capacity - m_writePos;
	}
	//得到剩余的可读的内存容量
	inline int ReadableSize() {
		return m_writePos - m_readPos;
	}
	//直接写
	int AppendString(const char* data, int size);
	int AppendString(const char* data);
	int AppendString(const string data);
	//接收套接字数据
	int SocketRead(int fd);
	//根据/r/n取出一行，找到其在数据块中的位置，返回该位置
	char* FindCRLF();
	//发送数据
	int SendData(int socket);
	//得到读数据的起始位置
	inline char* data() {
		return m_data + m_readPos;
	}

	inline int readPosIncrease(int count) {
		m_readPos += count;
		return m_readPos;
	}
private:
	char* m_data;
	int m_capacity;
	int m_readPos = 0;
	int m_writePos = 0;
};