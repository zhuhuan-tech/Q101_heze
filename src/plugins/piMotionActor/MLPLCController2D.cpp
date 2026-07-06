#include "MLPLCController2D.h"
#include "PLCControl/PLCController.h"
#include "log4cplus/log4cplus.h"
MLPLCController2D::MLPLCController2D(int type)
{
    m_State = MLMotionState::kMLStatusDisConnected;
    m_CallbackFlag = false;
    m_Callbacks.clear();


}

MLPLCController2D::~MLPLCController2D()
{
    if (m_IsConnected)
    {
        PLCController::instance()->motorPosClose();
        PLCController::instance()->DisConnect();

        m_State = MLMotionState::kMLStatusDisConnected;
    }
    if (m_CallbackFlag)
    {
        m_CallbackFlag = false;
        m_CallbackHandle.join();
    }


    m_Callbacks.clear();
}

Result MLPLCController2D::Connect(const char* type_name)
{
    string msg = "PLC Connect:";
    if (PLCController::instance()->IsConnected()) {
        if (!PLCController::instance()->motorPosOpen())
        {
            m_IsConnected = false;
            msg += "PLC set get pos instance connect failed,need check!";
            LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, msg.c_str());
            return Result(false, msg);
        }
        if (!PLCController::instance()->SendCommond("IPC_Active", true).success)
        {
            m_IsConnected = false;
            msg += "PLC set IPC_Active connect failed,need check!";
            LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, msg.c_str());
            return Result(false, msg);
        }
        SetEnable(true);
        m_IsConnected = true;
        SetSpeed(5);
        return Result();
    }
    else
    {
        m_IsConnected = false;
        m_State = MLMotionState::kMLStatusDisConnected;
        msg += "PLC connect failed,need check!";
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, msg.c_str());
        return Result(false, msg);
    }
}

bool MLPLCController2D::Disconnect()
{
    if (!m_IsConnected)
        return true;
    PLCController::instance()->SendCommond("AxisX_Enable", false);
    PLCController::instance()->SendCommond("AxisY_Enable", false);
    m_enableX = false;
    m_enableY = false;
    if (!PLCController::instance()->SendCommond("IPC_Active", false).success) {
        string msg = "PLC cancel IPC_Active failed!";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
        return false;
    }
    PLCController::instance()->motorPosClose();
    m_IsConnected = false;
    m_State = MLMotionState::kMLStatusDisConnected;
    string msg = "PLC is disconnect!";
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
    return true;
}

bool MLPLCController2D::IsConnected()
{
    //m_IsConnected = PLCController::instance()->IsConnected();
    return m_IsConnected;
}

void MLPLCController2D::SetEnable(bool enable)
{
    string msg = "PLC2D SetEnable:";
    if (!PLCController::instance()->IsConnected()) {
        m_IsConnected = false;
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "PLC is not connect");

    }
    if (!(PLCController::instance()->SendCommond("AxisX_Enable", true).success &&
        PLCController::instance()->SendCommond("AxisY_Enable", true).success)) {
        msg += "Axis Enable send command failed!";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
        m_enableX = false;
        m_enableY = false;
        return;
    }
    m_enableX = true;
    m_enableY = true;
    if (!(PLCController::instance()->SendCommond("AxisX_Stop", true).success &&
        PLCController::instance()->SendCommond("AxisY_Stop", true).success)) {
        msg += "AxisY Stop send command failed!";
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, msg.c_str());
        m_isStopX = false;
        m_isStopY = false;
        return;
    }
    m_isStopX = true;
    m_isStopY = true;
    m_State = MLMotionState::kMLStatusConnected;
    return;
}

bool MLPLCController2D::IsEnabled()
{
    PLCController::instance()->GetCommond("AxisX_Stop", m_enableX);
    PLCController::instance()->GetCommond("AxisX_Stop", m_enableY);

    return m_enableX & m_enableY;
}

