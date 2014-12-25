#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>

#include "redismap.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    RedisMapConnectionManager::initRedisConnection("192.168.0.110", 6379);
    RedisMap<qint64, qint64> rmap("BLUB");
    qint64 times = 1000000;
    QElapsedTimer timer;
    timer.start();
    for(qint64 i = 0; i < times; i++) {
        rmap.insert(i, i, true);
    }
    qint64 msElapsed = timer.nsecsElapsed();
    qDebug() << times << "changes in" << msElapsed << "ns (" << (double)msElapsed / times << "ns per Entry!)";
    return a.exec();
}
