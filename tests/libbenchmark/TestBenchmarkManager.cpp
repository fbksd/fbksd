#include "BenchmarkManager.h"
#include <QtTest>


class TestBenchmarkManager : public QObject
{
     Q_OBJECT
private slots:
    void initTestCase()
    {
    }

    void test()
    {
        BenchmarkManager manager;
        manager.runScene(RENDERER_FILE, "", CLIENT_FILE, "", 1, 8);
    }
};


QTEST_GUILESS_MAIN(TestBenchmarkManager)
#include "TestBenchmarkManager.moc"
