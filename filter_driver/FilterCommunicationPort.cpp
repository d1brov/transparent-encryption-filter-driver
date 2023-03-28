#include "pch.h"

NTSTATUS CreateFilterCommunicationPort(
	PFLT_PORT*      ServerPort, 
	PFLT_FILTER     Filter, 
	PUNICODE_STRING CommunicationPortName)
{
	KdPrint((
		"Creating filter communication port [%wZ]...", 
		*CommunicationPortName)
	);

	PSECURITY_DESCRIPTOR SecurityDescriptor;
	NTSTATUS status = FltBuildDefaultSecurityDescriptor(
		&SecurityDescriptor, 
		FLT_PORT_ALL_ACCESS
	);

	if (!NT_SUCCESS(status))
	{
		KdPrint(("FAILED to build security descriptor\n"));
		return status;
	}

	OBJECT_ATTRIBUTES ObjectAttributes;
	InitializeObjectAttributes(
		&ObjectAttributes,
		CommunicationPortName,
		OBJ_EXCLUSIVE | OBJ_KERNEL_HANDLE,
		NULL,
		SecurityDescriptor
	);

	status = FltCreateCommunicationPort(
		Filter,
		ServerPort,
		&ObjectAttributes,
		NULL,
		ConnectNotifyCallback,
		DisconnectNotifyCallback,
		MessageNotifyCallback,
		1
	);
	FltFreeSecurityDescriptor(SecurityDescriptor);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("FAILED\n"));
		return status;
	}
	KdPrint(("DONE\n"));

	return status;
}

NTSTATUS ConnectNotifyCallback(
	PFLT_PORT ClientPort,
	PVOID ServerPortCookie,
	PVOID ConnectionContext,
	ULONG SizeOfContext,
	PVOID* ConnectionCookie
)
{
	KdPrint(("\n--- COMMUNICATION SESSION BEGIN ---\n"));
	return STATUS_SUCCESS;
}

VOID DisconnectNotifyCallback(
	PVOID ConnectionCookie
)
{
	KdPrint(("---  COMMUNICATION SESSION END  ---\n"));
}

NTSTATUS MessageNotifyCallback(
	PVOID PortCookie,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength,
	PULONG ReturnOutputBufferLength
)
{
	KdPrint(("\nRECEIVED COMMAND ["));

	NTSTATUS CommandExecStatus = STATUS_INVALID_PARAMETER;
	if ((InputBuffer != NULL) &&
		(InputBufferLength >= sizeof(BaseCommandMessage)))
	{
		FILTER_COMMAND Command = ((BaseCommandMessage*)InputBuffer)->m_command;
		switch (Command)
		{
		case MOUNT:
			if (InputBufferLength == sizeof(MountCommandMessage))
			{
				KdPrint((
					"MOUNT DRIVE %C:\\]\n", 
					((MountCommandMessage*)InputBuffer)->m_driveLetter)
				);
				CommandExecStatus = MountRoutine((MountCommandMessage*)InputBuffer);
			}
		break;

		case UNMOUNT:
			if (InputBufferLength == sizeof(UnmountCommandMessage))
			{
				KdPrint((
					"UNMOUNT DRIVE %C:\\]\n", 
					((UnmountCommandMessage*)InputBuffer)->m_driveLetter)
				);
				CommandExecStatus = UnmountRoutine((UnmountCommandMessage*)InputBuffer);
			}
		break;

		default:
			KdPrint(("UNRECOGNIZED COMMAND]\n"));
			break;
		}
	}

	KdPrint(("COMMAND EXECUTION STATUS: %X\n", CommandExecStatus));

	*ReturnOutputBufferLength = sizeof(NTSTATUS);
	memcpy_s(
		OutputBuffer, 
		*ReturnOutputBufferLength, 
		&CommandExecStatus, 
		*ReturnOutputBufferLength
	);

	return STATUS_SUCCESS;
}