#include "Server.h"
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/sendfile.h>
#include <dirent.h>
#include <pthread.h>
struct inFo {
	int lfd, efd;
	pid_t pid;
};
int initListenLd(unsigned short port) {
	// 1. 创建监听的 fd
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("lfd");
		return -1;
	}
	// 2, 设置端口复用
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (ret == -1) {
		perror("setsockopt");
		return -1;
	}
	// 3. 绑定
	struct sockaddr_in addr;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof addr);
	if (ret == -1) {
		perror("bind");
		return -1;
	}
	// 4. 设置监听
	ret = listen(lfd, 128);
	if (ret == -1) {
		perror("listen");
		return -1;
	}
	// 返回 fd
	puts("listen 成功...");
	return lfd;
}
int epollRun(int lfd) {
	//创建 epoll 实例 
	int efd = epoll_create(1);
	if (efd == -1) {
		perror("epoll_create");
		return -1;
	}
	puts("epoll_create 成功....");
	struct epoll_event evn, work[1024];
	evn.data.fd = lfd;
	evn.events = EPOLLIN;
	int ret = epoll_ctl(efd, EPOLL_CTL_ADD, lfd, &evn);
	if (ret == -1) {
		perror("epoll_ctl");
		return -1;
	}
	puts("第一次 epoll_ctl 成功....");

	int sz = sizeof(work) / sizeof(struct epoll_event);
	while (1) {
		puts("开始epoll");
		int num = epoll_wait(efd, work, sz, -1);
		printf("num = %d\n", num);
		for (int i = 0; i < num; i++) {
			struct inFo* in = (struct inFo*)malloc(sizeof(struct inFo));
			in->efd = efd;
			in->lfd = work[i].data.fd;
			if (work[i].data.fd == lfd) {
				//建立新连接 accept
				//acceptClient(lfd, efd);
				pthread_create(&in->pid, NULL, acceptClient, (void*)in);
			}
			else {
				//主要接收对端的数据
				puts("接收消息.....");
				//recvHttpRequest(work[i].data.fd, efd);
				pthread_create(&in->pid, NULL, recvHttpRequest, (void*)in);
				puts("已发送....");
			}
		}
	}

	return 0;
}

void* acceptClient(void* arg) {
	//建立连接
	struct inFo* in = arg;
	printf("%d 正在准备 accept\n", in->pid);
	int cfd = accept(in->lfd, NULL, NULL);
	if (cfd == -1) {
		perror("accept");
		return (void*)-1;
	}
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	int ret = fcntl(cfd, F_SETFL, flag);

	struct epoll_event evn;
	evn.data.fd = cfd;
	evn.events = EPOLLIN | EPOLLET;
	ret = epoll_ctl(in->efd, EPOLL_CTL_ADD, cfd, &evn);
	printf("%d 已完成 accept\n", in->pid);

	free(in);
	if (ret == -1) {
		perror("epoll_ctl");
		return (void*)-1;
	}
	puts("建立新的连接...");
	return (void*)1;
}

void* recvHttpRequest(void* arg) {
	struct inFo* in = arg;
	printf("%d 正在准备 recv\n", in->pid);
	char buf[4096] = { 0 };
	char tmp[1024] = { 0 };
	int len = 0, tot = 0;
	while ((len = recv(in->lfd, tmp, sizeof tmp, 0)) > 0) {
		if (tot + len < sizeof buf) {
			memcpy(buf + tot, tmp, len);
		}
		tot += len;
	}
	printf("收到的请求为 : %s\n", buf);
	//判断数据是否被接收完毕
	if (len == -1 && errno == EAGAIN) {
		//解析请求行
		char* pt = strstr(buf, "\r\n");
		int reqlen = pt - buf;
		buf[reqlen] = '\0';
		printf("收到的请求行为 : %s\n", buf);
		puts("解析请求行....");
		parseRequestLine(buf, in->lfd);
		puts("已解析...");
	}
	else if (len == 0) {
		//客户端断开了连接
		puts("客户端断开连接...");
		epoll_ctl(in->efd, EPOLL_CTL_DEL, in->lfd, NULL);
		close(in->lfd);
	}
	else {
		perror("recv");
		return NULL;
	}
	printf("%d 已完成 recv\n", in->pid);
	free(in);
	return (void*)0;
}

int hexToDec(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}
int parseRequestLine(const char* line, int cfd) {
	//解析请求行 get /xxx/1.jpg http/1.1
	printf("收到的解析行 : %s\n", line);
	char method[12], path[1024];
	sscanf(line, "%[^ ] %[^ ]", method, path);
	//printf("method : %s \n path : %s\n", method, path);
	if (strcasecmp(method, "get") != 0) {
		return -1;
	}
	//处理客户端请求的静态资源（目录或文件）
	char* file = NULL;
	if (strcmp(path, "/") == 0) {
		file = "./";
	}
	else {
		file = path + 1;
	}
	printf("客户端想要访问的文件路径 : %s\n", file);
	//获取文件属性
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1) {
		//文件不存在 回复 404
		puts("文件不存在？");
		sendHeadMsg(cfd, 404, "Not Found", getFiletype(".html"), -1);
		sendFile("404.html", cfd);
		return 0;
	}
	if (S_ISDIR(st.st_mode)) {
		puts("是个目录！");
		//把这个目录中的内容发送给客户端
		puts("发送请求头");
		sendHeadMsg(cfd, 200, "OK", getFiletype(".html"), -1);
		puts("发送目录");
		sendDir(file, cfd);
	}
	else {
		//把这个文件的内容发送给客户端
		puts("是个文件！");
		sendHeadMsg(cfd, 200, "OK", getFiletype(file), st.st_size);
		sendFile(file, cfd);
	}
	puts("已发送");
	return 0;
}

