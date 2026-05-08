#include "threadworker.h"
#include "qDebug"
ThreadWorker::ThreadWorker(QObject *parent) : QObject(parent)
{
    m_thread = new QThread;
    this->moveToThread( m_thread );

    m_thread->start();
}

ThreadWorker::~ThreadWorker()
{
    qDebug() << __func__;

    //回收线程
    if( m_thread ){
        m_thread->quit();
        m_thread->wait(10);
        if( m_thread->isRunning() ){
            m_thread->terminate();
            m_thread->wait(10);
        }
        delete m_thread; m_thread = nullptr;
    }
}
