#include "DriverControl.h"
namespace utils
{
    DriverControl::DriverControl(const std::wstring& driverName):
        driverName_(driverName),
        _coreDevice(INVALID_HANDLE_VALUE)
    {
    }

    DriverControl::~DriverControl()
    {
        DisconnectFromDriver();
    }

    void DriverControl::ConnectToDriver()
    {
        if (_coreDevice != INVALID_HANDLE_VALUE)
            return;

        _coreDevice = CreateFileW( driverName_.c_str(), GENERIC_READ | GENERIC_WRITE, 
            FILE_SHARE_READ | FILE_SHARE_WRITE,    NULL, OPEN_EXISTING, 0, NULL);

        if (_coreDevice != INVALID_HANDLE_VALUE)
            return;
        throw std::exception("Driver isn't available");
    }

    BOOL DriverControl::DeviceIoSet(DWORD ControlCode, PVOID InBuffer, DWORD BufferSize)
    {
        DWORD BytesCount = 0;
        BOOL result = FALSE;
        if (_coreDevice != INVALID_HANDLE_VALUE) {
            result = DeviceIoControl( _coreDevice, ControlCode, InBuffer, BufferSize, NULL, 0, &BytesCount, NULL );
        } else {
            ConnectToDriver();
            result = DeviceIoControl( _coreDevice, ControlCode, InBuffer, BufferSize, NULL, 0, &BytesCount, NULL );
        }
        return result;
    }

    BOOL DriverControl::DeviceIoGet(DWORD ControlCode, PVOID InBuffer, DWORD BufferSize, 
        PVOID OutBuffer, DWORD OutBufferSize, DWORD* BytesCount)
    {
        BOOL result = FALSE;
        if (_coreDevice != INVALID_HANDLE_VALUE) {
            result = DeviceIoControl( _coreDevice, ControlCode, InBuffer, BufferSize, OutBuffer, OutBufferSize, BytesCount, NULL );
        } else {
            ConnectToDriver();
            result = DeviceIoControl( _coreDevice, ControlCode, InBuffer, BufferSize, OutBuffer, OutBufferSize, BytesCount, NULL );
        }
        return result;
    }

    bool DriverControl::IsDriverConnected()
    {
        return _coreDevice != INVALID_HANDLE_VALUE;
    }

    void DriverControl::DisconnectFromDriver() 
    { 
        if  (_coreDevice && ( _coreDevice != INVALID_HANDLE_VALUE )) 
        { 
            CloseHandle( _coreDevice ); 
            _coreDevice = INVALID_HANDLE_VALUE; 
        } 
    }
}