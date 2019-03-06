/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "fbksd/iqa/iqa.h"

#include <iostream>
#include <iomanip>
#include <limits>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfRgbaFile.h>
#include <OpenEXR/ImfArray.h>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <png.h>
namespace po = boost::program_options;
namespace fs = boost::filesystem;
using namespace Imath;
using namespace Imf;
using namespace fbksd;

namespace
{
void readEXRImg(const std::string& fileName, fbksd::Img& img)
{
    Array2D<Rgba> pixels;
    RgbaInputFile file(fileName.c_str());
    Box2i dw = file.dataWindow();
    int width = dw.max.x - dw.min.x + 1;
    int height = dw.max.y - dw.min.y + 1;
    pixels.resizeErase(height, width);
    file.setFrameBuffer(&pixels[0][0] - dw.min.x - dw.min.y * width, 1, width);
    file.readPixels(dw.min.y, dw.max.y);

    img = fbksd::Img(width, height);
    for(int y = 0; y < height; ++y)
    for(int x = 0; x < width; ++x)
    {
        float* p = img(x, y);
        p[0] = pixels[y][x].r;
        p[1] = pixels[y][x].g;
        p[2] = pixels[y][x].b;
    }
}

void saveEXRImg(const std::string& fileName, const fbksd::Img& img)
{
    int width = img.width();
    int height = img.height();
    Imf::Header header(width, height);
    header.channels().insert ("R", Imf::Channel(Imf::FLOAT));
    header.channels().insert ("G", Imf::Channel(Imf::FLOAT));
    header.channels().insert ("B", Imf::Channel(Imf::FLOAT));

    Imf::OutputFile file(fileName.c_str(), header);
    Imf::FrameBuffer frameBuffer;

    const float* data = img.data();
    frameBuffer.insert("R", Imf::Slice( Imf::PixelType(Imf::FLOAT), (char*)data, sizeof(*data)*3, sizeof(*data)*width*3));
    frameBuffer.insert("G", Imf::Slice( Imf::PixelType(Imf::FLOAT), (char*)(data + 1), sizeof(*data)*3, sizeof(*data)*width*3));
    frameBuffer.insert("B", Imf::Slice( Imf::PixelType(Imf::FLOAT), (char*)(data + 2), sizeof(*data)*3, sizeof(*data)*width*3));
    file.setFrameBuffer(frameBuffer);
    file.writePixels(height);
}

void savePNGImg(const std::string& fileName, const fbksd::Img& img)
{
    FILE *fp = fopen(fileName.c_str(), "wb");
    if(!fp)
        throw std::runtime_error("Failed opening \"" + fileName + "\" file.");

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if(!png)
        throw std::runtime_error("Failed creating png write structure.");

    png_infop info = png_create_info_struct(png);
    if(!info)
        throw std::runtime_error("Failed creating png info structure.");

    if(setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);

    // Output is 8bit depth, RGBA format.
    int width = img.width();
    int height = img.height();
    png_set_IHDR(png,
                 info,
                 width, height,
                 8,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT
                 );
    png_write_info(png, info);

    std::vector<png_byte> data(size_t(width) * size_t(height) * 3);
    std::vector<png_bytep> pp(height);
    for(int y = 0; y < height; ++y)
    {
        pp[y] = &data[y*width*3];
        for(int x = 0; x < width; ++x)
        for(int c = 0; c < 3; ++c)
        {
            float fv = img(x, y, c);
            fv = fmax(0.f, fv);
            fv = fmin(fv, 1.f);
            auto v = png_byte(std::min<int>(fv * 255, 255));
            data[y*width*3 + x*3 + c] = v;
        }
    }

    png_write_image(png, pp.data());
    png_write_end(png, nullptr);

    png_destroy_write_struct(&png, &info);
    fclose(fp);
}

float clamp(float v, float min, float max)
{
    return fmax(fmin(v, max), min);
}

fbksd::Img hot(const fbksd::Img& img)
{
    // 256 values for each channel
    static const float r[] = {0.01042f, 0.02083f, 0.03125f, 0.04167f, 0.05208f, 0.06250f, 0.07292f, 0.08333f,
                              0.09375f, 0.10417f, 0.11458f, 0.12500f, 0.13542f, 0.14583f, 0.15625f, 0.16667f,
                              0.17708f, 0.18750f, 0.19792f, 0.20833f, 0.21875f, 0.22917f, 0.23958f, 0.25000f,
                              0.26042f, 0.27083f, 0.28125f, 0.29167f, 0.30208f, 0.31250f, 0.32292f, 0.33333f,
                              0.34375f, 0.35417f, 0.36458f, 0.37500f, 0.38542f, 0.39583f, 0.40625f, 0.41667f,
                              0.42708f, 0.43750f, 0.44792f, 0.45833f, 0.46875f, 0.47917f, 0.48958f, 0.50000f,
                              0.51042f, 0.52083f, 0.53125f, 0.54167f, 0.55208f, 0.56250f, 0.57292f, 0.58333f,
                              0.59375f, 0.60417f, 0.61458f, 0.62500f, 0.63542f, 0.64583f, 0.65625f, 0.66667f,
                              0.67708f, 0.68750f, 0.69792f, 0.70833f, 0.71875f, 0.72917f, 0.73958f, 0.75000f,
                              0.76042f, 0.77083f, 0.78125f, 0.79167f, 0.80208f, 0.81250f, 0.82292f, 0.83333f,
                              0.84375f, 0.85417f, 0.86458f, 0.87500f, 0.88542f, 0.89583f, 0.90625f, 0.91667f,
                              0.92708f, 0.93750f, 0.94792f, 0.95833f, 0.96875f, 0.97917f, 0.98958f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f};
    static const float g[] = {0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.01042f, 0.02083f, 0.03125f, 0.04167f, 0.05208f, 0.06250f, 0.07292f, 0.08333f,
                              0.09375f, 0.10417f, 0.11458f, 0.12500f, 0.13542f, 0.14583f, 0.15625f, 0.16667f,
                              0.17708f, 0.18750f, 0.19792f, 0.20833f, 0.21875f, 0.22917f, 0.23958f, 0.25000f,
                              0.26042f, 0.27083f, 0.28125f, 0.29167f, 0.30208f, 0.31250f, 0.32292f, 0.33333f,
                              0.34375f, 0.35417f, 0.36458f, 0.37500f, 0.38542f, 0.39583f, 0.40625f, 0.41667f,
                              0.42708f, 0.43750f, 0.44792f, 0.45833f, 0.46875f, 0.47917f, 0.48958f, 0.50000f,
                              0.51042f, 0.52083f, 0.53125f, 0.54167f, 0.55208f, 0.56250f, 0.57292f, 0.58333f,
                              0.59375f, 0.60417f, 0.61458f, 0.62500f, 0.63542f, 0.64583f, 0.65625f, 0.66667f,
                              0.67708f, 0.68750f, 0.69792f, 0.70833f, 0.71875f, 0.72917f, 0.73958f, 0.75000f,
                              0.76042f, 0.77083f, 0.78125f, 0.79167f, 0.80208f, 0.81250f, 0.82292f, 0.83333f,
                              0.84375f, 0.85417f, 0.86458f, 0.87500f, 0.88542f, 0.89583f, 0.90625f, 0.91667f,
                              0.92708f, 0.93750f, 0.94792f, 0.95833f, 0.96875f, 0.97917f, 0.98958f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f,
                              1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f, 1.00000f};
    static const float b[] = {0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
                              0.01562f, 0.03125f, 0.04688f, 0.06250f, 0.07812f, 0.09375f, 0.10938f, 0.12500f,
                              0.14062f, 0.15625f, 0.17188f, 0.18750f, 0.20312f, 0.21875f, 0.23438f, 0.25000f,
                              0.26562f, 0.28125f, 0.29688f, 0.31250f, 0.32812f, 0.34375f, 0.35938f, 0.37500f,
                              0.39062f, 0.40625f, 0.42188f, 0.43750f, 0.45312f, 0.46875f, 0.48438f, 0.50000f,
                              0.51562f, 0.53125f, 0.54688f, 0.56250f, 0.57812f, 0.59375f, 0.60938f, 0.62500f,
                              0.64062f, 0.65625f, 0.67188f, 0.68750f, 0.70312f, 0.71875f, 0.73438f, 0.75000f,
                              0.76562f, 0.78125f, 0.79688f, 0.81250f, 0.82812f, 0.84375f, 0.85938f, 0.87500f,
                              0.89062f, 0.90625f, 0.92188f, 0.93750f, 0.95312f, 0.96875f, 0.98438f, 1.00000f};

    fbksd::Img map(img.width(), img.height());
    int width = img.width();
    int height = img.height();
    for(int y = 0; y < height; ++y)
    for(int x = 0; x < width; ++x)
    {
        auto* p = img(x, y);
        float rError = clamp(p[0], 0.f, 1.0);
        float gError = clamp(p[1], 0.f, 1.0);
        float bError = clamp(p[2], 0.f, 1.0);
        float avgError = (rError + gError + bError) / 3.f;
        auto i = std::min<int>(avgError * 256, 255);
        map(x, y, 0) = r[i];
        map(x, y, 1) = g[i];
        map(x, y, 2) = b[i];
    }
    return map;
}

void outputValue(const std::string& acronym, const std::string& fileName, float value)
{
    QJsonObject json;
    json[QString::fromStdString(acronym)] = value;
    QJsonDocument doc(json);
    if(fileName.empty())
        std::cout << QString(doc.toJson(QJsonDocument::Compact)).toStdString() << std::endl;
    else
    {
        QFile file(QString::fromStdString(fileName));
        if(!file.open(QFile::WriteOnly))
            throw std::runtime_error("Failed opening \"" + fileName + "\" file.");
        file.write(doc.toJson(QJsonDocument::Compact));
    }
}
}


