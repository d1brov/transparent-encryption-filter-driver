#include "pch.h"

AllocStringGuard::AllocStringGuard(SIZE_T BytesToAllocate)
{
	m_String.Buffer = (PWCH)ExAllocatePool2(
		POOL_FLAG_PAGED,
		BytesToAllocate,
		STR_ALLOCATION_TAG
	);
	if (BytesToAllocate > MAXUSHORT || m_String.Buffer == NULL)
	{
		return;
	}

	m_String.MaximumLength = (USHORT)BytesToAllocate;
	m_String.Length = 0;
}

AllocStringGuard::~AllocStringGuard()
{
	RtlFreeUnicodeString(&m_String);
}

PUNICODE_STRING AllocStringGuard::GetString()
{
	return &m_String;
}