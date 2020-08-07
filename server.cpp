#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include <sys/epoll.h>
#include <sys/time.h>
#include <fcntl.h>

#include <algorithm>
#include <string>

#include "MyThreadPool.h"


#define SERVER_IP "127.0.0.1"
#define LISTEN_PORT 6666
#define THREAD_NUM 4
#define MAX_OPEN 2048
#define BUFF_SIZE 256
#define MSG_MAX_LEN 5000


ssize_t epfd;

int SetNonBlock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    perror("SetNonBlock failed.");
  int rt = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  if (rt < 0)
    perror("SetNonBlock failed.");
  return rt;
}

class CMyTask: public CTask
{
 public:
    CMyTask() = default;
    int Run()
    {
        char finalmsg[MSG_MAX_LEN];
        char recvbuf[BUFF_SIZE];
        int index = 0;
        int curr = 0;
        int connfd = GetConnFd();
        while(1)
        {
            memset(recvbuf,0x00,sizeof(recvbuf));
            int len = recv(connfd, recvbuf, sizeof(recvbuf), 0);
            if(len > 0)
            {
                for(int i = 0; i < len; ++i) finalmsg[index++] = recvbuf[i];
            }
            else if(len == 0)  //client关闭连接
            {
                close(connfd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
                return 0;
            }
            else
            {
                break;
            }
        }
        finalmsg[index] = '\0';
        for(int i = 0; i < index - 1; ++i)
        {
            std::swap(finalmsg[i], finalmsg[index - i - 2]);
        }
        while(curr < index)
        {
            int len = send(connfd, finalmsg + curr, index - curr, 0);
            if(len < 0)
            {
                break;
            }
            curr += len;
        }
        return 0;
    }
};

int main(int argc, char* argv[])
{
    ///nready: numbers of ready fds
    ///epfd: epoll_create returns a handler
    ///ctl_res: epoll_ctl return
    ssize_t nready, ctl_res;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    char client_ip[INET_ADDRSTRLEN];
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd != -1);

    SetNonBlock(listenfd);

    /// set port reuse
    int opt = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        std::cout << "set port reuse faild." << std::endl;
    }


    /// tep: save input args in epoll_ctl
    /// events: save epoll_wait return values
    struct epoll_event tep, events[MAX_OPEN];

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    inet_aton(SERVER_IP, &server_addr.sin_addr);
    // server_addr.sin_addr.s_addr = htons(ANADDR_ANY);
    server_addr.sin_port=htons(LISTEN_PORT);

    int res=bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    assert(res!=-1);

    listen(listenfd, MAX_OPEN);

    /// epfd
    epfd = epoll_create(MAX_OPEN);
    assert(epfd != -1);

    tep.events = EPOLLIN | EPOLLET;
    tep.data.fd = listenfd;
    ctl_res = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &tep);
    assert(ctl_res != -1);

    std::cout << "waiting to connect......" << std::endl;

    //创建线程池
    CThreadPool pool(4);

    while(1)
    {
        ///-1表永久阻塞
        nready = epoll_wait(epfd, events, MAX_OPEN, -1);
        assert(nready != -1);

        for(int i = 0; i < nready; ++i)
        {
            if(events[i].data.fd == listenfd)
            {
                ///new connect
                socklen_t client_len = sizeof(client_addr);
                int connectfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_len);

                // std::cout << "client " << inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip)) << " : " << ntohs(client_addr.sin_port) << std::endl;

                SetNonBlock(connectfd);
                tep.events = EPOLLIN | EPOLLET;
                tep.data.fd = connectfd;
                ctl_res = epoll_ctl(epfd, EPOLL_CTL_ADD, connectfd, &tep);
                assert(ctl_res != -1);
            }
            else if(events[i].events & EPOLLIN)
            {
                ///read ready
                int sockfd = events[i].data.fd;
                CMyTask* task = new CMyTask();
                task->SetConnFd(sockfd);
                pool.AddTask(task);
            }
        }
    }
    pool.StopAll();
    close(listenfd);
    return 0;
}
