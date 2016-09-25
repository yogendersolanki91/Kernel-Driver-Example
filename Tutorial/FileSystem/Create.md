# IRP_MJ_CREATE Rules

- FSContext -  File Control Block ( per file ) 
- FSContext2 - Context Control Block ( per open instance of file)


## Algorithm 
* **VCB Validation** - Ensure that the operation has been directed to a valid VCB 
```cpp
ASSERT(PtrVCB->NodeIdentifier.NodeType == SFSD_NODE_TYPE_VCB);
```
*Coarse grained locking - Acquire the VCB exclusively. will lock out all other concurrent create/open If the VCB is also acquired by the cleanup/close dispatch you can count on create/opens being synchronized with these as well.*
```cpp
ExAcquireResourceExclusiveLite(&(PtrVCB->VCBResource), TRUE);
```
* **Verify** - Disk based file systems might decide to verify the logical volume
* **Check Volume Lock** -  If the volume has been locked, fail the request
```cpp
if (PtrVCB->VCBFlags & SFSD_VCB_FLAGS_VOLUME_LOCKED) {
			RC = STATUS_ACCESS_DENIED;
			try_return(RC);
		}
```
**Volume Open Request** - Check If a volume open is requested, satisfy it. If the supplied file name is NULL **and** either there no related file object **or** if a related file object was but it too refers to a previously opened instance of a volume, this open must be for a logical volume. 
*Note: your FSD might decide to do "special" things (whatever might be) in response to an open request for the logical volume.*
 - Logical volume open requests are done primarily to get/set information, lock the volume, dismount the volume (using the FSCTL_DISMOUNT_VOLUME) etc.  If a volume open is requested, perform checks to ensure invalid options have not also been specified.
```cpp
if ((PtrNewFileObject->FileName.Length == 0) && ((PtrRelatedFileObject == NULL) ||
			  (PtrRelatedFCB->NodeIdentifier.NodeType == SFSD_NODE_TYPE_VCB))) {
if ((OpenTargetDirectory) || (PtrExtAttrBuffer)) {
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
			}
			if (DirectoryOnlyRequested) {
				// a volume is not a directory
				RC = STATUS_NOT_A_DIRECTORY;
				try_return(RC);
			}
			if ((RequestedDisposition != FILE_OPEN) && (RequestedDisposition != FILE_OPEN_IF)) {
				// cannot create a new volume, I'm afraid ...
				RC = STATUS_ACCESS_DENIED;
				try_return(RC);
			}
RC = SFsdOpenVolume(PtrVCB, PtrIrpContext, PtrIrp, ShareAccess, PtrSecurityContext, PtrNewFileObject);
			ReturnedInformation = PtrIrp->IoStatus.Information;

			try_return(RC);
}
}
```

**OpenById** - Your FSD might wish to implement the open-by-id option. The "is some unique numerical representation of the on-disk object.The caller then therefore give you this file id and your FSD should be completely capable of "opening" the object (it must exist since the caller received an id for the object from your FSD in a "query file" call ... If the file has been deleted in the meantime, you can return "not found".
```cpp
if (OpenByFileId) {			
			RC = SFsdOpenByFileId(PtrIrpContext, PtrIrp ....);
			try_return(RC);
}
```

**Parsing the path** -
There is two way path can be specified in the create reuest either with related file object or using absolute path. 

*Parsing with related FileObejct* - We have a user supplied related file object. This implies a "relative" open i.e. relative to the directory represented by the related file object.

```cpp
if (!(PtrRelatedFCB->FCBFlags & SFSD_FCB_DIRECTORY)) {
				// we must have a directory as the "related" object
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
}
```
*Note: The only purpose FSD implementations ever have for the related file object is to determine whether this is a relative open or not. At all other times (including during I/O operations), this field is meaningless from the FSD's perspective.*

