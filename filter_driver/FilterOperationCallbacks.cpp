#include "pch.h"

bool IsSkipFilename(const PUNICODE_STRING FileName)
{
	// Skip any MSFT files...
	if (FileName->Length >= 1 && FileName->Buffer[1] == L'$')
		return true;

	// Skip "\:$I30..."
	UNICODE_STRING RecycleBinPrefix = RTL_CONSTANT_STRING(L"\\:$I30");
	if (RtlPrefixUnicodeString(&RecycleBinPrefix, FileName, false))
		return true;

	// Skip root System Volume Information
	UNICODE_STRING SystemVolumeInfoPrefix = 
		RTL_CONSTANT_STRING(L"\\System Volume Information");
	if (RtlPrefixUnicodeString(&SystemVolumeInfoPrefix, FileName, false))
		return true;

	// Skip metadata file
	UNICODE_STRING MetadataFileName = 
		RTL_CONSTANT_STRING(CRYPT_METADATA_FILE_NAME);
	if (!RtlCompareUnicodeString(&MetadataFileName, FileName, false))
		return true;

	return false;
}

bool isSkipOperation(const PFLT_CALLBACK_DATA Data)
{
	// Skip 0 bytes read/write operations
	if ((Data->Iopb->MajorFunction == IRP_MJ_WRITE && 
			Data->Iopb->Parameters.Write.Length == 0) ||
		(Data->Iopb->MajorFunction == IRP_MJ_READ && 
			Data->Iopb->Parameters.Read.Length == 0))
		return true;

	// Skip cached operations
	if (!FlagOn(Data->Iopb->IrpFlags, IRP_NOCACHE))
		return true;

	// Skip IO operations over certain filenames
	if (IsSkipFilename(&Data->Iopb->TargetFileObject->FileName))
		return true;

	return false;
}

void xorBuf(
	PVOID Buffer,
	ULONG Size)
{
	KdPrint(("------XOR BEGIN------\n"));
	KdPrint(("size:  %d\n", Size));
	KdPrint(("ORIG  :["));
	for (uint32_t i = 0; i < AES_BLOCK_SIZE; i++)
		KdPrint(("%c", ((char*)(Buffer))[i]));
	KdPrint(("...]\n"));

	for (uint32_t i = 0; i < Size; i++)
		((uint8_t*)Buffer)[i] ^= 'A';

	KdPrint(("KRYPT :["));
	for (uint32_t i = 0; i < AES_BLOCK_SIZE; i++)
		KdPrint(("%c", ((char*)(Buffer))[i]));
	KdPrint(("...]\n"));
	KdPrint(("-------XOR END-------\n"));
}

/*---------------------------------------------------------------------------*/

// PRE WRITE
FLT_PREOP_CALLBACK_STATUS PreopMjWriteCallback(
	PFLT_CALLBACK_DATA Data,
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID* CompletionContext
)
{
	// Disable fast IO to get regular IRPs
	if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION))
		return FLT_PREOP_DISALLOW_FASTIO;

	if (isSkipOperation(Data))
		return FLT_PREOP_SUCCESS_NO_CALLBACK;

	KdPrint(("\nPRE WRITE [%wZ]\n", Data->Iopb->TargetFileObject->FileName));

	// Allocating writing swap buffer
	const ULONG WriteLen = Data->Iopb->Parameters.Write.Length;
	PVOID SwpBuffer = FltAllocatePoolAlignedWithTag(
		FltObjects->Instance,
		NonPagedPool,
		(SIZE_T)WriteLen,
		BUFFER_ALLOCATION_TAG
	);
	if (SwpBuffer == nullptr)
		goto CLEANUP_AND_CANCEL_WRITE_OPERATION;

	// Getting buffer address from MDL
	PVOID OriginalBuffer = MmGetSystemAddressForMdlSafe(
		Data->Iopb->Parameters.Write.MdlAddress,
		NormalPagePriority | MdlMappingNoExecute
	);
	if (OriginalBuffer == nullptr)
		goto CLEANUP_AND_CANCEL_WRITE_OPERATION;

	// Copy original buffer to our swap buffer
	RtlCopyMemory(
		SwpBuffer,
		OriginalBuffer,
		WriteLen
	);

	// Encrypt swap buffer
	xorBuf(SwpBuffer, WriteLen);

	// Pass encrypted swap buffer to post write callback to deallocate
	*CompletionContext = SwpBuffer;

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;

CLEANUP_AND_CANCEL_WRITE_OPERATION:
	{
		KdPrint((
			"Buffer allocation error happened. Canceling write operation...\n"));

		if (SwpBuffer != nullptr)
			FltFreePoolAlignedWithTag(
				FltObjects->Instance,
				SwpBuffer,
				BUFFER_ALLOCATION_TAG
			);

		Data->IoStatus.Status = STATUS_CANCELLED;
		Data->IoStatus.Information = 0;

		return FLT_PREOP_COMPLETE;
	}
}