struct IQA::Imp
{
    std::string acronym;
    std::string fullName;
    std::string reference;
    bool lowerIsBetter = true;
    bool hasErrorMap = true;
    std::string refFileName;
    std::string testFileName;
    std::string valueFileName;
    std::string mapFileName;
    int width = 0;
    int height = 0;
    bool reportCalled = false;
};


IQA::IQA(int argc,
         const char * const *argv,
         const std::string& acronym,
         const std::string& fullName,
         const std::string& reference,
         bool lowerIsBetter,
         bool hasErrorMap):
    m_imp(std::make_unique<Imp>())
{
    if(acronym.empty())
        throw std::invalid_argument("Empty acronym argument.");
    if(argv == nullptr)
        throw std::invalid_argument("argc is null.");

    // positional args
    std::string refFileName;
    std::string testFileName;
    po::options_description args("Arguments");
    args.add_options()
      ("ref-img", po::value<std::string>(&refFileName)->required()->value_name("<ref-img>"), "Reference image path.")
      ("test-img", po::value<std::string>(&testFileName)->required()->value_name("<test-img>"), "Test image path.");
    po::positional_options_description pos;
    pos.add("ref-img", 1);
    pos.add("test-img", 1);
    // options
    std::string valueFileName;
    std::string mapFileName;
    po::options_description desc("Options");
    desc.add_options()
      ("help,h", "Displays this help message and exits.")
      ("info", "Displays information about this IQA method and exits.")
      ("value-file", po::value<std::string>(&valueFileName)->value_name("<value-file>"),
           "File name for the output IQA value text file.")
      ("map-file", po::value<std::string>(&mapFileName)->value_name("<map-file>"),
           "File name for the output error map image file.");

    po::options_description all("All options");
    all.add(args).add(desc);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(all).positional(pos).run(), vm);
    if(vm.count("help"))
    {
        auto exe = fs::path(argv[0]).filename();
        std::cout << "Usage: " << exe << " [options] <ref-img> <test-img>\n                                          \n"
                     "The IQA value is written to stdout in json format: `{\"" << acronym << "\":<value>}`.          \n"
                     "You can save the value in a file instead by using the option `--value-file`.                   \n"
                     "                                                                                               \n"
                     "If supported by the method, the error map can be saved by using the option `--map-file`.       \n"
                     "The file extension must be `.png`. Passing `--map-file` to a technique that doesn't support    \n"
                     "error map has not effect.\n                                                                    \n";
        std::cout << args << std::endl;
        std::cout << desc << std::endl;
        exit(0);
    }
    if(vm.count("info"))
    {
        QJsonObject json;
        json["acronym"] = QString::fromStdString(acronym);
        json["name"] = QString::fromStdString(fullName);
        json["reference"] = QString::fromStdString(reference);
        json["lower_is_better"] = lowerIsBetter;
        json["has_error_map"] = hasErrorMap;
        QJsonDocument doc(json);
        std::cout << QString(doc.toJson()).toStdString() << std::endl;
        exit(0);
    }
    po::notify(vm);

    if(!fs::exists(refFileName))
        throw std::runtime_error("Reference image doesn't exist.");
    if(fs::is_directory(refFileName))
        throw std::runtime_error("Reference image is not a file.");
    if(!fs::exists(testFileName))
        throw std::runtime_error("Test image doesn't exist.");
    if(fs::is_directory(testFileName))
        throw std::runtime_error("Test image is not a file.");

    if(vm.count("value-file"))
    {
        auto parentDir = fs::path(valueFileName).parent_path();
        if(!parentDir.empty() && !fs::is_directory(parentDir))
            throw std::runtime_error(parentDir.string() + " directory not found.");
    }

    if(vm.count("map-file"))
    {
        auto parentDir = fs::path(mapFileName).parent_path();
        if(!parentDir.empty() && !fs::is_directory(parentDir))
            throw std::runtime_error(parentDir.string() + " directory not found.");
        auto extension = fs::path(mapFileName).extension().string();
        if(extension != ".png")
            throw std::runtime_error("Invalid map file extension (\"" + extension + "\"). Only \".png\" supported.");
    }

    m_imp->acronym = acronym;
    m_imp->fullName = fullName;
    m_imp->reference = reference;
    m_imp->lowerIsBetter = lowerIsBetter;
    m_imp->hasErrorMap = hasErrorMap;
    m_imp->refFileName = refFileName;
    m_imp->testFileName = testFileName;
    m_imp->valueFileName = valueFileName;
    m_imp->mapFileName = mapFileName;
}