So we have a directory, ensure that the name begins witha "\" i.e. begins at the root and does *not* begin with a "\\" 
```cpp
if ((RelatedObjectName.Length == 0) || (RelatedObjectName.Buffer[0] != L'\\')) {
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
			}
````
   *NOTE: This is just an example of the kind of path-name string validation that a FSD must do. Although the remainder of the code may not include such checks, any commercial FSD *must* include such checking (no one else, including the I/O Manager will perform checks on your FSD's behalf)*

Similarly, if the target file name starts with a "\", it is wrong since the target file name can no longer be absolute
```cpp
if ((TargetObjectName.Length != 0) && (TargetObjectName.Buffer[0] == L'\\')) {
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
}
```
Create an absolute path-name. You could potentially the absolute path-name if you cache previously opened file/directory object names. i.e. Concate relative and target fileObjectPath which is stored in **AbsolutePathName**.

*Parsing with Absolute Path* - If related file object is null then it must be a absolute path. 
-  Check if path starting at the root of the file system tree.
```
 if (TargetObjectName.Buffer[0] != L'\\') {
				RC = STATUS_INVALID_PARAMETER;
				try_return(RC);
			}
```

**Open for root directory** -  check if the caller simply wishes to open the root of the file system tree. If, this is an open of the root directory, ensure that the caller has not requested a file only.  Open root directory here and Include creation of a new CCB structurture. 
```
		if (AbsolutePathName.Length == 2) {
			// this is an open of the root directory, ensure that	the caller has not requested a file only
			if (FileOnlyRequested || (RequestedDisposition == FILE_SUPERSEDE) || (RequestedDisposition == FILE_OVERWRITE) ||
				 (RequestedDisposition == FILE_OVERWRITE_IF)) {
				RC = STATUS_FILE_IS_A_DIRECTORY;
				try_return(RC);
			}
                        //CCB and Open opearion.			
			//	e.g. RC = SFsdOpenRootDirectory(...);

			try_return(RC);
		}
``` 
**Traversal using path** - If related object is mentioned then start from related object (to reduce travesal cost) other wise with absolute path and start with root object.
*NOTE: If your FSD does not support access checking i.e. your FSD does not check "traversal" privileges, you could easily maintain a "prefix" cache containing path-names and open FCB pointers. Then, if the requested path-name is already present in the cache i.e. someone had opened it earlier, you can avoid the tedious traversal of the entire path-name performed below and described in the book ...
If you do not maintain such a prefix table cache of previously opened object names or if you do not find the name to be opened in the cache, then: Get the "next" component in the name to be parsed. *
*Note that obtaining the next string component is similar to the strtok library routine where the separator is a "\" Your FSD should also always check the validity of the token to ensure that only valid characters comprise the path/file name*

Now start from staring directory and loop through complete path perform these actions on each node  -
  - acquire the parent directory FCB MainResource exclusively
  - ensure that the parent directory in which you will perform
  - a lookup operation is indeed a directory
  - if there are no more components left after this one in the pathname supplied by the user, then break;
  - attempt to lookup the sub-directory in the parent
  - if not found, return STATUS_OBJECT_PATH_NOT_FOUND
  -  Otherwise, open the new sub-directory and make it the "parent"
  - go back and repeat the loop for the next component in the path

*NOTE: If your FSD supports it, you should always check that the caller has appropriate privileges to traverse the directories being searched.* 

Now after reaching to parent directory of target, check it out to see if it exists.

**OpenTargetDirectory** - It used for opening the parent directory of target.
```
if (NT_SUCCESS(RC)) {
				// file exists, set this information in the Information field
				ReturnedInformation = FILE_EXISTS;
			} else {
				RC = STATUS_SUCCESS;

				// Tell the I/O Manager that file does not exit
				ReturnedInformation = FILE_DOES_NOT_EXIST;
			}
