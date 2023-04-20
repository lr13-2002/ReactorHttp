#include "HttpResponse.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ResHeaderSize 16;
HttpResponse::HttpResponse() {
	m_statusCode = StatusCode::Unknown;
	m_filename = "";
	m_headers.clear();
	sendDataFunc = nullptr;
}
HttpResponse::~HttpResponse() {

}

void HttpResponse::AddHeader(const string key, const string value) {
	if (key.empty() || value.empty()) {
		return ;
	}
	m_headers.insert({ key, value });
}

void HttpResponse::PrepareMsg(Buffer* sendBuf, int socket) {
	//状态行
	char tmp[1024] = { 0 };
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", m_statusCode, m_info.at((int)m_statusCode));
	sendBuf->AppendString(tmp);
	//响应头
	for (auto [x,y] : m_headers) {
		sprintf(tmp, "%s: %s\r\n", x.data(), y.data());
		sendBuf->AppendString(tmp);
	}
	//空行
	sendBuf->AppendString("\r\n");
#ifndef MSG_SEND_AUTO
	sendBuf->SendData(socket);
#endif // !MSG_SEND_AUTO


	//回复的数据
	sendDataFunc(m_filename, sendBuf, socket);

}
