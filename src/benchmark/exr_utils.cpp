#include "exr_utils.h"

#include <ImfOutputFile.h>
#include <ImfChannelList.h>
using namespace Imath;


void saveExr(const std::string& filePath, const float* img, int width, int height)
{
    Imf::Header header(width, height);
    header.channels().insert ("R", Imf::Channel(Imf::FLOAT));
    header.channels().insert ("G", Imf::Channel(Imf::FLOAT));
    header.channels().insert ("B", Imf::Channel(Imf::FLOAT));

    Imf::OutputFile file(filePath.c_str(), header);
    Imf::FrameBuffer frameBuffer;

    frameBuffer.insert("R", Imf::Slice( Imf::PixelType(Imf::FLOAT), (char*)img, sizeof(*img)*3, sizeof(*img)*width*3));
    frameBuffer.insert("G", Imf::Slice( Imf::PixelType(Imf::FLOAT), (char*)(img + 1), sizeof(*img)*3, sizeof(*img)*width*3));
    frameBuffer.insert("B", Imf::Slice( Imf::PixelType(Imf::FLOAT), (char*)(img + 2), sizeof(*img)*3, sizeof(*img)*width*3));

    file.setFrameBuffer(frameBuffer);
    file.writePixels(height);
}
