#include <QCoreApplication>
#include <QDateTime>
#include <QElapsedTimer>

#include "redismap.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    RedisMapConnectionManager::initRedisConnection("127.0.0.1", 6379);
    RedisMap<qint64, qint64> rmap("BLUB");
    qint64 times = 1000000;
    QElapsedTimer timer;
    timer.start();
    qsrand(QTime::currentTime().msec());
    for(qint64 i = 0; i < times; i++) {
        rmap.insert(i, qrand(), false);
    }
    a.processEvents();
    qint64 msElapsed = timer.nsecsElapsed();
    qDebug() << times << "changes in" << msElapsed << "ns (" << (double)msElapsed / times << "ns per Entry! (" << 1000000000 / ((double)msElapsed / times) << "per Second))";
    return a.exec();
}
