#include "pch.h"

NTSTATUS DriveMetadataFile::Create(
	OUT PHANDLE		  File,
	OUT PFILE_OBJECT* FileObject,
	OUT PULONG		  OpeningStatus,
	IN  PFLT_VOLUME   Volume
)
{
	///////////////////////////////////////////////////////////////////////////
	// Constructing full drive metadata file path at designated volume

	// Getting volume name size
	ULONG VolumeNameSize{};
	NTSTATUS Status = FltGetVolumeName(Volume, NULL, &VolumeNameSize);
	if (Status != STATUS_BUFFER_TOO_SMALL)
		return Status;

	// Alocating buffer for full file path L"VolumeName" + L'\\' + L"FileName"
	AllocStringGuard AlocatedStr(
		VolumeNameSize + sizeof(WCHAR) + CRYPT_METADATA_FILE_NAME_SIZE);
	PUNICODE_STRING FullFilePath = AlocatedStr.GetString();

	// Inserting volume name L"VolumeName"
	Status = FltGetVolumeName(Volume, FullFilePath, &VolumeNameSize);
	if (!NT_SUCCESS(Status))
		return Status;

	// Append L'\\'
	FullFilePath->Buffer[(FullFilePath->Length) / sizeof(WCHAR)] = L'\\';
	FullFilePath->Length += sizeof(WCHAR);

	// Append L"FileName"
	UNICODE_STRING FileName;
	RtlInitUnicodeString(&FileName, CRYPT_METADATA_FILE_NAME);
	Status = RtlUnicodeStringCat(FullFilePath, &FileName);
	if (!NT_SUCCESS(Status))
		return Status;

	// Done constructing full file path
	///////////////////////////////////////////////////////////////////////////

	// Init object attributes for metadata file
	OBJECT_ATTRIBUTES ObjectAttributes;
	InitializeObjectAttributes(
		&ObjectAttributes,
		FullFilePath,
		OBJ_KERNEL_HANDLE,
		NULL,
		NULL
	);

	// Open/create metadata file file
	IO_STATUS_BLOCK IoStatusBlock;
	LARGE_INTEGER FileAllocSize;
	FileAllocSize.QuadPart = sizeof(DriveMetadata);
	Status = FltCreateFileEx(
		g_FilterData.Filter,
		nullptr, // filter instance could be here
		File,
		FileObject,
#ifdef NDEBUG
		// nondebug(release)
		FILE_READ_DATA,
#else
		// debug
		FILE_ALL_ACCESS | DELETE,
#endif
		& ObjectAttributes,
		&IoStatusBlock,
		&FileAllocSize,
#ifdef NDEBUG
		// nondebug(release)
		FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM,
#else
		// debug
		FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN,
#endif
		FILE_SHARE_READ,
		FILE_OPEN_IF,
		FILE_NON_DIRECTORY_FILE,
		NULL,
		0,
		0
	);

	*OpeningStatus = IoStatusBlock.Information;

	return Status;
}

NTSTATUS DriveMetadataFile::Save(
	IN PFLT_INSTANCE  Instance,
	IN PFILE_OBJECT   FileObject,
	IN DriveMetadata* Metadata
)
{
	KdPrint(("Saving metadata file..."));

	LARGE_INTEGER ByteOffset;
	ByteOffset.QuadPart = 0;

	NTSTATUS Status = FltWriteFile(
		Instance,
		FileObject,
		&ByteOffset,
		sizeof(DriveMetadata),
		Metadata,
		FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
		nullptr,
		nullptr,
		nullptr
	);

	if (NT_SUCCESS(Status))
	{
		KdPrint(("DONE\n"));
	}
	else
	{
		KdPrint(("ERROR[%X]\n", Status));
	}
	return Status;
}

NTSTATUS DriveMetadataFile::Read(
	IN  PFLT_INSTANCE  Instance,
	IN  PFILE_OBJECT   FileObject,
	OUT DriveMetadata* Metadata
)
{
	KdPrint(("Reading metadata file..."));

	LARGE_INTEGER ByteOffset;
	ByteOffset.QuadPart = 0;

	NTSTATUS Status = FltReadFile(
		Instance,
		FileObject,
		&ByteOffset,
		sizeof(DriveMetadata),
		Metadata,
		FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
		nullptr,
		nullptr,
		nullptr
	);

	if (NT_SUCCESS(Status))
	{
		KdPrint(("DONE\n"));
	}
	else
	{
		KdPrint(("ERROR[%X]\n", Status));
	}
	return Status;
}