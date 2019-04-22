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

    | Features            | Enumerable |
    | --------------------|------------|
    | COLOR_R             | no         |
    | COLOR_G             | no         |
    | COLOR_B             | no         |
    | DIRECT_LIGHT_R      | no         |
    | DIRECT_LIGHT_G      | no         |
    | DIRECT_LIGHT_B      | no         |
    | DEPTH               | no         |
    | NORMAL_X            | yes (0, 1) |
    | NORMAL_Y            | yes (0, 1) |
    | NORMAL_Z            | yes (0, 1) |
    | TEXTURE_COLOR_R     | yes (0, 1) |
    | TEXTURE_COLOR_G     | yes (0, 1) |
    | TEXTURE_COLOR_B     | yes (0, 1) |
    | WORLD_X_NS          | no         |
    | WORLD_Y_NS          | no         |
    | WORLD_Z_NS          | no         |
    | NORMAL_X_NS         | no         |
    | NORMAL_Y_NS         | no         |
    | NORMAL_Z_NS         | no         |
    | TEXTURE_COLOR_R_NS  | no         |
    | TEXTURE_COLOR_G_NS  | no         |
    | TEXTURE_COLOR_B_NS  | no         |

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

    MSGPACK_DEFINE_ARRAY(parameters)
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
};

} // namespace fbksd

MSGPACK_ADD_ENUM(fbksd::SampleLayout::ElementIO);

#endif
