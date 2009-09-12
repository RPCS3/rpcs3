#include "Global.h"
#include "HidDevice.h"
#include <setupapi.h>

// Tons of annoying junk to avoid the hid dependencies, so no one has to download the
// DDK to compile.
#define  HIDP_STATUS_SUCCESS 0x110000

struct HIDD_ATTRIBUTES {
    ULONG   Size;

    USHORT  VendorID;
    USHORT  ProductID;
    USHORT  VersionNumber;
};
struct HIDP_PREPARSED_DATA;

typedef BOOLEAN (__stdcall *_HidD_GetAttributes) (HANDLE HidDeviceObject, HIDD_ATTRIBUTES *Attributes);
typedef void (__stdcall *_HidD_GetHidGuid) (GUID* HidGuid);
typedef BOOLEAN (__stdcall *_HidD_GetPreparsedData) (HANDLE HidDeviceObject, HIDP_PREPARSED_DATA **PreparsedData);
typedef NTSTATUS (__stdcall *_HidP_GetCaps) (HIDP_PREPARSED_DATA* PreparsedData, HIDP_CAPS *caps);
typedef BOOLEAN (__stdcall *_HidD_FreePreparsedData) (HIDP_PREPARSED_DATA *PreparsedData);
typedef BOOLEAN (__stdcall *_HidD_GetFeature) (HANDLE  HidDeviceObject, PVOID  ReportBuffer, ULONG  ReportBufferLength);
typedef BOOLEAN (__stdcall *_HidD_SetFeature) (HANDLE  HidDeviceObject, PVOID  ReportBuffer, ULONG  ReportBufferLength);

GUID GUID_DEVINTERFACE_HID;
_HidD_GetHidGuid pHidD_GetHidGuid;
_HidD_GetAttributes pHidD_GetAttributes;
_HidD_GetPreparsedData pHidD_GetPreparsedData;
_HidP_GetCaps pHidP_GetCaps;
_HidD_FreePreparsedData pHidD_FreePreparsedData;
_HidD_GetFeature pHidD_GetFeature;
_HidD_SetFeature pHidD_SetFeature;

HMODULE hModHid = 0;

int InitHid() {
	if (hModHid) {
		return 1;
	}
	hModHid = LoadLibraryA("hid.dll");
	if (hModHid) {
		if ((pHidD_GetHidGuid = (_HidD_GetHidGuid) GetProcAddress(hModHid, "HidD_GetHidGuid")) &&
			(pHidD_GetAttributes = (_HidD_GetAttributes) GetProcAddress(hModHid, "HidD_GetAttributes")) &&
			(pHidD_GetPreparsedData = (_HidD_GetPreparsedData) GetProcAddress(hModHid, "HidD_GetPreparsedData")) &&
			(pHidP_GetCaps = (_HidP_GetCaps) GetProcAddress(hModHid, "HidP_GetCaps")) &&
			(pHidD_FreePreparsedData = (_HidD_FreePreparsedData) GetProcAddress(hModHid, "HidD_FreePreparsedData")) &&
			(pHidD_GetFeature = (_HidD_GetFeature) GetProcAddress(hModHid, "HidD_GetFeature")) &&
			(pHidD_SetFeature = (_HidD_SetFeature) GetProcAddress(hModHid, "HidD_SetFeature"))) {
				pHidD_GetHidGuid(&GUID_DEVINTERFACE_HID);
				return 1;
		}
		UninitHid();
	}
	return 0;
}

int FindHids(HidDeviceInfo **foundDevs, int vid, int pid) {
	if (!InitHid()) return 0;
	int numFoundDevs = 0;
	*foundDevs = 0;
	HDEVINFO hdev = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_HID, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (hdev != INVALID_HANDLE_VALUE) {
		SP_DEVICE_INTERFACE_DATA devInterfaceData;
		devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		for (int i=0; SetupDiEnumDeviceInterfaces(hdev, 0, &GUID_DEVINTERFACE_HID, i, &devInterfaceData); i++) {

			DWORD size = 0;
			SetupDiGetDeviceInterfaceDetail(hdev, &devInterfaceData, 0, 0, &size, 0);
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || !size) continue;
			SP_DEVICE_INTERFACE_DETAIL_DATA *devInterfaceDetails = (SP_DEVICE_INTERFACE_DETAIL_DATA *) malloc(size);
			if (!devInterfaceDetails) continue;

			devInterfaceDetails->cbSize = sizeof(*devInterfaceDetails);
			SP_DEVINFO_DATA devInfoData;
			devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

			if (!SetupDiGetDeviceInterfaceDetail(hdev, &devInterfaceData, devInterfaceDetails, size, &size, &devInfoData)) continue;

			HANDLE hfile = CreateFileW(devInterfaceDetails->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
			if (hfile != INVALID_HANDLE_VALUE) {
				HIDD_ATTRIBUTES attributes;
				attributes.Size = sizeof(attributes);
				if (pHidD_GetAttributes(hfile, &attributes)) {
					if (attributes.VendorID == vid && attributes.ProductID == pid) {
						HIDP_PREPARSED_DATA *pData;
						HIDP_CAPS caps;
						if (pHidD_GetPreparsedData(hfile, &pData)) {
							if (HIDP_STATUS_SUCCESS == pHidP_GetCaps(pData, &caps)) {
								if (numFoundDevs % 32 == 0) {
									*foundDevs = (HidDeviceInfo*) realloc(*foundDevs, sizeof(HidDeviceInfo) * (32 + numFoundDevs));
								}
								HidDeviceInfo *dev = &foundDevs[0][numFoundDevs++];
								dev->caps = caps;
								dev->vid = attributes.VendorID;
								dev->pid = attributes.ProductID;
								dev->path = wcsdup(devInterfaceDetails->DevicePath);
							}
							pHidD_FreePreparsedData(pData);
						}
					}
				}
				CloseHandle(hfile);
			}
			free(devInterfaceDetails);
		}
		SetupDiDestroyDeviceInfoList(hdev);
	}
	return numFoundDevs;
}

void UninitHid() {
	if (hModHid) {
		FreeLibrary(hModHid);
		hModHid = 0;
	}
}
