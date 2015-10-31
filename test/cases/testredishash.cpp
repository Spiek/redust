#include <QtTest/QtTest>

#include "redismapconnectionmanager.h"

class TestRedisHash : public QObject
{
    Q_OBJECT
    private slots:
        void establishConnection();
};

void TestRedisHash::establishConnection()
{
    // add connection pool
    QVERIFY2(RedisConnectionManager::addConnection("redis", REDIS_SERVER, REDIS_SERVER_PORT), "Connection Pool could not be created");

    // request socket
    QVERIFY(RedisConnectionManager::requestConnection("redis", false));
}


QTEST_MAIN(TestRedisHash)
#include "testredishash.moc"