// POST WRITE
FLT_POSTOP_CALLBACK_STATUS PostopMjWriteCallback(
	PFLT_CALLBACK_DATA Data,
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID CompletionContext,
	FLT_POST_OPERATION_FLAGS Flags
)
{
	UNICODE_STRING FileName{ Data->Iopb->TargetFileObject->FileName };
	KdPrint(("POST WRITE [%wZ]\n", FileName));

	// Deallocate swapped buffer
	PVOID SwpBuffer = CompletionContext;
	FltFreePoolAlignedWithTag(
		FltObjects->Instance,
		SwpBuffer,
		BUFFER_ALLOCATION_TAG
	);

	return FLT_POSTOP_FINISHED_PROCESSING;
}

/*---------------------------------------------------------------------------*/
// PRE READ
FLT_PREOP_CALLBACK_STATUS PreopMjReadCallback(
	PFLT_CALLBACK_DATA Data,
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID* CompletionContext
)
{
	// Disable fast IO to get regular IRPs
	if (FlagOn(Data->Flags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION))
		return FLT_PREOP_DISALLOW_FASTIO;

	if (isSkipOperation(Data))
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	
	KdPrint(("\nPRE READ [%wZ]\n", Data->Iopb->TargetFileObject->FileName));

	// Allocate reading swap buffer
	const ULONG ReadLen = Data->Iopb->Parameters.Read.Length;
	PVOID SwpBuffer = FltAllocatePoolAlignedWithTag(
		FltObjects->Instance,
		NonPagedPool,
		(SIZE_T)ReadLen,
		BUFFER_ALLOCATION_TAG
	);
	if (SwpBuffer == nullptr)
		goto CLEANUP_AND_CANCEL_READ_OPERATION;

	PMDL SwpMdlBuffer = IoAllocateMdl(SwpBuffer,
		ReadLen,
		FALSE,
		FALSE,
		NULL
	);
	if (SwpMdlBuffer == nullptr) 
		goto CLEANUP_AND_CANCEL_READ_OPERATION;

	MmBuildMdlForNonPagedPool(SwpMdlBuffer);

	// Set swap buffers
	Data->Iopb->Parameters.Read.ReadBuffer = SwpBuffer;
	Data->Iopb->Parameters.Read.MdlAddress = SwpMdlBuffer;
	FltSetCallbackDataDirty(Data);

	// Pass swap buffer to post read
	*CompletionContext = SwpBuffer;

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;

CLEANUP_AND_CANCEL_READ_OPERATION:
	{
		KdPrint((
			"Buffer allocation error happened. Canceling read operation...\n"));

		if (SwpBuffer != nullptr)
			FltFreePoolAlignedWithTag(
				FltObjects->Instance,
				SwpBuffer,
				BUFFER_ALLOCATION_TAG
			);

		//if (SwpMdlBuffer != nullptr)
		//	IoFreeMdl(SwpMdlBuffer);

		Data->IoStatus.Status = STATUS_CANCELLED;
		Data->IoStatus.Information = 0;

		return FLT_PREOP_COMPLETE;
	}
}

// POST READ
FLT_POSTOP_CALLBACK_STATUS PostopMjReadCallback(
	PFLT_CALLBACK_DATA Data,
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID CompletionContext,
	FLT_POST_OPERATION_FLAGS Flags
)
{
	UNICODE_STRING FileName{ Data->Iopb->TargetFileObject->FileName };
	KdPrint(("POST READ [%wZ]\n", FileName));

	PVOID SwpBuffer = CompletionContext;

	PVOID OriginalBuffer = MmGetSystemAddressForMdlSafe(
		Data->Iopb->Parameters.Read.MdlAddress,
		NormalPagePriority | MdlMappingNoExecute
	);
	if (OriginalBuffer == nullptr)
	{
		Data->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		Data->IoStatus.Information = 0;
	}

	//RtlCopyMemory(
	//	SwpBuffer,
	//	OriginalBuffer,
	//	Data->Iopb->Parameters.Read.Length
	//);

	xorBuf(SwpBuffer, Data->Iopb->Parameters.Read.Length);

	RtlCopyMemory(
		OriginalBuffer,
		SwpBuffer,
		Data->Iopb->Parameters.Read.Length
	);

	FltFreePoolAlignedWithTag(
		FltObjects->Instance,
		SwpBuffer,
		BUFFER_ALLOCATION_TAG
	);

	return FLT_POSTOP_FINISHED_PROCESSING;
}