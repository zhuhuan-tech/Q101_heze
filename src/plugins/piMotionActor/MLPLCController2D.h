#ifndef MLPLCCONTROLLER2D_H_
#define MLPLCCONTROLLER2D_H_

//#define WSABUF byte_rename
//
#include <Windows.h>
//
//#undef WSABUF
#include "pimotionactor_global.h"
#include "PrjCommon/service/ml_motion2D.h"
#include "PrjCommon/service/ml.h"
#include <list>
#include <Qlist>
#include <LogPlus.h>
#include <QMap>

using namespace CORE;
class PIMOTIONACTOR_EXPORT MLPLCController2D : public MLMotion2D
{
public:
    MLPLCController2D(int type);
    ~MLPLCController2D();
public:
    virtual Result Connect(const char* type_name);

    virtual bool Disconnect();

    virtual bool IsConnected();

    virtual void SetEnable(bool enable);

    virtual bool IsEnabled();

    virtual Result MoveRelAsync(double posX, double posY);

    virtual Result MoveAbsAsync(double posX, double posY);

    virtual Result MoveRel(const char* axis, double posX);
    Result MoveAbsAsyn(const char* axis, double pos);
    virtual Result MoveRel(double posX, double posY);

    virtual Result MoveAbs(const char* axis, double pos);
    virtual Result MoveAbs(double posX, double posY);

    virtual Result StopMove();

    virtual Result Homing();

    virtual Result homingForced();

    virtual Result IsHome(bool& flag, int type);

    virtual void SetSpeed(int speed);
    virtual void SetSpeedX(double speed);
    virtual void SetSpeedY(double speed);

    virtual int GetSpeed();

    virtual bool IsMoving();

    virtual ML_Point GetPosition();

    virtual MLMotionState GetState();

    virtual void Subscribe(MLMotionEvent event, MLMotion2DCallback* callback);

    virtual void Unsubscribe(MLMotionEvent event, MLMotion2DCallback* callback);

    virtual void clearAlarm();

    void threadFunc();

    Result XMoveToRelative(double postion);

    Result YMoveToRelative(double postion);

    Result XMoveToAbsolute(double postion);

    Result YMoveToAbsolute(double postion);

    Result XMoveToAbsoluteAsyn(double postion);

    Result YMoveToAbsoluteAsyn(double postion);

    //Result XMoveToRelativeAsync(double postion);
    //Result YMoveToRelativeAsync(double postion);
    //Result XMoveToAbsoluteAsync(double postion);
    //Result YMoveToAbsoluteAsync(double postion);

public slots:
    void setBufFlag();
private:
    QList<int> numBuffer = {};
    bool m_IsConnected = false;
    int m_Reference = 6500;
    int m_RangeLow = 500;
    int m_RangeHigh = 12500;
    int m_VelocityHigh = 12000;
    int m_VelocityLow = 500;
    int m_VelocityNormalX = 9000;
    int m_VelocityNormalY = 9000;
    double m_Scale = 1000.0;
    MLMotionState m_State = MLMotionState::kMLStatusDisConnected;
    int m_Axis = 0;
    bool m_enableX = false;
    bool m_enableY = false;
    bool m_isStopX = false;
    bool m_isStopY = false;
    double m_Xpos = -999;
    double m_Ypos = -999;
    double m_speedX, m_speedY;
    ML_Point m_MLPoint;
    int m_TotalAxes = 0;
    std::map<MLMotionEvent, MLMotion2DCallback*> m_Callbacks;
    int X;
    int Y;
    bool m_IsHome;
    std::atomic<bool> m_runThread_;
    std::thread m_CallbackHandle;
    bool m_CallbackFlag = false;
    bool m_bufFlag = false;

    QMap<int, bool> m_directionMap;

};

#endif