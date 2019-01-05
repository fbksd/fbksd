#include "fbksd/core/SceneInfo.h"
#include <QtTest>
using namespace fbksd;


class TestSceneInfo : public QObject
{
     Q_OBJECT
private slots:

    void test()
    {
        SceneInfo info;
        QVERIFY(!info.has<bool>("none"));
        QVERIFY(!info.has<int64_t>("none"));
        QVERIFY(!info.has<float>("none"));
        QVERIFY(!info.has<std::string>("none"));

        info.set<bool>("bool", true);
        QVERIFY(info.has<bool>("bool"));
        QCOMPARE(info.get<bool>("bool"), true);

        info.set<int64_t>("int64", INT64_C(123));
        QVERIFY(info.has<int64_t>("int64"));
        QCOMPARE(info.get<int64_t>("int64"), INT64_C(123));

        info.set<float>("float", 0.123f);
        QVERIFY(info.has<float>("float"));
        QCOMPARE(info.get<float>("float"), 0.123f);

        info.set<std::string>("string", "str");
        QVERIFY(info.has<std::string>("string"));
        QCOMPARE(info.get<std::string>("string"), std::string("str"));
    }
};


QTEST_APPLESS_MAIN(TestSceneInfo)
#include "TestSceneInfo.moc"