Result MLPLCController2D::MoveRelAsync(double posX, double posY)
{
    string msg = "PLC2D MoveRelAsync:";
    Result res;
    if (!PLCController::instance()->IsConnected()) {
        m_IsConnected = false;
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "device is not connect");
    }

    //新增限位
    if (posX > 150 && posX < 210)
    {
        if (posY > 135)
        {
            return Result(false, "Y_degree out of limit at cur X_degree");
        }
    }
    if (posX > 330 || posX < 30)
    {
        if (posY < 60)
        {
            return Result(false, "Y_degree out of limit at cur X_degree");
        }
    }

    res = XMoveToRelative(posX);
    if (!res.success) {
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, msg.c_str() << res.errorMsg.c_str());
        return res;
    }
    res = YMoveToRelative(posY);
    if (!res.success) {
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, res.errorMsg.c_str());
        return res;
    }
    return Result(true, msg.c_str() + string("MoveRelAsync succeed!"));
}

Result MLPLCController2D::MoveAbsAsync(double posX, double posY)
{
    string msg = "PLC2D MoveAbsAsync:";
    Result res;
    if (!PLCController::instance()->IsConnected()) {
        m_IsConnected = false;
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "device is not connect");
    }

    //新增限位
    if (posX > 150 && posX < 210)
    {
        if (posY > 135)
        {
            return Result(false, "Y_degree out of limit at cur X_degree");
        }
    }
    if (posX > 330 || posX < 30)
    {
        if (posY < 60)
        {
            return Result(false, "Y_degree out of limit at cur X_degree");
        }
    }

    res = YMoveToAbsolute(posY);
    if (!res.success) {
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, msg.c_str() << res.errorMsg.c_str());
        return res;
    }
    res = XMoveToAbsolute(posX);
    if (!res.success) {
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, res.errorMsg.c_str());
        return res;
    }
    return Result(true, msg.c_str() + string("MoveAbsAsync succeed!"));
}

Result MLPLCController2D::MoveRel(double posX, double posY)
{
    string msg = "PLC2D MoveRel:";
    Result res;
    if (!PLCController::instance()->IsConnected()) {
        m_IsConnected = false;
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "device is not connect");
    }
    GetPosition();

    double cur_x = m_MLPoint.x / 1000.0;
    double cur_y = m_MLPoint.y / 1000.0;

    //新增限位
    if (cur_x + posX > 150 && cur_x + posX < 210)
    {
        if (cur_y + posY > 135)
        {
            return Result(false, "Y_degree out of limit at cur X_degree");
        }
    }
    if (cur_x + posX > 330 || cur_x + posX < 30)
    {
        if (cur_y + posY < 60)
        {
            return Result(false, "Y_degree out of limit at cur X_degree");
        }
    }

    res = YMoveToRelative(posY);
    if (!res.success) {
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, msg.c_str() << res.errorMsg.c_str());
        return res;
    }
    res = XMoveToRelative(posX);
    if (!res.success) {
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, res.errorMsg.c_str());
        return res;
    }
    return Result(true, msg.c_str() + string("MoveRel succeed!"));
}
Result MLPLCController2D::MoveRel(const char* axis, double pos)
{
    string msg = "PLC2D MoveRelByAxis:";
    Result res;
    if (!PLCController::instance()->IsConnected()) {
        m_IsConnected = false;
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "device is not connect");
    }
    if (axis == "X")
        res = XMoveToRelative(pos);
    else
    {
        res = YMoveToRelative(pos);
    }
    if (!res.success) {
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, msg.c_str() << res.errorMsg.c_str());
        return res;
    }
    return Result(true, msg.c_str() + string("MoveToRel by axis succeed!"));
}
Result MLPLCController2D::MoveAbsAsyn(const char* axis, double pos)
{
    string msg = "PLC2D MoveAbs by axis:";
    Result res;
    if (!PLCController::instance()->IsConnected()) {
        m_IsConnected = false;
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "device is not connect");
    }
    if (std::strcmp(axis, "X") == 0)
        res = XMoveToAbsoluteAsyn(pos);
    else
    {
        res = YMoveToAbsoluteAsyn(pos);
    }
    if (!res.success) {
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, msg.c_str() << res.errorMsg.c_str());
        return res;
    }
    return Result(true, msg.c_str() + string("MoveToAbsoluteAsync by axis succeed!"));
}

