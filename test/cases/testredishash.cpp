#include <QtTest/QtTest>

#include "redust/redisserver.h"
#include "redust/redishash.h"
#include "redust/redislistpoller.h"

// const variables
#define KEYNAMESPACE "RedisTemplates_TestCase"

// helper macros
#define FAIL(data) QFAIL(qPrintable(data))
#define VERIFY2(condition,message) QVERIFY2(condition, qPrintable(message))
#define GENKEYNAME(keyIndex) QString(KEYNAMESPACE"_Hash%1").arg(keyIndex).toLocal8Bit()
#define GENINTRANDRANGE(from,to) qrand() % ((to + 1) - from) + from
#define GENFLOATRANDRANGE(from,to) from + ((double)qrand() / RAND_MAX) * (to - from)

static RedisServer redisServer(REDIS_SERVER, REDIS_SERVER_PORT);

template<typename Key, typename Value>
class TestTemplateHelper
{
    public:
        static void hashInsert(QMap<Key, Value>& data, int keyIndex, RedisServer::RequestType mode, bool binarizeKey = true, bool binarizeValue = true)
        {
            QByteArray strHash = GENKEYNAME(keyIndex);
            QString strMode = mode == RedisServer::RequestType::PipeLine ? "pipeline" :
                              mode == RedisServer::RequestType::Asyncron ? "asyncron" :
                              mode == RedisServer::RequestType::Syncron  ? "syncron" : "";
            qInfo("Insert %i values into RedisHash<%s,%s>(\"%s\") (%s)", data.count(), typeid(Key).name(), typeid(Value).name(), qPrintable(strHash), qPrintable(strMode));
            RedisHash<Key, Value> rHash(redisServer, strHash, binarizeKey, binarizeValue);
            for(auto itr = data.begin(); itr != data.end(); itr++) {
                if(!rHash.insert(itr.key(), itr.value(), mode)) {
                    FAIL(QString("Failed to insert %1 into %2 (Key,Value):%3,%4")
                         .arg(strMode)
                         .arg(strHash.data())
                         .arg(TypeSerializer<Key>::serialize(itr.key(), binarizeKey).data())
                         .arg(TypeSerializer<Value>::serialize(itr.value(), binarizeValue).data())
                    );
                }
            }

            // execute pipeline request
            if(mode == RedisServer::RequestType::PipeLine) redisServer.executePipeline();

            // if we insert the data async, we wait until all set operations are processed by redis before continue
            else if(mode == RedisServer::RequestType::Asyncron) {
                // after all data was written to redis, we check constantly if all data were inserted into redis
                // after every check we wait one second (a)syncron, if no data was inserted during 10 checks, we fail
                for(int failed = 0; failed <= 10; failed++) {
                    if(rHash.count() == data.count()) break;
                    QTest::qWait(1000);
                    if(failed == 10) FAIL(" - We have pooled 10 seconds for insert changes, nothing happened so giving up...");
                }
            }

            VERIFY2(rHash.count() == data.count(), QString("Expect a list with %1-entries, but just got %2").arg(data.count()).arg(rHash.count()));
        }

