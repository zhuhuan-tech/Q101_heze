#include "PlcReadThread.h"
#include <snap7.h>
#include "Core/icore.h"
#include "PrjCommon/logindata.h"
#include "PrjCommon/PrjCommon.h"

class PlcReadThread::PLCControllerImpl
{
  public:
    PLCControllerImpl()
    {

    };
    ~PLCControllerImpl()
    {
        if (IsConnected())
        {
            Disconnect();
        }
    };

    bool Connect(const char *ip, int rack, int slot)
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
    void GetData(int m_DBNumber, unsigned char *buffer, int start, int len)
    {
        if (IsConnected())
            m_pClient.DBRead(m_DBNumber, start, len, buffer);
    };

    void SetData(int m_DBNumber, unsigned char *buffer, int start, int len)
    {
        if (IsConnected())
            m_pClient.DBWrite(m_DBNumber, start, len, buffer);
    }

  private:
    TS7Client m_pClient;
    //int m_DBNumber;
};

PlcReadThread::PlcReadThread(QObject *parent): 
    QThread(parent), 
    m_pImpl(new PLCControllerImpl()), 
    m_Stopping(true)
{
    m_DBBuffer_ = PLCConfig::GetInstance().GetDBConfigInfo();
    createDatabuffer();
    startProcessing();
}

PlcReadThread::~PlcReadThread()
{
    if (!m_Stopping)
    {
        stopProcessing();
        clearDatabuffer();
    }
}

void PlcReadThread::setCells(const QMap<int, PLCCell> &cells)
{
    m_Cells = cells;
}

void PlcReadThread::setCallback(PlcCallback *plcCallback)
{
    m_plcCallback = plcCallback;
}

bool PlcReadThread::getDoorState()
{
    return doorState;
}

bool PlcReadThread::getInterLockState()
{
    return interlock;
}

bool PlcReadThread::getState(QString name)
{
    return m_nameStateMap[name];
}

void PlcReadThread::batchRead()
{
    QMap<int, DBConfigCell>::Iterator iter;

    for (iter = m_DBBuffer_.begin(); iter != m_DBBuffer_.end(); ++iter)
    {
        int key = iter.key();

        if (m_DBBuffer_[key].data != nullptr)
        {
            m_pImpl->GetData(m_DBBuffer_[key].dbNumber, m_DBBuffer_[key].data, 0, m_DBBuffer_[key].maxLength);
        }
    }
}

void PlcReadThread::run()
{

    while (!m_Stopping)
    {
        if (m_pImpl->IsConnected())
        {
            batchRead();
            QMapIterator<int, PLCCell> iterator(m_Cells);
            while (iterator.hasNext())
            {
                iterator.next();
                int offsetByte, offsetBit;
                PLCCell cell = iterator.value();
                PLCConfig::GetInstance().getItemOffset(cell.offset, offsetByte, offsetBit);
                if (!m_DBBuffer_.keys().contains(cell.dbNumber) || (offsetByte > m_DBBuffer_[cell.dbNumber].maxOffsetByte))
                {
                    continue;
                }

                int value = m_DBBuffer_[cell.dbNumber].data[offsetByte];
                int state;
                if (cell.size == 8)
                {
                    state = value;
                    if (!m_cellStateMap.contains(cell.index) || m_cellStateMap[cell.index] != value)
                    {
                        if (cell.visible)
                            m_plcCallback->sendState(cell.index, value);
                        m_cellStateMap[cell.index] = value;
                    }
                }
                else
                {
                    state = GetBit(value, offsetBit) >> offsetBit;
                    if (cell.name == "EStop")
                        state = !state;
                    if (!m_cellStateMap.contains(cell.index) || m_cellStateMap[cell.index] != state)
                    {
                        if (cell.visible) {
                            m_plcCallback->sendState(cell.index, state);
                            m_cellStateMap[cell.index] = state;
                        }
                    }
                }
                m_nameStateMap[cell.name] = state;

                //if (cell.name == "EMG")
                //{
                //    state = GetBit(value, offsetBit) >> offsetBit;
                //    if (state > 0)
                //    {
                //        emit Core::ICore::instance()->emgStopBtnClicked();
                //    }
                //}
#pragma region InterLock process
                if (cell.name == "Door State")
                {
                    doorState = state;
                }
                if (cell.name == "InterLock")
                {
                    interlock = state;
                }
                else if (cell.name == "Testing")
                {
                    istesting = state;
                }
                if (interlock)
                {
                    /*   if (userLevel == "Operator")
                       {*/

                    if (cell.name == "Door State")
                    {
                        doorState = state;
                        if (istesting)
                        {
                            // 0:open 1:close
                            if (state == 1)
                            {
                                doorReplyState = false;
                            }
                            else if (!doorReplyState)
                            {
                                emit Core::ICore::instance()->emgStopBtnClicked();
                                emit Core::PrjCommon::instance()->InterLockRestart();
                                doorReplyState = true;
                            }
                        }
                    }
                    /*  }*/
                }
#pragma endregion
            }
            msleep(300);
        }
        else
        {
            static bool first = true;
            bool ret = open();
            if (ret && first)
            {
                //if (LoginData::instance()->getUserLevel() == OPERATOR)
                //{
                    m_plcCallback->interLock(false);
                //}
                //else
                //{
                //    m_plcCallback->interLock(false);
                //}
                m_plcCallback->testing(false);
                first = false;
            }
            msleep(1000);
        }
    }
}

bool PlcReadThread::open()
{
    ConnectInfo info = PLCConfig::GetInstance().GetConnectInfo();
    bool res = m_pImpl->Connect(info.ip.c_str(), info.rack, info.slot);
    if (res)
    {
        startProcessing();
    }
    return res;
}

void PlcReadThread::close()
{
    m_pImpl->Disconnect();
}

void PlcReadThread::startProcessing()
{
    m_Stopping = false;
    start();
}

void PlcReadThread::stopProcessing()
{
    m_Stopping = true;
    wait();
}

void PlcReadThread::createDatabuffer()
{
    m_DBBuffer_ = PLCConfig::GetInstance().GetDBConfigInfo();

    QMap<int, DBConfigCell>::Iterator iter = m_DBBuffer_.begin();

    for (; iter != m_DBBuffer_.end(); ++iter)
    {
        int key = iter.key();
        if (m_DBBuffer_[key].data == nullptr)
        {
            int size = sizeof(unsigned char) * m_DBBuffer_[key].maxLength;
            m_DBBuffer_[key].data = (unsigned char*)malloc(size);
            memset(m_DBBuffer_[key].data, 0x00, size);

        }
    }
}

void PlcReadThread::clearDatabuffer()
{
    QMap<int, DBConfigCell>::Iterator iter;

    for (iter = m_DBBuffer_.begin(); iter != m_DBBuffer_.end(); ++iter)
    {
        int key = iter.key();
        if (m_DBBuffer_[key].data != nullptr)
        {
            free(m_DBBuffer_[key].data);
            m_DBBuffer_[key].data = nullptr;
        }
    }
}
