
#include "CfgParser.h"
#include <QtXml>

using namespace std;


//==========================================================================================================
//                                          BenchmarkConfig
//==========================================================================================================
void BenchmarkConfig::addRenderingServer(const string &name, const string &path, int port)
{
    Renderer renderer;
    renderer.name = name;
    renderer.path = path;
    renderer.port = port;
    renderers.push_back(renderer);
}

void BenchmarkConfig::addScene(int sceneID,
                               const string &name,
                               const string &path,
                               std::vector<int> spps,
                               std::vector<int> sppsIDs,
                               SceneInfo description)
{
    Scene scene;
    scene.ID = sceneID;
    scene.name = name;
    scene.path = path;
    scene.spps = spps;
    scene.sppsIDs = sppsIDs;
    scene.info = description;
    renderers.back().scenes.push_back(scene);
}



//==========================================================================================================
//                                          CfgParser
//==========================================================================================================
std::unique_ptr<BenchmarkConfig> CfgParser::load(const string &filename)
{
    QFileInfo fileInfo(QString::fromStdString(filename));
    fileInfo.makeAbsolute();
    QString oldCurrentPath = QDir::currentPath();
    QDir::setCurrent(fileInfo.path());

    QFile file(fileInfo.filePath());

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        cout << "Error opening the configuration file." << endl;
        return nullptr;
    }

    QDomDocument doc;

    QString errorStr;
    int errorLine;
    int errorColumn;
    if (!doc.setContent(&file, false, &errorStr, &errorLine, &errorColumn))
    {
        std::cerr << "Error: Parse error at line " << errorLine << ", "
                  << "column " << errorColumn << ": "
                  << qPrintable(errorStr) << std::endl;
        return nullptr;
    }

    QDomElement root = doc.documentElement();
    if (root.tagName() != "benchmark")
    {
        std::cerr << "Error: File is not a benchmark configuration file." << std::endl;
        return nullptr;
    }

    std::unique_ptr<BenchmarkConfig> config(new BenchmarkConfig);

    // Parse all child elements
    QDomElement element = root.firstChildElement();
    while(!element.isNull())
    {
        if(element.tagName() == "benchmark_settings")
            parseBenchmarkSettings(element, config.get());
        else if(element.tagName() == "rendering_server")
            parseRenderingServer(element, config.get());

        element = element.nextSiblingElement();
    }

    QDir::setCurrent(oldCurrentPath);
    return config;
}

void CfgParser::parseBenchmarkSettings(const QDomElement &/*element*/, BenchmarkConfig */*manager*/)
{
//    QDomElement child = element.firstChildElement();

//    while(!child.isNull())
//    {
//        if(child.attribute("name") == "sppCounts")
//        {
//            QString values = child.attribute("value");
//            QStringList list = values.split(" ", QString::SkipEmptyParts);
//            vector<int> spps;
//            for(int i = 0; i < list.size(); ++i)
//                spps.push_back(list[i].toInt());

//            manager->setSppCounts(spps);
//        }

//        child = child.nextSiblingElement();
//    }
}

void CfgParser::parseRenderingServer(const QDomElement &element, BenchmarkConfig *config)
{
    QString render_name = element.attribute("name");
    QString render_path = element.attribute("path");
    QString render_port = element.attribute("port");
    config->addRenderingServer(render_name.toStdString(), render_path.toStdString(), render_port.toInt());

    QDomElement child = element.firstChildElement();
    if(!child.isNull() && child.tagName() == "scenes")
        parseScenes(child, config);
}

void CfgParser::parseScenes(const QDomElement &element, BenchmarkConfig *config)
{
    QDomElement child = element.firstChildElement("scene");
    while(!child.isNull())
    {
        int sceneID = child.attribute("id").toInt();
        QString name = child.attribute("name");
        QString path = child.attribute("path");
        vector<int> sppsIDs;
        vector<int> spps;
        SceneInfo desc;

        QDomElement desc_node = child.firstChildElement();
        while(!desc_node.isNull())
        {
            if(desc_node.tagName() == "scene_description")
            {
                desc = parseSceneInfo(desc_node);
            }
            else if(desc_node.tagName() == "spps")
            {
                QString att = desc_node.attribute("ids");
                QStringList list = att.split(" ", QString::SkipEmptyParts);
                for(int i = 0; i < list.size(); ++i)
                    sppsIDs.push_back(list[i].toInt());

                att = desc_node.attribute("values");
                list = att.split(" ", QString::SkipEmptyParts);
                for(int i = 0; i < list.size(); ++i)
                    spps.push_back(list[i].toInt());
            }

            desc_node = desc_node.nextSiblingElement();
        }

        config->addScene(sceneID, name.toStdString(), path.toStdString(), spps, sppsIDs, desc);
        child = child.nextSiblingElement("scene");
    }
}

SceneInfo CfgParser::parseSceneInfo(const QDomElement &element)
{
    SceneInfo desc;

    QDomElement child = element.firstChildElement();
    while(!child.isNull())
    {
        if(child.attribute("name") == "hasDOF")
            desc.set<bool>("has_dof", child.attribute("value") == "false" ? false : true);
        else if(child.attribute("name") == "hasMotionBlur")
            desc.set<bool>("has_motion_blur", child.attribute("value") == "false" ? false : true);
        else if(child.attribute("name") == "hasAreaLight")
            desc.set<bool>("has_area_lights", child.attribute("value") == "false" ? false : true);
        else if(child.attribute("name") == "hasTextures")
            desc.set<bool>("has_textures", child.attribute("value") == "false" ? false : true);
        else if(child.attribute("name") == "hasDiffuseMaterials")
            desc.set<bool>("has_diffuse_materials", child.attribute("value") == "false" ? false : true);
        else if(child.attribute("name") == "hasSpecularMaterials")
            desc.set<bool>("has_specular_materials", child.attribute("value") == "false" ? false : true);
        else if(child.attribute("name") == "hasGlossyMaterials")
            desc.set<bool>("has_glossy_materials", child.attribute("value") == "false" ? false : true);

        child = child.nextSiblingElement();
    }

    return desc;
}