```
Now, do the following:
- Replace the string in the FileName field in the PtrNewFileObject to identify the "target name" only (sans the path leading to the object)
```
{
					unsigned int	Index = (AbsolutePathName.Length - 1);

					// Back up until we come to the last '\'
					// But first, skip any trailing '\' characters

					while (AbsolutePathName.Buffer[Index] == L'\\') {
						ASSERT(Index >= sizeof(WCHAR));
						Index -= sizeof(WCHAR);
						// Skip this length also
						PtrNewFileObject->FileName.Length -= sizeof(WCHAR);
					}

					while (AbsolutePathName.Buffer[Index] != L'\\') {
						// Keep backing up until we hit one
						ASSERT(Index >= sizeof(WCHAR));
						Index -= sizeof(WCHAR);
					}

					// We must be at a '\' character
					ASSERT(AbsolutePathName.Buffer[Index] == L'\\');
					Index++;

					// We can now determine the new length of the filename
					// and copy the name over
					PtrNewFileObject->FileName.Length -= (unsigned short)(Index*sizeof(WCHAR));
					RtlCopyMemory(&(PtrNewFileObject->FileName.Buffer[0]),
									  &(PtrNewFileObject->FileName.Buffer[Index]),
                             PtrNewFileObject->FileName.Length);
				}
```
(b) Return with the target's parent directory opened
(c) Update the file object FsContext and FsContext2 fields to reflect the fact that the parent directory of the target has been opened.
### EndOfIrp

**Object was not found in travesal** - 
- **Crearte New** if IRP requestedDisposition asks to do. Further, note that since the file is being created, no other thread can have the file stream open at this time.Open the newly created object
Set the allocation size for the object is specified. Set extended attributes for the file.Set the Share Access for the file stream. The FCBShareAccess field will be set by the I/O Manager.
``` 
if ((RequestedDisposition == FILE_CREATE) || (RequestedDisposition == FILE_OPEN_IF) ||
				 (RequestedDisposition == FILE_OVERWRITE_IF)) {
  		               // Create a new file/directory here ...
IoSetShareAccess(DesiredAccess, ShareAccess, PtrNewFileObject, &(PtrNewFCB->FCBShareAccess));
                                RC = STATUS_SUCCESS;
				ReturnedInformation = FILE_CREATED;
}
```
**Object Found** -
- **Create New** - 
```
if (RequestedDisposition == FILE_CREATE) {
				ReturnedInformation = FILE_EXISTS;
				RC = STATUS_OBJECT_NAME_COLLISION;
				try_return(RC);
}
```
- **No Create** - 
- Obtain the FCB MainResource exclusively at this time.
- Check if caller wanted a **directory only** and target object not a directory, or caller wanted a file only and target object not a file .
```
if (FileOnlyRequested && (PtrNewFCB->FCBFlags & SFSD_FCB_DIRECTORY)) {
				// Close the new FCB and leave.
				//	SFsdCloseCCB(PtrNewCCB);
				RC = STATUS_FILE_IS_A_DIRECTORY;
				try_return(RC);
			}

			if ((PtrNewFCB->FCBFlags & SFSD_FCB_DIRECTORY) && ((RequestedDisposition == FILE_SUPERSEDE) ||
				  (RequestedDisposition == FILE_OVERWRITE) || (RequestedDisposition == FILE_OVERWRITE_IF))) {
				RC = STATUS_FILE_IS_A_DIRECTORY;
				try_return(RC);
			}

			if (DirectoryOnlyRequested && !(PtrNewFCB->FCBFlags & SFSD_FCB_DIRECTORY)) {
				// Close the new FCB and leave.
				//	SFsdCloseCCB(PtrNewCCB);
				RC = STATUS_NOT_A_DIRECTORY;
				try_return(RC);
			}
```
- Match **shared access** compatibility.
```
if (PtrNewFCB->OpenHandleCount > 0) {
				// The FCB is currently in use by some thread.
				// We must check whether the requested access/share access
				// conflicts with the existing open operations.

				if (!NT_SUCCESS(RC = IoCheckShareAccess(DesiredAccess, ShareAccess, PtrNewFileObject,
												&(PtrNewFCB->FCBShareAccess), TRUE))) {
					// SFsdCloseCCB(PtrNewCCB);
					try_return(RC);
				}
			} else {
					IoSetShareAccess(DesiredAccess, ShareAccess, PtrNewFileObject, &(PtrNewFCB->FCBShareAccess));
			}
