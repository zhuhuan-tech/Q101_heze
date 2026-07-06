#include "ML_CoordinateTransformation.h"
using namespace std;
using namespace MLImageDetection;
MLImageDetection::ML_CoordinateTransformation::ML_CoordinateTransformation()
{
}

MLImageDetection::ML_CoordinateTransformation::~ML_CoordinateTransformation()
{
}


cv::Point2f MLImageDetection::ML_CoordinateTransformation::getFiducialMotorLoc(cv::Point2f fidLoc, cv::Point2f motorLoc)
{
    cv::Point2f cen(2462, 2056);
    double deltx = fidLoc.x - cen.x;
    double delty = fidLoc.y - cen.y;
    double disx = deltx * PIXEL_SIZE / MAGNIFICATION;
    double disy = delty * PIXEL_SIZE / MAGNIFICATION;
    cv::Point2f fidMotorLoc;
    fidMotorLoc.x = motorLoc.x + disx;
    fidMotorLoc.y = motorLoc.y + disy;
    return fidMotorLoc;
}

RigidTrans2D MLImageDetection::ML_CoordinateTransformation::getRigidTrans2D(std::vector<cv::Point2f> srcPoints, std::vector<cv::Point2f> destPoints)
{
    RigidTrans2D rotateOffset;

    if (srcPoints.size() == 1 & destPoints.size() == 1)
    {
        rotateOffset.rotationAngle = 0;
        rotateOffset.offset.x = destPoints[0].x - srcPoints[0].x;
        rotateOffset.offset.y = destPoints[0].y - srcPoints[0].y;
    }
    else if (srcPoints.size() == 2 & destPoints.size() == 2)
    {
        double srcsubx = srcPoints[1].x - srcPoints[0].x;
        double srcsuby = srcPoints[1].y - srcPoints[0].y;
        double thetasrc = atan(srcsuby / (srcsubx + 1e-6));
        double dstsubx = destPoints[1].x - destPoints[0].x;
        double dstsuby = destPoints[1].y - destPoints[0].y;
        double thetadst = atan(dstsuby / (dstsubx + 1e-6));
        double theta = thetadst - thetasrc;
        cv::Point2f	srcRotated;
        srcRotated.x = srcPoints[0].x * cos(theta) - srcPoints[0].y * sin(theta);
        srcRotated.y = srcPoints[0].y * cos(theta) + srcPoints[0].x * sin(theta);
        double offsetx = destPoints[0].x - srcRotated.x;
        double offsety = destPoints[0].y - srcRotated.y;

        rotateOffset.rotationAngle = theta;
        rotateOffset.offset = cv::Point2f(offsetx, offsety);
    }
    return rotateOffset;
}

cv::Point2f MLImageDetection::ML_CoordinateTransformation::getPointAfterTrans2D(cv::Point2f srcPts, RigidTrans2D trans)
{
    cv::Point2f transPt;
    cv::Point2f offset = trans.offset;
    double rotationAngle = trans.rotationAngle;

    cv::Point2f temp;
    transPt.x = srcPts.x * cos(rotationAngle) + srcPts.y * sin(rotationAngle) + offset.x;
    transPt.y = srcPts.y * cos(rotationAngle) - srcPts.x * sin(rotationAngle) + offset.y;

    return transPt;
}

vector<cv::Point2f> MLImageDetection::ML_CoordinateTransformation::getPointsAfterTrans2D(vector<cv::Point2f> srcPts, RigidTrans2D trans)
{
    vector<cv::Point2f>transPts;
    cv::Point2f offset = trans.offset;
    double rotationAngle = trans.rotationAngle;
    for (int i = 0; i < srcPts.size(); i++)
    {
        cv::Point2f temp;
        temp.x = srcPts[i].x * cos(rotationAngle) - srcPts[i].y * sin(rotationAngle) + offset.x;
        temp.y = srcPts[i].y * cos(rotationAngle) + srcPts[i].x * sin(rotationAngle) + offset.y;
        transPts.push_back(temp);
    }
    return transPts;
}


void MLImageDetection::ML_CoordinateTransformation::getFeatureACSLoc(FeatureLoc fiducialLoc, PlaneInfo plane, FeatureLoc &eyeboxLoc)
{
    cv::Mat R = calculateRotationMatrix(plane);
    cv::Point3f fiducialPlane = calcuatePlaneLoc(R, fiducialLoc.WGLoc);
    cv::Point3f eyeboxPlane = calcuatePlaneLoc(R, eyeboxLoc.WGLoc);
    cv::Point3f delt = eyeboxPlane - fiducialPlane;
    eyeboxLoc.ACSLoc = fiducialLoc.ACSLoc + delt;
    return ;
}

void MLImageDetection::ML_CoordinateTransformation::getFeatureACSLoc(FeatureLoc fiducialLoc, PlaneInfo plane, vector<FeatureLoc> &eyeboxLocVec)
{
    cv::Mat R = calculateRotationMatrix(plane);
    cv::Point3f fiducialPlane = calcuatePlaneLoc(R, fiducialLoc.WGLoc);
    for (int i = 0; i < eyeboxLocVec.size(); i++)
    {
        cv::Point3f eyeboxPlane = calcuatePlaneLoc(R, eyeboxLocVec[i].WGLoc);
        cv::Point3f delt = eyeboxPlane - fiducialPlane;
        if (plane.isAdd)
        {
            eyeboxLocVec[i].ACSLoc = fiducialLoc.ACSLoc + delt;
        }
        else
        {
            eyeboxLocVec[i].ACSLoc = fiducialLoc.ACSLoc - delt;
        }
    }
    return ;
}

cv::Mat MLImageDetection::ML_CoordinateTransformation::calculateRotationMatrix(PlaneInfo plane)
{
    double thetax = plane.thetaX*CV_PI/180;
    double thetay = plane.thetaY * CV_PI / 180;
    double thetaz = plane.thetaZ * CV_PI / 180;
    cv::Mat Rx = (cv::Mat_<float>(3, 3) << 1,0,0,
        0,cos(thetax),-sin(thetax),
        0,sin(thetax),cos(thetax));
    cv::Mat Ry = (cv::Mat_<float>(3, 3) << cos(thetay), 0 ,sin(thetay),
    0, 1, 0,
    -sin(thetay), 0, cos(thetay));
    cv::Mat Rz = (cv::Mat_<float>(3, 3) << cos(thetaz), -sin(thetaz), 0,
    sin(thetaz), cos(thetaz), 0,
    0, 0, 1);
   // cv::Mat R = Rz * Ry * Rx;
    cv::Mat R = Rx * Ry * Rz;

    return R;
}

cv::Point3f MLImageDetection::ML_CoordinateTransformation::calcuatePlaneLoc(cv::Mat R, cv::Point3f wgLoc)
{
    cv::Mat wgLocMat = (cv::Mat_<float>(3, 1) << wgLoc.x, wgLoc.y, wgLoc.z);
    cv::Mat locMat=R* wgLocMat;
    cv::Point3f loc;
    loc.x = locMat.at<float>(0, 0);
    loc.y = locMat.at<float>(1, 0);
    loc.z = locMat.at<float>(2, 0);
    return loc;
}
