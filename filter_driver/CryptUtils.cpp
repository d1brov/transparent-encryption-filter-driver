#include "pch.h"

void CryptUtils::AesCtrProcessBuffer(
	IN AesCtrContext* AesCtr, 
	OUT PVOID Buffer, 
	IN ULONG Size, 
	IN ULONG ByteOffset
)
{
	KdPrint(("------AES-256 CTR BEGIN------\n"));
	KdPrint(("size:  %d\n", Size));
	KdPrint(("offset:%d\n", ByteOffset));
	

	KdPrint(("ORIG  :["));
	for (uint32_t i = 0; i < AES_BLOCK_SIZE; i++)
		KdPrint(("%c", ((char*)(Buffer))[i]));
	KdPrint(("...]\n"));

	//AesCtrSetStreamIndex(AesCtr, ByteOffset);
	//AesCtrXor(AesCtr, Buffer, Buffer, Size);

	KdPrint(("KRYPT :["));
	for (uint32_t i = 0; i < AES_BLOCK_SIZE; i++)
		KdPrint(("%c", ((char*)(Buffer))[i]));
	KdPrint(("...]\n"));


	KdPrint(("-------AES-256 CTR END-------\n"));
}

void CryptUtils::GenerateRandomBytes(OUT PVOID Buffer, IN ULONG Size)
{
	ULONG Seed;
	LARGE_INTEGER SystenTime{};
	KeQuerySystemTime(&SystenTime);
	Seed = SystenTime.HighPart;
	ULONG RandomUlong = RtlRandomEx(&Seed);

	uint8_t IV[AES_CTR_IV_SIZE]{};
	for (uint8_t i = 0; i < AES_CTR_IV_SIZE && i < sizeof(ULONG); i++)
		IV[i] = (RandomUlong >> (i * 8)) & 0xFF;

	uint8_t Key[AES_KEY_SIZE_256]{};
	SHA256_CTX Sha256Ctx{};
	sha256_init(&Sha256Ctx);
	sha256_update(&Sha256Ctx, IV, sizeof(IV));
	sha256_final(&Sha256Ctx, Key);

	AesCtrXorWithKey(Key, sizeof(Key), IV, Buffer, Buffer, Size);
}