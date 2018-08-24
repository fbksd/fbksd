#include "fbksd/client/BenchmarkClient.h"
#include <QtTest>
#include <rpc/server.h>


class ServerMock
{
public:
    ServerMock()
    {
        m_server = std::make_unique<rpc::server>("127.0.0.1", 2226);
        m_server->bind("GET_SCENE_DESCRIPTION",
                       [this](){ return onGetSceneInfo(); } );
        m_server->bind("SET_SAMPLE_LAYOUT",
                       [this](const SampleLayout& layout){ return onSetLayout(layout); });
        m_server->bind("EVALUATE_SAMPLES",
                       [this](bool isSpp, int numSamples){ return onEvaluateSamples(isSpp, numSamples); });
        m_server->bind("SEND_RESULT",
                       [this](){ onSendResult(); });

        m_resultShm.setKey("SAMPLES_MEMORY");
        m_resultShm.create(sizeof(float));
        m_samplesShm.setKey("RESULT_MEMORY");
        m_samplesShm.create(sizeof(float));
    }

    const SampleLayout& getLayout() const
    {
        return m_layout;
    }

    void run()
    { m_server->async_run(); }

private:
    SceneInfo onGetSceneInfo()
    {
        SceneInfo info;
        info.set("width", 10);
        info.set("height", 20);
        return info;
    }

    void onSetLayout(const SampleLayout& layout)
    {
        m_layout = layout;
    }

    int onEvaluateSamples(bool isSPP, int numSamples)
    {
    }

    void onSendResult()
    {

    }

    std::unique_ptr<rpc::server> m_server;
    SampleLayout m_layout;
    SharedMemory m_resultShm;
    SharedMemory m_samplesShm;
};

namespace
{

}


class TestBenchmarkClient : public QObject
{
     Q_OBJECT
private slots:
    void initTestCase()
    {
        m_server = std::make_unique<ServerMock>();
        m_server->run();

        m_client = std::make_unique<BenchmarkClient>();
    }

    void getSceneInfo()
    {
        auto info = m_client->getSceneInfo();
        QCOMPARE(info.get<int64_t>("width"), INT64_C(10));
        QCOMPARE(info.get<int64_t>("height"), INT64_C(20));
    }

    void setSampleLayout()
    {
        SampleLayout layout;
        layout("IMAGE_X", SampleLayout::INPUT)("IMAGE_Y", SampleLayout::INPUT);
        m_client->setSampleLayout(layout);
        QCOMPARE(layout.hasInput("IMAGE_X"), m_server->getLayout().hasInput("IMAGE_X"));
        QCOMPARE(layout.hasInput("IMAGE_Y"), m_server->getLayout().hasInput("IMAGE_Y"));
        QCOMPARE(layout.hasInput("IMAGE_Z"), m_server->getLayout().hasInput("IMAGE_Z"));
    }

    void testSamplesSize()
    {
        constexpr int64_t w = 2000;
        constexpr int64_t h = 2000;
        constexpr int64_t spp = 128;
        constexpr int64_t sampleSize = 30;
        constexpr auto total = w * h * spp * sampleSize;
        QVERIFY(total < std::numeric_limits<size_t>::max());
    }

private:
    std::unique_ptr<ServerMock> m_server;
    std::unique_ptr<BenchmarkClient> m_client;
};


QTEST_APPLESS_MAIN(TestBenchmarkClient)
#include "TestBenchmarkClient.moc"
