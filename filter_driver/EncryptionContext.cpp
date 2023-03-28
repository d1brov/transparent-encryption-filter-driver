#include "pch.h"

NTSTATUS AddEncryptionContext(
	IN PFLT_VOLUME Volume,
	IN const uint8_t Key[AES_KEY_SIZE_256]
)
{
	KdPrint(("Adding encryption context..."));
	PENCRYPTION_CONTEXT EncryptionContext = (PENCRYPTION_CONTEXT)ExAllocatePool2(
		POOL_FLAG_PAGED,
		sizeof(ENCRYPTION_CONTEXT),
		ENCR_CTX_ALLOCATION_TAG
	);
	if (EncryptionContext == 0)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// set Volume
	EncryptionContext->Volume = Volume;

	// set Key
	// todo derive Key
	// memcpy(EncryptionContext->Key, Key, sizeof(Key));
	
	// generate random IV
	CryptUtils::GenerateRandomBytes(EncryptionContext->IV, AES_CTR_IV_SIZE);

	// add volume to attachin whitelist
	InsertTailList(&g_EncrCtxList.ListEntry, &EncryptionContext->ListEntry);
	KdPrint(("DONE\n"));

	// trigger instance setup callback
	NTSTATUS Status = FltAttachVolume(
		g_FilterData.Filter,
		Volume,
		nullptr,
		&EncryptionContext->VolumeFilter
	);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("Attaching filter volume instance failed. [%X]\n", Status));
		RemoveEncryptionContext(EncryptionContext);
		return Status;
	}

	KdPrint(("Filter instance started filtering\n"));
	return STATUS_SUCCESS;
}

void RemoveEncryptionContext(IN PENCRYPTION_CONTEXT EncryptionContext)
{
	KdPrint((
		"Removing encryption context for VOL[%X]...", 
		EncryptionContext->Volume
		));
	FltObjectDereference(EncryptionContext->Volume);
	FltObjectDereference(EncryptionContext->VolumeFilter);
	RemoveEntryList(&EncryptionContext->ListEntry);
	ExFreePoolWithTag(EncryptionContext, ENCR_CTX_ALLOCATION_TAG);
	KdPrint(("DONE\n"));
}

NTSTATUS GetEncryptionContext(
	IN PFLT_VOLUME Volume,
	OUT PENCRYPTION_CONTEXT &EncryptionContext
)
{
	PLIST_ENTRY CtxEntryIterator = g_EncrCtxList.ListEntry.Flink;
	while (CtxEntryIterator != &g_EncrCtxList.ListEntry)
	{
		PENCRYPTION_CONTEXT TmpEncryptionContext = 
			(PENCRYPTION_CONTEXT)CONTAINING_RECORD(
				CtxEntryIterator, 
				ENCRYPTION_CONTEXT,
				ListEntry
			);

		if (TmpEncryptionContext->Volume == Volume)
		{
			EncryptionContext = TmpEncryptionContext;
			return STATUS_SUCCESS;
		}

		CtxEntryIterator = CtxEntryIterator->Flink;
	}
	return STATUS_NOT_FOUND;
}