Result MLPLCController2D::MoveAbs(const char* axis, double pos)
{
    string msg = "PLC2D MoveAbs by axis:";
    Result res;
    if (!PLCController::instance()->IsConnected()) {
        m_IsConnected = false;
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "device is not connect");
    }
    if (std::strcmp(axis, "X") == 0)
        res = XMoveToAbsolute(pos);
    else
    {
        res = YMoveToAbsolute(pos);
    }
    if (!res.success) {
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, msg.c_str() << res.errorMsg.c_str());
        return res;
    }
    return Result(true, msg.c_str() + string("MoveToAbsolute by axis succeed!"));
}
Result MLPLCController2D::MoveAbs(double posX, double posY)
{
    string msg = "PLC2D MoveAbs:";
    Result res;
    if (!PLCController::instance()->IsConnected()) {
        m_IsConnected = false;
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "device is not connect");
    }

    //新增限位
    if (posX > 150 && posX < 210)
    {
        if (posY > 135)
        {
            return Result(false, "Y_degree out of limit at cur X_degree");
        }
    }
    if (posX > 330 || posX < 30)
    {
        if (posY < 60)
        {
            return Result(false, "Y_degree out of limit at cur X_degree");
        }
    }

    res = YMoveToAbsolute(posY);
    if (!res.success) {
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, msg.c_str() << res.errorMsg.c_str());
        return res;
    }
    res = XMoveToAbsolute(posX);
    if (!res.success) {
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, res.errorMsg.c_str());
        return res;
    }
    return Result(true, msg.c_str() + string("MoveAbs succeed!"));
}

Result MLPLCController2D::StopMove()
{
    //string msg = "PLC2D StopMove ";
    Result res;
    res = PLCController::instance()->SendCommond("AxisY_Stop", false);
    if (!res.success) {
        m_isStopY = false;
        //msg += "command send failed!";
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, res.errorMsg.c_str());
        return res;

    }
    res = PLCController::instance()->SendCommond("AxisX_Stop", false);
    if (!res.success) {
        m_isStopX = false;
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, res.errorMsg.c_str());
        return res;
    }
    Sleep(100);
    res = PLCController::instance()->SendCommond("AxisX_Mov", false);
    res = PLCController::instance()->SendCommond("AxisY_Stop", true);
    if (!res.success) {
        m_isStopY = false;
        //msg += "command send failed!";
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, res.errorMsg.c_str());
        return res;

    }
    res = PLCController::instance()->SendCommond("AxisX_Stop", true);
    if (!res.success) {
        m_isStopX = false;
        LOG4CPLUS_ERROR(LogPlus::getInstance()->logger, res.errorMsg.c_str());
        return res;
    }
    m_isStopX = true;
    m_isStopY = true;
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "PLC2D StopMove succeed!");

    return Result();
}

Result MLPLCController2D::Homing()
{
    return Result();
}

Result MLPLCController2D::homingForced()
{
    return Result();
}

Result MLPLCController2D::IsHome(bool& flag, int type)
{
    return Result();
}

void MLPLCController2D::SetSpeed(int speed)
{
    double Speed = speed /*/ m_Scale*/;
    PLCController::instance()->SendCommond("AxisX_SpeedSet", Speed);
    PLCController::instance()->SendCommond("AxisY_SpeedSet", Speed);
    m_speedX = Speed;
    m_speedY = Speed;
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "PLC2D set speed is:" << to_string(m_speedX).c_str());

    return;
}
void MLPLCController2D::SetSpeedX(double speed)
{
    //double Speed = speed / m_Scale;
    PLCController::instance()->SendCommond("AxisX_SpeedSet", speed);
    m_speedX = speed;
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "PLC2D set speedX is:" << to_string(m_speedX).c_str());

    return;
}
void MLPLCController2D::SetSpeedY(double speed)
{
    //double Speed = speed / m_Scale;
    PLCController::instance()->SendCommond("AxisY_SpeedSet", speed);
    m_speedY = speed;
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "PLC2D set speedY is:" << to_string(m_speedX).c_str());

    return;
}
int MLPLCController2D::GetSpeed()
{
    PLCController::instance()->GetCommond("AxisX_SpeedSet", m_speedX);
    //PLCController::instance()->GetCommond("AxisY_SpeedSet", m_speedY);
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "PLC2D get speed X is:" << to_string(m_speedX).c_str());

    return m_speedX;
}

