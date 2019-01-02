/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include <iostream>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>
#include <QDir>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

namespace
{

bool sanitizeArgs(const QStringList& imgArgs)
{            
    if(imgArgs.size() != 2)
    {
        std::cout << "You should specify exactly two images." << std::endl;
        return false;
    }

    const QString& img1 = imgArgs[0];
    if(!QDir::current().exists(img1))
    {
        std::cout << img1.toStdString() << " not found." << std::endl;
        return false;
    }

    const QString& img2 = imgArgs[1];
    if(!QDir::current().exists(img2))
    {
        std::cout << img2.toStdString() << " not found." << std::endl;
        return false;
    }

    return true;
}


// Map an HDR image to the [0, 255] range.
// This is required by some error metrics.
void toneMap(Mat& img)
{
    if((img.depth() == CV_32F) || (img.depth() == CV_64F))
    {
        //======================================================================================
        // Image conversion based on the "exrtopng" code, a tool from the linux exrtools.
        //======================================================================================

        // Clamp all values to [0, 1]
        threshold(img, img, 1.0, 1.0, THRESH_TRUNC);
        threshold(img, img, 0.0, 1.0, THRESH_TOZERO);

        // NOTE: sRGB uses an exponent of 2.4 but it still advertises
        // a display gamma of 2.2.
        for(int y=0; y<img.rows; y++)
        {
            for(int x=0; x<img.cols; x++)
            {
                Vec3f color = img.at<Vec3f>(Point(x,y));
                for(int c = 0; c < 3; ++c)
                {
                    float v = color[c];
                    if(v <= 0.0031308)
                        v *= 12.92;
                    else
                        v = ( ( 1.055 * std::pow( v, 1.0 / 2.4 ) ) - 0.055 );
                    color[c] = v;
                }
                img.at<Vec3f>(Point(x,y)) = color;
            }
        }
        img = 255.0 * img + 0.5;
        //======================================================================================
    }
}


float computeMSE(const Mat& img1, const Mat& img2, Mat& mseMap)
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


float computeRMSE(const Mat& ref, const Mat& img, Mat& mseMap)
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


float computePSNR(const Mat& img1, const Mat& img2, float mseArg = -1.f)
{
    float mse = 0.f;
    Mat map;
    if(mseArg > 0)
        mse = mseArg;
    else
        mse = computeMSE(img1, img2, map);

    double psnr = 10.0 * std::log10((255 * 255) / mse);
    return psnr;
}


float computeSSIM(const Mat& img1, const Mat& img2, Mat& ssim_map)
{
    // Default settings for 255 dynamic range
    double C1 = 6.5025, C2 = 58.5225;

    // Inits
    Mat img1_sq = img1.mul(img1);
    Mat img2_sq = img2.mul(img2);
    Mat img1_img2 = img1.mul(img2);

    // Preliminary computing
    Mat mu1, mu2;
    GaussianBlur(img1, mu1, Size(11, 11), 1.5);
    GaussianBlur(img2, mu2, Size(11, 11), 1.5);

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
    divide(temp3, temp1, ssim_map);
    Scalar index_scalar = mean(ssim_map);

//    std::cout << "(R, G & B SSIM index)" << std::endl ;
//    std::cout << index_scalar[2] * 100 << "%" << std::endl;
//    std::cout << index_scalar[1] * 100 << "%" << std::endl;
//    std::cout << index_scalar[0] * 100 << "%" << std::endl;

    return (index_scalar[0] + index_scalar[1] + index_scalar[2]) / 3.f;
}


void saveMSEMap(const Mat& map)
{
    Mat tmp;
    map.copyTo(tmp);
    normalize(tmp, tmp, 0.0, 1.0, NORM_MINMAX);
    cv::pow(tmp, 0.5f, tmp);
    normalize(tmp, tmp, 0.0, 255.0, NORM_MINMAX);
    tmp.convertTo(tmp, CV_8U);
    Mat hot;
    applyColorMap(tmp, hot, COLORMAP_HOT);
    imwrite("mse_map.png", hot);
}

void saveSSIMMap(const Mat& map)
{
    Mat tmp;
    map.copyTo(tmp);
    normalize(map, tmp, 0, 255, NORM_MINMAX);
    tmp = Scalar::all(255) - tmp;
    tmp.convertTo(tmp, CV_8U);
    Mat hot;
    applyColorMap(tmp, hot, COLORMAP_HOT);
    imwrite("ssim_map.png", hot);
}


void saveRMSEMap(const Mat& map)
{
    Mat tmp;
    map.copyTo(tmp);
    normalize(tmp, tmp, 0, 1, NORM_MINMAX);
    cv::pow(tmp, 0.5f, tmp);
    normalize(tmp, tmp, 0, 255, NORM_MINMAX);
    tmp.convertTo(tmp, CV_8U);
    Mat hot;
    applyColorMap(tmp, hot, COLORMAP_HOT);
    imwrite("rmse_map.png", hot);
}

}


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("Compare utility for the benchmark system.");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("This utility compares two exr images and prints the error (MSE, SSIM, or PSNR) to the stdout. "
                                     "You can specify the error metric to be computed. "
                                     "If no metric is specified, all available ones will be computed. "
                                     "This tool works with LDR and HDR images."
                                     );
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("image1", "Reference image (reference).");
    parser.addPositionalArgument("image2", "Second image.");

    QCommandLineOption mseOption("mse", "Mean squared error.");
    QCommandLineOption psnrOption("psnr", "Peak signal-to-noise ratio.");
    QCommandLineOption ssimOption("ssim", "Structural similarity.");
    QCommandLineOption rmseOption("rmse", "Relative MSE.");
    QCommandLineOption mapOption("save-maps", "Save the error maps.");
    QCommandLineOption saveErrorsOption("save-errors", "Save a .json file with the errors.");
    QCommandLineOption xminOption("xmin", "Minimum x window coordinate (inclusive).", "xmin");
    QCommandLineOption yminOption("ymin", "Minimum y window coordinate (inclusive).", "ymin");
    QCommandLineOption xmaxOption("xmax", "Maximum x window coordinate.", "xmax");
    QCommandLineOption ymaxOption("ymax", "Maximum y window coordinate.", "ymax");
    parser.addOption(mseOption);
    parser.addOption(psnrOption);
    parser.addOption(ssimOption);
    parser.addOption(rmseOption);
    parser.addOption(mapOption);
    parser.addOption(saveErrorsOption);
    parser.addOption(xminOption);
    parser.addOption(yminOption);
    parser.addOption(xmaxOption);
    parser.addOption(ymaxOption);

    parser.process(app);
    setlocale(LC_NUMERIC,"C");

    bool mseSet = parser.isSet(mseOption);
    bool psnrSet = parser.isSet(psnrOption);
    bool ssimSet = parser.isSet(ssimOption);
    bool rmseSet = parser.isSet(rmseOption);
    bool saveMapsSet = parser.isSet(mapOption);
    bool saveErrorsSet = parser.isSet(saveErrorsOption);
    if(!(mseSet || psnrSet || ssimSet || rmseSet))
        mseSet = psnrSet = ssimSet = rmseSet = true;

    auto imgArgs = parser.positionalArguments();
    if(sanitizeArgs(imgArgs))
    {
        const QString& img1Filename = imgArgs[0];
        const QString& img2Filename = imgArgs[1];

        Mat img1 = imread(img1Filename.toStdString(), CV_LOAD_IMAGE_COLOR | CV_LOAD_IMAGE_ANYDEPTH);
        Mat img2 = imread(img2Filename.toStdString(), CV_LOAD_IMAGE_COLOR | CV_LOAD_IMAGE_ANYDEPTH);
        if(!img1.data)
        {
            std::cout << "Error when opening image 1" << std::endl;
            return 1;
        }
        if(!img2.data)
        {
            std::cout << "Error when opening image 2" << std::endl;
            return 1;
        }
        if(img1.size != img2.size)
        {
            std::cout << "Images don't have the same size" << std::endl;
            return 1;
        }

        // Read window coordinates
        int xmin = 0;
        int ymin = 0;
        int xmax = img1.cols;
        int ymax = img1.rows;
        if(parser.isSet(xminOption))
            xmin = parser.value(xminOption).toInt();
        if(parser.isSet(yminOption))
            ymin = parser.value(yminOption).toInt();
        if(parser.isSet(xmaxOption))
            xmax = parser.value(xmaxOption).toInt();
        if(parser.isSet(ymaxOption))
            ymax = parser.value(ymaxOption).toInt();

        img1 = img1(Range(ymin, ymax), Range(xmin, xmax));
        img1.convertTo(img1, CV_32F);
        img2 = img2(Range(ymin, ymax), Range(xmin, xmax));
        img2.convertTo(img2, CV_32F);

        // Normalizes the images to the [0, 255] range.
        // NOTE: I'm normalizing the images to [0, 255] because the I don't
        // know how to use PSNR and SSIM in HDR images otherwise.
        // I'm not sure if this normalization is a correct procedure in this case.
        Mat convertedImg1;
        img1.copyTo(convertedImg1);
        Mat convertedImg2;
        img2.copyTo(convertedImg2);
        if(psnrSet || ssimSet)
        {
            toneMap(convertedImg1);
            toneMap(convertedImg2);
        }

        std::cout.precision(std::numeric_limits<float>::max_digits10);

        float mse = -1.f;
        if(mseSet)
        {
            Mat mseMap;
            mse = computeMSE(img1, img2, mseMap);
            std::cout << "MSE = " << mse << std::endl;
            if(saveMapsSet)
                saveMSEMap(mseMap);
        }

        float psnr = 0.f;
        if(psnrSet)
        {
            psnr = computePSNR(convertedImg1, convertedImg2, -1);
            std::cout << "PSNR = " << psnr << std::endl;
        }

        float ssim = 0.f;
        if(ssimSet)
        {
            Mat ssimMap;
            ssim = computeSSIM(convertedImg1, convertedImg2, ssimMap);
            std::cout << "SSIM = " << ssim << std::endl;
            if(saveMapsSet)
                saveSSIMMap(ssimMap);
        }

        float rmse = 0.f;
        if(rmseSet)
        {
            Mat rmseMap;
            rmse = computeRMSE(img1, img2, rmseMap);
            std::cout << "rMSE = " << rmse << std::endl;
            if(saveMapsSet)
                saveRMSEMap(rmseMap);
        }

        if(saveErrorsSet)
        {
            QFile file("errors.json");
            if(!file.open(QFile::WriteOnly))
                qWarning("Couldn't open save json errors file");
            else
            {
                QJsonObject logObj;
                if(mseSet)
                    logObj["mse"] = mse;
                if(psnrSet)
                    logObj["psnr"] = psnr;
                if(ssimSet)
                    logObj["ssim"] = ssim;
                if(rmseSet)
                    logObj["rmse"] = rmse;
                QJsonDocument logDoc(logObj);
                file.write(logDoc.toJson());
            }
        }
    }
    else
    {
        parser.showHelp();
    }

    return 0;
}
