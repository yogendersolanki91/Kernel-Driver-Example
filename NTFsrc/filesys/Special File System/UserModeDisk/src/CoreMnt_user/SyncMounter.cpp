#include "SyncMounter.h"

#include "shlobj.h"

#include "mntImage.h"
#include <string>
#include <sstream>
#include <vector>
#include <assert.h>
#include <iostream>
#include "CoreMntInterface.h"
#include "mntDriverControl.h"
#include "boost\scope_exit.hpp"

int DefineLogicDrive(wchar_t mountPoint, int deviceId)
{
    //define drive
    std::wstring volumeName = L"\\\\.\\_:";
    volumeName[4] = mountPoint;
    std::wstringstream tmpStream;
    tmpStream << DIRECT_DISK_PREFIX << deviceId;
    std::wstring deviceName = tmpStream.str();
    if (!DefineDosDeviceW(DDD_RAW_TARGET_PATH, &volumeName[4], deviceName.c_str()))
        return GetLastError();
    return ERROR_SUCCESS;
}
int UndefineLogicDrive(wchar_t mountPoint)
{
    // try to dismount the volume
    WCHAR volumeName[] = L"\\\\.\\X:";
    volumeName[4] = mountPoint;

    if (!DefineDosDevice(DDD_REMOVE_DEFINITION,&volumeName[4],NULL))
        return GetLastError();
    return ERROR_SUCCESS;
}
void UnmountImage(int devId, wchar_t mountPoint, DriverControl * drvCtrl)
{
    wchar_t    VolumeName[] = L"\\\\.\\ :";
    HANDLE  hVolume;
    DWORD   BytesReturned;

    VolumeName[4] = mountPoint;

    hVolume = CreateFile(VolumeName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
    if(hVolume != INVALID_HANDLE_VALUE)
    {
        BOOST_SCOPE_EXIT( (&hVolume) )
        {
            ::CloseHandle(hVolume);
        } BOOST_SCOPE_EXIT_END
        if (!DeviceIoControl(hVolume,FSCTL_LOCK_VOLUME,NULL,
                             0,NULL,0,&BytesReturned,NULL))
        {
            throw std::exception("Unable to lock logical drive");
        }
        else if (!DeviceIoControl(hVolume,FSCTL_DISMOUNT_VOLUME,
                                  NULL,0,NULL,0,&BytesReturned,NULL))
        {
            throw std::exception("Unable to dismount logical drive");
        }
        else if (!DeviceIoControl(hVolume,FSCTL_UNLOCK_VOLUME,NULL,
                                  0,NULL,0,&BytesReturned,NULL))
        {
            throw std::exception("Unable to unlock logical drive");
        }
    }else
        throw std::exception("Unable to open logical drive");


    if (UndefineLogicDrive(mountPoint))
        throw std::exception("Unable to undefine logical drive");

    wchar_t root[] = L"_:\\";
    root[0] = mountPoint;
    SHChangeNotify(SHCNE_DRIVEREMOVED, SHCNF_PATH, root, NULL);

    drvCtrl->Unmount(devId);
}
int MountImage(IImage * image, 
               wchar_t mountPoint,
               DriverControl * drvCtrl)
{
    int deviceId = drvCtrl->Mount(image->Size(), mountPoint);

    if(DefineLogicDrive(mountPoint, deviceId) != ERROR_SUCCESS)
    {
        UnmountImage(deviceId, mountPoint,drvCtrl);
        throw std::exception("DefineDosDeviceW fail.");
    }
    wchar_t root[] = L"_:\\";
    root[0] = mountPoint;
    SHChangeNotify(SHCNE_DRIVEADD,SHCNF_PATH,root,NULL);
    return deviceId;
}

void DispatchImage(int deviceId, 
                   wchar_t mountPoint,
                   IImage * image, 
                   DriverControl * drvCtrl)
{
    BOOST_SCOPE_EXIT( (&mountPoint) )
    {
        DWORD error = UndefineLogicDrive(mountPoint);
        if (error != ERROR_SUCCESS)
        {
            std::cout << __FUNCTION__" DDD_REMOVE_DEFINITION fail. error = " << error;
        }
    } BOOST_SCOPE_EXIT_END

    std::vector<char> dataBuf(1024*1024);
    int lastType = 0;
    int lastStatus = 0;
    int lastSize = 0;
    while(true)
    {
        int type = 0;
        int size = 0;
        __int64 offset = 0;
        drvCtrl->RequestExchange(deviceId, lastType, lastStatus, lastSize, &dataBuf[0], dataBuf.size(),
                                 &type, &size, &offset);

        //do requested operation
        DispatchImageOperation(image, type, size, offset, &dataBuf[0], dataBuf.size(), &lastStatus);
        lastType = type;
        lastSize = size;
    }
}
void DispatchImageOperation(IImage * image, 
                            int type, int size, __int64 in_offset, char* buf, int bufsize,
                            int* status)
{
    switch(type)
    {
    case directOperationEmpty:
        break;
    case directOperationRead:
        {
           image->Read((char*)buf, in_offset, size);
           *status = 0;
            break;
        }
    case directOperationWrite:
        {    
            image->Write((const char*)buf, in_offset, size);
            *status = 0;
            break;
        }
    default:
        assert(!"Unknown operation type.");
    }
}

