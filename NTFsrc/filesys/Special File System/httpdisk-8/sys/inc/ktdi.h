/*
    HTTP virtual disk driver for Windows.
    Copyright (C) 2006-2009 Bo Brantén.
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

#if !defined(KTDI_H)
#define KTDI_H

#if defined(__cplusplus)
extern "C" {
#endif

NTSTATUS tdi_open_transport_address(PUNICODE_STRING devName, ULONG addr, USHORT port, BOOLEAN shared, PHANDLE addressHandle, PFILE_OBJECT *addressFileObject);
NTSTATUS tdi_open_connection_endpoint(PUNICODE_STRING devName, PVOID connectionContext, BOOLEAN shared, PHANDLE connectionHandle, PFILE_OBJECT *connectionFileObject);
NTSTATUS tdi_set_event_handler(PFILE_OBJECT addressFileObject, LONG eventType, PVOID eventHandler, PVOID eventContext);
NTSTATUS tdi_unset_event_handler(PFILE_OBJECT addressFileObject, LONG eventType);
NTSTATUS tdi_associate_address(PFILE_OBJECT connectionFileObject, HANDLE addressHandle);
NTSTATUS tdi_disassociate_address(PFILE_OBJECT connectionFileObject);
NTSTATUS tdi_connect(PFILE_OBJECT connectionFileObject, ULONG addr, USHORT port);
NTSTATUS tdi_disconnect(PFILE_OBJECT connectionFileObject, ULONG flags);
NTSTATUS tdi_listen(PFILE_OBJECT connectionFileObject, PULONG addr, PUSHORT port);
NTSTATUS tdi_send_dgram(PFILE_OBJECT addressFileObject, ULONG addr, USHORT port, const char *buf, int len);
NTSTATUS tdi_send_dgram_mdl(PFILE_OBJECT addressFileObject, ULONG addr, USHORT port, PMDL mdl);
NTSTATUS tdi_recv_dgram(PFILE_OBJECT addressFileObject, PULONG addr, PUSHORT port, char *buf, int len, ULONG flags);
NTSTATUS tdi_send_stream(PFILE_OBJECT connectionFileObject, const char *buf, int len, ULONG flags);
NTSTATUS tdi_send_stream_mdl(PFILE_OBJECT connectionFileObject, PMDL mdl, ULONG flags);
NTSTATUS tdi_send_stream_mdl_async(PFILE_OBJECT connectionFileObject, PMDL mdl, ULONG flags, void (*completionRoutine)(int status, void *context), void *context);
NTSTATUS tdi_recv_stream(PFILE_OBJECT connectionFileObject, char *buf, int len, ULONG flags);
NTSTATUS tdi_query_address(PFILE_OBJECT addressFileObject, PULONG addr, PUSHORT port);

#if defined(__cplusplus)
}
#endif

#endif // !defined(KTDI_H)
