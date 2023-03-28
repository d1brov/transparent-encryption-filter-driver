#pragma once

namespace CryptUtils
{
	void AesCtrProcessBuffer(
		IN  AesCtrContext* AesCtr,
		OUT PVOID Buffer,
		IN  ULONG Size,
		IN  ULONG ByteOffset = 0
	);

	void GenerateRandomBytes(
		OUT PVOID Buffer,
		IN  ULONG Size
	);
}