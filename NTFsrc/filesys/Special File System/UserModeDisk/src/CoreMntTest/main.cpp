#include <iostream>
#include "mntFileImage.h"
#include "mntSparseImage.h"
#include "mntSyncMountmanager.h"
#include <conio.h>
#define _STATIC_CPPLIB
#define BLOCK_LENGTH         0x1000

int main(int argc, char* argv[])
{
    try
    {
        std::auto_ptr<IImage> img(new FileImage(L"tst_img"));
        std::auto_ptr<IImage> sparse(new SparseImage(img, 0, 0x10000000LL, BLOCK_LENGTH, 512, true));
        SyncMountManager mountManager;
        int devId = mountManager.AsyncMountImage(sparse, L'z');
        std::cout << "Image was mounted. Press any key for unmount.\n";
        for(bool isUnmounted = false;!isUnmounted;)
        {
            _getch();
            try
            {
                mountManager.UnmountImage(devId);
                isUnmounted = true;
                std::cout << "Image was unmounted. Press any key for exit.\n";
            }
            catch(const std::exception & ex)
            {
                std::cout << ex.what() << "\n";
            }
        }
        _getch();
    }
    catch(const std::exception & ex)
    {
        std::cout << ex.what();
    }
    return 0;
}