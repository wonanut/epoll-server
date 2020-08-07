#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>
#include <sys/time.h>

#define SERVER_PORT 6666
#define SERVER_IP "127.0.0.1"
#define RECV_BUFF_SIZE 5120
#define THREAD_NUM 8
#define SEND_ONCE 8000


std::mutex mtx;
int sum_finish;

int str_len;

void GenerateStr(char *buff, int len)
{
     for (int i = 0; i < len - 1; ++i)
     {
          buff[i] = rand() % 26 + 'a';
     }
     buff[len - 1] = '\0';
}


int Send2Server(const char *buff, int len, int sockfd)
{
     int index = 0;
     while(index < len)
     {
          int n = send(sockfd, buff, len - index, 0);
          if(n <= 0)
          {
               break;
          }
          index += n;
     }
     return 0;
}

int RecvFromServer(int sockfd)
{
     char recv_buff[RECV_BUFF_SIZE] = {0};
     char tmp[256] = {0};
     int index = 0;
     bool stop = false;
     while(!stop)
     {
          int len = recv(sockfd, tmp, sizeof(tmp), 0);
          if(len < 0)
          {
               //TODO judge EAGAIN
               break;
          }
          else if(len == 0)
          {
               break;
          }
          for(int i = 0; i < len; ++i)
          {
               recv_buff[index++] = tmp[i];
               if(index == str_len - 1) stop = true;
          }
     }
     recv_buff[index] = '\0';
     return 0;
}

void run(int sockfd, std::string &str)
{
     const char *buff = str.c_str();
     for(int i = 0; i < SEND_ONCE; ++i)
     {
          Send2Server(buff, str.size(), sockfd);
          RecvFromServer(sockfd);
     }
     std::unique_lock<std::mutex> lck(mtx);
     ++sum_finish;
     lck.unlock();
}

int conn2server()
{
     int fd = socket(AF_INET, SOCK_STREAM, 0);
     assert(fd != -1);

     struct sockaddr_in ser;
     memset(&ser, 0x00, sizeof(ser));
     ser.sin_family = AF_INET;
     inet_aton(SERVER_IP, &ser.sin_addr);
     ser.sin_port = htons(SERVER_PORT);

     int res = connect(fd, (struct sockaddr *)&ser, sizeof(ser));
     assert(res != -1);

     return fd;
}

int main(int argc, char *argv[])
{
     if(argc < 2)
     {
          std::cout << "Error args, uasge: " << argv[0] <<  " [str_len]\n";
          return 0;
     }
     str_len = atoi(argv[1]);
     char sendbuff[str_len];
     memset(sendbuff,0x00,sizeof(sendbuff));
     GenerateStr(sendbuff, str_len);

     struct timeval start;
     struct timeval finish;
     int fd_arr[8] = {0};
     for(int i = 0; i < THREAD_NUM; ++i)
     {
          fd_arr[i] = conn2server();
     }

     std::vector<std::thread> thds;
     std::string str = sendbuff;
     gettimeofday(&start, NULL);
     for(int i = 0; i < THREAD_NUM; ++i)
     {
          thds.push_back(std::thread(run, fd_arr[i], std::ref(str)));
     }

     for(int i = 0; i < THREAD_NUM; ++i)
     {
          thds[i].join();
     }
     gettimeofday(&finish, NULL);
     double cost  = (finish.tv_sec - start.tv_sec) * 1000  + (double)((finish.tv_usec - start.tv_usec) / 1000);
     int final_finish = sum_finish * SEND_ONCE;

     std::cout << "final_finish: " << final_finish << std::endl;
     std::cout << "cost: " << cost << std::endl;

     double qps = (double)(final_finish) / (cost / 1000.0);

     double rate = qps * (str_len - 1) / 1024 / 1024;
     std::cout << "qps: " << qps / 10000 << " w/s" << std::endl;
     std::cout << "rate: " << rate  << " MBps" << std::endl;

     // close(sockfd);
     for(int i = 0; i < thds.size(); ++i)
     {
          close(fd_arr[i]);
     }
     return 0;
}