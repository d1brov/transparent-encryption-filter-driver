#include "pch.h"

FltHandleGuard::~FltHandleGuard()
{
	if (!NT_SUCCESS(FltClose(m_Handle)))
	{
		KdPrint(("Invalid handle provided\n"));
	}
}

PHANDLE FltHandleGuard::getHandle()
{
	return &m_Handle;
}