int sendFile(const char* fileName, int cfd) {
	//打开文件
	int fd = open(fileName, O_RDONLY);
	puts(fileName);
	assert(fd > 0);
	off_t offset = 0;
	int size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	while (offset < size) {
		int ret = sendfile(cfd, fd, &offset, size);
		printf("ret value %d\n", ret);
		if (ret == -1 && errno == EAGAIN) {
			//printf("没数据，再等会");
		}
		else if (ret == -1) {
			perror("sendfile");
		}
	}
	close(fd);
	return 0;
}

int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length) {
	char buf[4096] = { 0 };
	sprintf(buf, "http/1.1 %d %s\r\n", status, descr);
	printf("1 发送的请求头： %s\n", buf);
	//响应头
	sprintf(buf + strlen(buf), "content-type: %s\r\n", type);
	printf("2 发送的请求头： %s\n", buf);

	sprintf(buf + strlen(buf), "content-length: %d\r\n\r\n", length);

	printf("3 发送的请求头： %s\n", buf);
	send(cfd, buf, strlen(buf), 0);

	return 0;
}
const char* getFiletype(const char* name)
{
	//a.jpg a.mp4 a.html
	//自右向左查找 '.' 字符，如不存在返回 NULL
	const char* dot = strrchr(name, '.'); 
	if (dot == NULL)
		return "text/plain; charset=utf-8"; //纯文本
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";
	return "text/plain; charset=utf-8";
}
#if 0
int sendDir(const char* dirName, int cfd) {
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>$s</title></head><body><table>", dirName);
	struct dirent** namelist;
	int num = scandir(dirName, &namelist, NULL, alphasort);
	puts("开始浏览文件中的路径...");
	for (int i = 0; i < num; i++) {
		puts("??");
		char* name = namelist[i]->d_name;
		puts("!!");
		struct stat st;
		char subPath[1024] = { 0 };
		puts("???");
		printf("dirName : %s\nname : %s\n", dirName, name);
		sprintf(subPath, "%s/%s", dirName, name);
		puts("!!!");
		printf("子路径为 : %s\n", subPath);
		stat(subPath, &st);
		if (S_ISDIR(st.st_mode)) {
			//a 标签 <a href=""></a>
			sprintf(buf + strlen(buf), "<tr><td><a href=\"%s/\"></a>%s</td><td>%ld</td></tr>", 
				name, name, st.st_size);
		}
		else {
			sprintf(buf + strlen(buf), "<tr><td><a href=\"%s\"></a>%s</td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		free(namelist[i]);
		send(cfd, buf, strlen(buf), 0);
		memset(buf, 0, sizeof(buf));
	}
	sprintf(buf, "</table></bode></html>");
	send(cfd, buf, strlen(buf), 0);
	free(namelist);
	return 0;
}
#else
int sendDir(const char* dirName, int cfd)
{
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
	struct dirent** namelist;
	int num = scandir(dirName, &namelist, NULL, alphasort);
	puts("开始浏览文件中的路径...");
	for (int i = 0; i < num; i++)
	{
		//鍙栧嚭鏂囦欢鍚?namelist鎸囧悜鐨勬槸涓€涓寚閽堟暟缁?struct dirent* tmp[]
		puts("??");
		char* name = namelist[i]->d_name;
		if (name[0] == '.') continue;
		puts("!!");
		struct stat st;
		char subPath[1024] = { 0 };
		puts("???");
		printf("dirName : %s\nname : %s\n", dirName, name);
		sprintf(subPath, "%s/%s", dirName, name);
		puts("!!!");
		printf("子路径为 : %s\n", subPath);
		stat(subPath, &st);
		if (S_ISDIR(st.st_mode))
		{
			//a鏍囩 <a href="">name</a>
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else
		{
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		printf("发送的报文为 %s\n", buf);
		send(cfd,buf,strlen(buf),0);
		/*sendBuf->appendString(buf);
#ifndef MSG_SEND_AUTO
		sendBuf->sendData(cfd);
#endif*/
		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	sprintf(buf, "</table></body></html>");
	send(cfd,buf,strlen(buf),0);
//	sendBuf->appendString(buf);
//#ifndef MSG_SEND_AUTO
//	sendBuf->sendData(cfd);
//#endif
	free(namelist);
	return 0;
}
#endif
