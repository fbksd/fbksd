#include "fbksd/iqa/iqa.h"
#include <QtTest>
#include <iostream>
#include <sstream>
using namespace fbksd;


class StreamCapture
{
public:
    StreamCapture(std::ostream& stream):
        m_prev(stream.rdbuf(m_stream.rdbuf()))
    {}

    ~StreamCapture()
    { std::cout.rdbuf(m_prev); }

    std::string string()
    { return m_stream.str(); }

private:
    std::stringstream m_stream;
    std::streambuf* m_prev;
};


class TestIqa : public QObject
{
     Q_OBJECT
private slots:
    void report()
    {
        auto refFilename = QFINDTESTDATA("data/ref.exr").toStdString();
        auto testFilename = QFINDTESTDATA("data/test.exr").toStdString();
        std::vector<char> refArg(refFilename.c_str(), refFilename.c_str() + refFilename.size() + 1);
        std::vector<char> testArg(testFilename.c_str(), testFilename.c_str() + testFilename.size() + 1);
        const char* argv[] = {"TestIqa", refArg.data(), testArg.data()};
        IQA iqa(3, argv, "TEST_IQA", "Test IQA Metric", "", true, false);

        Img refImg;
        Img testImg;
        iqa.loadInputImages(refImg, testImg);
        QCOMPARE(refImg.width(), 300);
        QCOMPARE(refImg.height(), 300);
        QCOMPARE(refImg.width(), testImg.width());
        QCOMPARE(refImg.height(), testImg.height());

        StreamCapture cpt(std::cout);
        iqa.report(0.5);
        auto out = cpt.string();
        QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(out).toUtf8());
        QVERIFY(!doc.isNull());
        auto obj = doc.object();
        QCOMPARE(obj["TEST_IQA"].toDouble(), 0.5);
    }
};


QTEST_APPLESS_MAIN(TestIqa)
#include "TestIqa.moc"
