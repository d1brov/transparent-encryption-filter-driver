#include "pch.h"

FilterDriverData g_FilterData{};
ENCRYPTION_CONTEXT g_EncrCtxList{};
/*
	Registrations
*/
const FLT_OPERATION_REGISTRATION Callbacks[] = {
	{ IRP_MJ_READ,   0, PreopMjReadCallback,   PostopMjReadCallback},
	{ IRP_MJ_WRITE,  0, PreopMjWriteCallback,  PostopMjWriteCallback},
	{ IRP_MJ_OPERATION_END }
};

const FLT_REGISTRATION FilterRegistration = {
	sizeof(FLT_REGISTRATION),
	FLT_REGISTRATION_VERSION,
	0,
	NULL,
	Callbacks,
	FilterUnloadCallback,
	InstanceSetupCallback,
	InstanceQueryTeardownCallback,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

/*
	Entry point
*/
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	KdPrint(("\n--- FILTER INITIALIZING BEGIN ---\n"));

	DriverObject->DriverUnload = DriverUnload;

	// Register filter
	NTSTATUS Status = FltRegisterFilter(
		DriverObject,
		&FilterRegistration,
		&g_FilterData.Filter
	);
	if (!NT_SUCCESS(Status))
		return Status;
	KdPrint(("Filter registered\n"));

	// Create communication port
	UNICODE_STRING FilterPortName;
	RtlInitUnicodeString(&FilterPortName, FILTER_PORT_NAME);
	Status = CreateFilterCommunicationPort(
		&g_FilterData.ServerPort, 
		g_FilterData.Filter,
		&FilterPortName
	);
	if (!NT_SUCCESS(Status))
	{
		FltUnregisterFilter(g_FilterData.Filter);
		return Status;
	}
	KdPrint(("Communication port opened\n"));


	// Init ecnryption context list
	InitializeListHead(&g_EncrCtxList.ListEntry);

	// Start filtering
	Status = FltStartFiltering(g_FilterData.Filter);
	if (!NT_SUCCESS(Status))
	{
		FltUnregisterFilter(g_FilterData.Filter);
		return Status;
	}
	KdPrint(("Started filtering\n"));
	KdPrint(("---  FILTER INITIALIZING END  ---\n\n"));

	///////////////////// DEBUG DELETE ////////////////////////////////////////
	DECLARE_CONST_UNICODE_STRING(DrivePath, L"Q:");
	PFLT_VOLUME Volume{};
	Status = FltGetVolumeFromName(
		g_FilterData.Filter,
		&DrivePath,
		&Volume);
	uint8_t Key[sizeof(ENCRYPTION_CONTEXT::Key)];
	memset(Key, 1, sizeof(Key));
	Status = AddEncryptionContext(Volume, Key);
	if (!NT_SUCCESS(Status))
	{
		FltUnregisterFilter(g_FilterData.Filter);
		return Status;
	}
	///////////////////////////////////////////////////////////////////////////

	return Status;
}

void DriverUnload(
	_DRIVER_OBJECT* DriverObject
)
{
	KdPrint(("DRIVER UNLOADED\n"));
}

NTSTATUS FilterUnloadCallback(FLT_FILTER_UNLOAD_FLAGS Flags)
{
	KdPrint(("\n--- FILTER UNLOADING BEGIN ---\n"));

	if (g_FilterData.ServerPort)
	{
		FltCloseCommunicationPort(g_FilterData.ServerPort);
		KdPrint(("Communication port closed\n"));
	}

	while (!IsListEmpty(&g_EncrCtxList.ListEntry))
	{
		RemoveEncryptionContext(
			(PENCRYPTION_CONTEXT)CONTAINING_RECORD(g_EncrCtxList.ListEntry.Flink, ENCRYPTION_CONTEXT, ListEntry)
		);
	}

	if (g_FilterData.Filter)
	{
		FltUnregisterFilter(g_FilterData.Filter);
		KdPrint(("Filter unregistered\n"));
	}
	
	KdPrint(("---  FILTER UNLOADING END  ---\n"));
	
	return STATUS_SUCCESS;
}

NTSTATUS FLTAPI InstanceSetupCallback(
	IN PCFLT_RELATED_OBJECTS  FltObjects,
	IN FLT_INSTANCE_SETUP_FLAGS  Flags,
	IN DEVICE_TYPE  VolumeDeviceType,
	IN FLT_FILESYSTEM_TYPE  VolumeFilesystemType)
{
	PENCRYPTION_CONTEXT EncryptionContext{};
	if (NT_SUCCESS(GetEncryptionContext(FltObjects->Volume, EncryptionContext)))
	{
		KdPrint(("Attached at volume [%X]\n", FltObjects->Volume));
		return STATUS_SUCCESS;
	}

	return STATUS_FLT_DO_NOT_ATTACH;
}

NTSTATUS FLTAPI InstanceQueryTeardownCallback(
	IN PCFLT_RELATED_OBJECTS FltObjects,
	IN FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
{
	KdPrint(("FLT INSTANCE TEARDOWN\n"));
	return STATUS_SUCCESS;
}