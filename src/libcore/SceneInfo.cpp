#include "fbksd/core/SceneInfo.h"
#include <iostream>


#define SceneInfo_NEW_TYPE(type, map) \
    template<> \
    void SceneInfo::set(const std::string& name, const type& value) \
    { map[name] = value; } \
    template<> \
    type SceneInfo::get(const std::string &name) const { \
        auto item = map.find(name); \
        if(item != map.end()){ \
            return item->second; \
        } \
        return type(); \
    }


SceneInfo_NEW_TYPE(float, floatMap)
SceneInfo_NEW_TYPE(int64_t, int64Map)
SceneInfo_NEW_TYPE(bool, boolMap)
SceneInfo_NEW_TYPE(std::string, stringMap)


template<>
void SceneInfo::set(const std::string &name, const int& value)
{
    int64Map[name] = value;
}

SceneInfo SceneInfo::merged(const SceneInfo& scene) const
{
    SceneInfo result = *this;

    for(auto& item : scene.int64Map)
        result.int64Map[item.first] = item.second;
    for(auto& item : scene.floatMap)
        result.floatMap[item.first] = item.second;
    for(auto& item : scene.boolMap)
        result.boolMap[item.first] = item.second;
    for(auto& item : scene.stringMap)
        result.stringMap[item.first] = item.second;

    return result;
}
