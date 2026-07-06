#include "DUTAA4Recipe.h"
#include "metricsdata.h"
#include <Core\modemanager.h>
#include <Core\loggingwrapper.h>
#include "Alignment2D.h"
#include "pluginsystem\Services.h"
#include "MotionProcess.h"
#include "dutTypeConfig.h"
#include "MLUtilCommon.h"
#include "LimitMove.h"
//#include "OrientalMotor/OrientalMotorControl.h"

using namespace DutAA;
using namespace AAProcess;
DutAA4Recipe* DutAA4Recipe::self = nullptr;
DutAA4Recipe* DutAA4Recipe::getInstance(CalibrateWidget* calibrateWidget) {

    if (!self)
    {
        self = new DutAA4Recipe(calibrateWidget);
    }
    return self;
}

DutAA4Recipe::~DutAA4Recipe()
{
}

DutAA4Recipe::DutAA4Recipe(CalibrateWidget* calibrateWidget,QObject* parent)
    : m_calibrateWidget(calibrateWidget), QObject(parent)
{
    MotionProcess::getInstance().setTreeSystemRun(true);
    ObjectManager::getInstance()->registerObject("DutAA4Recipe", static_cast<void*>(this));
}

QString DutAA4Recipe::getNodeValueByName(BT::TreeNode& node, std::string name)
{
    auto f_value = node.getInput<std::string>(name);
    if (!f_value)
    {
        throw BT::RuntimeError("Missing input [force]: ", f_value.error());
    }
    return QString::fromStdString(f_value.value());
}

void RegisterStopHandler(BT::TreeNode& node) {
    node.RegisterStopSignalCallback(
        [](bool isStopTreeSystem) {
            MotionProcess::getInstance().StopTreeSystemCallback(isStopTreeSystem);
        }
    );
}

