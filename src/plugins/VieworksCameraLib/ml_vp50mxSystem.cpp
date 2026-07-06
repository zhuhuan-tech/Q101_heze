#include "ml_vp50mxSystem.h"
#include "LogPlus.h"

ml_vp50mxSystem::ml_vp50mxSystem() : m_isOpen(false)
{
	m_OpenConnection = false;
    m_CloseConnection = false;
}

ml_vp50mxSystem& ml_vp50mxSystem::GetInstance()
{
    static ml_vp50mxSystem ml_vp50mx_system;
    return ml_vp50mx_system;
}

void ml_vp50mxSystem::OpenConnection()
{
    string _str = "[--ml_vp50mxSystem ]--";
    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, _str.c_str() << " enter into OpenConnection");
    if (m_OpenConnection == false && m_CloseConnection == false && MilSystem <= 0 && MilApplication <= 0)
    {
        try
        {
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "Allocate a MIL application");

            /* Allocate a MIL application. */
            MappAlloc(M_NULL, M_DEFAULT, &MilApplication);

            LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "Allocate a MIL system");

            /* Allocate a MIL system. */
            MsysAlloc(M_DEFAULT, MIL_TEXT("M_DEFAULT"), M_DEFAULT, M_DEFAULT, &MilSystem);

            ml_vp.MilSystem = MilSystem;
            ml_vp.MilApplication = MilApplication;

            MappControl(M_DEFAULT, M_ERROR, M_PRINT_DISABLE);//829

            //m_isOpen = true;
			m_OpenConnection = true;
			m_CloseConnection = true;

        }
        catch (const std::exception& e)
        {
            LOG4CPLUS_INFO(LogPlus::getInstance()->logger,  _str.c_str() << e.what());
        }
    }
    else
    {
        m_CloseConnection = false;
        ml_vp.MilSystem = MilSystem;
        ml_vp.MilApplication = MilApplication;
        LOG4CPLUS_INFO(LogPlus::getInstance()->logger, _str.c_str() << " MilSystem and MilApplication already Allocate");
    }

    LOG4CPLUS_INFO(LogPlus::getInstance()->logger, "OpenConnection " << m_isOpen);
}

void ml_vp50mxSystem::CloseConnection()
{
    string _str = "[--ml_vp50mxSystem ]--";
    if (m_OpenConnection == true && m_CloseConnection == false)
    {
        m_CloseConnection = true;
        return;
    }
    if (m_OpenConnection == true && m_CloseConnection == true && MilSystem > 0 && MilApplication > 0)
    {
        MsysFree(MilSystem);
        MappFree(MilApplication);
        MilSystem = 0;
        MilApplication = 0;
        m_isOpen = false;
		LOG4CPLUS_INFO(LogPlus::getInstance()->logger, _str.c_str() << "CloseConnection " << m_isOpen);
        m_OpenConnection = false;
        m_CloseConnection = false;
        return;
    }
}

ml_vp50mxStruct ml_vp50mxSystem::Getml_vp50mx()
{
    return ml_vp;
}

