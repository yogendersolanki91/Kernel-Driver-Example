#include<WUFS.h>

PUFS_WAITER AllocateWaiterForUFS(PIRP Irp){
	PUFS_WAITER PtrUFSWaiter = MALLOC_NP(sizeof(UFS_WAITER));
	if (PtrUFSWaiter == NULL)
		return	NULL;
	RtlZeroMemory(PtrUFSWaiter, sizeof(UFS_CONTEXT));
	PtrUFSWaiter->UFSContext = MALLOC_UFSCONTEXT_NP();

	if (PtrUFSWaiter->UFSContext == NULL)
		return	NULL;
	RtlZeroMemory(PtrUFSWaiter->UFSContext, sizeof(UFS_CONTEXT));
	PtrUFSWaiter->UFSContext->InQueue = FALSE;
	PtrUFSWaiter->UFSContext->IsCompleted = FALSE;
	PtrUFSWaiter->UFSContext->MdlUFSBuffer = Irp->MdlAddress;
	PtrUFSWaiter->UFSContext->Irp = Irp;
	return PtrUFSWaiter;
}
NTSTATUS ProceessIOCTLForController(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	PIO_STACK_LOCATION PtrIoStackLocation;
	ULONG OutBufferLen;
	UNICODE_STRING ProcessedIOCTLStr;
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG Information = 0;
	PtrIoStackLocation = IoGetCurrentIrpStackLocation(Irp);
	OutBufferLen = PtrIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
	switch (PtrIoStackLocation->Parameters.DeviceIoControl.IoControlCode)
	{
			case QUERY_IRP:
			{
									PUFS_WAITER PtrUFSWaiter = AllocateWaiterForUFS(Irp);
									RtlInitUnicodeString(&ProcessedIOCTLStr, L" QUERY_PENDING_IRP");
									if (PtrUFSWaiter != NULL)
										Status = EnqueueUFSThread(&GET_CONTROLLER_STATE(DeviceObject)->Waitlist, PtrUFSWaiter);
									else
										Status = STATUS_INSUFFICIENT_RESOURCES;
									break;
			}
			case COMPLETE_AND_QUERY_IRP:
			{
									RtlInitUnicodeString(&ProcessedIOCTLStr, L"COMPLETE_AND_QUERY_IRP");
									PKERNEL_PACKET PtrPacket = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
									ProcessUFSResponse(PtrPacket);
									PUFS_WAITER PtrUFSWaiter = AllocateWaiterForUFS(Irp);		
									if (PtrUFSWaiter != NULL)
										Status = EnqueueUFSThread(& GET_CONTROLLER_STATE(DeviceObject)->Waitlist, PtrUFSWaiter);
									else
										Status = STATUS_INSUFFICIENT_RESOURCES;
									break;
			}
			case COMPLETE_OR_QUERY_IRP:
			{
									RtlInitUnicodeString(&ProcessedIOCTLStr, L"COMPLETE_OR_QUERY_IRP");
									PKERNEL_PACKET PtrPacket = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
									ProcessUFSResponse(PtrPacket);
									PUFS_WAITER PtrUFSWaiter = AllocateWaiterForUFS(Irp);
									KIRQL OldIRQL = KeGetCurrentIrql();
									KeAcquireSpinLock(& GET_CONTROLLER_STATE(DeviceObject)->Waitlist.ListLock, &OldIRQL);
									if (PtrUFSWaiter != NULL &&  GET_CONTROLLER_STATE(DeviceObject)->Waitlist.Who == IO_THREAD)
									{
										KeReleaseSpinLock(& GET_CONTROLLER_STATE(DeviceObject)->Waitlist.ListLock, OldIRQL);
										Status = EnqueueUFSThread(& GET_CONTROLLER_STATE(DeviceObject)->Waitlist, PtrUFSWaiter);
									}
									else if (PtrUFSWaiter == NULL)
									{
										KeReleaseSpinLock(& GET_CONTROLLER_STATE(DeviceObject)->Waitlist.ListLock, OldIRQL);
										Status = STATUS_INSUFFICIENT_RESOURCES;
										PtrPacket->HasIRP = FALSE;
									}
									else
									{
										KeReleaseSpinLock(& GET_CONTROLLER_STATE(DeviceObject)->Waitlist.ListLock, OldIRQL);
										Status = STATUS_SUCCESS;
										PtrPacket->HasIRP = FALSE;
									}
									break;
			}
			case COMPLETE_IRP:
			{
									RtlInitUnicodeString(&ProcessedIOCTLStr, L"COMPLETE_IRP");
									PKERNEL_PACKET PtrPacket = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
									ProcessUFSResponse(PtrPacket);
									Status = STATUS_SUCCESS;									
									break;
			}

			case QUERY_WRITE_DATA:
			{
									 RtlInitUnicodeString(&ProcessedIOCTLStr, L"QUERY_WRITE_DATA");
									 PKERNEL_PACKET PtrPacket = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
									 PIRP_CONTEXT PtrContext = (PIRP_CONTEXT)PtrPacket->Response.KernelIrpContext;
									 WUFSGetWriteData(PtrPacket, PtrContext);
									 Status = STATUS_SUCCESS;
									 break;;
			}
			default:
			{
									RtlInitUnicodeString(&ProcessedIOCTLStr, L"Unknow Operation");
									Status = STATUS_SUCCESS;
									break;
			}
	}	
	//DEBUG_WUFS("OUT:CONTRL_IOCTL\t %wZ \n", &ProcessedIOCTLStr);
	Irp->IoStatus.Information = Information;
	return Status;
}