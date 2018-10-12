//*******************************************************************************
//
//	Developer : Amul Patel
//
//	USE : This mini-filter is used for device control, means 
//	this driver provides block particular devices, or
//	all devices.
//
//	How to Install mini filter : Just right click to .inf file, Click Install.
//	After that open cmd with Administrator privilleges. Type following command
//	net start <ServiceName> // For starting device control mini filter.
//	net stop <ServiceName>  // For stoping device control mini filter.
//	sc delete <ServiceName> // For removing minifilter entry from system.
//
//	dcminifilter.c	9/11/2018
//
//*******************************************************************************

#include "dcminifilter.h"


NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT pDriverObj,
	__in PUNICODE_STRING pwszRegistryPath
	)
{
	NTSTATUS iNTStatus;

	const FLT_OPERATION_REGISTRATION Callbacks[] = {
		{IRP_MJ_CREATE, 0, DevicePreCreate, NULL},
		{IRP_MJ_OPERATION_END}
	};

	const FLT_CONTEXT_REGISTRATION contextRegistration[] = {
		{FLT_INSTANCE_CONTEXT, 0, DeviceContextCleanup, DEVICE_INSTANCE_CONTEXT_SIZE, 'cIxC'},
		{FLT_CONTEXT_END}
	};

	const FLT_REGISTRATION FilterRegistration = {
		sizeof(FLT_REGISTRATION),
		FLT_REGISTRATION_VERSION,
		0,
		contextRegistration,
		Callbacks,
		DeviceFltUnload,
		DeviceInstanceSetup,
		NULL,
		NULL,
		DeviceInstanceTeardownComplete,
		NULL,
		NULL,
		NULL
	};

	DbgPrint("DCMiniFilter : Enter In DriverEntry.\n");

	iNTStatus = FltRegisterFilter(pDriverObj, &FilterRegistration, &Filter);
	if (!NT_SUCCESS(iNTStatus))
	{
		DbgPrint("DCMiniFilter : Failed to register filter.\n");
		return iNTStatus;
	}

	iNTStatus = FltStartFiltering(Filter);
	if (NT_SUCCESS(iNTStatus))
	{
		DbgPrint("DCMiniFilter : Filtering started.\n");
		return STATUS_SUCCESS;
	}

	FltUnregisterFilter(Filter);

	DbgPrint("DCMiniFilter : Exit Driver Entry.\n");
	return iNTStatus;
}


NTSTATUS
DeviceFltUnload(
	__in FLT_FILTER_UNLOAD_FLAGS ulFltUnloadFlag
	)
{
	DbgPrint("DCMiniFilter : Enter UloadFilter.\n");

	UNREFERENCED_PARAMETER(ulFltUnloadFlag);
	PAGED_CODE();

	FltUnregisterFilter(Filter);

	DbgPrint("DCMiniFilter : Exit UloadFilter.\n");
	return STATUS_SUCCESS;
}


int
VolumeToDosName(
	__in PFLT_VOLUME pFV
	)
{
	int iReturn = 0;
	NTSTATUS iNTStatus;
	UNICODE_STRING usDosName;
	PDEVICE_OBJECT pDO = NULL;

	iNTStatus = FltGetDiskDeviceObject(pFV, &pDO);
	if (NT_SUCCESS(iNTStatus))
	{
		iNTStatus = IoVolumeDeviceToDosName(pDO, &usDosName);
		if (NT_SUCCESS(iNTStatus))
		{
			iReturn = RtlUpcaseUnicodeChar(usDosName.Buffer[0]);
			DbgPrint("DCMiniFilter : Device Name - %wZ\n", usDosName);

			ExFreePool(usDosName.Buffer);
		}
	}

	return iReturn;
}

