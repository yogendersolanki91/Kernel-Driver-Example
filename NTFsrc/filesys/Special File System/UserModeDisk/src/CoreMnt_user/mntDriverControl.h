#ifndef MNT_DRIVER_CONTROL_H_INCLUDED
#define MNT_DRIVER_CONTROL_H_INCLUDED
#include "DriverControl.h"
class DriverControl
{
    utils::DriverControl m_coreControl;
public:
    DriverControl();
    int Mount(__int64 totalSize, wchar_t mountPoint);
    void RequestExchange(int deviceId, int lastType, int lastStatus, int lastSize, char * data, int dataSize,
                                 int *type, int *size, __int64 * offset);
    void Unmount(int devId);
};
#endif