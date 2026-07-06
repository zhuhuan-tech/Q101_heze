#include "PLCController.h"
#include "Core/icore.h"
#include <QMutex>
#include <basetsd.h>
#include "PrjCommon/logindata.h"
#include <snap7.h>
#include "PrjCommon/PrjCommon.h"
#include "Recipe2.h"
#include "Core/modemanager.h"
#include "Core/imode.h"
#include "RecipeXMLEngine/RecipeXMLEngine.h"
#define GetBit(v, n) ((v) & ((UINT32)1 << (n)))
#define SetBit(v, n) ((v) |= ((UINT32)1 << (n)))
#define ClearBit(v, n) ((v) &= ~((UINT32)1 << (n)))

PLCController* PLCController::self = nullptr;

class PLCController::PLCControllerImpl
{
public:
    PLCControllerImpl()
        ///*: m_DBNumber(1)*/
    {

    };
    ~PLCControllerImpl()
    {
        if (IsConnected())
        {
            Disconnect();
        }
    };

    bool Connect(const char* ip, int rack, int slot)
    {
        m_pClient.ConnectTo(ip, rack, slot);
        return m_pClient.Connected();
    };
    void Disconnect()
    {
        m_pClient.Disconnect();
    };

    bool IsConnected()
    {
        return m_pClient.Connected();
    }
    void GetData(int dbNum, unsigned char* buffer, int start, int len)
    {
        if (IsConnected())
            m_pClient.DBRead(dbNum, start, len, buffer);
    };

    void SetData(int dbNum, unsigned char* buffer, int start, int len)
    {
        if (IsConnected())
            m_pClient.DBWrite(dbNum, start, len, buffer);
    }

private:
    TS7Client m_pClient;
    //int m_DBNumber;
};

PLCController* PLCController::instance(QObject* parent)
{
    if (!self)
    {
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        if (!self)
        {
            self = new PLCController(parent);
        }
    }
    return self;
}

PLCController::PLCController(QObject* parent) : QObject(parent), m_pImpl(new PLCControllerImpl())
{
    if (PLCConfig::GetInstance().Load())
    {
        m_Cells = PLCConfig::GetInstance().ReadInfo();
    }

    Core::IMode* mode = Core::ModeManager::instance()->findMode("Recipe.RecipeXMLMode");
    bool success = connect(static_cast<RecipeXMLEngine::Internal::RecipeXMLMode*>(mode)->GetRecipe2Plugin(), &Recipe2::updateTreeSystemIsRunDone,
        this, [this](bool value) {
            recipeRunStartOrEnd(!value);
        });
    //connect(Core::PrjCommon::instance(), SIGNAL(recipeRunStartOrEnd(bool)), this, SLOT(recipeRunStartOrEnd(bool)));
    connect(Core::PrjCommon::instance(), SIGNAL(updateUserLevel()), this, SLOT(updateUserLevel()));

    {
        m_readThread = new PlcReadThread();
        m_plcReadPos = new plcReadPosClass();
        m_readThread->setCells(m_Cells);
        m_readThread->setCallback(this);
    }
}

PLCController::~PLCController()
{
    if (m_readThread != nullptr) {
        delete m_readThread;
        m_readThread = nullptr;
    }

    testing(false);
}

bool PLCController::Connect()
{
    ConnectInfo info = PLCConfig::GetInstance().GetConnectInfo();
    bool res = m_pImpl->Connect(info.ip.c_str(), info.rack, info.slot);
    m_isConnect = res;
    return res;
}

void PLCController::DisConnect()
{
    m_pImpl->Disconnect();
    m_isConnect = false;
}

bool PLCController::IsConnected()
{
    return m_pImpl->IsConnected();
}

bool PLCController::motorPosOpen() {
    return m_plcReadPos->open();
}
bool PLCController::motorPosClose() {
    return m_plcReadPos->close();
}

