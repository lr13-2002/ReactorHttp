#define _GNU_SOURCE
#include "Buffer.h"
#include <stdio.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>

struct Buffer* bufferInit(int size) {
	struct Buffer* buffer = (struct Buffer*)malloc(sizeof(struct Buffer));

	if (buffer != NULL) {
		buffer->data = (char*)malloc(size);
		buffer->capacity = size;
		buffer->readPos = 0;
		buffer->writePos = 0;
		memset(buffer->data, 0, sizeof(buffer->data));
	}
	return buffer;
}

void bufferDestroy(struct Buffer* buffer) {
	if (buffer != NULL) {
		free(buffer->data);
		free(buffer);
	}
}

void bufferExtendRoom(struct Buffer* buffer, int size) {
	//内存不够用 不需要扩容
	if (bufferWriteableSize(buffer) >= size) {
	}
	//内存需要合并才够用
	else if (buffer->readPos + bufferWriteableSize(buffer) >= size) {
		int readable = bufferReadableSize(buffer);
		memcpy(buffer->data, buffer->data + buffer->readPos, readable);
		buffer->writePos = readable;
		buffer->readPos = 0;
	}
	//内存不够用扩容
	else {
		void* temp = realloc(buffer->data, buffer->capacity + size);
		if (temp == NULL) {
			return;
		}
		memset(temp + buffer->capacity, 0, size);
		buffer->data = temp;
		buffer->capacity += size;
	}
}

int bufferWriteableSize(struct Buffer* buffer) {
	return buffer->capacity - buffer->writePos;
}

int bufferReadableSize(struct Buffer* buffer) {
	return buffer->writePos - buffer->readPos;
}

int bufferAppendData(struct Buffer* buffer, const char* data, int size) {
	if (buffer == NULL || data == NULL || data <= 0) {
		return -1;
	}
	//扩容
	bufferExtendRoom(buffer, size);
	//数据拷贝
	memcpy(buffer->data + buffer->writePos, data, size);
	buffer->writePos += size;
	return 0;
}

int bufferAppendString(struct Buffer* buffer, const char* data) {
	if (buffer == NULL || data == NULL || data <= 0) {
		return -1;
	}
	int size = strlen(data);
	//扩容
	bufferExtendRoom(buffer, size);
	//数据拷贝
	memcpy(buffer->data + buffer->writePos, data, size);
	buffer->writePos += size;
	return 0;
}

int bufferSocketRead(struct Buffer* buffer, int fd) {
	//read/recv/readv;
	struct iovec vec[2];
	int writeable = bufferWriteableSize(buffer);
	vec[0].iov_base = buffer->data + buffer->writePos;
	vec[0].iov_len = writeable;
	char* tmpbuf = (char*)malloc(40960);
	vec[1].iov_base = buffer->data + buffer->writePos;
	vec[1].iov_len = 40960;
	int result = readv(fd, vec, 2);
	if (result == -1) {
		return -1;
	}
	else if (result <= writeable) {
		buffer->writePos += result;
	}
	else {
		buffer->writePos = buffer->capacity;
		bufferAppendData(buffer, tmpbuf, result - writeable);
	}
	free(tmpbuf);
	return result;
}

char* bufferFindCRLF(struct Buffer* buffer) {
	//strstr  大字符串中匹配子字符串 遇到\0 结束

	//memmem  大数据块中匹配子数据块 需要制定数据块大小
	char* ptr = memmem(buffer->data + buffer->readPos, bufferReadableSize(buffer), "\r\n", 2);
	return ptr;
}

int bufferSendData(struct Buffer* buffer, int socket) {
	//判断有无数据
	int readable = bufferReadableSize(buffer);
	if (readable > 0) {
		int count = send(socket, buffer->data + buffer->readPos, readable, MSG_NOSIGNAL);
		if (count) {
			buffer->readPos += count;
			usleep(1);
		}
	}
	return 0;
}
