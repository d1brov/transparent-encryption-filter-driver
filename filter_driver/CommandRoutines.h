#pragma once

/*
	Callback routine for handling MOUNT command
*/
NTSTATUS MountRoutine(
	IN const MountCommandMessage* MountMsg
);

/*
	Callback routine for handling UNMOUNT command
*/
NTSTATUS UnmountRoutine(
	IN const UnmountCommandMessage* UnmountMsg
);

/*
	Gets volume handle on specified drive by its letter
*/
NTSTATUS GetVolumeByLetter(
	OUT PFLT_VOLUME* Volume,
	IN  const WCHAR DriveLetter
);