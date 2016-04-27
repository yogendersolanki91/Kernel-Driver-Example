#ifndef __DRVCORECONTROL_H__
#define __DRVCORECONTROL_H__

#include <string>
#include <memory>
#include "windows.h"


namespace utils
{
    class DriverControl
    {
        HANDLE  _coreDevice;
        std::wstring driverName_;
    public:
        DriverControl(const std::wstring& driverName);
        ~DriverControl();

        BOOL DeviceIoSet(DWORD ControlCode, PVOID InBuffer, DWORD BufferSize);
        BOOL DeviceIoGet(DWORD ControlCode, PVOID InBuffer, DWORD BufferSize, 
            PVOID OutBuffer, DWORD OutBufferSize, DWORD* BytesCount);

        void ConnectToDriver();
        void DisconnectFromDriver();
        bool IsDriverConnected();
    };
}

#endif 