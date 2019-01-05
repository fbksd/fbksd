/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#ifndef SCENEINFO_H
#define SCENEINFO_H

#include <map>
#include <string>
#include <cstdint>
#include <rpc/msgpack.hpp>

namespace fbksd
{

/**
 * \brief The SceneInfo class gives information about the scene being rendered.
 *
 * The SceneInfo class stores information about the scene being rendered.
 * You can use this information to tune the parameters of your algorithm.
 *
 * For example, to get the image size maximum number of samples,
 * you do the following:
 * \snippet SceneInfo_snippet.cpp 0
 *
 * The available query names are:
 *
 * | Query             | Type       |
 * | ------------------|------------|
 * | width             | int64_t    |
 * | height            | int64_t    |
 * | has_motion_blur   | bool       |
 * | has_dof           | bool       |
 * | max_samples       | int64_t    |
 * | max_spp           | int64_t    |
 * | shutter_open      | float      |
 * | shutter_close     | float      |
 *
 * @note Not all query names are available for all scenes.
 *
 * \ingroup Core
 */
class SceneInfo
{
public:
    /**
     *	@brief Return bool if a info with the given name and type exists.
     */
    template<typename T>
    bool has(const std::string& name) const;

    /**
     * \brief Get the a info with the given type and name.
     *
     * If the info doesn't exist, returns T().
     */
    template<typename T>
    T get(const std::string& name) const;

    /**
     * \brief Set the a value with type T, key name, and value.
     */
    template<typename T>
    void set(const std::string& name, const T& value);

    /**
     * \brief Add all items from scene.
     *
     * If this SceneInfo and `scene` have items with the same key, they will be overwritten.
     */
    SceneInfo merged(const SceneInfo& scene) const;

    MSGPACK_DEFINE_ARRAY(int64Map, floatMap, boolMap, stringMap)
private:
    std::map<std::string, int64_t> int64Map;
    std::map<std::string, float> floatMap;
    std::map<std::string, bool> boolMap;
    std::map<std::string, std::string> stringMap;
};

} // namespace fbksd

#endif
