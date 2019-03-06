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


static float computeSSIM(const Mat& ref, const Mat& test, Mat& map)
{
    // Default settings for 255 dynamic range
    double C1 = 6.5025, C2 = 58.5225;

    // Inits
    Mat img1_sq = ref.mul(ref);
    Mat img2_sq = test.mul(test);
    Mat img1_img2 = ref.mul(test);

    // Preliminary computing
    Mat mu1, mu2;
    GaussianBlur(ref, mu1, Size(11, 11), 1.5);
    GaussianBlur(test, mu2, Size(11, 11), 1.5);

    Mat mu1_sq = mu1.mul(mu1);
    Mat mu2_sq = mu2.mul(mu2);
    Mat mu1_mu2 = mu1.mul(mu2);

    Mat sigma1_sq;
    GaussianBlur(img1_sq, sigma1_sq, Size(11, 11), 1.5);
    sigma1_sq -= mu1_sq;

    Mat sigma2_sq;
    GaussianBlur(img2_sq, sigma2_sq, Size(11, 11), 1.5);
    sigma2_sq -= mu2_sq;

    Mat sigma12;
    GaussianBlur(img1_img2, sigma12, Size(11, 11), 1.5);
    sigma12 -= mu1_mu2;

    // Formula
    Mat temp1, temp2, temp3;

    // (2*mu1_mu2 + C1)
    temp1 = 2.f * mu1_mu2 + C1;

    // (2*sigma12 + C2)
    temp2 = 2.f * sigma12 + C2;

    // ((2*mu1_mu2 + C1).*(2*sigma12 + C2))
    temp3 = temp1.mul(temp2);

    // (mu1_sq + mu2_sq + C1)
    temp1 = mu1_sq + mu2_sq + C1;

    // (sigma1_sq + sigma2_sq + C2)
    temp2 = sigma1_sq + sigma2_sq + C2;

    // ((mu1_sq + mu2_sq + C1).*(sigma1_sq + sigma2_sq + C2))
    temp1 = temp1.mul(temp2);

    // ((2*mu1_mu2 + C1).*(2*sigma12 + C2))./((mu1_sq + mu2_sq + C1).*(sigma1_sq + sigma2_sq + C2))
    divide(temp3, temp1, map);
    Scalar index_scalar = mean(map);
    return (index_scalar[0] + index_scalar[1] + index_scalar[2]) / 3.f;
}

static fbksd::Img convertMap(Mat& map)
{
    Mat tmp;
    map.copyTo(tmp);
    tmp = Scalar::all(1.0) - tmp;
    normalize(tmp, tmp, 0.0, 1.0, NORM_MINMAX);
    cv::pow(tmp, 0.9f, tmp);
    return fbksd::Img(map.cols, map.rows, reinterpret_cast<float*>(tmp.data));
}


int main(int argc, char* argv[])
{
    fbksd::IQA iqa(argc, argv, "SSIM", "Structural similarity", "", false, true);
    fbksd::Img ref;
    fbksd::Img test;
    iqa.loadInputImages(ref, test);
    ref.toneMap();
    test.toneMap();

    Mat refMat(ref.height(), ref.width(), CV_32FC3, ref.data());
    Mat testMat(ref.height(), ref.width(), CV_32FC3, test.data());
    Mat map(ref.height(), ref.width(), CV_32F);
    float ssim = computeSSIM(refMat, testMat, map);
    iqa.report(ssim, convertMap(map));

    return 0;
}
