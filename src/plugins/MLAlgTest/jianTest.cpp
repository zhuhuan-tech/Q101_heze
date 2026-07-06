#include <opencv2\opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdlib.h>
#include<numeric>
#include <string>
#include <algorithm>
#include <functional>
#include "ml_image_public.h"
#include"lsd.h"
#include"CrossCenter.h"
#include <filesystem>  // C++17
#include"FiducialDetect.h"
#include"ml_image_public.h"
#include"ML_RainbowCalculation.h"
using namespace std;
using namespace cv;
using namespace MLImageDetection;
using namespace MLIQMetrics;
void crosstest()
{
	string dir = "F:/imgs/jian/collimator/";
	std::string IMG_PATH = dir + "*.tif"; //"E:\\LiNing\\十字线检测\\处理异常图\\*.tiff";//遍历文件夹下的所有.jpg文件
	// write file
	ofstream ofs;
	vector<cv::String> filenames;
	cv::glob(IMG_PATH, filenames);
	for (int num = 0; num < filenames.size(); num++)
	{
		//获取路径下的文件名
		const size_t last_idx = filenames[num].rfind('\\');
		string basename = filenames[num].substr(last_idx + 1);
		//cout << num + 1 << "=" << basename << endl;
		string dir1 = dir + basename;
		cv::Mat img = cv::imread(dir1, -1);
		//cv::Mat img = cv::imread("F:\\imgs\\jian\\collimator\\1.tif");
		CrossCenter cc(img, 10);
		//	cc.get_crossCenterGaussian();
		//	cc.setROILength(100);
		cv::Point2f center = cc.find_centerGaussian(img, true);
		cout << center << endl;
		string savepath = dir + basename + "_Re.png";
		cv::imwrite(savepath, cc.getImgDraw());

	}


}
void fiducialTest()
{
	string dir = "H:\\IMGS\\caihongwen\\fid\\";
	std::string IMG_PATH = dir + "*.png"; //"E:\\LiNing\\十字线检测\\处理异常图\\*.tiff";//遍历文件夹下的所有.jpg文件
	// write file
	ofstream ofs;
	vector<cv::String> filenames;
	cv::glob(IMG_PATH, filenames);
	for (int num = 0; num < filenames.size(); num++)
	{
		//获取路径下的文件名
		const size_t last_idx = filenames[num].rfind('\\');
		string basename = filenames[num].substr(last_idx + 1);
		//cout << num + 1 << "=" << basename << endl;
		string dir1 = dir + basename;
		cv::Mat img = cv::imread(dir1, -1);
		FiducialDetect fd;
		FiducialRe re = fd.getFiducialCoordinate(img);
		string savepath = dir1 + "Re.png";
		cv::imwrite(savepath, re.imgdraw);
		cout << re.loc << endl;

	}

}
void rectangleDetection()
{
	cv::Mat gray = cv::imread("F:\\imgs\\jian\\投影pupil\\R\\1.tif",0);
    MLimagePublic pl;
    cv::Mat imgdraw = pl.convertTo3Channels(gray);
    cv::Mat imgth;
    cv::threshold(gray, imgth, 0, 255, THRESH_OTSU);
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(imgth, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    //   drawContours(img_draw, contours, -1, Scalar(0, 0, 255), 2);
    for (int i = 0; i < contours.size(); i++)
    {
        cv::RotatedRect rectR = cv::minAreaRect(contours[i]);
        cv::Size s = rectR.size;
        cv::Rect rect = boundingRect(contours[i]);
        double area = rectR.size.area(); // cv::contourArea(contours[i]);
     //   cv::Size s = rectR.size;
        double ratio = double(min(s.width, s.height)) / double(max(s.width, s.height));
        cv::drawContours(imgdraw, contours, i, cv::Scalar(0, 0, 255), 1);
        Point2f P[4];
        rectR.points(P);
        double w = pl.Getdistance(P[0], P[1]);
        double h = pl.Getdistance(P[1], P[2]);
    //    double ratio = min(w, h) / max(w, h);
   
            cv::line(imgdraw, P[0], P[1], Scalar(0, 0, 255), 1);
            cv::line(imgdraw, P[1], P[2], Scalar(0, 0, 255), 1);
            cv::line(imgdraw, P[2], P[3], Scalar(0, 0, 255), 1);
            cv::line(imgdraw, P[0], P[3], Scalar(0, 0, 255), 1);

			cout << w << "," << h << "," << rectR.angle << endl;
			//cout << w << h << rectR.angle << endl;



    }

}
void rainbowTest()
{
	string path = "I:\\IMGS\\rainbow\\Rainbow\\SL1_20260415T110627_410_134_86\\IQ\\Raw\\GrayImage_Raw.tif";
	MLRainbowCalculation rainbow;
	//RainbowInfo flag=rainbow.processSingleRainbow(path);
	cv::Mat img=cv::imread(path,-1);
	//RainbowInfo flag1=rainbow.rainbowDetection(img,86,134,410);
	string rootpath = "I:\\IMGS\\rainbow\\Rainbow\\";
	//bool flag=rainbow.processBatchRainbow(rootpath);
    rainbow.collectRainbowData(rootpath);

}
void main()
{
	rainbowTest();
	//rectangleDetection();
	//fiducialTest();
	//crosstest();
}

