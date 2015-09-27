#include <QCoreApplication>

#include "test.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // start test
    Test test;
    QMetaObject::invokeMethod(&test, "start", Qt::QueuedConnection);

    return a.exec();
}
