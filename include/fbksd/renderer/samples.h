/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#ifndef RENDERINGSERVERSAMPLES_H
#define RENDERINGSERVERSAMPLES_H

#include <vector>
#include <map>
#include <set>
#include <memory>

namespace fbksd
{

/**
 * \defgroup RenderingServer Rendering Server Library
 * @{
 */


/**
 * \brief The RandomParameter enum contains the supported random parameters.
 */
enum RandomParameter
{
    IMAGE_X,
    IMAGE_Y,
    LENS_U,
    LENS_V,
    TIME,
    LIGHT_X,
    LIGHT_Y,

    NUM_RANDOM_PARAMETERS
};


/**
 * \brief The Feature enum contains the supported features.
 */
enum Feature
{
    COLOR_R,
    COLOR_G,
    COLOR_B,
    DEPTH,
    DIRECT_LIGHT_R,
    DIRECT_LIGHT_G,
    DIRECT_LIGHT_B,
    WORLD_X,
    WORLD_Y,
    WORLD_Z,
    NORMAL_X,
    NORMAL_Y,
    NORMAL_Z,
    TEXTURE_COLOR_R,
    TEXTURE_COLOR_G,
    TEXTURE_COLOR_B,
    WORLD_X_1,
    WORLD_Y_1,
    WORLD_Z_1,
    NORMAL_X_1,
    NORMAL_Y_1,
    NORMAL_Z_1,
    TEXTURE_COLOR_R_1,
    TEXTURE_COLOR_G_1,
    TEXTURE_COLOR_B_1,
    WORLD_X_NS,
    WORLD_Y_NS,
    WORLD_Z_NS,
    NORMAL_X_NS,
    NORMAL_Y_NS,
    NORMAL_Z_NS,
    TEXTURE_COLOR_R_NS,
    TEXTURE_COLOR_G_NS,
    TEXTURE_COLOR_B_NS,

    NUM_FEATURES
};


inline std::set<std::string> getAllElements()
{
    return {
        "IMAGE_X",
        "IMAGE_Y",
        "LENS_U",
        "LENS_V",
        "TIME",
        "LIGHT_X",
        "LIGHT_Y",
        "COLOR_R",
        "COLOR_G",
        "COLOR_B",
        "DEPTH",
        "DIRECT_LIGHT_R",
        "DIRECT_LIGHT_G",
        "DIRECT_LIGHT_B",
        "WORLD_X",
        "WORLD_Y",
        "WORLD_Z",
        "NORMAL_X",
        "NORMAL_Y",
        "NORMAL_Z",
        "TEXTURE_COLOR_R",
        "TEXTURE_COLOR_G",
        "TEXTURE_COLOR_B",
        "WORLD_X_1",
        "WORLD_Y_1",
        "WORLD_Z_1",
        "NORMAL_X_1",
        "NORMAL_Y_1",
        "NORMAL_Z_1",
        "TEXTURE_COLOR_R_1",
        "TEXTURE_COLOR_G_1",
        "TEXTURE_COLOR_B_1",
        "WORLD_X_NS",
        "WORLD_Y_NS",
        "WORLD_Z_NS",
        "NORMAL_X_NS",
        "NORMAL_Y_NS",
        "NORMAL_Z_NS",
        "TEXTURE_COLOR_R_NS",
        "TEXTURE_COLOR_G_NS",
        "TEXTURE_COLOR_B_NS"
    };
}


/**
 * \brief Converts the feature \a f to it's numbered version with number \a i.
 *
 * Ex: toNumbered(WORLD_X, 1) returns WORLD_X_1.
 */
inline Feature toNumbered(Feature f, int i)
{
    // NOTE: This implementation is highly dependent on the order of the elements in the Feature enum.
    return Feature(f + i*9);
}


/**
 * \brief Converts a string to a RandomParameter. Returns true if succeeded.
 */
inline bool stringToRandomParameter(const std::string& name, RandomParameter* elel)
{
    static const std::map<std::string, RandomParameter> map = {{"IMAGE_X", IMAGE_X},
                                                             {"IMAGE_Y", IMAGE_Y},
                                                             {"LENS_U", LENS_U},
                                                             {"LENS_V", LENS_V},
                                                             {"TIME", TIME},
                                                             {"LIGHT_X", LIGHT_X},
                                                             {"LIGHT_Y", LIGHT_Y}};

    auto e = map.find(name);
    if(e != map.end())
    {
        *elel = e->second;
        return true;
    }
    return false;
}


/**
 * \brief Converts a string to a Feature. Returns true if succeeded.
 *
 * The string should not contain a number suffix. To obtain the numbered version of a feature, use \a toNumbered().
 * Ex: To obtain WORLD_X_1, use
 * \code{.cpp}
 *     Feature f;
 *     stringToFeature("WORLD_X", &f);
 *     f = toNumbered(f, 1);
 * \endcode
 */
inline bool stringToFeature(const std::string& name, Feature* elel)
{
    static const std::map<std::string, Feature> map = {{"COLOR_R", COLOR_R},
                                                       {"COLOR_G", COLOR_G},
                                                       {"COLOR_B", COLOR_B},
                                                       {"DEPTH", DEPTH},
                                                       {"DIRECT_LIGHT_R", DIRECT_LIGHT_R},
                                                       {"DIRECT_LIGHT_G", DIRECT_LIGHT_G},
                                                       {"DIRECT_LIGHT_B", DIRECT_LIGHT_B},
                                                       {"WORLD_X", WORLD_X},
                                                       {"WORLD_Y", WORLD_Y},
                                                       {"WORLD_Z", WORLD_Z},
                                                       {"NORMAL_X", NORMAL_X},
                                                       {"NORMAL_Y", NORMAL_Y},
                                                       {"NORMAL_Z", NORMAL_Z},
                                                       {"TEXTURE_COLOR_R", TEXTURE_COLOR_R},
                                                       {"TEXTURE_COLOR_G", TEXTURE_COLOR_G},
                                                       {"TEXTURE_COLOR_B", TEXTURE_COLOR_B},
                                                       {"WORLD_X_NS", WORLD_X_NS},
                                                       {"WORLD_Y_NS", WORLD_Y_NS},
                                                       {"WORLD_Z_NS", WORLD_Z_NS},
                                                       {"NORMAL_X_NS", NORMAL_X_NS},
                                                       {"NORMAL_Y_NS", NORMAL_Y_NS},
                                                       {"NORMAL_Z_NS", NORMAL_Z_NS},
                                                       {"TEXTURE_COLOR_R_NS", TEXTURE_COLOR_R_NS},
                                                       {"TEXTURE_COLOR_G_NS", TEXTURE_COLOR_G_NS},
                                                       {"TEXTURE_COLOR_B_NS", TEXTURE_COLOR_B_NS}};

    auto e = map.find(name);
    if(e != map.end())
    {
        *elel = e->second;
        return true;
    }
    return false;
}

/**@}*/

} // namespace fbksd

#endif