        static void hashCheck(QMap<Key, Value> data, int keyIndex, bool binarizeKey = true, bool binarizeValue = true)
        {
            QByteArray strHash = GENKEYNAME(keyIndex);
            RedisHash<Key, Value> rHash(redisServer, strHash, binarizeKey, binarizeValue);

			// reverse the data (to speedup reverse lookups)
            QMap<Value, Key> dataReversed;
            for(auto itr = data.begin(); itr != data.end(); itr++) dataReversed.insert(itr.value(), itr.key());

            // 1. check over iterator foreach wird random cache
            int cache = GENINTRANDRANGE(3, 63);
            qInfo(" - Iterate over all values using Iterator Foreach approach (cache = %d) and check the values", cache);
            int count = 0;
            bool valueLengthCheck = rHash.valueLength(data.firstKey()) != -2;
            if(!valueLengthCheck) qWarning(" - Skip Value length check because redis doesn't support HSTRLEN commmand!");
            for(auto itr = rHash.begin(cache); itr != rHash.end(); itr++) {
                if(data.value(itr.key()) != itr.value()) {
                    FAIL(QString("A Check Failed:\nstored: %1,%2\nValue of hashmap for key (if exists):%3")
                         .arg(TypeSerializer<Key>::serialize(itr.key(), binarizeKey).toHex().data())
                         .arg(TypeSerializer<Value>::serialize(itr.value(), binarizeValue).toHex().data())
                         .arg(TypeSerializer<Value>::serialize(data.value(itr.key()), binarizeValue).data())
                    );
                }
                if(valueLengthCheck && TypeSerializer<Value>::serialize(itr.value(), binarizeValue).length() != rHash.valueLength(itr.key())) {
                    FAIL(QString("Value length no equal! (local: %1:%2, remote:%3:%4)")
                         .arg(TypeSerializer<Value>::serialize(data.value(itr.key()), binarizeValue).toHex().data())
                         .arg(TypeSerializer<Value>::serialize(data.value(itr.key()), binarizeValue).length())
                         .arg(TypeSerializer<Value>::serialize(itr.value(), binarizeValue).toHex().data())
                         .arg(rHash.valueLength(itr.key()))
                    );
                }
                count++;
            }
            VERIFY2(data.count() == count, QString("Failed field count not equal (Redis: %1, Local: %2)").arg(count).arg(data.count()));

            // 2. check over QMap
            qInfo(" - Iterate over all values using RedisHash::toMap() approach and check the values");
            QMap<Key, Value> mapData = rHash.toMap();
            VERIFY2(mapData.count() == rHash.count(), QString("Failed field count not equal (Redis: %1, Local: %2)").arg(rHash.count()).arg(mapData.count()));
            for(auto itr = mapData.begin(); itr != mapData.end(); itr++) {
                if(data.value(itr.key()) != itr.value()) {
                    FAIL(QString("A Check Failed:\nstored: %1,%2\nValue of hashmap for key (if exists):%3")
                         .arg(TypeSerializer<Key>::serialize(itr.key(), binarizeKey).toHex().data())
                         .arg(TypeSerializer<Value>::serialize(itr.value(), binarizeValue).toHex().data())
                         .arg(TypeSerializer<Value>::serialize(data.value(itr.key()), binarizeValue).data())
                    );
                }
            }

            // 3. check over QHash
            qInfo(" - Iterate over all values using RedisHash::toHash() approach and check the values");
            QHash<Key, Value> mapHash = rHash.toHash();
            VERIFY2(mapHash.count() == rHash.count(), QString("Failed field count not equal (Redis: %1, Local: %2)").arg(rHash.count()).arg(mapHash.count()));
            for(auto itr = mapHash.begin(); itr != mapHash.end(); itr++) {
                if(data.value(itr.key()) != itr.value()) {
                    FAIL(QString("A Check Failed:\nstored: %1,%2\nValue of hashmap for key (if exists):%3")
                         .arg(TypeSerializer<Key>::serialize(itr.key(), binarizeKey).toHex().data())
                         .arg(TypeSerializer<Value>::serialize(itr.value(), binarizeValue).toHex().data())
                         .arg(TypeSerializer<Value>::serialize(data.value(itr.key()), binarizeValue).data())
                    );
                }
            }

            // 4. check keys (with cache)
            qInfo(" - Check keys using RedisHash::keys() (with a cache of %d)", cache);
            for(Key key : rHash.keys(cache)) {
                if(!data.contains(key)) {
                    FAIL(QString("A Check Failed:\nstored: %1,%2\nValue of hashmap for key (if exists):%3")
                         .arg(TypeSerializer<Key>::serialize(key, binarizeKey).toHex().data())
                         .arg(TypeSerializer<Value>::serialize(rHash.value(key), binarizeValue).toHex().data())
                         .arg(TypeSerializer<Value>::serialize(data.value(key), binarizeValue).toHex().data())
                    );
                }
            }

            // 5. check values (with cache)
            qInfo(" - Check values using RedisHash::values() (with a cache of %d)", cache);
            for(Value value : rHash.values(cache)) {
                if(!dataReversed.contains(value)) {
                    FAIL(QString("A Check Failed:\nstored: %1")
                         .arg(TypeSerializer<Value>::serialize(value, binarizeValue).toHex().data())
                    );
                }
            }

            // 6. take all values
            qInfo(" - Loop and take all elements one by one (check the returned values if it exists in the list), until the list is empty");
            for(Key key : data.keys()) {
                bool result = false;
                if(data.value(key) != rHash.take(key, RedisServer::RequestType::Syncron, &result) || !result) {
                    FAIL(QString("A Check Failed:\nstored: %1,%2\nValue of hashmap for key (if exists):%3")
                         .arg(TypeSerializer<Key>::serialize(key, binarizeKey).toHex().data())
                         .arg(TypeSerializer<Value>::serialize(rHash.value(key), binarizeValue).toHex().data())
                         .arg(TypeSerializer<Value>::serialize(data.value(key), binarizeValue).toHex().data())
                    );
                }
            }
            VERIFY2(rHash.count() == 0, QString("Expect an empty list, but it's not (Count: %1)").arg(rHash.count()));
        }
};

