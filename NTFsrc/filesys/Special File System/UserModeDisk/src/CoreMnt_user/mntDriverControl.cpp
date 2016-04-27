#include "mntDriverControl.h"
#include "CoreMntInterface.h"
DriverControl::DriverControl()
    : m_coreControl(L"\\\\.\\CoreMnt")
{
}
int DriverControl::Mount(__int64 totalSize, wchar_t mountPoint)
{
    CORE_MNT_MOUNT_REQUEST request;
    request.totalLength = totalSize;
    request.mountPojnt = mountPoint;
    DWORD bytesWritten = 0;
    CORE_MNT_MOUNT_RESPONSE response;
    if(!m_coreControl.DeviceIoGet(CORE_MNT_MOUNT_IOCTL, 
                                  &request, 
                                  sizeof(request),
                                  &response,
                                  sizeof(response),
                                  &bytesWritten))
    {
        throw std::exception(__FUNCTION__" DeviceIoGet failed.");
    }
    return response.deviceId;
}
void DriverControl::RequestExchange(int deviceId, 
                                    int lastType, 
                                    int lastStatus, 
                                    int lastSize, 
                                    char * data, 
                                    int dataSize,
                                    int *type, 
                                    int *size, 
                                    __int64 * offset)
{
    CORE_MNT_EXCHANGE_REQUEST request;
    request.deviceId = deviceId;
    request.lastType = lastType;
    request.lastStatus = lastStatus;
    request.lastSize = lastSize;
    request.data = data;
    request.dataSize = dataSize;
    DWORD bytesWritten = 0;
    CORE_MNT_EXCHANGE_RESPONSE response;
    if(!m_coreControl.DeviceIoGet(CORE_MNT_EXCHANGE_IOCTL, 
                                  &request, 
                                  sizeof(request), 
                                  &response, 
                                  sizeof(response), 
                                  &bytesWritten))
    {
        throw std::exception(__FUNCTION__" DeviceIoGet failed.");
    }
    *type = response.type;
    *size = response.size;
    *offset = response.offset;
}

void DriverControl::Unmount(int devId)
{
    CORE_MNT_UNMOUNT_REQUEST request;
    request.deviceId = devId;
    if(!m_coreControl.DeviceIoSet(CORE_MNT_UNMOUNT_IOCTL, 
                                  &request, 
                                  sizeof(request)))
    {
        throw std::exception(__FUNCTION__" DeviceIoGet failed.");
    }
}