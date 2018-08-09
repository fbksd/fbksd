#include "fbksd/core/SceneInfo.h"
#include <iostream>
#include <QDataStream>


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
        std::cout << "SceneInfo: no scene attribute called " << name << std::endl; \
        return type(); \
    }

SceneInfo_NEW_TYPE(float, floatMap)
SceneInfo_NEW_TYPE(int, intMap)
SceneInfo_NEW_TYPE(bool, boolMap)
SceneInfo_NEW_TYPE(std::string, stringMap)


SceneInfo SceneInfo::merged(const SceneInfo& scene) const
{
    SceneInfo result = *this;

    for(auto& item : scene.intMap)
        result.intMap[item.first] = item.second;
    for(auto& item : scene.floatMap)
        result.floatMap[item.first] = item.second;
    for(auto& item : scene.boolMap)
        result.boolMap[item.first] = item.second;
    for(auto& item : scene.stringMap)
        result.stringMap[item.first] = item.second;

    return result;
}


QDataStream& operator<<(QDataStream& stream, const SceneInfo& desc)
{
    stream << (int)desc.intMap.size();
    for(auto it = desc.intMap.begin(); it != desc.intMap.end(); ++it)
        stream << QString::fromStdString(it->first) << it->second;

    stream << (int)desc.floatMap.size();
    for(auto it = desc.floatMap.begin(); it != desc.floatMap.end(); ++it)
        stream << QString::fromStdString(it->first) << it->second;

    stream << (int)desc.boolMap.size();
    for(auto it = desc.boolMap.begin(); it != desc.boolMap.end(); ++it)
        stream << QString::fromStdString(it->first) << it->second;

    stream << (int)desc.stringMap.size();
    for(auto it = desc.stringMap.begin(); it != desc.stringMap.end(); ++it)
        stream << QString::fromStdString(it->first) << QString::fromStdString(it->second);

    return stream;
}

QDataStream& operator>>(QDataStream& stream, SceneInfo& desc)
{
    int size = 0;
    QString key;
    int intValue = 0;
    float floatValue = 0.f;
    bool boolValue = false;
    QString stringValue;

    stream >> size;
    for(int i = 0; i < size; ++i)
    {
        stream >> key >> intValue;
        desc.set<int>(key.toStdString(), intValue);
    }

    stream >> size;
    for(int i = 0; i < size; ++i)
    {
        stream >> key >> floatValue;
        desc.set<float>(key.toStdString(), floatValue);
    }

    stream >> size;
    for(int i = 0; i < size; ++i)
    {
        stream >> key >> boolValue;
        desc.set<bool>(key.toStdString(), boolValue);
    }

    stream >> size;
    for(int i = 0; i < size; ++i)
    {
        stream >> key >> stringValue;
        desc.set<std::string>(key.toStdString(), stringValue.toStdString());
    }

    return stream;
}
