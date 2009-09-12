#ifndef HID_DEVICE_H
#define HID_DEVICE_H

int InitHid();

typedef USHORT USAGE;
struct HIDP_CAPS {
    USAGE    Usage;
    USAGE    UsagePage;
    USHORT   InputReportByteLength;
    USHORT   OutputReportByteLength;
    USHORT   FeatureReportByteLength;
    USHORT   Reserved[17];

    USHORT   NumberLinkCollectionNodes;

    USHORT   NumberInputButtonCaps;
    USHORT   NumberInputValueCaps;
    USHORT   NumberInputDataIndices;

    USHORT   NumberOutputButtonCaps;
    USHORT   NumberOutputValueCaps;
    USHORT   NumberOutputDataIndices;

    USHORT   NumberFeatureButtonCaps;
    USHORT   NumberFeatureValueCaps;
    USHORT   NumberFeatureDataIndices;
};

struct HidDeviceInfo {
	HIDP_CAPS caps;
	wchar_t *path;
	unsigned short vid;
	unsigned short pid;
};

void UninitHid();
int FindHids(HidDeviceInfo **foundDevs, int vid, int pid);

#endif
