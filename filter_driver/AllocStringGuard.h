#pragma once

/*
	RAII guard class for correct allocating and deallocating pool memory
	for strings
*/
class AllocStringGuard
{
public:
	AllocStringGuard() = delete;
	AllocStringGuard(const AllocStringGuard&) = delete;
	AllocStringGuard& operator=(const AllocStringGuard&) = delete;
	AllocStringGuard& operator=(AllocStringGuard&&) = delete;

	AllocStringGuard(SIZE_T BytesToAllocate);
	~AllocStringGuard();
	PUNICODE_STRING GetString();

private:
	UNICODE_STRING m_String;
};