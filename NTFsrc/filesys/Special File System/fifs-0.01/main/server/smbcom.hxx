/*

   Copyright (C) 1998 Danilo Almeida.  All rights reserved.

   SMB command handler header

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

#ifndef __SMB_COM_H__
#define __SMB_COM_H__

BOOL
SmbComUnknown(
    DPB* dpb
    );

BOOL
SmbComNegotiate(
    DPB* dpb
    );

BOOL
SmbComSessionSetupAndx(
    DPB* dpb
    );

BOOL
SmbComTreeConnectAndx(
    DPB* dpb
    );

BOOL
SmbComNoAndx(
    DPB* dpb
    );

BOOL
SmbComTrans2(
    DPB* dpb
    );

BOOL
SmbComQueryInformation(
    DPB* dpb
    );

BOOL
SmbComSetInformation(
    DPB* dpb
    );

BOOL
SmbComCheckDirectory(
    DPB* dpb
    );

BOOL
SmbComFindClose2(
    DPB* dpb
    );

BOOL
SmbComDelete(
    DPB* dpb
    );

BOOL
SmbComRename(
    DPB* dpb
    );

BOOL
SmbComCreateDirectory(
    DPB* dpb
    );

BOOL
SmbComDeleteDirectory(
    DPB* dpb
    );

BOOL
SmbComOpenAndx(
    DPB* dpb
    );

BOOL
SmbComWrite(
    DPB* dpb
    );

BOOL
SmbComClose(
    DPB* dpb
    );

BOOL
SmbComReadAndx(
    DPB* dpb
    );

BOOL
SmbComQueryInformation2(
    DPB* dpb
    );

BOOL
SmbComSetInformation2(
    DPB* dpb
    );

BOOL
SmbComLockingAndx(
    DPB* dpb
    );

BOOL
SmbComSeek(
    DPB* dpb
    );

BOOL
SmbComFlush(
    DPB* dpb
    );

BOOL
SmbComLogoffAndx(
    DPB* dpb
    );

BOOL
SmbComTreeDisconnect(
    DPB* dpb
    );

#endif
