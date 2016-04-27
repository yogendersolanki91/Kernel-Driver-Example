/*
    This is a romfs file system driver for Windows NT/2000/XP.
    Copyright (C) 1999, 2000, 2001, 2002 Bo Brantén.
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "ntifs.h"
#include "fsd.h"
#include "rom_fs.h"
#include "border.h"

#pragma code_seg(FSD_PAGED_CODE)

NTSTATUS
FsdIsDeviceRomfs (
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PUCHAR          Buffer;
    LARGE_INTEGER   Offset;
    NTSTATUS        Status;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);

    Buffer = FsdAllocatePool(NonPagedPool, SECTOR_SIZE, '1ceR');

    if (!Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Offset.QuadPart = ROMFS_MAGIC_OFFSET;

    Status = FsdReadBlockDeviceOverrideVerify(
        DeviceObject,
        &Offset,
        SECTOR_SIZE,
        Buffer
        );

    if (!NT_SUCCESS(Status) ||
        RtlCompareMemory(Buffer, ROMFS_MAGIC, sizeof(ROMFS_MAGIC) - sizeof(UCHAR)) != (sizeof(ROMFS_MAGIC) - sizeof(UCHAR))
        )
    {
        Status = STATUS_UNRECOGNIZED_VOLUME;
    }

    FsdFreePool(Buffer);

    return Status;
}

NTSTATUS
FsdIsDeviceSameRomfs (
    IN PDEVICE_OBJECT   DeviceObject,
    IN ULONG            CheckSum
    )
{
    struct romfs_super_block*   romfs_super_block;
    LARGE_INTEGER               Offset;
    NTSTATUS                    Status;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);

    romfs_super_block = FsdAllocatePool(NonPagedPool, SECTOR_SIZE, '2ceR');

    if (!romfs_super_block)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Offset.QuadPart = ROMFS_MAGIC_OFFSET;

    Status = FsdReadBlockDeviceOverrideVerify(
        DeviceObject,
        &Offset,
        SECTOR_SIZE,
        romfs_super_block
        );

    if (!NT_SUCCESS(Status) ||
        RtlCompareMemory(romfs_super_block, ROMFS_MAGIC, sizeof(ROMFS_MAGIC) - sizeof(UCHAR)) != (sizeof(ROMFS_MAGIC) - sizeof(UCHAR)) ||
        be32_to_cpu(romfs_super_block->checksum) != CheckSum
        )
    {
        Status = STATUS_WRONG_VOLUME;
    }

    FsdFreePool(romfs_super_block);

    return Status;
}

#pragma code_seg() // end FSD_PAGED_CODE
