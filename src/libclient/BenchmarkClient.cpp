/*
 * Copyright (c) 2018 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "fbksd/client/BenchmarkClient.h"
#include "fbksd/core/definitions.h"
#include "fbksd/core/SharedMemory.h"
#include "BenchmarkManager.h"
#include "tcp_utils.h"
#include "version.h"

#include <rpc/client.h>
#include <iostream>
#include <QFileInfo>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using namespace fbksd;


/*
 * ===== Note about the use of shared memory ======
 *
 * On Linux, if a process attached to a shared memory block crashes,
 * the memory will not be correctly detached, causing the memory block to not be
 * released by the SO. This will prevent the creation of the block with
 * the same key on a subsequent run of the system.
 *
 * In this case, you have to manually delete the "zombie" shared memory block:
 *  - run 'ipcs -m' end look for a block with zero nattch;
 *  - run 'ipcrm -m <shmid>' to delete the block.
 *
 * You can also see what processes are using the shm blocks:
 *  - 'lsof | egrep "<shmid>|COMMAND"'.
 *
 * This command deletes blocks with zero nattch automatically:
 * $ ipcs -m | awk '$6 == 0' | tr -s " " | cut -d " " -f 2 | xargs -L1 ipcrm -m
 */

namespace{
void startProcess(const QString& execPath, const QStringList& args, QProcess* process)
{
    process->start(QFileInfo(execPath).absoluteFilePath(), args);
    if(!process->waitForStarted(-1))
    {
        std::cerr << "Error starting process " << execPath.toStdString() << std::endl;
        std::cerr << "Error code = " << process->errorString().toStdString() << std::endl;
        exit(EXIT_FAILURE);
    }
}
}


struct BenchmarkClient::Imp
{
    Imp(int argc, char* argv[]):
        m_samplesMemory("SAMPLES_MEMORY"),
        m_resultMemory("RESULT_MEMORY")
    {
        if(argc != 0 && argv != nullptr)
        {
            po::options_description desc("Allowed options");
            desc.add_options()
                    ("fbksd-version", "Print version number.")
                    ("fbksd-renderer", po::value<std::string>(), "Calls a renderer server.")
                    ("fbksd-spp", po::value<int>(), "Number of samples poer pixel.");

            po::variables_map vm;
            po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
            po::notify(vm);

            if(vm.count("fbksd-version"))
            {
                std::cout << FBKSD_VERSION_MAJOR << "." << FBKSD_VERSION_MINOR << "." << FBKSD_VERSION_PATCH << std::endl;
                exit(0);
            }

            if(vm.count("fbksd-renderer") || vm.count("fbksd-spp"))
            {
                std::cout << "(fbksd) Running in bypass mode." << std::endl;
                if(vm.count("fbksd-renderer"))
                {
                    QString renderCall = QString::fromStdString(vm["fbksd-renderer"].as<std::string>());
                    std::cout << "(fbksd) Starting renderer with call \"" << renderCall.toStdString() << "\"" << std::endl;
                    auto values = renderCall.split(" ");
                    m_rendererProcess = std::make_unique<QProcess>();
                    auto args = values.mid(1, values.size()-1);
                    startProcess(values[0], args, m_rendererProcess.get());
                }
                std::cout << "(fbksd) Waiting for renderer port..." << std::endl;
                waitPortOpen(2227);
                std::cout << "(fbksd) Renderer port open." << std::endl;

                int spp = 1;
                if(vm.count("fbksd-spp"))
                    spp = vm["fbksd-spp"].as<int>();
                std::cout << "(fbksd) spp = " << spp << std::endl;

                m_bmkManager = std::make_unique<BenchmarkManager>();
                m_bmkManager->runPassive(spp);
                waitPortOpen(2226);
            }
        }

        m_client = std::make_unique<rpc::client>("127.0.0.1", 2226);

        // verify server version compatibility
        auto version = m_client->call("GET_VERSION").as<std::pair<int,int>>();
        if(version.first != FBKSD_VERSION_MAJOR)
        {
            auto error = std::string("Benchmark server version (") +
                    std::to_string(version.first) + ") is incompatible with client version (" +
                    std::to_string(FBKSD_VERSION_MAJOR) + ").";
            throw std::runtime_error(error);
        }
    }

    ~Imp()
    {
        if(m_rendererProcess)
        {
            m_rendererProcess->kill();
            m_rendererProcess->waitForFinished();
        }
    }

    void fetchSceneInfo()
    {
        m_sceneInfo = m_client->call("GET_SCENE_DESCRIPTION").as<SceneInfo>();
        m_maxNumSamples = m_sceneInfo.get<int64_t>("max_samples");

        if(!m_resultMemory.attach())
            throw std::runtime_error("Couldn't attach result shared memory:\n - " + m_resultMemory.error());
    }

    std::unique_ptr<rpc::client> m_client;
    SharedMemory m_samplesMemory;
    SharedMemory m_resultMemory;
    SceneInfo m_sceneInfo;
    int64_t m_maxNumSamples = 0;

    //bypass mode
    std::unique_ptr<BenchmarkManager> m_bmkManager;
    std::unique_ptr<QProcess> m_rendererProcess;
};


BenchmarkClient::BenchmarkClient(int argc, char* argv[]):
    m_imp(std::make_unique<Imp>(argc, argv))
{
    if(m_imp->m_samplesMemory.isAttached())
        throw std::logic_error("Shared memory already attached.");

    m_imp->fetchSceneInfo();
}

BenchmarkClient::~BenchmarkClient() = default;

SceneInfo BenchmarkClient::getSceneInfo()
{
    return m_imp->m_sceneInfo;
}

void BenchmarkClient::setSampleLayout(const SampleLayout& layout)
{
   m_imp-> m_client->call("SET_SAMPLE_LAYOUT", layout);

    if(!m_imp->m_samplesMemory.attach())
        throw std::runtime_error("Couldn't attach samples shared memory:\n - " + m_imp->m_samplesMemory.error());
}

float *BenchmarkClient::getSamplesBuffer()
{
    return static_cast<float*>(m_imp->m_samplesMemory.data());
}

float *BenchmarkClient::getResultBuffer()
{
    return static_cast<float*>(m_imp->m_resultMemory.data());
}

int64_t BenchmarkClient::evaluateSamples(int64_t numSamples)
{
    return m_imp->m_client->call("EVALUATE_SAMPLES", false, numSamples).as<int64_t>();
}

int64_t BenchmarkClient::evaluateSamples(SPP spp)
{
    return m_imp->m_client->call("EVALUATE_SAMPLES", true, spp.getValue()).as<int64_t>();
}

void BenchmarkClient::sendResult()
{
    m_imp->m_client->call("SEND_RESULT");
}
