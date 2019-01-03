#include "fbksd/core/SharedMemory.h"
#include <QtTest>
using namespace fbksd;


class TestSharedMemory : public QObject
{
     Q_OBJECT
private slots:

    void test()
    {
        SharedMemory shm2("TEST_SHM");
        {
            SharedMemory shm("TEST_SHM");
            QVERIFY(shm.create(sizeof(float)));
            QCOMPARE(shm.size(), sizeof(float));
            float* data = static_cast<float*>(shm.data());
            QVERIFY(data != nullptr);
            data[0] = 0.5;

            QVERIFY(shm2.attach());
            QCOMPARE(static_cast<float*>(shm2.data())[0], 0.5);
        }
        QCOMPARE(static_cast<float*>(shm2.data())[0], 0.5);

        SharedMemory shm3("TEST_SHM");
        QVERIFY(!shm3.attach());

        SharedMemory shm4("NON_EXISTING_TEST_SHM");
        QVERIFY(!shm4.attach());
    }
};


QTEST_APPLESS_MAIN(TestSharedMemory)
#include "TestSharedMemory.moc"
