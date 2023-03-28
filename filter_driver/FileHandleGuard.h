#pragma once

/*
	RAII guard for HANDLE that was opened by FltCreateFile or FltCreateFileEx.
*/
class FltHandleGuard
{
public:
	FltHandleGuard(const FltHandleGuard&) = delete;
	FltHandleGuard(FltHandleGuard&&) = delete;
	FltHandleGuard& operator=(const FltHandleGuard&) = delete;
	FltHandleGuard& operator=(FltHandleGuard&&) = delete;

	FltHandleGuard() = default;
	~FltHandleGuard();
	PHANDLE getHandle();

private:
	HANDLE m_Handle;
};