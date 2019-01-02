/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "BenchmarkManager.h"

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>
#include <QDir>
#include <QStringList>
#include <iostream>
#include <memory>


bool sanitizeArgs(const QStringList& inputFiles, const QStringList& outputFolders)
{
    for(const QString& inFile: inputFiles)
    {
        if(!QDir::current().exists(inFile))
        {
            std::cout << inFile.toStdString() << " not found." << std::endl;
            return false;
        }
    }

    for(const QString& outFolder: outputFolders)
    {
        if(!QDir(outFolder).exists() && !QDir::current().mkpath(outFolder))
        {
            std::cout << "Output folder " << outFolder.toStdString() << " could not be created." << std::endl;
            return false;
        }
    }

    return true;
}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("fbksd core application");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption configOption("config", "Configuration file.", "config_file");
    parser.addOption(configOption);
    QCommandLineOption rendererOption("renderer", "Renderer executable file", "renderer_file");
    parser.addOption(rendererOption);
    QCommandLineOption sceneOption("scene", "Scene file", "scene_file");
    parser.addOption(sceneOption);
    QCommandLineOption clientOption("filter", "Executable file of the filter to run the benchmark on.", "executable");
    parser.addOption(clientOption);
    QCommandLineOption outputOption("output", "Folder where the results will be saved (current directory is the default).", "folder");
    parser.addOption(outputOption);
    QCommandLineOption resumeOption("resume", "Skip scenes that already have results in the 'output' directory.");
    parser.addOption(resumeOption);
    QCommandLineOption repeatOption("n", "Repeat each benchmark n times.", "n");
    parser.addOption(repeatOption);
    QCommandLineOption sppOption("spp", "Number of samples per pixel", "spp");
    parser.addOption(sppOption);

    parser.process(app);
    setlocale(LC_NUMERIC,"C");

    if(parser.isSet(configOption) && parser.isSet(rendererOption))
    {
        std::cout << "Set --config or --renderer, not both." << std::endl;
        exit(EXIT_FAILURE);
    }

    if(parser.isSet(clientOption))
    {
        QString asrClient = parser.value(clientOption);
        QString outputFolder;
        if(parser.isSet(outputOption))
            outputFolder = parser.value(outputOption);
        else
            outputFolder = QDir::currentPath();

        int n = 1;
        if(parser.isSet(repeatOption))
        {
            bool ok = false;
            int tn = 0;
            tn = parser.value(repeatOption).toInt(&ok);
            if(ok)
                n = tn;
        }

        if(parser.isSet(rendererOption) && parser.isSet(sceneOption))
        {
            QString renderer = parser.value(rendererOption);
            QString scene = parser.value(sceneOption);

            int spp = 0;
            if(parser.isSet(sppOption))
            {
                bool ok = false;
                int tn = 0;
                tn = parser.value(sppOption).toInt(&ok);
                if(ok)
                    spp = tn;
            }

            if(sanitizeArgs({renderer, scene, asrClient}, {outputFolder}))
            {
                std::unique_ptr<BenchmarkManager> manager(new BenchmarkManager());
                manager->runScene(renderer, scene, asrClient, outputFolder, n, spp);
            }
            else
                app.exit(EXIT_FAILURE);
        }
        else if(parser.isSet(configOption))
        {
            QString configFileName = parser.value(configOption);

            if(sanitizeArgs({configFileName, asrClient}, {outputFolder}))
            {
                std::unique_ptr<BenchmarkManager> manager(new BenchmarkManager());
                manager->runAll(configFileName, asrClient, outputFolder, n, parser.isSet(resumeOption));
            }
            else
                app.exit(EXIT_FAILURE);
        }
    }
    else
    {
        parser.showHelp();
    }

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &app, &QCoreApplication::quit);
    timer.start();
    return app.exec();
}
