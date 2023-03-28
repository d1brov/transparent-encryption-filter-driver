#pragma once

#include <Fltkernel.h> // Filter driver operation
#include <ntstrsafe.h> // UNICODE_STRING operations

#include "FilterDriver.h"

// Cryptography
#include "sha256.h"
#include "WjCryptLib_AesCtr.h"
#include "CryptUtils.h"

// Helper structures and classes
#include "Message.h"
#include "EncryptionContext.h"	
#include "DriveMetadata.h"
#include "AllocStringGuard.h"
#include "FileHandleGuard.h"

// Native filter operations and callbacks
#include "FilterOperationsCallbacks.h"
#include "FilterCommunicationPort.h"

#include "CommandRoutines.h"
#include "DriveMetadataFile.h"