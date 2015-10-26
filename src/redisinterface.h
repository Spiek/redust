#ifndef REDISMAPPRIVATE_H
#define REDISMAPPRIVATE_H

// qt core
#include <QString>
#include <QByteArray>
#include <QMutex>

// redis
#include "redismapconnectionmanager.h"

class RedisInterface
{
    public:
        RedisInterface(QString list, QString connectionName = "redis");
        void del(bool async = true);
        int hlen();
        bool insert(QByteArray key, QByteArray value, bool waitForAnswer = false);
        bool contains(QByteArray key);
        bool exists();
        bool remove(QByteArray key, bool waitForAnswer = true);
        QByteArray value(QByteArray key);
        void fetchKeys(QList<QByteArray>* data, int count = 100, int pos = 0, int *newPos = 0);
        void fetchValues(QList<QByteArray>* data, int count = 100, int pos = 0, int *newPos = 0);
        void fetchAll(QList<QByteArray>* data, int count = 100, int pos = 0, int *newPos = 0);

        // command simplifier
        void simplifyHScan(QList<QByteArray> *data, int count, int pos, bool key, bool value, int *newPos);

    private:
        // helper
        bool execRedisCommand(std::initializer_list<QByteArray> cmd, QByteArray* result = 0, QList<QByteArray> *lstResultArray1 = 0, QList<QByteArray> *lstResultArray2 = 0);

        // data
        QString connectionName;
        QByteArray redisList;
};

#endif // REDISMAPPRIVATE_H
