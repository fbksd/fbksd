/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "fbksd/iqa/Img.h"
#include <cmath>
#include <cstring>
using namespace fbksd;


Img::Img(int width, int height):
    m_width(width),
    m_height(height),
    m_data(std::make_unique<float[]>(size_t(width) * size_t(height) * 3))
{}

Img::Img(int width, int height, float v):
    m_width(width),
    m_height(height),
    m_data(std::make_unique<float[]>(size_t(width) * size_t(height) * 3))
{
    std::fill_n(m_data.get(), size_t(width) * size_t(height) * 3, v);
}

Img::Img(int width, int height, std::unique_ptr<float[]> data):
    m_width(width),
    m_height(height),
    m_data(std::move(data))
{}

Img::Img(int width, int height, const float *data):
    m_width(width),
    m_height(height),
    m_data(std::make_unique<float[]>(size_t(width) * size_t(height) * 3))
{
    std::memcpy(m_data.get(), data, sizeof(float) * size_t(width) * size_t(height) * 3);
}

Img::Img(const Img &img):
    m_width(img.m_width),
    m_height(img.m_height)
{
    if(!img.m_data)
        return;
    size_t size = size_t(m_width) * size_t(m_height) * 3;
    m_data = std::make_unique<float[]>(size);
    std::memcpy(m_data.get(), img.m_data.get(), sizeof(float) * size);
}

Img &Img::operator=(const Img &img)
{
    bool needReallocate = m_width*m_height != img.m_width*img.m_height;
    m_width = img.width();
    m_height = img.height();
    if(!img.m_data)
    {
        m_data.reset();
        return *this;
    }
    if(needReallocate)
        m_data = std::make_unique<float[]>(size_t(m_width) * size_t(m_height) * 3);
    std::memcpy(m_data.get(), img.m_data.get(), sizeof(float) * size_t(m_width) * size_t(m_height) * 3);
    return *this;
}

void Img::toneMap()
{
    // NOTE: sRGB uses an exponent of 2.4 but it still advertises a display gamma of 2.2.
    for(int y = 0; y < m_height; y++)
    for(int x = 0; x < m_width; x++)
    {
        float* pixel = (*this)(x, y);
        for(int c = 0; c < 3; ++c)
        {
            float v = std::fmin(pixel[c], 1.0f);
            v = std::fmax(pixel[c], 0.0f);

            if(v <= 0.0031308f)
                v *= 12.92f;
            else
                v = 1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f;
            pixel[c] = v * 255.f + 0.5f;
        }
    }
}
