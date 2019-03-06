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


static float computeMSE(const Mat& img1, const Mat& img2, Mat& mseMap)
{
    Mat s1;
    absdiff(img1, img2, s1);
    s1 = s1.mul(s1);
    mseMap = s1;
    Scalar s = sum(s1);
    double sse = s.val[0] + s.val[1] + s.val[2];
    double mse  = sse / (double)(img1.channels() * img1.total());
    return mse;
}

static fbksd::Img convertMSEMap(Mat& map)
{
    Mat tmp;
    map.copyTo(tmp);
    normalize(tmp, tmp, 0.0, 1.0, NORM_MINMAX);
    cv::pow(tmp, 0.15, tmp);
    return fbksd::Img(map.cols, map.rows, reinterpret_cast<float*>(tmp.data));
}


int main(int argc, char* argv[])
{
    fbksd::IQA iqa(argc, argv, "MSE", "Mean squared error", "", true, true);
    fbksd::Img ref;
    fbksd::Img test;
    iqa.loadInputImages(ref, test);

    Mat refMat(ref.height(), ref.width(), CV_32FC3, ref.data());
    Mat testMat(ref.height(), ref.width(), CV_32FC3, test.data());
    Mat mseMap(ref.height(), ref.width(), CV_32F);
    float error = computeMSE(refMat, testMat, mseMap);
    iqa.report(error, convertMSEMap(mseMap));

    return 0;
}