bool MLPLCController2D::IsMoving()
{

    bool movingX = false, movingY = false;
    PLCController::instance()->GetCommond("AxisX_Status", movingX);
    PLCController::instance()->GetCommond("AxisY_Status", movingY);
    bool sss = !(movingX && movingY);
    return !(movingX && movingY);
}

ML_Point MLPLCController2D::GetPosition()
{
    if (!m_IsConnected) {
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "PLC2D is disconncted!");
        return ML_Point();
    }
    //LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "PLC2D current pos start" );
    PLCController::instance()->GetCommond((QString)"AxisX_PosGet", m_Xpos);
    PLCController::instance()->GetCommond((QString)"AxisY_PosGet", m_Ypos);
    m_MLPoint.x = m_Xpos*1000.0;
    m_MLPoint.y = m_Ypos*1000.0;
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "PLC2D current pos is:" << to_string(m_Xpos).c_str() << "," << to_string(m_Ypos).c_str());
    return m_MLPoint;
}

MLMotionState MLPLCController2D::GetState()
{
    if (IsMoving())
    {
        m_State = MLMotionState::kMLStatusIsMoving;
    }
    else
    {
        m_State = MLMotionState::kMLStatusStationary;
    }
    return m_State;
}

void MLPLCController2D::threadFunc()
{
    while (m_CallbackFlag)
    {
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "PLC2D threadFunc start");

        if (m_IsConnected)
        {
            if (m_Callbacks.count(kStateChanged))
            {
                m_Callbacks[kStateChanged]->NotifyMotion2DStateChanged(m_State, GetState());
            }

            if (m_Callbacks.count(kPositionChanged))
            {
                m_Callbacks[kPositionChanged]->NotifyMotion2DPosition();
            }
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "PLC2D threadFunc end");
            //if (m_Callbacks.count(kSpeedChanged))
            //{
            //    m_Callbacks[kSpeedChanged]->NotifyMotion2DSpeed();
            //}
        }

        //std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

void MLPLCController2D::Subscribe(MLMotionEvent event, MLMotion2DCallback* callback)
{
    if (!m_Callbacks.count(event))
    {
        m_Callbacks[event] = callback;
        if (!m_CallbackFlag)
        {
            m_CallbackHandle = std::thread(&MLPLCController2D::threadFunc, this);
            m_CallbackFlag = true;
        }
    }
}

void MLPLCController2D::Unsubscribe(MLMotionEvent event, MLMotion2DCallback* callback)
{
    if (m_Callbacks.count(event))
    {
        if (m_CallbackFlag)
        {
            m_CallbackFlag = false;
            m_CallbackHandle.join();
            m_Callbacks.erase(event);
        }
    }
}

void MLPLCController2D::clearAlarm()
{
    return;
}

