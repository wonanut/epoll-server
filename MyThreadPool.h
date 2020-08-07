#ifndef __THREAD_H
#define __THREAD_H

#include <deque>
#include <string>
#include <pthread.h>

using namespace std;

class CTask {
protected:
    int connfd;  // task's recv address
public:
    CTask() = default;
    CTask(string& taskName): connfd(0) {}
    virtual int Run() = 0;  //任务接口
    void SetConnFd(int data); // 设置接收的套接字
    int GetConnFd();
    virtual ~CTask() {}
};

class CThreadPool {
private:
    static deque<CTask*> m_deqTaskList;
    static bool shutdown;
    int m_iThreadNum;
    pthread_t *pthread_id;
    static pthread_mutex_t m_pthreadMutex;
    static pthread_cond_t m_pthreadCond;

protected:
    static void* ThreadFun(void *threadData); // callback func
    static int MoveToIdle(pthread_t tid);
    static int MoveToBusy(pthread_t tid);
    int Create();

public:
    CThreadPool(int threadNum = 4);
    int AddTask(CTask *task);
    int StopAll();
    int getTaskSize();
};

#endif