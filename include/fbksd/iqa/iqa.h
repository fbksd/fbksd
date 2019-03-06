/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#pragma once

#include "Img.h"

namespace fbksd
{

/**
 * @brief Main API for implementing an image quality assessment (IQA) technique for fbksd.
 *
 * The IQA techniques are used to compute a value representing how close the results generated
 * by the sampling and denoising techniques are from the reference images.
 *
 * A fbksd::IQA object should be constructed passing the `argc` and `argv`
 * parameters from `main()`. Only one instance must be created.
 *
 * The following CLI becomes available in your program:
 *
 * ```text
 * Usage: ./<iqa_exec> [options] <ref-img-path> <test-img-path>
 *
 * The IQA value is written to stdout in json format: `{"<acronym>":<value>}`.
 * You can save the value in a file instead by using the option `--value-file`.
 *
 * If supported by the method, the error map can be saved by using the option `--map-file`.
 * The file extension must be `.png`. Passing `--map-file` to a technique that doesn't provide
 * the error map to report() has not effect.
 *
 * Arguments:
 *   <ref-img-path>        Reference image path.
 *   <test-img-path>       Test image path.
 *
 * Options:
 *   -h, --help                         Displays this help message and exits.
 *   --info                             Displays information about this IQA method and exits.
 *   --value-file <path/value-file>     File name for the output value text file.
 *   --map-file <path/map-file>         File name for the output error map image file.
 * ```
 *
 * The two input images can be read using the loadInputImages() method.
 * After you compare the two images and compute the IQA value, report the value by calling report().
 *
 * Note that there are two overloads for the report method: report(float) and report(float, const Img&).
 * If your IQA method can produce an error map with values in the [0, 1) range, passing the error map
 * is recommended. In this case, also pass the flag `hasErrorMap == true` when creating the IQA instance.
 *
 */
class IQA
{
public:
    /**
     * @brief Constructs a IQA object.
     *
     * If the images are not found, throws a std::runtime_error exception.
     *
     * @param argc
     * argc from `main(int argc, char* argv[])`.
     * @param argv
     * argv from `main(int argc, char* argv[])`. Can not be null.
     * @param acronym
     * Acronym used as ID for this IQA technique (ex: "MSE"). Can not be empty.
     * @param fullName
     * Full name of the technique (ex: "Mean square error").
     * @param reference
     * Bibliographical reference for the technique.
     * @param lowerIsBetter
     * Flag indicating that lower values are to be considered better for this IQA method.
     * @param hasErrorMap
     * Flag indicating that this method provides an error map. If this flag is true,
     * `report(float, const Img&)` should be called, passing the error map.
     */
    IQA(int argc,
        char const* const* argv,
        const std::string& acronym,
        const std::string& fullName,
        const std::string& reference,
        bool lowerIsBetter,
        bool hasErrorMap);

    ~IQA();

    /**
     * @brief Loads the input images.
     *
     * The image files are read from disk.
     *
     * @param refImg  Reference image.
     * @param testImg Test image.
     */
    void loadInputImages(fbksd::Img& refImg, fbksd::Img& testImg);

    /**
     * @brief Reports the IQA value.
     *
     * @note If the IQA instance was created with `hasErrorMap == false`, call this overload.
     *
     * @param value
     * The IQA value obtained by comparing the reference and the test images.
     */
    void report(float value);

    /**
     * @brief Reports the IQA value and the	error map.
     *
     * @note If the IQA instance was created with `hasErrorMap == true`, call this overload.
     *
     * @param value
     * The IQA value obtained by comparing the reference and the test images.
     * @param errorMap
     * An image representing the error map, with pixel values in the [0, 1) range. Values outside
     * this rage are clamped. The image must have the same size as the reference and test images.
     * Also note that the map contains error values (e.g. larger is always worse), regardless of the flag
     * `lowerIsBetter` passed to IQA::IQA().
     */
    void report(float value, const Img& errorMap);

private:
    struct Imp;
    std::unique_ptr<Imp> m_imp;
};

}
