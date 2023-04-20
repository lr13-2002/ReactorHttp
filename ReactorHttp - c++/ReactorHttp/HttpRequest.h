#pragma once
#include <stdbool.h>
#include "Buffer.h"
#include "HttpResponse.h"
#include <map>
#include <functional>

using namespace std;
//当前的解析状态
enum class PrecessState :char {
	ParseReqLine,
	ParseReqHeaders,
	ParseReqBody,
	ParseReqDone
};
//定义 http 请求结构体
class HttpRequest {
public:
	HttpRequest();
	void Reset();
	//根据 key 得到请求头的 value
	string GetHeader(const string key);
	//添加请求头
	void AddHeader(const string key, const string value);
	//解析请求行
	bool parseRequestLine(Buffer* readBuf);
	//解析请求头
	bool parseRequestHeader(struct Buffer* readBuf);
	//解析 http 请求协议
	bool parseRequest(Buffer* readBuf, HttpResponse* response, Buffer* sendBuf, int socket);

	//处理基于 get 的 http 请求协议
	bool processRequest(HttpResponse* response);

	//to存储解码之后的数据，传出参数，from被解码的数据，传入参数
	string decodeMsg(string from);

	const string getFiletype(const string name);

	static int sendDir(const string dirName, Buffer* sendBuf, int cfd);

	static void sendFile(const string fileName, Buffer* sendBuf, int cfd);

	inline void setMethod(const string method) {
		m_method = method;
	}

	inline void setUrl(string url) {
		m_url = url;
	}

	inline void setVersion(string version) {
		m_version = version;
	}

	//获取处理状态
	inline PrecessState getState() {
		return m_curState;
	}

	inline void setState(PrecessState curState) {
		m_curState = curState;
	}

private:
	char* splitRequestLine(const char* start, const char* end, const char* sub, function<void(string)> callback);
	int hexToDec(char c);
	string m_method;
	string m_url;
	string m_version;
	map<string, string>m_reqHeaders;
	PrecessState m_curState;
};

//


