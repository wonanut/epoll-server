#include <iostream>
#include <deque>
#include <stdio.h>

#include "MyThreadPool.h"

using namespace std;

void CTask::SetConnFd(int data) {
    connfd = data;
}

int CTask::GetConnFd() {
    return connfd;
}


deque<CTask*> CThreadPool::m_deqTaskList;
bool CThreadPool::shutdown = false;

pthread_mutex_t CThreadPool::m_pthreadMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t CThreadPool::m_pthreadCond = PTHREAD_COND_INITIALIZER;

CThreadPool::CThreadPool(int threadNum) {
    this->m_iThreadNum = threadNum;
    cout << "I will create " << threadNum << " threads" << endl;
    Create();
}

int CThreadPool::Create() {
    pthread_id = (pthread_t*)malloc(sizeof(pthread_t) * m_iThreadNum);
    for(int i  = 0; i < m_iThreadNum; ++i) {
        pthread_create(&pthread_id[i], NULL, ThreadFun, NULL);
    }
    return  0;
}

/*callback func*/
void* CThreadPool::ThreadFun(void *threadData) {
    pthread_t tid = pthread_self();
    while(1) {
        /*线程开始时先上锁*/
        pthread_mutex_lock(&m_pthreadMutex);
        while(m_deqTaskList.size() == 0 && !shutdown)
        {
            pthread_cond_wait(&m_pthreadCond, &m_pthreadMutex);  // 如果没有任务且退出标志为false，阻塞线程，等待唤醒
        }

        if(shutdown)
        {
            pthread_mutex_unlock(&m_pthreadMutex);
            printf("thread %lu will exit\n", pthread_self());
            pthread_exit(NULL);
        }
        /*取任务队列处理*/
        CTask* task = m_deqTaskList.front();
        m_deqTaskList.pop_front();
        pthread_mutex_unlock(&m_pthreadMutex);
        task->Run();  //执行相应的任务
        delete task;
    }
    return (void*)0;
}

/**
 * 往任务队列里边添加任务并发出线程同步信号
 */
int CThreadPool::AddTask(CTask *task)
{
    pthread_mutex_lock(&m_pthreadMutex);
    this->m_deqTaskList.push_back(task);
    pthread_mutex_unlock(&m_pthreadMutex);

    /* 添加任务 条件变量发信号，非阻塞  */
    pthread_cond_signal(&m_pthreadCond);
    return 0;
}


/**
 * 停止所有线程
 */
int CThreadPool::StopAll()
{
    /** 避免重复调用 */
    if (shutdown)
    {
        return -1;
    }
    printf("Now I will end all threads!!\n");
    /** 唤醒所有等待线程，线程池要销毁了 */
    shutdown = true;
    pthread_cond_broadcast(&m_pthreadCond);

    /** 阻塞等待线程退出，否则就成僵尸了 */
    for (int i = 0; i < m_iThreadNum; i++)
    {
        pthread_join(pthread_id[i], NULL);
    }

    free(pthread_id);
    pthread_id = NULL;

    /** 销毁条件变量和互斥体 */
    pthread_mutex_destroy(&m_pthreadMutex);
    pthread_cond_destroy(&m_pthreadCond);

    return 0;
}

/**
 * 获取当前队列中任务数
 */
int CThreadPool::getTaskSize()
{
    return m_deqTaskList.size();
}

int CThreadPool::MoveToIdle(pthread_t tid)
{
    pthread_mutex_lock(&m_pthreadMutex);
    // printf("tid %lu move to idle...\n", tid);
    pthread_cond_wait(&m_pthreadCond, &m_pthreadMutex);

    pthread_mutex_unlock(&m_pthreadMutex);
    return 0;
}

int CThreadPool::MoveToBusy(pthread_t tid)
{
    return 0;
}