NodeStatus DutAA::DutAA4Recipe::AA_Connect_MVCamera()
{
    std::string msg = MotionProcess::getInstance().ConnectMVCamera();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Connect_MVCamera ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Connect_MeasureCameraMotionModule()
{
    std::string msg = MotionProcess::getInstance().ConnectMeasureCameraMotionModule();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Connect_MeasureCameraMotionModule ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Connect_DutMotionModule()
{
    std::string msg = MotionProcess::getInstance().ConnectDutMotionModule();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Connect_DutMotionModule ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Connect_ProjectorMotionModule()
{
    std::string msg = MotionProcess::getInstance().ConnectProjectorMotionModule();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Connect_ProjectorMotionModule ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Connect_Keyence()
{
    std::string msg = MotionProcess::getInstance().ConnectKeyence();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Connect_Keyence ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Connect_Collimator()
{
    std::string msg = MotionProcess::getInstance().ConnectCollimator();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Connect_Collimator ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Connect_PLC()
{
    std::string msg = MotionProcess::getInstance().ConnectPLC();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Connect_PLC ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Connect_Spectrometer()
{
    std::string msg = MotionProcess::getInstance().ConnectSpec();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Connect_Spectrometer ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

//NodeStatus DutAA::DutAA4Recipe::AA_Connect_Polarizer()
//{
//    std::string msg = MotionProcess::getInstance().ConnectPolarizer();
//    if (msg != "")
//    {
//        QString message = QString("Recipe Node [ AA_Connect_Polarizer ] run error, %1").arg(QString::fromStdString(msg));
//        LoggingWrapper::instance()->error(message);
//        return BT::NodeStatus::FAILURE;
//    }
//    return BT::NodeStatus::SUCCESS;
//}

NodeStatus DutAA::DutAA4Recipe::AA_Dut_LoadDUT()
{
    std::string msg = MotionProcess::getInstance().LoadDUT();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Dut_LoadDUT ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Dut_AdjustLevel()
{
    std::string msg = MotionProcess::getInstance().DutParallelAdjustment();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Dut_AdjustLevel ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Dut_FindFiducial(BT::TreeNode& node)
{
    QString isAuto = getNodeValueByName(node, "is_auto");
    bool isSave = getNodeValueByName(node, "is_save").toInt();
    bool is_auto_exposure = getNodeValueByName(node, "is_auto_exposure").toInt();
    double exposure_time = getNodeValueByName(node, "exposure_time").toDouble();
    QString rootDir = getNodeValueByName(node, "root_dir");
	QString dut_seq = getNodeValueByName(node, "dut_seq");

    MotionProcess::getInstance().SetExposureTime(is_auto_exposure, exposure_time);
    std::string msg = MotionProcess::getInstance().SetIsSaveFiducialImage(isSave, rootDir, dut_seq);
    msg = MotionProcess::getInstance().IsAutoIdentifyFiducial(isAuto.toInt());
    msg = MotionProcess::getInstance().FindFiducial();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Dut_FindFiducial ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Dut_Ranging()
{
    std::string msg = MotionProcess::getInstance().Ranging();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Dut_Ranging ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Dut_Align_EntrancePupil()
{
    std::string msg = MotionProcess::getInstance().EntrancePupilAlignment();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Dut_Align_EntrancePupil ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Dut_Align_EyeBox(BT::TreeNode& node)
{
    QString eyebox_id = getNodeValueByName(node, "eyebox_id");

    std::string msg = MotionProcess::getInstance().EyeboxScanning(eyebox_id.toInt());
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Dut_Align_EyeBox ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Dut_GetDutType(BT::TreeNode& node)
{
    QString eye_relief = getNodeValueByName(node, "eye_relief");
    QString eye_type = getNodeValueByName(node, "eye_type");

    std::string dut_name;
    std::string msg = MotionProcess::getInstance().GetDutTypeName(eye_type.toStdString(), eye_relief.toStdString(), dut_name);
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Dut_GetDutType ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    
    MLUtils::TestState state;
    std::vector<std::string> parts = MLUtils::MLUtilCommon::instance()->split(dut_name, '_');
    if (parts.size() != 3)
    {
        QString message = QString("Recipe Node [ AA_Dut_GetDutType ] run error, dut name %1 is nonconforming.")
            .arg(QString::fromStdString(dut_name));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }

    LoggingWrapper::instance()->info(QString("Current Dut type: %1").arg(QString::fromStdString(dut_name)));

    state.IsDut = true;
    state.IsUpdateSLB = false;
    state.size = MLUtils::MLUtilCommon::instance()->TransStrToSize(parts[0]);
    state.eyeType = MLUtils::MLUtilCommon::instance()->TransStrToEyeType(parts[1]);
    state.relief = MLUtils::MLUtilCommon::instance()->TransIntToEyeRelief(std::stoi(parts[2]));
    MetricsData::instance()->SetTestState(state);
    node.setOutput("size_key", parts[0]);
    node.setOutput("eyetype_key", parts[1]);

    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_SLB_Alignment(BT::TreeNode& node)
{
    RegisterStopHandler(node);

    std::string msg = MotionProcess::getInstance().SLBAlignment();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_SLB_Alignment ] run error, %1") .arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_SLB_LoadSLB(BT::TreeNode& node)
{
    RegisterStopHandler(node);

    std::string msg = MotionProcess::getInstance().LoadSLB();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_SLB_LoadSLB ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Move_PupilDecenter(BT::TreeNode& node)
{
    QString relative_x = getNodeValueByName(node, "relative_x");
    QString relative_y = getNodeValueByName(node, "relative_y");
    QString relative_z = getNodeValueByName(node, "relative_z");

    cv::Point3f offsetPos = cv::Point3f(relative_x.toDouble(),relative_y.toDouble(), relative_z.toDouble());
    std::string ret = LimitMove::getInstance()->motion3DMoveRel(offsetPos, motion3DType::withCamera);
    if (!ret.empty())
    {
        QString message = QString("Recipe Node [ AA_Move_PupilDecenter ] run error, %1").arg(QString::fromStdString(ret));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_SetExposureTime(BT::TreeNode& node)
{
    bool is_auto_exposure = getNodeValueByName(node, "is_auto_exposure").toInt();
    double exposure_time = getNodeValueByName(node, "exposure_time").toDouble();

    MotionProcess::getInstance().SetExposureTime(is_auto_exposure, exposure_time);
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_SetMotionMoveSpeed(BT::TreeNode& node)
{
    QString projector_dxdy = getNodeValueByName(node, "projector_dxdy");
    QString dut_x = getNodeValueByName(node, "dut_x");
    QString dut_y = getNodeValueByName(node, "dut_y");
    QString dut_z = getNodeValueByName(node, "dut_z");
    //int dut_dxdydz = getNodeValueByName(node, "dut_dxdydz").toInt();
    QString imaging_x = getNodeValueByName(node, "imaging_x");
    QString imaging_y = getNodeValueByName(node, "imaging_y");
    QString imaging_z = getNodeValueByName(node, "imaging_z");
    QString imaging_dxdy = getNodeValueByName(node, "imaging_dxdy");

    QString reticle_x = getNodeValueByName(node, "reticle_x");
    QString reticle_y = getNodeValueByName(node, "reticle_y");

    /*if (Motion3DModel::getInstance(withDUT)->Motion3DisConnected())
    {
        if(!dut_x.isEmpty())
            Motion3DModel::getInstance(withDUT)->SetSpeedX(dut_x.toDouble());
        int speed = Motion3DModel::getInstance(withDUT)->GetSpeed();
        if (!dut_y.isEmpty())
            Motion3DModel::getInstance(withDUT)->SetSpeedY(dut_y.toDouble());
        if (!dut_z.isEmpty())
            Motion3DModel::getInstance(withDUT)->SetSpeedZ(dut_z.toDouble());
    }*/
    if (Motion3DModel::getInstance(withCamera)->Motion3DisConnected())
    {
        if (!imaging_x.isEmpty())
            Motion3DModel::getInstance(withCamera)->SetSpeedX(imaging_x.toDouble());
        if (!imaging_y.isEmpty())
            Motion3DModel::getInstance(withCamera)->SetSpeedY(imaging_y.toDouble());
        if (!imaging_z.isEmpty())
            Motion3DModel::getInstance(withCamera)->SetSpeedZ(imaging_z.toDouble());
    }
    if (Motion2DModel::getInstance(PLC2D)->Motion2DisConnected())
    {
        if (!projector_dxdy.isEmpty())
            Motion2DModel::getInstance(PLC2D)->SetSpeed(projector_dxdy.toDouble());
    }
    /*if (Motion2DModel::getInstance(ACS2DCameraTilt)->Motion2DisConnected())
    {
        if (!imaging_dxdy.isEmpty())
            Motion2DModel::getInstance(ACS2DCameraTilt)->SetSpeed(imaging_dxdy.toDouble());
    }
    if (Motion2DModel::getInstance(ACS2DReticle)->Motion2DisConnected())
    {
        if (!reticle_y.isEmpty())
            Motion2DModel::getInstance(ACS2DReticle)->SetSpeedY(reticle_y.toDouble());
        if (!reticle_x.isEmpty())
            Motion2DModel::getInstance(ACS2DReticle)->SetSpeedX(reticle_x.toDouble());
        int speed = Motion2DModel::getInstance(ACS2DReticle)->GetSpeed();

    }*/
    /*if (OrientalMotorControl::getInstance()->IsConnected())
    {
        OrientalMotorControl::getInstance()->SetSpeed(dut_dxdydz);
    }*/

    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Imaging_LoadImaging()
{
    std::string msg = MotionProcess::getInstance().LoadImaging();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Imaging_LoadImaging ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Imaging_Ranging()
{
    std::string msg = MotionProcess::getInstance().ImagingRanging();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Imaging_Ranging ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Imaging_FindFiducial(BT::TreeNode& node)
{
    QString isAuto = getNodeValueByName(node, "is_auto");
    bool isSave = getNodeValueByName(node, "is_save").toInt();
    bool is_auto_exposure = getNodeValueByName(node, "is_auto_exposure").toInt();
    double exposure_time = getNodeValueByName(node, "exposure_time").toDouble();
    QString rootDir = getNodeValueByName(node, "root_dir");
    QString dut_seq = getNodeValueByName(node, "dut_seq");

    MotionProcess::getInstance().SetExposureTime(is_auto_exposure, exposure_time);
    std::string msg = MotionProcess::getInstance().SetIsSaveFiducialImage(isSave, rootDir, dut_seq);
    msg = MotionProcess::getInstance().IsAutoIdentifyFiducial(isAuto.toInt());
    msg = MotionProcess::getInstance().FindImagingFiducial();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Imaging_FindFiducial ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Imaging_Align_EntrancePupil()
{
    std::string msg = MotionProcess::getInstance().EntranceAlignment();
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Imaging_Align_EntrancePupil ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Imaging_Move(BT::TreeNode& node)
{
    float X = getNodeValueByName(node, "X").toFloat();
    float Y = getNodeValueByName(node, "Y").toFloat();
    std::string msg = MotionProcess::getInstance().ImagingMove(X,Y);
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Imaging_Move ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_Imaging_Rotary_Compensation(BT::TreeNode& node)
{
    double cant = getNodeValueByName(node, "cant").toDouble();
    double pant = getNodeValueByName(node, "pant").toDouble();
    std::string msg = MotionProcess::getInstance().Imaging_Rotary_Compensation(cant, pant);
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_Imaging_Rotary_Compensation ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }

    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_2DMotion_Set(BT::TreeNode& node)
{
    int phi_degree = getNodeValueByName(node, "phi").toInt();
    int theta_degree = getNodeValueByName(node, "theta").toInt();
    QString message;
    //y轴硬件实际角度有偏移，大概是7度
    int offset_y = 7;

    if (phi_degree < -180 || phi_degree > 180 || (phi_degree > 85 && phi_degree < 90))
    {
        message = QString("Recipe Node [ AA_2DMotion_Set ] run error, phi beyond limit");
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }

    if (theta_degree < 0 || theta_degree > 90)
    {
        message = QString("Recipe Node [ AA_2DMotion_Set ] run error, theta beyond limit");
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }

    //phi和theta与大轴小轴角度的转换 
    int X_degree;
    if (phi_degree >= -180 && phi_degree <= 85)
    {
        X_degree = phi_degree + 270;
    }
    else
    {
        X_degree = phi_degree - 90;
    }

    int Y_degree = theta_degree + 90 + offset_y;

    
    if (X_degree > 150 && X_degree < 210)
    {
        if (Y_degree > 135)
        {   
            Y_degree = 135;
            message = QString("Y degree should be less than 135 at cur X degree");
            LoggingWrapper::instance()->warn(message);
        }
    }
    //if (X_degree > 330 || X_degree < 30)
    //{
    //    if (Y_degree < 60)
    //    {
    //        Y_degree = 60;
    //        message = QString("Y Degree should be more than 60 at cur X degree");
    //        LoggingWrapper::instance()->warn(message);
    //    }
    //}

    std::string msg = MotionProcess::getInstance().Motion2DSet(X_degree, Y_degree);
    if (msg != "")
    {
        message = QString("Recipe Node [ AA_2DMotion_Set ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_WaveGrating_Set(BT::TreeNode& node)
{
    int wave_type = getNodeValueByName(node, "wave_type").toInt();

    std::string msg = MotionProcess::getInstance().WaveTypeSet(wave_type);
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_WaveGrating_Set ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_WaveLength_Set(BT::TreeNode& node)
{
    float wave_length = getNodeValueByName(node, "wave_length").toFloat();

    std::string msg = MotionProcess::getInstance().WaveLengthSet(wave_length);
    if (msg != "")
    {
        QString message = QString("Recipe Node [ AA_WaveLength_Set ] run error, %1").arg(QString::fromStdString(msg));
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }
    return BT::NodeStatus::SUCCESS;
}

NodeStatus DutAA::DutAA4Recipe::AA_CreateResultFile(BT::TreeNode& node)
{
    float wave_length = getNodeValueByName(node, "w1").toFloat();
    float phi_degree = getNodeValueByName(node, "phi").toFloat();
    float theta_degree = getNodeValueByName(node, "theta").toFloat();
    QString root_dir = getNodeValueByName(node, "root_dir");
    QString csv_name = getNodeValueByName(node, "csv_name");

    QString message;
    //y轴硬件实际角度有偏移，大概是7度
    int offset_y = 7;

    if (phi_degree < -180 || phi_degree > 180)
    {
        message = QString("Recipe Node [ AA_CreateResultFile ] run error, phi beyond limit");
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }

    if (theta_degree < 0 || theta_degree > 90)
    {
        message = QString("Recipe Node [ AA_CreateResultFile ] run error, theta beyond limit");
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }

    //phi和theta与大轴小轴角度的转换 
    int X_degree;
    if (phi_degree >= -180 && phi_degree <= 90)
    {
        X_degree = phi_degree + 270;
    }
    else
    {
        X_degree = phi_degree - 90;
    }

    int Y_degree = theta_degree + 90 + offset_y;

    if (X_degree > 150 && X_degree < 210)
    {
        if (Y_degree > 135)
        {
            Y_degree = 135;
            theta_degree = Y_degree - 90 - offset_y;
        }
    }

    QString all_reult_path = root_dir + QDir::separator() + csv_name + ".csv";

    CORE::ML_Point3D currentImagingPos = Motion3DModel::getInstance(motion3DType::withCamera)->getPosition();

    QFile file(all_reult_path);

    if (!file.open(QIODevice::Append | QIODevice::Text))
    {
        QString message = QString("Recipe Node [ AA_CreateResultFile ] run error,Unable to open file for writing.");
        LoggingWrapper::instance()->error(message);
        return BT::NodeStatus::FAILURE;
    }

    QTextStream out(&file);

    if (file.size() == 0) {
        out << "w1,theta,phi,x,y,z\n";  
    }

    out << QString::number(wave_length, 'f', 3) << ","
        << QString::number(phi_degree, 'f', 3) << ","
        << QString::number(theta_degree, 'f', 3) << ","
        << QString::number(currentImagingPos.x/1000.0, 'f', 3) << ","
        << QString::number(currentImagingPos.y/1000.0, 'f', 3) << ","
        << QString::number(currentImagingPos.z/1000.0, 'f', 3) << "\n";

    file.close();

    return BT::NodeStatus::SUCCESS;
}