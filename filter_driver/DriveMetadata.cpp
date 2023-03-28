#include "pch.h"

NTSTATUS InitDriveMetadata(
	OUT DriveMetadata* DriveData,
	IN const uint8_t hash[SHA256_BLOCK_SIZE]
)
{
	KdPrint(("Initializing disk metadata..."));
	// generate random Salt
	CryptUtils::GenerateRandomBytes(&DriveData->Salt, sizeof(DriveMetadata::Salt));

	// concatenate received pass_hash with salt
	uint8_t PassHashSalt[sizeof(hash) + sizeof(DriveMetadata::Salt)]{};
	memcpy(&PassHashSalt, hash, sizeof(hash));
	memcpy(
		&(PassHashSalt[sizeof(hash)]), 
		DriveData->Salt, 
		sizeof(DriveMetadata::Salt)
	);

	// hash(password_hash+salt)
	SHA256_CTX Sha256Ctx{};
	sha256_init(&Sha256Ctx);
	sha256_update(&Sha256Ctx, PassHashSalt, sizeof(PassHashSalt));
	sha256_final(&Sha256Ctx, DriveData->SaltedPasswordHash);

	KdPrint(("DONE\n"));
	return STATUS_SUCCESS;
}

NTSTATUS ValidatePasswordHash(
	IN const DriveMetadata* DriveData,
	IN const uint8_t PasswordHash[SHA256_BLOCK_SIZE]
)
{
	KdPrint(("Validating password..."));
	// concatenate received pass_hash with salt
	uint8_t PassHashSalt[sizeof(PasswordHash) + sizeof(DriveMetadata::Salt)]{};
	memcpy(&PassHashSalt, PasswordHash, sizeof(PasswordHash));
	memcpy(
		&(PassHashSalt[sizeof(PasswordHash)]),
		DriveData->Salt,
		sizeof(DriveMetadata::Salt)
	);

	// hash(password_hash+salt)
	SHA256_CTX Sha256Ctx{};
	uint8_t SaltedPasswodHash[sizeof(DriveMetadata::SaltedPasswordHash)]{};
	sha256_init(&Sha256Ctx);
	sha256_update(&Sha256Ctx, PassHashSalt, sizeof(PassHashSalt));
	sha256_final(&Sha256Ctx, SaltedPasswodHash);

	if (memcmp(
		DriveData->SaltedPasswordHash,
		SaltedPasswodHash,
		sizeof(DriveMetadata::SaltedPasswordHash)) == 0)
	{
		KdPrint(("SUCCESS\n"));
		return STATUS_SUCCESS;
	}
	else
	{
		KdPrint(("PASSWORD INVALID\n"));
		return STATUS_WRONG_PASSWORD;
	}
}