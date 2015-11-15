#include <QtTest/QtTest>

#include "redishash.h"
#include "redismapconnectionmanager.h"

// const variables
#define KEYNAMESPACE "RedisTemplates_TestCase"

// helper macros
#define FAIL(data) QFAIL(qPrintable(data))
#define VERIFY2(condition,message) QVERIFY2(condition, qPrintable(message))
#define GENKEYNAME(keyIndex) QString(KEYNAMESPACE"_%1").arg(keyIndex).toLocal8Bit()
#define GENINTRANDRANGE(from,to) qrand() % ((to + 1) - from) + from
#define GENFLOATRANDRANGE(from,to) from + ((double)qrand() / RAND_MAX) * (to - from)

template<typename Key, typename Value>
class TestTemplateHelper
{
    public:
        static void hashInsert(QMap<Key, Value> data, int keyIndex, bool sync, bool binarizeKey = true, bool binarizeValue = true)
        {
            QByteArray strHash = GENKEYNAME(keyIndex);
            QString strMode = !sync ? "asyncron" : "syncron";
            qInfo("Insert %i values into RedisHash<%s,%s>(\"%s\") (%s)", data.count(), typeid(Key).name(), typeid(Value).name(), qPrintable(strHash), qPrintable(strMode));
            RedisHash<Key, Value> rHash(strHash, binarizeKey, binarizeValue);
            for(auto itr = data.begin(); itr != data.end(); itr++) {
                if(!rHash.insert(itr.key(), itr.value(), sync)) {
                    FAIL(QString("Failed to insert %1 into %2 (Key,Value):%3,%4")
                         .arg(strMode)
                         .arg(strHash.data())
                         .arg(RedisValue<Key>::serialize(itr.key(), binarizeKey).data())
                         .arg(RedisValue<Value>::serialize(itr.value(), binarizeValue).data())
                    );
                }
            }
        }

        static void hashCheck(QMap<Key, Value> data, int keyIndex, bool binarizeKey = true, bool binarizeValue = true)
        {
            QByteArray strHash = GENKEYNAME(keyIndex);
            RedisHash<Key, Value> rHash(strHash, binarizeKey, binarizeValue);

            // 1. check over simple foreach
            qInfo(" - Iterate over all values using Foreach approach and check the values");
            VERIFY2(data.count() == rHash.keys().count(), QString("Failed field count not equal (Redis: %1, Local: %2)").arg(rHash.keys().count()).arg(data.count()));
            for(Key key : rHash.keys()) {
                if(data.value(key) != rHash.value(key)) {
                    FAIL(QString("A Check Failed:\nstored: %2,%3\nValue of hasmap for key (if exists):%4")
                         .arg(RedisValue<Key>::serialize(key, binarizeKey).toHex().data())
                         .arg(RedisValue<Value>::serialize(rHash.value(key), binarizeValue).toHex().data())
                         .arg(RedisValue<Value>::serialize(data.value(key), binarizeValue).toHex().data())
                    );
                }
            }

            // 2. check over iterator foreach (cache = 1)
            int cache = GENINTRANDRANGE(1, 63);
            qInfo(" - Iterate over all values using Iterator Foreach approach (cache = %d) and check the values", cache);
            int count = 0;
            for(auto itr = rHash.begin(cache); itr != rHash.end(); itr++) {
                if(data.value(itr.key()) != itr.value()) {
                    FAIL(QString("A Check Failed:\nstored: %2,%3\nValue of hashmap for key (if exists):%4")
                         .arg(RedisValue<Key>::serialize(itr.key(), binarizeKey).toHex().data())
                         .arg(RedisValue<Value>::serialize(itr.value(), binarizeValue).toHex().data())
                         .arg(RedisValue<Value>::serialize(data.value(itr.key()), binarizeValue).data())
                    );
                }
                count++;
            }
            VERIFY2(data.count() == count, QString("Failed field count not equal (Redis: %1, Local: %2)").arg(count).arg(data.count()));
        }
};

class TestRedisHash : public QObject
{
    Q_OBJECT
    private slots:
        void initTestCase();
        void clear();
        void hash();
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
    for(QByteArray key : RedisInterface::keys(KEYNAMESPACE"_*")) {
        if(RedisInterface::del(key, false)) qInfo("Delete key %s", qPrintable(key));
        else FAIL(QString("Cannot Delete key %1").arg(QString(key)));
    }
}

void TestRedisHash::hash()
{
    // key index
    int index = 0;
    int count = 10000;

    // double float test
    {
        QMap<float, double> data;
        while(data.count() != count) {
            data.insert(GENFLOATRANDRANGE(563,545321), GENFLOATRANDRANGE(563,545321));
        };
        TestTemplateHelper<float, double>::hashInsert(data, ++index, false, true, true);
        TestTemplateHelper<float, double>::hashCheck(data, index, true, true);
        qInfo();
        TestTemplateHelper<float, double>::hashInsert(data, ++index, true, true, true);
        TestTemplateHelper<float, double>::hashCheck(data, index, true, true);
        qInfo();
    }

    // QString int test
    {
        QMap<QByteArray, int> data;
        while(data.count() != count) {
            QByteArray content;
            for(int i = 0; i < 20; i++) {
                content.append((char)GENINTRANDRANGE(0, 255));
            }
            data.insert(content, GENINTRANDRANGE(563,545321));
        };

        TestTemplateHelper<QByteArray, int>::hashInsert(data, ++index, false, true, true);
        TestTemplateHelper<QByteArray, int>::hashCheck(data, index, true, true);
        qInfo();
        TestTemplateHelper<QByteArray, int>::hashInsert(data, ++index, true, true, true);
        TestTemplateHelper<QByteArray, int>::hashCheck(data, index, true, true);
    }
}

QTEST_MAIN(TestRedisHash)
#include "testredishash.moc"
