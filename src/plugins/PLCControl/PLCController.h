#pragma once
#include "PLCConfig.h"
#include "plccontrol_global.h"
#include <QThread>
#include <string>
#include "PLCConfig.h"
#include "Result.h"
#include "PlcReadThread.h"
#include "plcReadPosClass.h"
class PLCCONTROL_EXPORT PLCController : public QObject, public PlcCallback
{
    Q_OBJECT

signals:
    void sendStateToUI(int, int);

public:
    static PLCController* instance(QObject* parent = nullptr);
    ~PLCController();

public:
    bool Connect();
    void DisConnect();
    bool IsConnected();
    bool motorPosOpen();
    bool motorPosClose();
    bool SendBoolCommond(int index, bool value);
    bool GetBoolCommond(int index, bool& value);
    bool SendIntCommond(int index, int value);
    bool GetIntCommond(int index, int& value);
    bool SendFloatCommond(int index, float value);
    bool GetFloatCommond(int index, float& value);
    bool SendDoubleCommond(int index, double value);
    bool GetDoubleCommond(int index, double& value);
    bool GetPosCommond(int index, double& value);
    bool GetPosStatus(int index, bool& value);
    void GetBool(int dbNum, int start, int bit, bool& value);
    QMap<int, PLCCell>& GetAllPLCCells();

    Result coaxialLight(bool isOpen);
    Result coaxialLight2(bool isOpen);
    Result lightBoard(bool isOpen);
    Result lightBoardUp(bool isOpen);
    Result keyenceLight(bool isOpen);
    Result keyenceLightUp(bool isOpen);
    Result collisionControl(bool isOpen);
    Result collimatorLight(bool isOpen);

    Result projectorUp(bool up);

    Result collisionControlPreMove();
    Result closeLightBeforeRecipe();

    bool GetDoorState();
    bool GetInterlockState();

public:
    PLCController(QObject* parent = nullptr);
    //template<typename T>
    template<typename T>
    Result SendCommond(const QString& name, T val)
    {
        QMap<int, PLCCell>::Iterator iter = m_Cells.begin();
        bool ret;
        while (iter != m_Cells.end())
        {
            if (iter.value().name.compare(name, Qt::CaseInsensitive) == 0)
            {
                if constexpr ((std::is_same_v<T, bool>))
                    ret = SendBoolCommond(iter.key(), val);
                else if constexpr ((std::is_same_v<T, int>))
                    ret = SendIntCommond(iter.key(), val);
                else if constexpr ((std::is_same_v<T, float>))
                    ret = SendFloatCommond(iter.key(), val);
                else if constexpr ((std::is_same_v<T, double>))
                    ret = SendDoubleCommond(iter.key(), val);
                if (!ret)
                    return Result(ret, "send commond failed");
                else
                    return Result();
            }
            ++iter;
        }
        return Result(false, QString("%1 control failed.PLC dont have the key:").arg(name).toStdString());
    }
    template<typename T>
    Result GetCommond(const QString name, T& val)
    {
        bool ret = false;
        QMap<int, PLCCell>::Iterator iter = m_Cells.begin();
        while (iter != m_Cells.end())
        {
            if (iter.value().name.compare(name, Qt::CaseInsensitive) == 0)
            {
                if constexpr ((std::is_same_v<T, bool>)) {
                    ret = GetBoolCommond(iter.key(), val);
                }
                else if constexpr ((std::is_same_v<T, int>))
                    ret = GetIntCommond(iter.key(), val);
                else if constexpr ((std::is_same_v<T, float>))
                    ret = GetFloatCommond(iter.key(), val);
                else if constexpr ((std::is_same_v<T, double>)) {
                    if (name == "AxisX_PosGet" || name == "AxisY_PosGet")
                        ret = GetPosCommond(iter.key(), val);
                    else
                        ret = GetDoubleCommond(iter.key(), val);
                }
                if (!ret)
                    return Result(ret, "send commond failed");
                else
                    return Result();
            }
            ++iter;
        }
        return Result(false, QString("%1 control failed.PLC dont have the key:").arg(name).toStdString());
    }

    Result testing(bool isOpen) override;
    Result interLock(bool isOpen) override;
    //Result getDoubleCommond(const QString name, double& val);

    void SetFloat(int dbNum, int start, float value);
    void GetFloat(int dbNum, int start, float& value);
    void SetDouble(int dbNum, int start, double value);
    void GetDouble(int dbNum, int start, double& value);
    void SetInt(int dbNum, int start, int value);
    void GetInt(int dbNum, int start, int& value);
    void GetByte(int dbNum, int start, bool& value);
    void sendState(int index, int value) override;
    PLCMotorLimits getMotorLimits();
private slots:
    void recipeRunStartOrEnd(bool isStart);
    void updateUserLevel() override;

private:
    class PLCControllerImpl;
    std::unique_ptr<PLCControllerImpl> m_pImpl;

public:
    QMap<int, PLCCell> m_Cells;

private:
    static PLCController* self;
    PlcReadThread* m_readThread;
    plcReadPosClass* m_plcReadPos;
    bool m_isConnect = false;
};
