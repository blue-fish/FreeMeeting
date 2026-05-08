#ifndef THERADWORKER_H
#define THERADWORKER_H

#include <QObject>
#include<QThread>
#include<QDebug>
class ThreadWorker : public QObject
{
    Q_OBJECT
public:
    explicit ThreadWorker(QObject *parent = nullptr);
    ~ThreadWorker();
signals:
protected:
    QThread* m_pThread;
};

//定义工作者:例子
class worker:  public ThreadWorker
{
    Q_OBJECT
public:
    ~worker();
public slots:
    void slot_doWork();
};

#endif // THERADWORKER_H
