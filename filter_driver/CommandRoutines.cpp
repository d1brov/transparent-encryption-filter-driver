#include "pch.h"

NTSTATUS MountRoutine(
	IN const MountCommandMessage* MountMsg
)
{
	PFLT_VOLUME Volume{};
	NTSTATUS Status = GetVolumeByLetter(&Volume, MountMsg->m_driveLetter);
	if (!NT_SUCCESS(Status))
		return Status;

	// Opening/Creating metadata file at volume
	FltHandleGuard FileGuard{};
	PFILE_OBJECT FileObject{};
	ULONG OpeningStatus{};
	Status = DriveMetadataFile::Create(
		FileGuard.getHandle(), 
		&FileObject, 
		&OpeningStatus, 
		Volume
	);
	if (!NT_SUCCESS(Status))
		return Status;

	// Getting filter instance at given volume
	PFLT_INSTANCE MetadataInstance{};
	Status = FltGetTopInstance(Volume, &MetadataInstance);
	if (!NT_SUCCESS(Status))
		return Status;

	// Check if metadata file was opened or created
	DriveMetadata DriveData {};
	switch (OpeningStatus)
	{
	case FILE_CREATED:
	{
		KdPrint(("Metadata file not found\n"));

		Status = InitDriveMetadata(&DriveData, MountMsg->m_hash);
		if (!NT_SUCCESS(Status))
			break;

		Status = DriveMetadataFile::Save(
			MetadataInstance,
			FileObject,
			&DriveData
		);
		FltObjectDereference(MetadataInstance);
		if (!NT_SUCCESS(Status))
			break;
		
		Status = AddEncryptionContext(Volume, MountMsg->m_hash);
		break;
	}

	case FILE_OPENED:
	{
		KdPrint(("Found metadata file\n"));

		Status = DriveMetadataFile::Read(
			MetadataInstance,
			FileObject,
			&DriveData
		);
		FltObjectDereference(MetadataInstance);
		if (!NT_SUCCESS(Status))
			break;

		Status = ValidatePasswordHash(&DriveData, MountMsg->m_hash);
		if (NT_SUCCESS(Status))
			Status = AddEncryptionContext(Volume, MountMsg->m_hash);
		
		break;
	}

	default:
		Status = STATUS_FILE_INVALID;
	}

	return Status;
}

NTSTATUS UnmountRoutine(
	IN const UnmountCommandMessage* UnmountMsg
)
{
	PFLT_VOLUME Volume{};
	NTSTATUS Status = GetVolumeByLetter(&Volume, UnmountMsg->m_driveLetter);
	if (!NT_SUCCESS(Status))
		return Status;

	PENCRYPTION_CONTEXT ContextToRemove;
	Status = GetEncryptionContext(Volume, ContextToRemove);
	if (!NT_SUCCESS(Status))
		return Status;

	RemoveEncryptionContext(ContextToRemove);

	Status = FltDetachVolume(g_FilterData.Filter, Volume, NULL);
	return Status;
}

NTSTATUS GetVolumeByLetter(
	OUT PFLT_VOLUME* Volume, 
	IN  const WCHAR DriveLetter)
{
	// Converting drive letter to drive path 'C' -> L"C:"
	UNICODE_STRING DrivePath{};
	WCHAR DrivePathBuffer[3]{ DriveLetter, L':', L'\0' };
	RtlInitUnicodeString(&DrivePath, DrivePathBuffer);

	return FltGetVolumeFromName(g_FilterData.Filter, &DrivePath, Volume);

}