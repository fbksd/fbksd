
#include <iostream>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>
#include <QDir>
#include <QStringList>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/contrib/contrib.hpp>

using namespace cv;

namespace
{

bool sanitizeArgs(const QStringList& imgArgs)
{
    if(imgArgs.size() != 1)
    {
        std::cout << "You should specify only one input image." << std::endl;
        return false;
    }

    const QString& img1 = imgArgs[0];
    if(!QDir::current().exists(img1))
    {
        std::cout << img1.toStdString() << " not found." << std::endl;
        return false;
    }

    return true;
}


void TMO_Filmic(Mat& img)
{
    bool isHDR = false;
    if((img.depth() == CV_32F) || (img.depth() == CV_64F))
        isHDR = true;

    img.convertTo(img, CV_32F);
    if(isHDR)
    {
        for(int y=0; y<img.rows; y++)
        {
            for(int x=0; x<img.cols; x++)
            {
                Vec3f color = img.at<Vec3f>(Point(x,y));
                for(int c = 0; c < 3; ++c)
                {
                    float v = color[c];
                    v = std::max(0.f, v - 0.004f);
                    v = (v*(6.2f*v + 0.5f))/(v*(6.2f*v + 1.7f) + 0.06f);
                    color[c] = v;
                }
                img.at<Vec3f>(Point(x,y)) = color;
            }
        }
        img = 255.0 * img;
    }
}
}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("Compare utility for the benchmark system.");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("This tool converts an OpenEXR image into a PNG image using a Filmic tonemap." );
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("image1", "Input image.");
    QCommandLineOption outArg("o", "Output image.", "out_filepath");
    parser.addOption(outArg);
    parser.process(app);
    setlocale(LC_NUMERIC,"C");

    auto imgArgs = parser.positionalArguments();
    if(sanitizeArgs(imgArgs))
    {
        const QString& img1Filename = imgArgs[0];
        QString img2Filename;
        if(parser.isSet(outArg))
            img2Filename = parser.value(outArg);
        else
        {
            QFileInfo in(img1Filename);
            img2Filename = in.path() + QDir::separator() + in.baseName() + ".png";
        }
        std::cout << "out: " << img2Filename.toStdString() << std::endl;

        Mat img1 = imread(img1Filename.toStdString(), CV_LOAD_IMAGE_COLOR | CV_LOAD_IMAGE_ANYDEPTH);
        if(!img1.data)
        {
            std::cout << "Error when opening image 1" << std::endl;
            return 1;
        }

        TMO_Filmic(img1);
        imwrite(img2Filename.toStdString().c_str(), img1, {CV_IMWRITE_PNG_COMPRESSION, 9});
    }
    else
    {
        parser.showHelp();
    }

    return 0;
}
