/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   NetBIOS snooper SMB TRANS2 command handler

   This file is part of FIFS (Framework for Implementing File Systems). 

   This software is distributed with NO WARRANTY OF ANY KIND.  No
   author or distributor accepts any responsibility for the
   consequences of using it, or for whether it serves any particular
   purpose or works at all, unless he or she says so in writing.
   Refer to the included modified Alladin Free Public License (the
   "License") for full details.

   Every copy of this software must include a copy of the License, in
   a plain ASCII text file named COPYING.  The License grants you the
   right to copy, modify and redistribute this software, but only
   under certain conditions described in the License.  Among other
   things, the License requires that the copyright notice and this
   notice be preserved on all copies.

*/

#include "smball.h"

#define DEVICE_TYPE ULONG

typedef struct _FILE_FS_VOLUME_INFORMATION {
    LARGE_INTEGER VolumeCreationTime;
    ULONG VolumeSerialNumber;
    ULONG VolumeLabelLength;
    BOOLEAN SupportsObjects;
    WCHAR VolumeLabel[1];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FS_SIZE_INFORMATION {
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER AvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct _FILE_FS_DEVICE_INFORMATION {                    // ntddk nthal
    DEVICE_TYPE DeviceType;                                     // ntddk nthal
    ULONG Characteristics;                                      // ntddk nthal
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;     // ntddk nthal
                                                                // ntddk nthal
typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
    ULONG FileSystemAttributes;
    LONG MaximumComponentNameLength;
    ULONG FileSystemNameLength;
    WCHAR FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

LPSTR
Trans2QueryFsInfo(
    UCHAR command,
    LPSTR buffer,
    PNT_SMB_HEADER pSmb,
    DWORD dwSize,
    DWORD dwOffset,
    BOOL bRequest,
    PUCHAR *ppParameter,
    PUCHAR *ppData
    )
{
    PVOID pData = *ppParameter;
    if (bRequest) {
        PREQ_QUERY_FS_INFORMATION pReq = pData;

        int *value = malloc(sizeof(int));
        *value = pReq->InformationLevel;

        buffer += wsprintf(buffer,
                           "InformationLevel: 0x%04x\n",
                           pReq->InformationLevel);

        *ppParameter += sizeof(PREQ_QUERY_FS_INFORMATION);

        if (!IntHash_Add(ht, pSmb->Mid, value))
            buffer += wsprintf(buffer, "<BAD CONTEXT ADD>\n");

    } else {
        int *value;
        int level;

        if (!IntHash_Del(ht, pSmb->Mid, &value)) {
            buffer += wsprintf(buffer,
                               "<COULD NOT FIND CONTEXT>\n");
        } else {
            level = *value;
            free(value);

            buffer += wsprintf(buffer,
                               "InformationLevel: 0x%04x\n",
                               level);
            switch(level) {
            case SMB_INFO_ALLOCATION: {
                PFSALLOCATE pResp = pData;
                buffer += wsprintf(buffer,
                                   "idFileSystem: 0x%08x\n"
                                   "cSectorUnit : 0x%08x\n"
                                   "cUnit       : 0x%08x\n"
                                   "cUnitAvail  : 0x%08x\n"
                                   "cbSector    : 0x%04x\n",
                                   pResp->idFileSystem,
                                   pResp->cSectorUnit,
                                   pResp->cUnit,
                                   pResp->cUnitAvail,
                                   pResp->cbSector);
                break;
            }
            case SMB_INFO_VOLUME: {
                PFSINFO pResp = pData;
                int count = pResp->vol.cch;
                char* pch = pResp->vol.szVolLabel;
                buffer += wsprintf(buffer,
                                   "ulVsn: 0x%08x\n"
                                   "cch  : 0x%02x\n"
                                   "Label: ",
                                   pResp->ulVsn,
                                   pResp->vol.cch);
                
                while (count--) {
                    if (isalpha(*pch))
                        buffer += wsprintf(buffer, "%c", *pch);
                    pch++;
                }
                buffer += wsprintf(buffer, "\n");
                break;
            }
//             case SMB_QUERY_FS_VOLUME_INFO: {
//                 PFILE_FS_VOLUME_INFORMATION pResp = pData;
//                 ULONG count = pResp->VolumeLabelLength;
//                 WCHAR* pwch = pResp->VolumeLabel;
//             }
            default:
                buffer += wsprintf(buffer,
                                   "<COULD NOT UNDERSTAND INFO LEVEL>\n");
            }
        }
    }
    return buffer;
}

LPSTR
Trans2FindFirst2(
    UCHAR command,
    LPSTR buffer,
    PNT_SMB_HEADER pSmb,
    DWORD dwSize,
    DWORD dwOffset,
    BOOL bRequest,
    PUCHAR *ppParameter,
    PUCHAR *ppData
    )
{
    return buffer;
}

LPSTR
Trans2FindNext2(
    UCHAR command,
    LPSTR buffer,
    PNT_SMB_HEADER pSmb,
    DWORD dwSize,
    DWORD dwOffset,
    BOOL bRequest,
    PUCHAR *ppParameter,
    PUCHAR *ppData
    )
{
    return buffer;
}
