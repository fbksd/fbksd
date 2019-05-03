/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#ifndef SAMPLELAYOUT_H
#define SAMPLELAYOUT_H

#include <vector>
#include <string>
#include <set>
#include <rpc/msgpack.hpp>

namespace fbksd
{

/**
   \brief The SampleLayout class permits to specify the layout of the samples in memory.

    The sample layout is the order in which the sample elements appear in memory.

    To specify a layout, use the operator(), like this:
    \snippet BenchmarkClient_snippet.cpp 0
    In this case the samples will follow the specified layout in memory, e.g:
    \a sample = [<IMAGE_X>, <IMAGE_Y>, <COLOR_R>, <COLOR_G>, <COLOR_B>, <repeat...>].

    The elements can be random parameters, or features. The available random parameters and features are:

    | Random parameters |
    | ------------------|
    | IMAGE_X           |
    | IMAGE_Y           |
    | LENS_U            |
    | LENS_V            |
    | TIME              |
    | LIGHT_X           |
    | LIGHT_Y           |

    | Features            | Enumerable | Description                                                         |
    | --------------------|------------|---------------------------------------------------------------------|
    | COLOR_R             | no         | Final sample radiance value                                         |
    | COLOR_G             | no         |                                                                     |
    | COLOR_B             | no         |                                                                     |
    | DIRECT_LIGHT_R      | no         | Incident direct light value on the first intersection point         |
    | DIRECT_LIGHT_G      | no         |                                                                     |
    | DIRECT_LIGHT_B      | no         |                                                                     |
    | DEPTH               | no         | Depth of the first intersection point                               |
    | NORMAL_X            | yes (0, 1) | World normal                                                        |
    | NORMAL_Y            | yes (0, 1) |                                                                     |
    | NORMAL_Z            | yes (0, 1) |                                                                     |
    | TEXTURE_COLOR_R     | yes (0, 1) | Texture value (albedo)                                              |
    | TEXTURE_COLOR_G     | yes (0, 1) |                                                                     |
    | TEXTURE_COLOR_B     | yes (0, 1) |                                                                     |
    | WORLD_X_NS          | no         | World position on the first non-specular intersection point         |
    | WORLD_Y_NS          | no         |                                                                     |
    | WORLD_Z_NS          | no         |                                                                     |
    | NORMAL_X_NS         | no         | World normal on the first non-specular intersection point           |
    | NORMAL_Y_NS         | no         |                                                                     |
    | NORMAL_Z_NS         | no         |                                                                     |
    | TEXTURE_COLOR_R_NS  | no         | Texture value (albedo) on the first non-specular intersection point |
    | TEXTURE_COLOR_G_NS  | no         |                                                                     |
    | TEXTURE_COLOR_B_NS  | no         |                                                                     |
    | DIFFUSE_COLOR_R     | no         | Diffuse component of the final sample radiance                      |
    | DIFFUSE_COLOR_G     | no         |                                                                     |
    | DIFFUSE_COLOR_B     | no         |                                                                     |

    When implementing adaptive techniques, you may want to generate your own random parameters. In this case,
    random parameters can be given an optional SampleLayout::ElementIO flag, specifying the element as input
    (the user generates the element and gives it as input to the rendering system)
    or output (the rendering system generates the element). All features are always considered input.
    You can set the SampleLayout::ElementIO flag when specifying the layout:
    \snippet BenchmarkClient_snippet.cpp 1
    or latter, using the setElementIO() method:
    \snippet BenchmarkClient_snippet.cpp 2

    Some features can appear more than once, for example, in a path tracing renderer, the user may want access to
    the world position of the first two intersections. These features are called enumerable (see Features table), and can be given an index
    specifying the corresponding intersection point (from 0 to N). The total index N depends on the kind of scene and integrator being used.
    \snippet BenchmarkClient_snippet.cpp 3

    \ingroup Core
 */
class SampleLayout
{
public:
    /**
     * \brief Used to specify if a sample element is \e input or \e output.
     */
    enum ElementIO : bool
    {
        OUTPUT = false,
        INPUT = true,
    };

    /**
     * \brief Adds an element to the sample layout.
     *
     * \param name Name of the element
     * \param io   Defines the element as OUTPUT or INPUT
     */
    SampleLayout& operator()(const std::string& name, ElementIO io = OUTPUT);

    /**
     * \brief Defines the number of the last added element.
     */
    SampleLayout& operator[](int num);

    /**
     * \brief Sets the element name as \a input or \a output.
     */
    SampleLayout& setElementIO(const std::string& name, ElementIO io);

    SampleLayout& setElementIO(int index, ElementIO io);

    /**
     * \brief Returns the number of elements in the layout.
     */
    int getSampleSize() const;

    /**
     * \brief Returns the number of INPUT elements in the layout.
     */
    int getInputSize() const;

    /**
     * \brief Returns the number of OUTPUT elements in the layout.
     *
     * If you already have the input size, it's cheaper to call getSampleSize() - inputSize.
     */
    int getOutputSize() const;

    /**
     * \brief Checks if the element `name` exists and is INPUT.
     */
    bool hasInput(const std::string& name) const;

    /**
     * \brief Checks if any element is INPUT.
     */
    bool hasInput() const;

    /**
     * @brief Sets the Beckmann roughness threshold used to decompose the diffuse sample radiance values.
     *
     * The features `DIFFUSE_COLOR_{R, G, B}`, contain the diffuse part of the final sample radiance values (`COLOR_{R,G,B}`).
     * A light-surface interaction is considered diffuse when the Beckmann roughness (alpha value) of the material
     * is >= than the threshold.
     *
     * Increasing the threshold causes less energy to be included in the diffuse color features.
     *
     * @arg a Beckmann roughness value in the [0, inf) range.
     */
    void setRoughnessThreshold(float a);

    /**
     * @brief Returns the Beckmann roughness threshold value.
     *
     * The default value is 0.1.
     */
    float getRoughnessThreshold() const;

    MSGPACK_DEFINE_ARRAY(parameters, m_roughness)
private:
    friend class SampleAdapter;
    friend class SampleBuffer;
    friend class SamplesPipe;
    friend class BenchmarkManager;
    friend class TileSamplesPipe;
    friend class TilePool;

    bool isValid(const std::set<std::string>& reference) const;

    struct ParameterEntry
    {
        ParameterEntry() :
            name(""),
            number(0),
            io(OUTPUT)
        {}

        std::string name;
        int number;
        ElementIO io;

        MSGPACK_DEFINE_ARRAY(name, number, io)
    };
    std::vector<ParameterEntry> parameters;
    float m_roughness = 0.1f;
};

} // namespace fbksd

MSGPACK_ADD_ENUM(fbksd::SampleLayout::ElementIO);

#endif