bool PLCController::SendBoolCommond(int index, bool value)
{
    if (!IsConnected())
    {
        bool ret = Connect();
        if (!ret) {
            return false;
        }
    }

    PLCCell cell = m_Cells.value(index);
    byte myDB[1];
    int byte, bit;
    PLCConfig::GetInstance().getItemOffset(cell.offset, byte, bit);
    //unsigned char buffer;

    m_pImpl->GetData(cell.dbNumber, myDB, byte, 1);
    if (value)
    {
        SetBit(myDB[0], bit);
    }
    else
    {
        ClearBit(myDB[0], bit);
        //myDB[0] &= ~(1 << bit);
    }

    //int val = GetBit(myDB[0], bit);
    m_pImpl->SetData(cell.dbNumber, &myDB[0], byte, 1);
    return true;
}


bool PLCController::GetBoolCommond(int index, bool& value)
{
    if (!IsConnected())
    {
        bool ret = Connect();
        if (!ret) {
            return false;
        }
    }

    PLCCell cell = m_Cells.value(index);
    int byte, bit;
    PLCConfig::GetInstance().getItemOffset(cell.offset, byte, bit);
    GetBool(cell.dbNumber, byte, bit, value);
    return true;
}

bool PLCController::SendIntCommond(int index, int value)
{
    if (!IsConnected())
    {
        bool ret = Connect();
        if (!ret) {
            return false;
        }
    }

    PLCCell cell = m_Cells.value(index);
    int byte, bit;
    PLCConfig::GetInstance().getItemOffset(cell.offset, byte, bit);
    SetInt(cell.dbNumber, byte, value);

    return true;
}
bool PLCController::GetIntCommond(int index, int& value)
{
    if (!IsConnected())
    {
        bool ret = Connect();
        if (!ret) {
            return false;
        }
    }

    PLCCell cell = m_Cells.value(index);
    int byte, bit;
    PLCConfig::GetInstance().getItemOffset(cell.offset, byte, bit);
    GetInt(cell.dbNumber, byte, value);
    return true;
}
bool PLCController::SendFloatCommond(int index, float value)
{
    if (!IsConnected())
    {
        bool ret = Connect();
        if (!ret) {
            return false;
        }
    }

    PLCCell cell = m_Cells.value(index);
    int byte, bit;
    PLCConfig::GetInstance().getItemOffset(cell.offset, byte, bit);

    SetFloat(cell.dbNumber, byte, value);
    return true;
}

bool PLCController::GetFloatCommond(int index, float& value)
{
    if (!IsConnected())
    {
        bool ret = Connect();
        if (!ret) {
            return false;
        }
    }

    PLCCell cell = m_Cells.value(index);
    int byte, bit;
    PLCConfig::GetInstance().getItemOffset(cell.offset, byte, bit);
    GetFloat(cell.dbNumber, byte, value);
    return true;
}

bool PLCController::SendDoubleCommond(int index, double value)
{
    if (!IsConnected())
    {
        bool ret = Connect();
        if (!ret) {
            return false;
        }
    }

    PLCCell cell = m_Cells.value(index);
    int byte, bit;
    PLCConfig::GetInstance().getItemOffset(cell.offset, byte, bit);

    SetDouble(cell.dbNumber, byte, value);
    return true;
}

bool PLCController::GetDoubleCommond(int index, double& value)
{
    if (!IsConnected())
    {
        bool ret = Connect();
        if (!ret) {
            return false;
        }
    }

    PLCCell cell = m_Cells.value(index);
    int byte, bit;
    PLCConfig::GetInstance().getItemOffset(cell.offset, byte, bit);
    GetDouble(cell.dbNumber, byte, value);
    return true;
}
bool PLCController::GetPosCommond(int index, double& value)
{
    PLCCell cell = m_Cells.value(index);
    int byte, bit;
    PLCConfig::GetInstance().getItemOffset(cell.offset, byte, bit);
    m_plcReadPos->GetDouble(cell.dbNumber, byte, value);
    return true;
}
bool PLCController::GetPosStatus(int index, bool& value)
{
    PLCCell cell = m_Cells.value(index);
    int byte, bit;
    PLCConfig::GetInstance().getItemOffset(cell.offset, byte, bit);
    GetBool(cell.dbNumber, byte, bit, value);
    return true;
}
void PLCController::SetFloat(int dbNum, int start, float value)
{
    unsigned char buf[4];
    union change
    {
        float d;
        unsigned char dat[4];
    }r1;

    r1.d = value;
    buf[3] = r1.dat[0];
    buf[2] = r1.dat[1];
    buf[1] = r1.dat[2];
    buf[0] = r1.dat[3];
    m_pImpl->SetData(dbNum, buf, start, 2);
}

