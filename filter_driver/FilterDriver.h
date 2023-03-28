#pragma once

struct FilterDriverData
{
	PFLT_FILTER Filter;
	PFLT_PORT   ServerPort;
};
extern FilterDriverData g_FilterData;
/*
	Pool memory allocation tag for strings
*/
#define STR_ALLOCATION_TAG 'rtsU'
#define BUFFER_ALLOCATION_TAG 'fubR'
#define ENCR_CTX_ALLOCATION_TAG 'xtcE'

#define CRYPT_METADATA_FILE_NAME L"\\CRYPT"
#define CRYPT_METADATA_FILE_NAME_SIZE (sizeof(CRYPT_METADATA_FILE_NAME) - sizeof(WCHAR))

/*
	Function prototypes
*/
NTSTATUS DriverEntry(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath);

void DriverUnload(
	_DRIVER_OBJECT* DriverObject
);

NTSTATUS FilterUnloadCallback(
	FLT_FILTER_UNLOAD_FLAGS Flags);

NTSTATUS FLTAPI InstanceSetupCallback(
	IN PCFLT_RELATED_OBJECTS  FltObjects,
	IN FLT_INSTANCE_SETUP_FLAGS  Flags,
	IN DEVICE_TYPE  VolumeDeviceType,
	IN FLT_FILESYSTEM_TYPE  VolumeFilesystemType
);

NTSTATUS FLTAPI InstanceQueryTeardownCallback(
	IN PCFLT_RELATED_OBJECTS FltObjects,
	IN FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);