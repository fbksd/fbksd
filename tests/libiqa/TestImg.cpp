#include "fbksd/iqa/iqa.h"
#include <QtTest>
using namespace fbksd;


class TestImg : public QObject
{
     Q_OBJECT
private slots:
    void ctor()
    {
        {
            Img img;
            QCOMPARE(img.width(), 0);
            QCOMPARE(img.height(), 0);
            QCOMPARE(img.data(), nullptr);
        }
        {
            Img img(2, 3);
            QCOMPARE(img.width(), 2);
            QCOMPARE(img.height(), 3);
            QVERIFY(img.data() != nullptr);
            for(int y = 0; y < img.height(); ++y)
            for(int x = 0; x < img.width(); ++x)
            for(int c = 0; c < 3; ++c)
                QCOMPARE(img(x, y, c), 0.f);
        }
        {
            Img img(3, 2, 0.5f);
            QCOMPARE(img.width(), 3);
            QCOMPARE(img.height(), 2);
            QVERIFY(img.data() != nullptr);
            for(int y = 0; y < img.height(); ++y)
            for(int x = 0; x < img.width(); ++x)
            for(int c = 0; c < 3; ++c)
                QCOMPARE(img(x, y, c), 0.5f);
        }
        {
            auto data = std::make_unique<float[]>(3 * 2 * 3);
            for(int y = 0; y < 2; ++y)
            for(int x = 0; x < 3; ++x)
            for(int c = 0; c < 3; ++c)
            {
                int i = y*9 + x*3 + c;
                float v = i*0.01f;
                data[i] = v;
            }

            Img img(3, 2, std::move(data));
            QCOMPARE(img.width(), 3);
            QCOMPARE(img.height(), 2);
            QVERIFY(img.data() != nullptr);
            for(int y = 0; y < 2; ++y)
            for(int x = 0; x < 3; ++x)
            for(int c = 0; c < 3; ++c)
            {
                float v = (y*9 + x*3 + c)*0.01f;
                QCOMPARE(img(x, y, c), v);
            }
        }
        {
            auto data = std::make_unique<float[]>(3 * 2 * 3);
            for(int y = 0; y < 2; ++y)
            for(int x = 0; x < 3; ++x)
            for(int c = 0; c < 3; ++c)
            {
                int i = y*9 + x*3 + c;
                float v = i*0.01f;
                data[i] = v;
            }

            Img img(3, 2, data.get());
            QCOMPARE(img.width(), 3);
            QCOMPARE(img.height(), 2);
            QVERIFY(img.data() != nullptr);
            for(int y = 0; y < 2; ++y)
            for(int x = 0; x < 3; ++x)
            for(int c = 0; c < 3; ++c)
            {
                int i = y*9 + x*3 + c;
                QCOMPARE(img(x, y, c), data[i]);
            }
        }
    }

    void copy()
    {
        Img img(3, 2);
        for(int y = 0; y < img.height(); ++y)
        for(int x = 0; x < img.width(); ++x)
        for(int c = 0; c < 3; ++c)
        {
            float v = (y*img.width()*3 + x*3 + c)*0.01f;
            img(x, y, c) = v;
        }
        {
            Img img2 = img;
            QCOMPARE(img2.width(), img.width());
            QCOMPARE(img2.height(), img.height());
            QVERIFY(img2.data() != img.data());
            for(int y = 0; y < img.height(); ++y)
            for(int x = 0; x < img.width(); ++x)
            for(int c = 0; c < 3; ++c)
                QCOMPARE(img2(x, y, c), img(x, y, c));
        }
        {
            Img img2;
            img2 = img;
            QCOMPARE(img2.width(), img.width());
            QCOMPARE(img2.height(), img.height());
            QVERIFY(img2.data() != img.data());
            for(int y = 0; y < img.height(); ++y)
            for(int x = 0; x < img.width(); ++x)
            for(int c = 0; c < 3; ++c)
                QCOMPARE(img2(x, y, c), img(x, y, c));
        }
    }

    void move()
    {
        auto data = std::make_unique<float[]>(3 * 2 * 3);
        for(int y = 0; y < 2; ++y)
        for(int x = 0; x < 3; ++x)
        for(int c = 0; c < 3; ++c)
        {
            int i = y*9 + x*3 + c;
            float v = i*0.01f;
            data[i] = v;
        }

        {
            Img img(3, 2, data.get());
            Img img2 = std::move(img);
            QVERIFY(img.data() == nullptr);
            QCOMPARE(img2.width(), img.width());
            QCOMPARE(img2.height(), img.height());
            QVERIFY(img2.data() != img.data());
            for(int y = 0; y < img.height(); ++y)
            for(int x = 0; x < img.width(); ++x)
            for(int c = 0; c < 3; ++c)
            {
                int i = y*9 + x*3 + c;
                QCOMPARE(img2(x, y, c), data[i]);
            }
        }
        {
            Img img(3, 2, data.get());
            Img img2;
            img2 = std::move(img);
            QVERIFY(img.data() == nullptr);
            QCOMPARE(img2.width(), img.width());
            QCOMPARE(img2.height(), img.height());
            QVERIFY(img2.data() != img.data());
            for(int y = 0; y < img.height(); ++y)
            for(int x = 0; x < img.width(); ++x)
            for(int c = 0; c < 3; ++c)
            {
                int i = y*9 + x*3 + c;
                QCOMPARE(img2(x, y, c), data[i]);
            }
        }
    }
};


QTEST_APPLESS_MAIN(TestImg)
#include "TestImg.moc"