IQA::~IQA() = default;

void IQA::loadInputImages(Img &refImg, Img &testImg)
{
    readEXRImg(m_imp->refFileName, refImg);
    readEXRImg(m_imp->testFileName, testImg);
    if(refImg.width() != testImg.width() || refImg.height() != testImg.height())
        throw std::logic_error("Reference and test images have different sizes.");
    m_imp->width = refImg.width();
    m_imp->height = refImg.height();
}

void IQA::report(float value)
{
    if(m_imp->reportCalled)
        throw std::logic_error("IQA::report method called more than once.");
    if(m_imp->hasErrorMap)
        throw std::logic_error("IQA instance was created with \'hasErrorMap == true\', but none was reported.");
    outputValue(m_imp->acronym, m_imp->valueFileName, value);
    m_imp->reportCalled = true;
}

void IQA::report(float value, const Img &errorMap)
{
    if(m_imp->reportCalled)
        throw std::logic_error("IQA::report method called more than once.");
    if(errorMap.width() != m_imp->width || errorMap.height() != m_imp->height)
        throw std::logic_error("Error map size is different than the input images.");
    if(!m_imp->hasErrorMap)
        throw std::logic_error("IQA instance was created with \'hasErrorMap == false\', but one was reported.");
    outputValue(m_imp->acronym, m_imp->valueFileName, value);
    if(!m_imp->mapFileName.empty())
    {
        Img map = hot(errorMap);
        savePNGImg(m_imp->mapFileName, map);
    }
    m_imp->reportCalled = true;
}
