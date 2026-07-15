#pragma once
#include "Result.h"

#include "BD3ESerialManage.h"
#include "BD3EMotor.h"
#include "BD3EMotorConfig.h"

#include "orientalmotor_global.h"

class ORIENTALMOTOR_EXPORT BD3EMotorControl {
public:
    static BD3EMotorControl* getInstance();

    BD3EMotorControl();
    ~BD3EMotorControl();

    Result Connect();
    Result Disconnect();
    bool IsConnected();

    Result ServoOn(int type);
    Result ServoOff(int type);
    Result ServoOnAll();
    Result ServoOffAll();

    Result SetSpeed(int rpm);
    int  GetSpeed(int type);
    int  GetCurrPulse(int type);
    double GetCurrDegree(int type);

    Result MoveByDegreeAsync(int type, double degree);
    Result MoveByDegreeSync(int type, double degree);

    Result StopMove(int type);
    Result StopMove();

    bool IsMoving(int type);
    bool IsMoving();
    bool IsAlarm(int type);
    bool IsAlarm();
    Result ClearAlarm();

    int getXLimitMin() { return Xmin; }
    int getXLimitMax() { return Xmax; }
    int getYLimitMin() { return Ymin; }
    int getYLimitMax() { return Ymax; }
    int getZLimitMin() { return Zmin; }
    int getZLimitMax() { return Zmax; }

private:
    void Init();
    Result JudgeOpen(int type);

public:
    int Xmin, Xmax, Ymin, Ymax, Zmin, Zmax;

private:
    bool m_bIsOpen = false;
    static BD3EMotorControl* m_instance;
    std::map<int, BD3EMotor*> m_MotorMap;
};
