/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#ifndef EXR_UTILS_H
#define EXR_UTILS_H

#include <string>


/**
 * @brief Saves a RGB exr image.
 *
 * @param filePath
 * Full path of the output file (including the file name and extension).
 * @param img
 * Pointer to the image data.
 * @param width
 * Number of pixels width-wide.
 * @param height
 * Number of pixels height-wide.
 */
void saveExr(const std::string& filePath, const float* img, int width, int height);

#endif // EXR_UTILS_H