NTSTATUS
DeviceInstanceSetup(
	__in PCFLT_RELATED_OBJECTS pFltRelatedObj,
	__in FLT_INSTANCE_SETUP_FLAGS ulFltInstanceSetupFlag,
	__in DEVICE_TYPE iDeviceType,
	__in FLT_FILESYSTEM_TYPE ulFltFileSysType
	)
{
	int iIter;
	int iDrive;
	NTSTATUS iNTStatus;
	__int64 iSystemTime;
	ULONG ulLengthReceived;
	NTSTATUS iNTStatusReturn;
	LARGE_INTEGER liSystemTime;
	PDEVICE_INSTANCE_CONTEXT pAIC = NULL;
	UCHAR ucBuffer[sizeof(FLT_VOLUME_PROPERTIES) + 512];
	PFLT_VOLUME_PROPERTIES pFVP = (PFLT_VOLUME_PROPERTIES)ucBuffer;

	UNREFERENCED_PARAMETER(ulFltInstanceSetupFlag);
	UNREFERENCED_PARAMETER(ulFltFileSysType);

	ulLengthReceived = 0;
	PAGED_CODE();

	DbgPrint("DCMiniFilter : Enter pasthroughInstanceSetup.\n");

	iDrive = VolumeToDosName(pFltRelatedObj->Volume);

	iNTStatus = FltAllocateContext(pFltRelatedObj->Filter, FLT_INSTANCE_CONTEXT, DEVICE_INSTANCE_CONTEXT_SIZE, NonPagedPool, &pAIC);
	if (!NT_SUCCESS(iNTStatus))
	{
		if (pAIC != NULL)
		{
			FltReleaseContext(pAIC);
		}

		DbgPrint("DCMiniFilter : Failed to allocate context memory.\n");
		return STATUS_FLT_DO_NOT_ATTACH;
	}

	pAIC->ucFlags = FALSE;

	iNTStatus = FltGetVolumeProperties(
									pFltRelatedObj->Volume,
									pFVP,
									sizeof(ucBuffer),
									&ulLengthReceived
									);
	if (FILE_DEVICE_DISK_FILE_SYSTEM == iDeviceType)
	{
		if (NT_SUCCESS(iNTStatus))
		{
			DbgPrint(
					"DCMiniFilter : RealDeviceName: %wZ DeviceCharacteristics: %08x\n",
					&pFVP->RealDeviceName,
					pFVP->DeviceCharacteristics
					);

			if (pFVP->DeviceCharacteristics & FILE_REMOVABLE_MEDIA)
			{
				pAIC->ucFlags = TRUE;
				iNTStatusReturn = STATUS_SUCCESS;

				DbgPrint("DCMiniFilter : Removable media found.\n");
			}
		}
	}
	else if (FILE_DEVICE_CD_ROM_FILE_SYSTEM == iDeviceType)
	{
		if (NT_SUCCESS(iNTStatus))
		{
			DbgPrint(
					"DCMiniFilter : RealDeviceName: %wZ DeviceCharacteristics: %08x\n",
					&pFVP->RealDeviceName,
					pFVP->DeviceCharacteristics
					);

			pAIC->ucFlags = TRUE;
			iNTStatusReturn = STATUS_SUCCESS;

			DbgPrint("DCMiniFilter : CDROM drive Detected.\n");
		}
	}
	else
	{
		iNTStatusReturn = STATUS_FLT_DO_NOT_ATTACH;
	}

	iNTStatus = FltSetInstanceContext(
									pFltRelatedObj->Instance,
									FLT_SET_CONTEXT_KEEP_IF_EXISTS,
									pAIC,
									NULL
									);
	if (!NT_SUCCESS(iNTStatus))
	{
		if (pAIC != NULL)
		{
			FltReleaseContext(pAIC);
		}

		DbgPrint("DCMiniFilter : Failed to set instance to filter.\n");
		return STATUS_FLT_DO_NOT_ATTACH;
	}

	if (pAIC != NULL)
	{
		FltReleaseContext(pAIC);
	}

	if (NT_SUCCESS(iNTStatusReturn))
	{
		if (ulLengthReceived <= 0)
		{
			DbgPrint(
				"DCMiniFilter : iDT - %d RealDeviceName: %wZ DeviceCharacteristics - %08x Dosname - %c\n",
				iDeviceType,
				&pFVP->RealDeviceName,
				pFVP->DeviceCharacteristics,
				iDrive
				);
		}
	}

	DbgPrint("DCMiniFilter : Exit pasthroughInstanceSetup.\n");
	return iNTStatusReturn;
}


FLT_PREOP_CALLBACK_STATUS
DevicePreCreate(
	__inout PFLT_CALLBACK_DATA pFltCallbackData,
	__in PCFLT_RELATED_OBJECTS pcFltRelatedObjs,
	__deref_out_opt PVOID *ppvCompletionContext
	)
{
	NTSTATUS iNTStatus;
	BOOLEAN bMatchFound;
	BOOLEAN ucFlags = FALSE;
	PFLT_FILE_NAME_INFORMATION pFFNI;
	PDEVICE_INSTANCE_CONTEXT pAIC = NULL;

	UNREFERENCED_PARAMETER(pcFltRelatedObjs);
	UNREFERENCED_PARAMETER(ppvCompletionContext);

	PAGED_CODE();

	DbgPrint("DCMiniFilter : Enter pasthroughPreCreate.\n");

	iNTStatus = FltGetInstanceContext(pFltCallbackData->Iopb->TargetInstance, &pAIC);
	if (!NT_SUCCESS(iNTStatus))
	{
		DbgPrint("DCMiniFilter : Failed to get device instance context.\n");
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	ucFlags = pAIC->ucFlags;

	if (pAIC != NULL)
	{
		FltReleaseContext(pAIC);
	}

	if (!ucFlags)
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	else
	{
		pFltCallbackData->IoStatus.Status = STATUS_ACCESS_DENIED;
		pFltCallbackData->IoStatus.Information = 0;
		
		DbgPrint("DCMiniFilter : Access denied\n");
		return FLT_PREOP_COMPLETE;
	}

	DbgPrint("DCMiniFilter : Exit pasthroughPreCreate.\n");
}


VOID
DeviceContextCleanup(
	__in PFLT_CONTEXT pFltContext,
	__in FLT_CONTEXT_TYPE pFltContextType
	)
{
	PDEVICE_INSTANCE_CONTEXT pAIC;

	PAGED_CODE();

	DbgPrint("DCMiniFilter : Enter pasthroughContextCleanup.\n");

	if (FLT_INSTANCE_CONTEXT == pFltContextType)
	{
		pAIC = (PDEVICE_INSTANCE_CONTEXT) pFltContext;
	}

	DbgPrint("DCMiniFilter : Exit pasthroughContextCleanup.\n");
}

VOID
DeviceInstanceTeardownComplete(
	__in PCFLT_RELATED_OBJECTS pFltRelatedObjs,
	__in FLT_INSTANCE_TEARDOWN_FLAGS ulFltInstanceTearDownFlag
	)
{
	NTSTATUS iNTStatus;
	PDEVICE_INSTANCE_CONTEXT pAIC;

	UNREFERENCED_PARAMETER(ulFltInstanceTearDownFlag);

	PAGED_CODE();

	DbgPrint("DCMiniFilter : Enter pasthroughInstanceTeardownComplete.\n");

	iNTStatus = FltGetInstanceContext(pFltRelatedObjs->Instance, &pAIC);
	if (NT_SUCCESS(iNTStatus))
	{
		FltReleaseContext(pAIC);
	}

	DbgPrint("DCMiniFilter : Exit pasthroughInstanceTeardownComplete.\n");
}
