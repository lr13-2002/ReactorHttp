#include "Buffer.h"
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>

Buffer::Buffer(int size) : m_capacity(size) {
	m_data = (char*)malloc(size);
	bzero(m_data, size);
}

Buffer::~Buffer() {
	if (m_data != nullptr) {
		free(m_data);
	}
}

void Buffer::extendRoom(int size) {
	//内存不够用 不需要扩容
	if (WriteableSize() >= size) {
		return;
	}
	//内存需要合并才够用
	else if (m_readPos + WriteableSize() >= size) {
		int readable = ReadableSize();
		memcpy(m_data, m_data + m_readPos, readable);
		m_writePos = readable;
		m_readPos = 0;
	}
	//内存不够用扩容
	else {
		void* temp = realloc(m_data, m_capacity + size);
		if (temp == nullptr) {
			return;
		}
		memset((char*)temp + m_capacity, 0, size);
		m_data = static_cast<char*>(temp);
		m_capacity += size;
	}
}

int Buffer::AppendString(const char* data, int size) {
	if (data == nullptr || size <= 0) {
		return -1;
	}
	//扩容
	extendRoom(size);
	//数据拷贝
	memcpy(m_data + m_writePos, data, size);
	m_writePos += size;
	return 0;
}

int Buffer::AppendString(const char* data) {
	int size = strlen(data);
	return AppendString(data, size);
}

int Buffer::AppendString(const string data) {
	return AppendString(data.data());
}

int Buffer::SocketRead(int fd) {
	//read/recv/readv;
	struct iovec vec[2];
	int writeable = WriteableSize();
	vec[0].iov_base = m_data + m_writePos;
	vec[0].iov_len = writeable;
	char* tmpbuf = (char*)malloc(40960);
	vec[1].iov_base = m_data + m_writePos;
	vec[1].iov_len = 40960;
	int result = readv(fd, vec, 2);
	if (result == -1) {
		return -1;
	}
	else if (result <= writeable) {
		m_writePos += result;
	}
	else {
		m_writePos = m_capacity;
		AppendString(tmpbuf, result - writeable);
	}
	free(tmpbuf);
	return result;
}

char* Buffer::FindCRLF() {
	//strstr  大字符串中匹配子字符串 遇到\0 结束

	//memmem  大数据块中匹配子数据块 需要制定数据块大小
	char* ptr = (char*)memmem(m_data + m_readPos, ReadableSize(), "\r\n", 2);
	return ptr;
}

int Buffer::SendData(int socket) {
	//判断有无数据
	int readable = ReadableSize();
	if (readable > 0) {
		int count = send(socket, m_data + m_readPos, readable, MSG_NOSIGNAL);
		if (count) {
			m_readPos += count;
			usleep(1);
		}
		return count;
	}
	return 0;
}
