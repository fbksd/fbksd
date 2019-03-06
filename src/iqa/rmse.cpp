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


static float computeRMSE(const Mat& ref, const Mat& img, Mat& mseMap)
{
    Mat diffSq;
    absdiff(ref, img, diffSq);
    diffSq = diffSq.mul(diffSq);
    Mat refSq = ref.mul(ref) + Scalar(0.01f, 0.01f, 0.01f);
    Mat rmse = diffSq / refSq;
    mseMap = rmse;
    Scalar s = sum(rmse);
    double sse = s.val[0] + s.val[1] + s.val[2];
    double mse  = sse / (double)(ref.channels() * ref.total());
    return mse;
}

static fbksd::Img convertRMSEMap(Mat& map)
{
    Mat tmp;
    map.copyTo(tmp);
    normalize(tmp, tmp, 0.0, 1.0, NORM_MINMAX);
    cv::pow(tmp, 0.15, tmp);
    return fbksd::Img(map.cols, map.rows, reinterpret_cast<float*>(tmp.data));
}


int main(int argc, char* argv[])
{
    fbksd::IQA iqa(argc, argv, "RMSE", "Relative mean squared error", "", true, true);
    fbksd::Img ref;
    fbksd::Img test;
    iqa.loadInputImages(ref, test);

    Mat refMat(ref.height(), ref.width(), CV_32FC3, ref.data());
    Mat testMat(ref.height(), ref.width(), CV_32FC3, test.data());
    Mat mseMap(ref.height(), ref.width(), CV_32F);
    float error = computeRMSE(refMat, testMat, mseMap);
    iqa.report(error, convertRMSEMap(mseMap));

    return 0;
}