class TestRedisHash : public QObject
{
    Q_OBJECT
    private slots:
        void initTestCase();
        void clear();
        void redispoller();
        void hash();
};

void TestRedisHash::initTestCase()
{
    // init random
    qsrand(QTime::currentTime().msec());

    // create connections
    if(!redisServer.requestConnection(RedisServer::ConnectionType::ReadWrite)) qFatal("Cannot establish ReadWrite connection to Redis Server!");
    if(!redisServer.requestConnection(RedisServer::ConnectionType::WriteOnly)) qFatal("Cannot establish WriteOnly connection to Redis Server!");
}

void TestRedisHash::clear()
{
    for(QByteArray key : redisServer.keys(GENKEYNAME("") + "*")->response()->array()) {
        if(!redisServer.del(key)->hasError()) qInfo("Delete key %s", qPrintable(key));
        else FAIL(QString("Cannot Delete key %1").arg(QString(key)));
    }
}

void TestRedisHash::redispoller()
{
    // init vars
    int pushValuesCount = 10000;

    // init poller and signal spyer
    RedisListPoller poller(redisServer, {"hallo", "test"}, RedisListPoller::PollTimeType::UntilTimeout, 1);
    QSignalSpy spy(&poller, SIGNAL(popped(QByteArray,QByteArray)));
    poller.start();

    // push pushValuesCount left
    for(int i = 0; i < pushValuesCount; i++) {
        redisServer.lpush("hallo", QByteArray::fromRawData((char*)&i, sizeof(int)));
    }

    // push pushValuesCount right
    for(int i = 0; i < pushValuesCount; i++) {
        redisServer.rpush("test", QByteArray::fromRawData((char*)&i, sizeof(int)));
    }

    // wait until timeout reached
    QEventLoop loop;
    loop.connect(&poller, SIGNAL(timeoutReached()), &loop, SLOT(quit())),
    loop.exec();

    // check return code
    QCOMPARE(spy.count(), pushValuesCount * 2);
}

void TestRedisHash::hash()
{
    // key index
    int index = 0;
    int count = 10000;

    // <float, double>-Test
    {
        QMap<float, double> data;
        int i = 0;
        while(data.count() != count) {
            i++;
            data.insert(i + GENFLOATRANDRANGE(0, 0.9), i + GENINTRANDRANGE(2343, 2324252) + GENFLOATRANDRANGE(0, 0.9));
        };
        for(RedisServer::RequestType type : {RedisServer::RequestType::PipeLine, RedisServer::RequestType::Asyncron, RedisServer::RequestType::Syncron}) {
            TestTemplateHelper<float, double>::hashInsert(data, ++index, type, true, true);
            TestTemplateHelper<float, double>::hashCheck(data, index, true, true);
            qInfo();
        }
    }

    // <QByteArray, int>-Test
    {
        QMap<QByteArray, int> data;
        while(data.count() != count) {
            QByteArray content;
            for(int i = 0; i < 20; i++) {
                content.append((char)GENINTRANDRANGE(0, 255));
            }
            data.insert(content, GENINTRANDRANGE(563,545321));
        };
        for(RedisServer::RequestType type : {RedisServer::RequestType::PipeLine, RedisServer::RequestType::Asyncron, RedisServer::RequestType::Syncron}) {
            TestTemplateHelper<QByteArray, int>::hashInsert(data, ++index, type, true, true);
            TestTemplateHelper<QByteArray, int>::hashCheck(data, index, true, true);
            qInfo();
        }
    }
}

QTEST_MAIN(TestRedisHash)
#include "testredishash.moc"
