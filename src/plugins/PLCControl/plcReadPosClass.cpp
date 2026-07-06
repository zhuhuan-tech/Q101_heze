#include "plcReadPosClass.h"
#include <PLCConfig.h>
#include <snap7.h>

class plcReadPosClass::PLCControllerImpl
{
public:
    PLCControllerImpl()
        /*: m_DBNumber(1)*/ {

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
    void GetData(int DBNumber, unsigned char* buffer, int start, int len)
    {
        if (IsConnected())
            m_pClient.DBRead(DBNumber, start, len, buffer);
    };

    void SetDate(int DBNumber, unsigned char* buffer, int start, int len)
    {
        if (IsConnected())
            m_pClient.DBWrite(DBNumber, start, len, buffer);
    }

private:
    TS7Client m_pClient;
    //int m_DBNumber;
};

plcReadPosClass::plcReadPosClass(QObject* parent)
    : QObject(parent), m_pImpl(new PLCControllerImpl())
{}

plcReadPosClass::~plcReadPosClass()
{}
bool plcReadPosClass::open()
{
    ConnectInfo info = PLCConfig::GetInstance().GetConnectInfo();
    bool res = m_pImpl->Connect(info.ip.c_str(), info.rack, info.slot);
    return res;
}
bool plcReadPosClass::close()
{
    //ConnectInfo info = PLCConfig::GetInstance().GetConnectInfo();
    m_pImpl->Disconnect();
    return true;
}
void plcReadPosClass::GetDouble(int dbNum, int start, double& value)
{
    //LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "PLC2D GetDouble start");
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
    //LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "PLC2D GetDouble end");

}

