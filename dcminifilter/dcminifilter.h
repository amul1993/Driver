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
//	dcminifilter.h	9/11/2018
//
//*******************************************************************************


#ifndef DRIVER_H_
#define	DRIVER_H_

//
//	HEADER FILES
//
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>


//
//	MACROS AND PRAGMAS
//
#define DEVICE_INSTANCE_CONTEXT_SIZE sizeof(DEVICE_INSTANCE_CONTEXT)


//
//	When filter registration is occured then it contain filter object
//	and unregister filter at unloading.
//
PFLT_FILTER Filter;


//
//	STRUCTURES
//
typedef struct _DEVICE_INSTANCE_CONTEXT{
	BOOLEAN ucFlags;
} DEVICE_INSTANCE_CONTEXT, *PDEVICE_INSTANCE_CONTEXT;


//
//	FUNCTIONS DECLARATION
//
NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT DriverObject,
	__in PUNICODE_STRING RegistryPath
	);

NTSTATUS
DeviceFltUnload(
	__in FLT_FILTER_UNLOAD_FLAGS Flags
	);

FLT_PREOP_CALLBACK_STATUS
DevicePreCreate(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__deref_out_opt PVOID *CompletionContext
	);

NTSTATUS
DeviceInstanceSetup(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_SETUP_FLAGS Flags,
	__in DEVICE_TYPE VolumeDeviceType,
	__in FLT_FILESYSTEM_TYPE VolumeFilesystemType
	);

VOID
DeviceContextCleanup(
	__in PFLT_CONTEXT Context,
	__in FLT_CONTEXT_TYPE ContextType
	);

VOID
DeviceInstanceTeardownComplete(
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in FLT_INSTANCE_TEARDOWN_FLAGS Flags
	);


#endif // DRIVER_H_
