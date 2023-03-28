#pragma once

/*
	Serializable structure that hold data for storing in metadata file
*/
typedef struct 
{
	uint8_t Salt[SHA256_BLOCK_SIZE / 2];
	uint8_t SaltedPasswordHash[SHA256_BLOCK_SIZE];
} DriveMetadata;

NTSTATUS InitDriveMetadata(
	OUT DriveMetadata* DriveData,
	IN const uint8_t hash[SHA256_BLOCK_SIZE]
);

NTSTATUS ValidatePasswordHash(
	IN const DriveMetadata* DriveData,
	IN const uint8_t hash[SHA256_BLOCK_SIZE]
);