void PLCController::GetFloat(int dbNum, int start, float& value)
{
    unsigned char buf[4];
    m_pImpl->GetData(dbNum, buf, start, 4);
    union change
    {
        float d;
        unsigned char dat[4];
    }r1;

    r1.dat[0] = buf[3];
    r1.dat[1] = buf[2];
    r1.dat[2] = buf[1];
    r1.dat[3] = buf[0];

    value = r1.d;
}

void PLCController::SetDouble(int dbNum, int start, double value)
{
    unsigned char buf[8];
    union change
    {
        double d;
        unsigned char dat[8];
    }r1;

    r1.d = value;
    buf[7] = r1.dat[0];
    buf[6] = r1.dat[1];
    buf[5] = r1.dat[2];
    buf[4] = r1.dat[3];
    buf[3] = r1.dat[4];
    buf[2] = r1.dat[5];
    buf[1] = r1.dat[6];
    buf[0] = r1.dat[7];
    m_pImpl->SetData(dbNum, buf, start, 8);
}

void PLCController::GetDouble(int dbNum, int start, double& value)
{
    unsigned char buf[8];
    m_pImpl->GetData(dbNum, buf, start, 8);
    union change
    {
        double d;
        unsigned char dat[8];
    }r1;

    r1.dat[0] = buf[7];
    r1.dat[1] = buf[6];
    r1.dat[2] = buf[5];
    r1.dat[3] = buf[4];
    r1.dat[4] = buf[3];
    r1.dat[5] = buf[2];
    r1.dat[6] = buf[1];
    r1.dat[7] = buf[0];
    value = r1.d;
}
void PLCController::SetInt(int dbNum, int start, int value)
{
    unsigned char buf[2];
    union change
    {
        int d;
        unsigned char dat[2];
    }r1;

    r1.d = value;
    buf[0] = r1.dat[1];
    buf[1] = r1.dat[0];
    m_pImpl->SetData(dbNum, buf, start, 2);
}
void PLCController::GetInt(int dbNum, int start, int& value)
{
    unsigned char buf[2];
    m_pImpl->GetData(dbNum, buf, start, 2);
    union change
    {
        int d;
        unsigned char dat[2];
    }r1;

    r1.dat[0] = buf[1];
    r1.dat[1] = buf[0];

    value = r1.d;
}
//Result PLCController::getDoubleCommond(const QString name, double& val)
//{
//    bool ret=false;
//    QMap<int, PLCCell>::Iterator iter = m_Cells.begin();
//    while (iter != m_Cells.end())
//    {
//        if (iter.value().name.compare(name, Qt::CaseInsensitive) == 0)
//        {
//           ret = GetDoubleCommond(iter.key(), val);
//            if (!ret)
//                return Result(ret, "send commond failed");
//            else
//                return Result();
//        }
//        ++iter;
//    }
//    return Result(false, QString("%1 control failed.PLC dont have the key:").arg(name).toStdString());
//}
void PLCController::GetBool(int dbNum, int start, int bit, bool& value)
{
    unsigned char buf[1];
    m_pImpl->GetData(dbNum, buf, start, 1);
    value = GetBit(buf[0], bit);
    //value = (buf[0] >> bit) & 1;
}
QMap<int, PLCCell>& PLCController::GetAllPLCCells()
{
    return m_Cells;
}

Result PLCController::coaxialLight(bool isOpen)
{
    return SendCommond("Coaxia Light", isOpen);
}