Result MLPLCController2D::XMoveToRelative(double postion)
{
    Result res;
    if (!PLCController::instance()->IsConnected())
        return Result(false, "PLC is disconnect!");
    if (!m_enableX)
    {
        return Result(false, "PLC X axis is unenable!");
    }
    if (!m_isStopX)
    {
        return Result(false, "PLC is not at stop status before moving.");
    }
    //todo
    //PLCController::instance()->SendCommond("AxisX_Stop", true);
   // m_isStopX = false;
    res = PLCController::instance()->SendCommond("AxisX_Stop", true);
    if (!res.success) {
        m_isStopX = false;
        return res;
    }

    //AxisX_Mov
    res = PLCController::instance()->SendCommond("AxisX_Mode", 1);
    if (!res.success)
        return res;
    res = PLCController::instance()->SendCommond("AxisX_PosSet", postion);
    if (!res.success)
        return res;
    res = PLCController::instance()->SendCommond("AxisX_Mov", true);
    if (!res.success)
        return res;
    bool AxisX_Status = false;
    int i = 0;
    while (!AxisX_Status)//todo
    {
        PLCController::instance()->GetCommond("AxisX_Status", AxisX_Status);
        Sleep(50);
        if (i > 2000) {
            StopMove();
            return Result(false, "AxisX move over time");
        }
        i++;
    };

    res = PLCController::instance()->SendCommond("AxisX_Mov", false);
    if (!res.success)
        return res;
    //res=PLCController::instance()->SendCommond("AxisX_Mode", false);
    //if (!res.success)
    //    return res;
    res = PLCController::instance()->SendCommond("AxisX_Stop", true);
    if (!res.success) {
        m_isStopX = false;
        return res;
    }

    return Result();

}

Result MLPLCController2D::YMoveToRelative(double postion)
{
    Result res;
    if (!PLCController::instance()->IsConnected())
        return Result(false, "PLC is disconnect!");
    if (!m_enableX)
    {
        return Result(false, "PLC Y axis is unenable!");
    }
    if (!m_isStopY)
    {
        return Result(false, "PLC is not at stop status before moving.");
    }
    //todo
    //PLCController::instance()->SendCommond("AxisX_Stop", true);
   // m_isStopX = false;
    res = PLCController::instance()->SendCommond("AxisY_Stop", true);
    if (!res.success) {
        m_isStopX = false;
        return res;
    }
    res = PLCController::instance()->SendCommond("AxisY_Mode", 1);
    if (!res.success)
        return res;
    res = PLCController::instance()->SendCommond("AxisY_PosSet", postion);
    if (!res.success)
        return res;
    //AxisX_Mov

    res = PLCController::instance()->SendCommond("AxisY_Mov", true);
    if (!res.success)
        return res;
    bool AxisY_Status = false;
    int i = 0;
    while (!AxisY_Status)//todo
    {
        PLCController::instance()->GetCommond("AxisY_Status", AxisY_Status);
        Sleep(50);
        if (i > 3000) {
            StopMove();
            return Result(false, "AxisX move over time");
        }
        i++;
    };
    res = PLCController::instance()->SendCommond("AxisY_Mov", false);
    if (!res.success)
        return res;
    //res = PLCController::instance()->SendCommond("AxisY_Mode", false);
    //if (!res.success)
    //    return res;
    res = PLCController::instance()->SendCommond("AxisY_Stop", true);
    if (!res.success) {
        m_isStopY = false;
        return res;
    }

    return Result();
}

Result MLPLCController2D::XMoveToAbsolute(double postion)
{
    Result res;
    if (!PLCController::instance()->IsConnected())
        return Result(false, "PLC is disconnect!");
    if (!m_enableX)
    {
        return Result(false, "PLC X axis is unenable!");
    }
    if (!m_isStopX)
    {
        return Result(false, "PLC is not at stop status before moving.");
    }
    //todo
    PLCController::instance()->SendCommond("AxisX_Stop", true);
    // m_isStopX = false;
    res = PLCController::instance()->SendCommond("AxisX_Mov", false);

    res = PLCController::instance()->SendCommond("AxisX_PosSet", postion);
    if (!res.success)
        return res;
    //AxisX_Mov
    res = PLCController::instance()->SendCommond("AxisX_Mode", 2);
    if (!res.success)
        return res;
    res = PLCController::instance()->SendCommond("AxisX_Mov", true);
    if (!res.success)
        return res;
    bool AxisX_Status = false;
    int i = 0;
    //Sleep(500);
    while (!AxisX_Status)//todo
    {
        PLCController::instance()->GetCommond("AxisX_Status", AxisX_Status);
        Sleep(30);
        if (i > 3000) {
            StopMove();
            return Result(false, "AxisX move over time");
        }
        i++;
    };

    res = PLCController::instance()->SendCommond("AxisX_Mov", false);
    if (!res.success)
        return res;
    //res = PLCController::instance()->SendCommond("AxisX_Mode", false);
    //if (!res.success)
    //    return res;
    res = PLCController::instance()->SendCommond("AxisX_Stop", true);
    if (!res.success) {
        m_isStopX = false;
        return res;
    }

    return Result();

}

