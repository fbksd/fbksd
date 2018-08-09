#ifndef SCENEFILE_H
#define SCENEFILE_H

#include "fbksd/core/SceneInfo.h"
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <memory>

class QFile;
class QDomDocument;
class QDomElement;
class QDomNode;
class QString;


class BenchmarkConfig
{
public:
    void addRenderingServer(const std::string& name,
                            const std::string& path,
                            int port);

    void addScene(int sceneID,
                  const std::string& name,
                  const std::string& path,
                  std::vector<int> spps,
                  std::vector<int> sppsIDs,
                  SceneInfo description);

    struct Scene
    {
        int ID;
        std::string name;
        std::string path;
        std::vector<int> spps;
        std::vector<int> sppsIDs;
        SceneInfo info;
    };

    struct Renderer
    {
        std::string name;
        std::string path;
        int port;
        std::vector<Scene> scenes;
    };

    std::vector<Renderer> renderers;
};


/**
 * \brief The CfgParser class
 *
 * \ingroup BenchmarkServer
 */
class CfgParser
{
public:
    static std::unique_ptr<BenchmarkConfig> load(const std::string& filename);

private:
    static void parseBenchmarkSettings(const QDomElement &element, BenchmarkConfig *config);
    static void parseRenderingServer(const QDomElement &element, BenchmarkConfig *config);
    static void parseScenes(const QDomElement &element, BenchmarkConfig *config);
    static SceneInfo parseSceneInfo(const QDomElement &config);
};

#endif // SCENEFILE_H