Result PLCController::coaxialLight2(bool isOpen)
{
    return SendCommond("Coaxia Light2", isOpen);
}

Result PLCController::lightBoard(bool isOpen)
{
    return SendCommond("Light Board2", isOpen);
}

Result PLCController::lightBoardUp(bool isOpen)
{
    return SendCommond("Light Board", isOpen);
}

Result PLCController::keyenceLight(bool isOpen)
{
    return SendCommond("CL_3000", isOpen);
}

Result PLCController::keyenceLightUp(bool isOpen)
{
    return SendCommond("CL_3000 Above", isOpen);
}

Result PLCController::collisionControl(bool isOpen)
{
    return Result();
    return SendCommond("Stop_Sensor", !isOpen);
}

Result PLCController::collimatorLight(bool isOpen)
{
    return SendCommond("Collimator Light", isOpen);
}

Result PLCController::projectorUp(bool up)
{
    if (up)
    {
        Result res = SendCommond("Projector Down", false);
        if (!res.success)
        {
            return res;
        }

        QThread::msleep(500);
        res = SendCommond("Projector Up", true);
        if (!res.success)
        {
            return res;
        }
    }
    else
    {
        Result res = SendCommond("Projector Up", false);
        if (!res.success)
        {
            return res;
        }

        QThread::msleep(500);
        res = SendCommond("Projector Down", true);
        if (!res.success)
        {
            return res;
        }
    }
    return Result();
}

Result PLCController::collisionControlPreMove()
{
    // TODO:
    return Result();

    if (!IsConnected())
    {
        return Result(false, "Collision control error, PLC is not connected.");
    }

    bool state = false;;
    int number = 0;
    while (number++ < 10)
    {
        state = m_readThread->getState("Collision Light");
        if (state) {
            break;
        }

        Result ret = collisionControl(true);
        if (!ret.success)
        {
            return ret;
        }
        QThread::msleep(200);
    }

    if (!state) {
        state = m_readThread->getState("Collision Light");
    }

    if (!state)
    {
        return Result(false, "Collision light open failed.");
    }
    return Result();
}

Result PLCController::closeLightBeforeRecipe()
{
    bool isOpen = false;
    Result ret = coaxialLight(isOpen);
    if (!ret.success)
    {
        return ret;
    }
    ret = coaxialLight2(isOpen);
    if (!ret.success)
    {
        return ret;
    }
    ret = lightBoard(isOpen);
    if (!ret.success)
    {
        return ret;
    }
    ret = lightBoardUp(isOpen);
    if (!ret.success)
    {
        return ret;
    }
    ret = keyenceLight(isOpen);
    if (!ret.success)
    {
        return ret;
    }

    ret = SendCommond("Lamp", isOpen);
    if (!ret.success)
    {
        return ret;
    }

    ret = collisionControl(isOpen);
    if (!ret.success)
    {
        return ret;
    }

    ret = collimatorLight(isOpen);
    if (!ret.success)
    {
        return ret;
    }

    return Result();
}

Result PLCController::testing(bool isOpen)
{
    return SendCommond("Testing", isOpen);
}

Result PLCController::interLock(bool isOpen)
{
    return SendCommond("InterLock", isOpen);
}

void PLCController::sendState(int index, int value)
{
    emit sendStateToUI(index, value);
}

void PLCController::recipeRunStartOrEnd(bool isStart)
{
    if (isStart)
    {
        //interLock(false);
        testing(true);
    }
    else {
        interLock(false);
        testing(false);
        coaxialLight(false);
    }
}

void PLCController::updateUserLevel()
{
    if (LoginData::instance()->getUserLevel() == USERLEVEL::OPERATOR) {
        interLock(true);
    }
    else {
        interLock(false);
    }
}

bool PLCController::GetDoorState()
{
    return m_readThread->getDoorState();
}

bool PLCController::GetInterlockState()
{
    return m_readThread->getInterLockState();
}

PLCMotorLimits PLCController::getMotorLimits() {
    return PLCConfig::GetInstance().getLimits();
}
