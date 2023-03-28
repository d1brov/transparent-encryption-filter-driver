#pragma once

typedef struct {
	LIST_ENTRY ListEntry;

	PFLT_VOLUME Volume;
	PFLT_INSTANCE VolumeFilter;
	uint8_t     Key[AES_KEY_SIZE_256];
	uint8_t		IV [AES_CTR_IV_SIZE];
} ENCRYPTION_CONTEXT, *PENCRYPTION_CONTEXT;

extern ENCRYPTION_CONTEXT g_EncrCtxList;

NTSTATUS AddEncryptionContext(
	IN PFLT_VOLUME Volume,
	IN const uint8_t Key[AES_KEY_SIZE_256]
);

NTSTATUS GetEncryptionContext(
	IN PFLT_VOLUME Volume,
	OUT PENCRYPTION_CONTEXT &EncryptionContext
);

void RemoveEncryptionContext(
	IN PENCRYPTION_CONTEXT EncryptionContext
);