Result MLPLCController2D::YMoveToAbsolute(double postion)
{
    Result res;
    if (!PLCController::instance()->IsConnected())
        return Result(false, "PLC is disconnect!");
    if (!m_enableX)
    {
        return Result(false, "PLC Y axis is unenable!");
    }
    if (!m_isStopY)
    {
        return Result(false, "PLC is not at stop status before moving.");
    }
    //todo
    PLCController::instance()->SendCommond("AxisY_Stop", true);
    // m_isStopX = false;
    res = PLCController::instance()->SendCommond("AxisY_Mov", false);

    res = PLCController::instance()->SendCommond("AxisY_PosSet", postion);
    if (!res.success)
        return res;
    //AxisX_Mov
    res = PLCController::instance()->SendCommond("AxisY_Mode", 2);
    if (!res.success)
        return res;
    res = PLCController::instance()->SendCommond("AxisY_Mov", true);
    if (!res.success)
        return res;
    bool AxisY_Status = false;
    int i = 0;
    //Sleep(500);
    while (!AxisY_Status)//todo
    {
        PLCController::instance()->GetCommond("AxisY_Status", AxisY_Status);
        Sleep(50);
        if (i > 3000) {
            StopMove();
            return Result(false, "AxisY move over time");
        }
        i++;
    };
    res = PLCController::instance()->SendCommond("AxisY_Mov", false);
    if (!res.success)
        return res;
    //res = PLCController::instance()->SendCommond("AxisY_Mode", false);
    //if (!res.success)
    //    return res;
    res = PLCController::instance()->SendCommond("AxisY_Stop", true);
    if (!res.success) {
        m_isStopY = false;
        return res;
    }

    return Result();

}

Result MLPLCController2D::XMoveToAbsoluteAsyn(double postion)
{
    Result res;
    if (!PLCController::instance()->IsConnected())
        return Result(false, "PLC is disconnect!");
    if (!m_enableX)
    {
        return Result(false, "PLC X axis is unenable!");
    }
    if (!m_isStopX)
    {
        return Result(false, "PLC is not at stop status before moving.");
    }
    //todo
    PLCController::instance()->SendCommond("AxisX_Stop", true);
    // m_isStopX = false;
    res = PLCController::instance()->SendCommond("AxisX_Mov", false);

    res = PLCController::instance()->SendCommond("AxisX_PosSet", postion);
    if (!res.success)
        return res;

    //AxisX_Mov
    res = PLCController::instance()->SendCommond("AxisX_Mode", 2);
    if (!res.success)
        return res;
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "rolling PLC2D X start rolling ");

    res = PLCController::instance()->SendCommond("AxisX_Mov", true);
    if (!res.success)
        return res;

    return Result();

}

Result MLPLCController2D::YMoveToAbsoluteAsyn(double postion)
{
    Result res;
    if (!PLCController::instance()->IsConnected())
        return Result(false, "PLC is disconnect!");
    if (!m_enableX)
    {
        return Result(false, "PLC Y axis is unenable!");
    }
    if (!m_isStopY)
    {
        return Result(false, "PLC is not at stop status before moving.");
    }
    //todo
    PLCController::instance()->SendCommond("AxisY_Stop", true);
    // m_isStopX = false;
    res = PLCController::instance()->SendCommond("AxisY_Mov", false);

    res = PLCController::instance()->SendCommond("AxisY_PosSet", postion);
    if (!res.success)
        return res;
    //AxisX_Mov
    res = PLCController::instance()->SendCommond("AxisY_Mode", 2);
    if (!res.success)
        return res;
    res = PLCController::instance()->SendCommond("AxisY_Mov", true);
    if (!res.success)
        return res;

    return Result();

}




