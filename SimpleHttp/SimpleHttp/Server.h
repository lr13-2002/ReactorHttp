#pragma once
//��ʼ���������׽���
int initListenLd(unsigned short port);
//���� epoll
int epollRun(int lfd);
//�Ϳͻ��˽����µ�����
//int acceptClient(int lfd, int efd);
void* acceptClient(void* arg);
//����http��������Ϣ
//int recvHttpRequest(int cfd, int efd);
void* recvHttpRequest(void* arg);
//����http��������
int parseRequestLine(const char* line, int cfd);
//�����ļ�
int sendFile(const char* fileName, int cfd);
//������Ӧͷ��״̬�к���Ӧͷ��
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length);

const char* getFiletype(const char* name);

int sendDir(const char* dirName, int cfd);

int hexToDec(char c);
