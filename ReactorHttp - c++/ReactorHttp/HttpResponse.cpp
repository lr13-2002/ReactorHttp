#include "HttpResponse.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>

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
	int code = static_cast<int>(m_statusCode);
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", code, m_info.at(code).data());
	cout << tmp << endl;
	sendBuf->AppendString(tmp);
	//响应头
	for (auto it = m_headers.begin(); it != m_headers.end(); ++it) {
		sprintf(tmp, "%s: %s\r\n", it->first.data(), it->second.data());
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
