/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#ifndef RENDERINGSERVER_H
#define RENDERINGSERVER_H

#include "SamplesPipe.h"
#include "fbksd/core/definitions.h"
#include "fbksd/core/SharedMemory.h"
#include <map>
#include <memory>
#include <functional>

namespace rpc { class server; }

namespace fbksd
{

/**
 * \defgroup RenderingServer Rendering Server Library
 * @{
 */


/**
 * \brief The RenderingServer class implements the server by which a rendering system provides sampling
 * computations to the benchmark system.
 *
 * This class provides an asynchronous communication interface using signals. Each time a request is received,
 * the appropriate signal is emitted.
 *
 */
class RenderingServer
{
public:
    using GetSceneInfo
        = std::function<SceneInfo()>;
    using SetParameters
        = std::function<void(const SampleLayout& layout)>;
    using EvaluateSamples
        = std::function<bool(int64_t spp, int64_t remainingCount)>;
    using Finish
        = std::function<void()>;

    RenderingServer();

    RenderingServer(const RenderingServer&) = delete;

    RenderingServer(RenderingServer&&) = default;

    ~RenderingServer();

    void onGetSceneInfo(const GetSceneInfo& callback);

    void onSetParameters(const SetParameters& callback);

    void onEvaluateSamples(const EvaluateSamples& callback);

    void onFinish(const Finish& callback);

    void run();

    RenderingServer& operator=(const RenderingServer&) = delete;

    RenderingServer& operator=(RenderingServer&&) = default;

private:
    struct Imp;
    std::unique_ptr<Imp> m_imp;
};

/**@}*/

} // namespace fbksd

#endif // RENDERINGSERVER_H