```
- Process further a do the remainig if target open completed succesfully.
```
ReturnedInformation = FILE_OPENED;

			// If a supersede or overwrite was requested, do so now ...
			if (RequestedDisposition == FILE_SUPERSEDE) {
				// Attempt the operation here ...
				//	RC = SFsdSupersede(...);
				if (NT_SUCCESS(RC)) {
					ReturnedInformation = FILE_SUPERSEDED;
				}
			} else if ((RequestedDisposition == FILE_OVERWRITE) || (RequestedDisposition == FILE_OVERWRITE_IF)){
				// Attempt the operation here ...
				//	RC = SFsdOverwrite(...);
				if (NT_SUCCESS(RC)) {
					ReturnedInformation = FILE_OVERWRITTEN;
				}
			}
```
- Final Bookkeeping task.
- Update the file object such that **(Succesfull operation)**:
  - the FsContext field points to the NTRequiredFCB field in the FCB
  - the FsContext2 field points to the CCB created as a result of the open operation
  - If write-through was requested, then mark the file object appropriately
-  Always
    * Release VCB lock
    * Free AbsolutePath

------------------------------------------------
- **Opening a volume ** -   Check for exclusive open requests (using share modes supplied) and determine whether it is even possible to open the volume with the specified share modes (e.g. if caller does not wish to share read or share write ...) Use IoCheckShareAccess() and IoSetShareAccess() here ...They are defined in the DDK. You might also wish to check the caller's security context to see whether you wish to allow the volume open or not.Use the SeAccessCheck() routine described in the DDK for

- Create New CCB
```
if (!(PtrCCB = SFsdAllocateCCB())) {
			RC = STATUS_INSUFFICIENT_RESOURCES;
			try_return(RC);
		}
```
- Initialize the CCB
```
PtrCCB->PtrFCB = (PtrSFsdFCB)(PtrVCB);
InsertTailList(&(PtrVCB->VolumeOpenListHead), &(PtrCCB->NextCCB));
```
- Update file object (given by IO Manager)
```
PtrCCB->PtrFileObject = PtrNewFileObject;
```
- Setup FS Context
```
	PtrNewFileObject->FsContext = (void *)(PtrVCB);
	PtrNewFileObject->FsContext2 = (void *)(PtrCCB);
```

- Increment the number of outstanding open operations on this logical volume (i.e. volume cannot be dismounted). You might be concerned about 32 bit wrap-around though I would
argue that it is unlikely ... :-)
```
(PtrVCB->VCBOpenCount)++;
	
		// now set the IoStatus Information value correctly in the IRP
		//	(caller will set the status field)
		PtrIrp->IoStatus.Information = FILE_OPENED;
