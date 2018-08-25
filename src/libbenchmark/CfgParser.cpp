#include "CfgParser.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>


/**
 * @brief A simple RAII class for changing the current directory.
 */
class CurrentPathGuard
{
public:
    CurrentPathGuard(const QString& path):
        m_prevPath(QDir::currentPath())
    { QDir::setCurrent(path); }

    ~CurrentPathGuard()
    { QDir::setCurrent(m_prevPath); }

private:
    QString m_prevPath;
};


BenchmarkConfig loadConfig(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    fileInfo.makeAbsolute();
    CurrentPathGuard currentPathGuard(fileInfo.path());

    QFile file(fileInfo.filePath());
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString errorStr = "Couldn't open config file \"" +
                            fileInfo.filePath() + "\".\n - " +
                            file.errorString();
        throw std::runtime_error(errorStr.toStdString());
    }

    QByteArray saveData = file.readAll();
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(saveData, &error);
    if(doc.isNull())
    {
        QString errorStr = "Error parsing config file.\n - " + error.errorString();
        throw std::runtime_error(errorStr.toStdString());
    }

    BenchmarkConfig config;
    auto root = doc.object();
    if(root.contains("renderers"))
    {
        auto renderers = root["renderers"].toArray();
        config.renderers.reserve(renderers.size());

        for(const auto rendererRef: renderers)
        {
            auto rendererObj = rendererRef.toObject();
            BenchmarkConfig::Renderer renderer;
            renderer.name = rendererObj["name"].toString();
            renderer.path = rendererObj["path"].toString();

            auto scenesArray = rendererObj["scenes"].toArray();
            renderer.scenes.reserve(scenesArray.size());
            for(const auto sceneRef: scenesArray)
            {
                auto sceneObj = sceneRef.toObject();
                BenchmarkConfig::Scene scene;
                scene.name = sceneObj["name"].toString();
                scene.path = sceneObj["path"].toString();

                auto sppsArray = sceneObj["spps"].toArray();
                scene.spps.reserve(sppsArray.size());
                for(const auto sppRef: sppsArray)
                    scene.spps.push_back(sppRef.toInt());

                auto sceneInfo = sceneObj["scene_info"].toObject();
                scene.info.set("has_area_light", sceneInfo["has_area_light"].toBool());
                scene.info.set("has_gi", sceneInfo["has_gi"].toBool());
                scene.info.set("has_motion_blur", sceneInfo["has_motion_blur"].toBool());
                scene.info.set("has_dof", sceneInfo["has_dof"].toBool());
                scene.info.set("has_glossy_materials", sceneInfo["has_glossy_materials"].toBool());

                renderer.scenes.push_back(scene);
            }

            config.renderers.push_back(renderer);
        }
    }

    return config;
}
