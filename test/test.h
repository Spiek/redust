#ifndef TEST_H
#define TEST_H

#include <QCoreApplication>
#include <QObject>
#include <QDateTime>
#include <QElapsedTimer>

#include "redismap.h"
#include "test.pb.h"

class Test : public QObject
{
    Q_OBJECT
    public:
        Test(QObject *parent = 0);

    public slots:
        void start();
};

#endif // TEST_H
