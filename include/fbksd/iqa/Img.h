/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#pragma once

#include <memory>

namespace fbksd
{

/**
 * @brief A simple three-channel image class with float data type.
 */
class Img
{
public:
    /**
     * @brief Constructs an empty image.
     *
     * An empty image has zero width and height and not memory buffer allocated
     * (e.g. data() returns nullptr).
     */
    Img() = default;

    /**
     * @brief Constructs a uninitialized image of the given size.
     */
    Img(int width, int height);

    /**
     * @brief Constructs an image of the given size and initializes it with the value `v`.
     */
    explicit Img(int width, int height, float v);

    /**
     * @brief Constructs an image taking ownership of the given data.
     */
    explicit Img(int width, int height, std::unique_ptr<float[]> data);

    /**
     * @brief Constructs an image copying the given data.
     */
    explicit Img(int width, int height, const float* data);

    Img(const Img& img);

    Img(Img&&) = default;

    /**
     * @brief Returns the image width (number of columns).
     */
    int width() const
    { return m_width; }

    /**
     * @brief Returns the image height (number of rows).
     */
    int height() const
    { return m_height; }

    /**
     * @brief Returns a pointer to the internal data buffer.
     */
    float* data()
    { return m_data.get(); }

    /**
     * @brief Returns a pointer to the internal data buffer.
     */
    const float* data() const
    { return m_data.get(); }

    /**
     * @brief Returns a pointer to pixel (x, y).
     *
     * The pointer contains the RGB data: Ex:
     * ```cpp
     * float* p = img(x, y);
     * float R = p[0];
     * float G = p[1];
     * float B = p[2];
     * ```
     *
     * @param x Pixel x coordinate in the [0, width - 1] range.
     * @param y Pixel y coordinate in the [0, height - 1] range.
     */
    float* operator()(int x, int y)
    { return &m_data[size_t(y)*size_t(m_width)*3 + size_t(x)*3]; }

    /**
     * @brief Returns a pointer to pixel (x, y).
     */
    const float* operator()(int x, int y) const
    { return &m_data[size_t(y)*size_t(m_width)*3 + size_t(x)*3]; }

    /**
     * @brief Returns the value of channel c for the pixel (x, y).
     *
     * @param x Pixel x coordinate in the [0, width - 1] range.
     * @param y Pixel y coordinate in the [0, height - 1] range.
     * @param c Channel c in the [0, 2] range.
     */
    float& operator()(int x, int y, int c)
    { return m_data[size_t(y)*size_t(m_width)*3 + size_t(x)*3 + size_t(c)]; }

    /**
     * @brief Returns the value of channel c for the pixel (x, y).
     */
    const float& operator()(int x, int y, int c) const
    { return m_data[size_t(y)*size_t(m_width)*3 + size_t(x)*3 + size_t(c)]; }

    Img& operator=(const Img& img);

    Img& operator=(Img&&) = default;

    /**
     * @brief Convert the pixel values to the [0, 256) range.
     *
     * The algorithm is based on the "exrtopng" tool from the exrtools package (http://scanline.ca/exrtools/).
     */
    void toneMap();

private:
    int m_width = 0;
    int m_height = 0;
    std::unique_ptr<float[]> m_data;
};

}
