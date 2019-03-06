/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "fbksd/iqa/iqa.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;


static float computePSNR(const Mat& ref, const Mat& test, Mat& map)
{
    Mat s1;
    absdiff(ref, test, s1);
    s1 = s1.mul(s1);
    map = s1;
    Scalar s = sum(s1);
    double sse = s.val[0] + s.val[1] + s.val[2];
    double mse  = sse / (double)(ref.channels() * ref.total());
    double psnr = 10.0 * std::log10((255 * 255) / mse);
    return psnr;
}


int main(int argc, char* argv[])
{
    fbksd::IQA iqa(argc, argv, "PSNR", "Peak signal-to-noise ratio", "", false, false);
    fbksd::Img ref;
    fbksd::Img test;
    iqa.loadInputImages(ref, test);
    ref.toneMap();
    test.toneMap();

    Mat refMat(ref.height(), ref.width(), CV_32FC3, ref.data());
    Mat testMat(ref.height(), ref.width(), CV_32FC3, test.data());
    Mat mseMap(ref.height(), ref.width(), CV_32F);
    float psnr = computePSNR(refMat, testMat, mseMap);
    iqa.report(psnr);

    return 0;
}
