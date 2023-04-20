#pragma once
#include "Buffer.h"
#include <map>
#include <functional>
//定义状态码枚举
enum class StatusCode {
	Unknown = 0,
	OK = 200,
	MovedPermanently = 301,
	MovedTemporarily = 302,
	BadRequest = 400,
	NotFound = 404
};
//定义一个函数指针，用来组织要恢复的客户端的数据块
//定义结构体
class HttpResponse {
public:
	HttpResponse();
	~HttpResponse();
	function<void(const string, Buffer*, int)> sendDataFunc;
	//添加响应头
	void AddHeader(const string key, const string value);
	//组织 http 响应数据
	void PrepareMsg(Buffer* sendBuf, int socket);
	inline void setFileName(string name) {
		m_filename = name;
	}
	inline void setStatusCode(StatusCode statusCode) {
		m_statusCode = statusCode;
	}
private:
	//状态行：状态码，状态描述
	StatusCode m_statusCode;
	string m_filename;
	//响应头-键值对
	map<string, string> m_headers;
	//定义状态码和描述的对应关系
	const map<int, string> m_info = {
		{200,"OK"},
		{301,"MovedPermanently"},
		{302,"MovedTemporarily"},
		{400,"BadRequest"},
		{404,"NotFound"}
	};
};
