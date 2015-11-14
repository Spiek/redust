#include <QtTest/QtTest>

#include "redishash.h"
#include "redismapconnectionmanager.h"

// helper macros
#define FAIL(data) QFAIL(qPrintable(data))

template<typename Key, typename Value>
class TestTemplateHelper
{
    public:
        static void insert(int randomValuesCount, int keyIndex = 0)
        {
            for(int run = 0; run < 2; run++) {
                QByteArray strHash = QString("Test_%1%2").arg(keyIndex).arg(run + 1).toLocal8Bit();
                QString strMode = !run ? "asyncron" : "syncron";
                qInfo("Insert %i Random values into RedisHash<%s,%s>(\"%s\") (%s)", randomValuesCount, typeid(Key).name(), typeid(Value).name(), qPrintable(strHash), qPrintable(strMode));
                RedisHash<Key, Value> rHash(strHash, true, true);
                for(int i = 1; i <= randomValuesCount; i++) {
                    Key key = RedisValue<Key>::deserialize(QString::number(i).toLocal8Bit(), false);
                    Value value = RedisValue<Value>::deserialize(QString::number(i + qrand()).toLocal8Bit(), false);
                    if(!rHash.insert(key, value, run)) {
                        FAIL(QString("Failed to insert %1 into %2 (Key,Value):%3,%4").arg(strMode).arg(QString(strHash)).arg(i).arg(value));
                    }
                }
            }
        }
};

class TestRedisHash : public QObject
{
    Q_OBJECT
    private slots:
        void initTestCase();
        void clear();
        void insert();
};

void TestRedisHash::initTestCase()
{
    // init random
    qsrand(QTime::currentTime().msec());

    // add connection pool
    if(!RedisConnectionManager::addConnection("redis", REDIS_SERVER, REDIS_SERVER_PORT)) qFatal("Connection Pool could not be created");

    // request socket
    if(!RedisConnectionManager::requestConnection("redis", false)) qFatal("Cannot establish connection to Redis Server");
}

void TestRedisHash::clear()
{
    for(QByteArray key : RedisInterface::keys("Test*")) {
        if(RedisInterface::del(key, false)) qInfo("Delete key %s", qPrintable(key));
        else FAIL(QString("Cannot Delete key %1").arg(QString(key)));
    }
}

void TestRedisHash::insert()
{
    int values = 10;
    int index = 0;
	TestTemplateHelper<double, float>::insert(values, ++index);
    TestTemplateHelper<int, QString>::insert(values, ++index);
    TestTemplateHelper<QString, int>::insert(values, ++index);
}



QTEST_MAIN(TestRedisHash)
#include "testredishash.moc"
