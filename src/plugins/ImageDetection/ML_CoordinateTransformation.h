#pragma once
#ifdef ALGORITHM_EXPORTS
#define ALGORITHM_API __declspec(dllexport)
#else
#define ALGORITHM_API __declspec(dllimport)
#endif
#include <opencv2/core/base.hpp>
#include <opencv2/core/mat.hpp>
#include<vector>
#include"ml_image_public.h"
#define PIXEL_SIZE 3.45
#define MAGNIFICATION 0.5
namespace MLImageDetection
{
	struct FeatureLoc
	{
		cv::Point3f ACSLoc;
		cv::Point3f WGLoc;
		bool flag = true;
		string message = "";
	};
	struct PlaneInfo
	{
		double thetaX=0;
		double thetaY = 0;
		double thetaZ = 0;
        bool isAdd = true;
	};
	struct RigidTrans2D
	{
		double rotationAngle = 0;//radian
		cv::Point2f offset;
		bool flag = true;
		std::string errMsg = "";
	};
	class ALGORITHM_API ML_CoordinateTransformation
	{
	public:
		ML_CoordinateTransformation();
		~ML_CoordinateTransformation();
	public:
		cv::Point2f getFiducialMotorLoc(cv::Point2f fidLoc,cv::Point2f motorLoc);	
		RigidTrans2D getRigidTrans2D(std::vector<cv::Point2f>srcPts, std::vector<cv::Point2f>dstPts);
		cv::Point2f getPointAfterTrans2D(cv::Point2f srcPts, RigidTrans2D trans);
		vector<cv::Point2f>getPointsAfterTrans2D(vector<cv::Point2f>srcPts, RigidTrans2D trans);
		void getFeatureACSLoc(FeatureLoc fiducialLoc, PlaneInfo plane, FeatureLoc &eyeboxLoc);
	    void getFeatureACSLoc(FeatureLoc fiducialLoc, PlaneInfo plane, vector<FeatureLoc> &eyeboxLoc);
		cv::Mat calculateRotationMatrix(PlaneInfo plane);
		cv::Point3f calcuatePlaneLoc(cv::Mat R, cv::Point3f wgLoc);				
	};

}