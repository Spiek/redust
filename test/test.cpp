#include "test.h"

Test::Test(QObject *parent) : QObject(parent)
{

}

void Test::start()
{
    RedisMapConnectionManager::initRedisConnection("127.0.0.1", 6379);
    RedisMap<qint64, Protobuffer::Test> rmap("BLUB");
    qint64 times = 1000000;
    QElapsedTimer timer;
    timer.start();
    qsrand(QTime::currentTime().msec());
    Protobuffer::Test test;
    for(qint64 i = 0; i < times; i++) {
        test.set_int64test(i);
        test.set_stringtest("djskdjk");
        rmap.insert(i, test, false);
    }
    qApp->processEvents();

    qint64 msElapsed = timer.nsecsElapsed();
    qDebug() << times << "changes in" << msElapsed << "ns (" << (double)msElapsed / times << "ns per Entry! (" << 1000000000 / ((double)msElapsed / times) << "per Second))";
}
