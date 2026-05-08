#ifndef THREADWORKER_H
#define THREADWORKER_H

#include <QObject>
#include <QThread>

class ThreadWorker : public QObject
{
    Q_OBJECT
public:
    explicit ThreadWorker(QObject *parent = nullptr);
    virtual ~ThreadWorker();
signals:

private:
    QThread * m_thread;
};

#endif // THREADWORKER_H
