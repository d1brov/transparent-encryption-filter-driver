#pragma once

NTSTATUS CreateFilterCommunicationPort(
    PFLT_PORT*      ServerPort,
    PFLT_FILTER     Filter,
    PUNICODE_STRING CommunicationPortName
);

NTSTATUS ConnectNotifyCallback(
    PFLT_PORT ClientPort,
    PVOID     ServerPortCookie,
    PVOID     ConnectionContext,
    ULONG     SizeOfContext,
    PVOID*    ConnectionCookie
);

VOID DisconnectNotifyCallback(
    PVOID ConnectionCookie
);

NTSTATUS MessageNotifyCallback(
    PVOID  PortCookie,
    PVOID  InputBuffer,
    ULONG  InputBufferLength,
    PVOID  OutputBuffer,
    ULONG  OutputBufferLength,
    PULONG ReturnOutputBufferLength
);