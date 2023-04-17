#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "TcpServer.h"
#include "Log.h"
#if 1

int main() {
    /*int cnt = 0;
    while (1) {
        struct Channel* channel = (struct Channel*)malloc(sizeof(struct Channel));
        cnt++;
        printf("%d\n", cnt);
    }   */
    unsigned short port = atoi("10000");
    //切换服务器路径
    chdir("/home/lr13-2002/projects/SimpleHttp/pkl");
    //启动服务器
    Debug("初始化tcpServer...");
    struct TcpServer* server = tcpServerInit(port, 4);
    Debug("初始化完成!!! 开始 runing....");
    tcpServerRun(server);
    system("pause");
    return 0;
}

#else
int main(int argc, char* argv[]) {
    if (argc < 3) {
        puts("./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    //切换服务器路径
    chdir(argv[2]);
    //启动服务器
    struct TcpServer* server = tcpServerInit(port, 4);
    tcpServerRun(server);
    system("pause");
    return 0;
}
#endif  //  