#include "fbksd/client/BenchmarkClient.h"
#include "BenchmarkManager.h"
#include "tcp_utils.h"
#include <QtTest>
#include <cmath>

using namespace fbksd;

namespace
{
void startProcess(const QString& execPath, const QStringList& args, QProcess* process)
{
    QString logFilename = QFileInfo(execPath).baseName().append(".log");
    process->setStandardOutputFile(logFilename);
    process->setStandardErrorFile(logFilename);
    process->setWorkingDirectory(QFileInfo(execPath).absolutePath());
    process->start(QFileInfo(execPath).absoluteFilePath(), args);
    if(!process->waitForStarted(-1))
    {
        qDebug() << "Error starting process " << execPath;
        qDebug() << "Error code = " << process->error();
        exit(EXIT_FAILURE);
    }
}
}


class TestBenchmarkClient : public QObject
{
     Q_OBJECT
private slots:
    void initTestCase()
    {
        m_rendererProcess = std::make_unique<QProcess>();
        startProcess(RENDERER_FILE, {"--img-size", "300x300"}, m_rendererProcess.get());
        waitPortOpen(2227);
        m_manager = std::make_unique<BenchmarkManager>();
        m_manager->runPassive(4);
        waitPortOpen(2226);
        m_client = std::make_unique<BenchmarkClient>();
    }

    void getSceneInfo()
    {
        auto info = m_client->getSceneInfo();
        m_width = info.get<int64_t>("width");
        m_height = info.get<int64_t>("height");
        m_spp = info.get<int64_t>("max_spp");
        QCOMPARE(m_width, INT64_C(300));
        QCOMPARE(m_height, INT64_C(300));
        QCOMPARE(m_spp, INT64_C(4));
    }

    void setSampleLayout()
    {
        SampleLayout layout;
        layout("IMAGE_X")("IMAGE_Y")("COLOR_R")("COLOR_G")("COLOR_B");
        m_client->setSampleLayout(layout);
        m_sampleSize = layout.getSampleSize();
    }

    void evaluateSamples()
    {
        auto ncp = m_client->evaluateSamples(SPP(m_spp));
        QCOMPARE(ncp, m_spp * m_width * m_height);

        float* samples = m_client->getSamplesBuffer();
        for(int64_t y = 0; y < m_height; ++y)
        for(int64_t x = 0; x < m_width; ++x)
        {
            float* pixel =  &samples[y*m_width*m_spp*m_sampleSize + x*m_sampleSize*m_spp];
            for(int64_t s = 0; s < m_spp; ++s)
            for(int64_t c = 0; c < m_sampleSize; ++c)
            {
                float v =  pixel[s*m_sampleSize + c];
                float exp = getValue(x, y, s, c);
                if(!qFuzzyCompare(v, exp))
                {
                    QWARN(QString("pixel = (%1, %2); sample = %3; component = %4")
                          .arg(x).arg(y).arg(s).arg(c).toStdString().c_str());
                    QCOMPARE(v, exp);
                }
            }
        }
    }

    void cleanupTestCase()
    {
        m_client->sendResult();
        m_rendererProcess->kill();
        m_rendererProcess->waitForFinished();
    }

private:
    float getValue(int64_t x, int64_t y, int64_t s, int64_t c)
    {
        // FIXME: This function produces subnormal floats,
        // that may not be very good (lower performance).
        constexpr int64_t totalSampleSize = 41;
        constexpr int64_t map[] = {0, 1, 7, 8, 9};
        int64_t i = y * m_width * m_spp * totalSampleSize;
        i += x * totalSampleSize * m_spp;
        i+= s * totalSampleSize + map[c];
        int32_t k = i % std::numeric_limits<int32_t>::max();
        return *reinterpret_cast<float*>(&k);
    }

    std::unique_ptr<BenchmarkManager> m_manager;
    std::unique_ptr<QProcess> m_rendererProcess;
    std::unique_ptr<BenchmarkClient> m_client;
    int64_t m_width = 0;
    int64_t m_height = 0;
    int64_t m_spp = 0;
    int64_t m_sampleSize = 0;
};


QTEST_APPLESS_MAIN(TestBenchmarkClient)
#include "TestBenchmarkClient.moc"