```



-------------------------------------------------

## Parmeters

### Basic Irp Parameter
- **Allocation Size** - Allocation size is only used if a new file is or a file is superseded.
```cpp
AllocationSize    = PtrIrp->Overlay.AllocationSize.LowPart;
```
*Note: Some FSD implementations support file sizes > 2 The following check is only used if your FSD does not a large file size. With NT version 5.0, 64 bit support become available and your FSD should ideally support large files*
```
if (PtrIrp->Overlay.AllocationSize.HighPart) {
			RC = STATUS_INVALID_PARAMETER;
			try_return(RC);
}
```
- ** Security Context and Desired Access ** - Ptr to the supplied security context. The desired access can be obtained from the SecurityContext
```
PtrSecurityContext = PtrIoStackLocation->Parameters.Create.SecurityContext;
DesiredAccess = PtrSecurityContext->DesiredAccess;
```

### Create Disposition
The file disposition is packed with the user Disposition includes FILE_SUPERSEDE, FILE_OPEN_IF, et
```cpp
RequestedDisposition = ((PtrIoStackLocation->Parameters.Create.Options >> 24) && 0xFF);
```

### User supplied option
Option are related to create option that are user supplied
```
RequestedOptions = (PtrIoStackLocation->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS);
```


- **DirectoryOnlyRequested** - User specifies that returned object MUST be a directory, Lack of presence of this flag does not mean it *cannot* be a directory *unless* FileOnlyRequested is set. Presence of the flag however, does require that the returned object be a directory (container) object.
```cpp
DirectoryOnlyRequested = ((RequestedOptions & FILE_DIRECTORY_FILE) ? TRUE : FALSE);
```
- **FileOnlyRequested** - User specifies that returned object MUST NOT be a Lack of presence of this flag does not mean it *cannot* be file *unless* DirectoryOnlyRequested is set (see Presence of the flag however does require that the returned object a simple file (non-container) object.
```cpp
FileOnlyRequested = ((RequestedOptions & FILE_NON_DIRECTORY_FILE) ? TRUE : FALSE);
```
- **NoBufferingSpecified ** We cannot cache the file if the following flag is However, things do get a little bit interesting if has been already initiated due to a previous (maintaining consistency then becomes a little bit of a headache - see read/write file descriptions)
```cpp
NoBufferingSpecified = ((RequestedOptions & FILE_NO_INTERMEDIATE_BUFFERING) ? TRUE : FALSE);
```	

- **WriteThroughRequested**- Write-through simply means that the FSD must *not* return a user write request until the data has been flushed to storage (either to disks directly connected to the node or the network in the case of a redirector)
```cpp
WriteThroughRequested = ((RequestedOptions & FILE_WRITE_THROUGH) ? TRUE : FALSE);
```	
- **DeleteOnCloseSpecified** Not all of the native file system implementations the delete-on-close option. All this means is that after last close on the FCB has been performed, your FSD delete the file. It simply saves the caller from issuing separate delete request. Also, some FSD implementations might to implement a Windows NT idiosyncratic behavior wherein could create such "delete-on-close" marked files under marked for deletion. Ordinarily, a FSD will not allow you to a new file under a directory that has been marked for deletion.
```cpp
DeleteOnCloseSpecified = ((RequestedOptions & FILE_DELETE_ON_CLOSE) ? TRUE : FALSE);
```	
- **NoExtAttrKnowledge** 
```cpp
NoExtAttrKnowledge = ((RequestedOptions & FILE_NO_EA_KNOWLEDGE) ? TRUE : FALSE);
```	
- **CreateTreeConnection** The following flag is only used by the LAN Manager to	initiate a "new mapping" to a remote share. a FSD will not see this flag (especially disk based FSD's)
```cpp
CreateTreeConnection = ((RequestedOptions & FILE_CREATE_TREE_CONNECTION) ? TRUE : FALSE);
```	
- **OpenByFileId**   The NTFS file system for exmaple supports the OpenByFileId Your FSD may also be able to associate a unique numerical ID an on-disk object. The caller would get this ID in a "query information" Later, the caller might decide to reopen the object, this though it may supply your FSD with the file identifier instead a file/path name.
```cpp
OpenByFileId = ((RequestedOptions & FILE_OPEN_BY_FILE_ID) ? TRUE : FALSE);
```	
- **PageFileManipulation** - Are we deailing with  page file.
```cpp
PageFileManipulation = ((PtrIoStackLocation->Flags & SL_OPEN_PAGING_FILE) ? TRUE : FALSE);
```	
- **OpenTargetDirectory** - The open target directory flag is used as part of the sequence operations performed by the I/O Manager is response to a file rename operation. See the explanation in the book for details.
```cpp
OpenTargetDirectory = ((PtrIoStackLocation->Flags & SL_OPEN_TARGET_DIRECTORY) ? TRUE : FALSE);
```	
- **IgnoreCaseWhenChecking** If your FSD supports case-sensitive file name checks, you choose to honor the following flag 
```cpp
IgnoreCaseWhenChecking = ((PtrIoStackLocation->Flags & SL_CASE_SENSITIVE) ? TRUE : FALSE);
```

 
