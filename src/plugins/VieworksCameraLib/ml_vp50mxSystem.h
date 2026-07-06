#ifndef ML_VP50MXSYSTEM_H
#define ML_VP50MXSYSTEM_H

#include <opencv2/opencv.hpp>
#include "ml_camera.h"
#include "Mil.h"
#include <windows.h>

typedef struct
{
    MIL_ID MilApplication;
    MIL_ID MilSystem;
    MIL_ID MilDisplay;
}ml_vp50mxStruct;

class ml_vp50mxSystem
{
public:
    static ml_vp50mxSystem& GetInstance();
    void OpenConnection();
    void CloseConnection();
    ml_vp50mxStruct Getml_vp50mx();
    ml_vp50mxStruct ml_vp;
    MIL_ID MilApplication,  /* MIL Application identifier.  */
           MilSystem,       /* MIL System identifier.       */
           MilDisplay;

private:
    ml_vp50mxSystem();
    ml_vp50mxSystem(ml_vp50mxSystem&) = delete;
    ml_vp50mxSystem& operator=(const ml_vp50mxSystem&) = delete;
    bool m_isOpen;
    bool m_OpenConnection;
    bool m_CloseConnection;
};

#endif //ML_VP50MXSYSTEM_H