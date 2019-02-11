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
 * \brief Implements the server that provides samples and scene information to FBKSD.
 *
 * This class uses a callback mechanism. The renderer should provide the appropriate
 * callback functions that will be called when the client makes the corresponding request.
 */
class RenderingServer
{
public:
    using GetTileSize
        = std::function<int()>;
    using GetSceneInfo
        = std::function<SceneInfo()>;
    using SetParameters
        = std::function<void(const SampleLayout& layout)>;
    using EvaluateSamples
        = std::function<void(int64_t spp, int64_t remainingCount, int pipeSize)>;
    using LastTileConsumed
        = std::function<void()>;
    using Finish
        = std::function<void()>;

    /**
     * @brief Creates a rendering server.
     */
    RenderingServer();

    RenderingServer(const RenderingServer&) = delete;

    RenderingServer(RenderingServer&&) = default;

    ~RenderingServer();

    /**
     * @brief Sets the GetTileSize callback.
     *
     * The callback is called just once when the renderer is started.
     */
    void onGetTileSize(const GetTileSize& callback);

    /**
     * @brief Sets the GetSceneInfo callback.
     *
     * The callback is called when the client asks for scene information.
     * The callback should return a SceneInfo object.
     *
     * Callback signature:
     * \code{.cpp}
     * SceneInfo callback();
     * \endcode
     */
    void onGetSceneInfo(const GetSceneInfo& callback);

    /**
     * @brief Sets the SetParameters callback.
     *
     * The callback is called when the client sets the sample layout.
     * The sample layout is passed to the callback.
     *
     * Callback signature:
     * \code{.cpp}
     * void callback(const SampleLayout& layout);
     * \endcode
     */
    void onSetParameters(const SetParameters& callback);

    /**
     * @brief Sets the EvaluateSamples callback.
     *
     * The callback is called every time the client asks for samples to be rendered.
     * The parameters passed to the callback are:
     * - spp: number of requested samples per pixel
     * - remainingCount: an extra number of samples (not multiple of the number of pixels)
     * - pipeSize: maximum number of samples allowable when creating a SamplesPipe.
     *
     * The callback should return true on success.
     *
     * Callback signature:
     * \code{.cpp}
     * bool callback(int64_t spp, int64_t remainingCount);
     * \endcode
     */
    void onEvaluateSamples(const EvaluateSamples& callback);

    /**
     * @brief Sets the LastTileConsumed callback.
     *
     * The callback is called when the client consumes the last tile.
     */
    void onLastTileConsumed(const LastTileConsumed& callback);

    /**
     * @brief Sets the Finish callback.
     *
     * the callback is called when the client wants the renderer to finish and exit.
     *
     * Callback signature:
     * \code{.cpp}
     * void callback();
     * \endcode
     */
    void onFinish(const Finish& callback);

    /**
     * @brief run
     */
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
