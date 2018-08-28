#ifndef RENDERCLIENT_H
#define RENDERCLIENT_H

#include "fbksd/core/Definitions.h"

#include <vector>
#include <string>

class BenchmarkManager;
namespace rpc { class client; }


/**
 * \brief The RenderClient class is used by the BenchmarkManager to communicate with a rendering server
 *
 * \ingroup BenchmarkServer
 */
class RenderClient
{
public:
    RenderClient(BenchmarkManager* manager, int port);

    ~RenderClient();

    /**
     * \brief Requests scene information from the rendering system. This information is obtained from the
     * current scene being rendered.
     *
     * \return SceneInfo
     */
    SceneInfo getSceneInfo();

    /**
     * \brief Detaches shared memory
     */
    void detachMemory();

    /**
     * \brief Informs the sample layout required by the ASR technique.
     *
     * \param[in] SampleLayout: layout
     */
    void setParameters(const SampleLayout& layout);

    /**
     * \brief Compute the samples values for the given samples positions.
     *
     * \param[in] isSPP         Flag indicating the unit of `numSamples`.
     * \param[in] numSamples    Number of samples.
     * \param[in] inSize        Number of features in the samples array.
     * \param[in] samples       Input sample values.
     * \param[in] outSize       Number of features in the output array.
     * \param[out] output       Output sample values.
     * \return Number of samples evaluated.
     */
    bool evaluateSamples(int64_t spp, int64_t remainintCount);

    /**
     * \brief Finishes the rendering system for the current scene.
     */
    void finishRender();

private:
    BenchmarkManager *manager;
    std::unique_ptr<rpc::client> m_client;
};


#endif
