#pragma once

namespace DriveMetadataFile
{
/*
	Opens(creates) metadata file in root of specified volume
*/
NTSTATUS Create(
	OUT PHANDLE		  File,
	OUT PFILE_OBJECT* FileObject,
	OUT PULONG		  OpeningStatus,
	IN  PFLT_VOLUME   Volume
);

/*
	Writes DriveMetadata to given file by filter instance
	attached to volume
*/
NTSTATUS Save(
	IN PFLT_INSTANCE  Instance,
	IN PFILE_OBJECT   FileObject,
	IN DriveMetadata* Metadata
);

/*
	Reads DriveMetadata from given file by filter instance
	attached to volume
*/
NTSTATUS Read(
	IN  PFLT_INSTANCE  Instance,
	IN  PFILE_OBJECT   FileObject,
	OUT DriveMetadata* Metadata
);
}