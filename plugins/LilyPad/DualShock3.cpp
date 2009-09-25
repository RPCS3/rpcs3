#include "Global.h"
#include "InputManager.h"

#include "usb.h"
#include "HidDevice.h"


#define VID 0x054c
#define PID 0x0268

// Unresponsive period required before calling DS3Check().
#define DEVICE_CHECK_DELAY 2000
// Unresponsive period required before calling DS3Enum().  Note that enum is always called on first check.
#define DEVICE_ENUM_DELAY 10000

// Delay between when DS3Check() and DS3Enum() actually do stuff.
#define DOUBLE_CHECK_DELAY 1000
#define DOUBLE_ENUM_DELAY 20000

// Send at least one message every 3 seconds - basically just makes sure the right light(s) are on.
// Not really necessary.
#define UPDATE_INTERVAL 3000

unsigned int lastDS3Check = 0;
unsigned int lastDS3Enum = 0;

typedef void (__cdecl *_usb_init)(void);
typedef int (__cdecl *_usb_close)(usb_dev_handle *dev);
typedef int (__cdecl *_usb_get_string_simple)(usb_dev_handle *dev, int index, char *buf, size_t buflen);
typedef usb_dev_handle *(__cdecl *_usb_open)(struct usb_device *dev);
typedef int (__cdecl *_usb_find_busses)(void);
typedef int (__cdecl *_usb_find_devices)(void);
typedef struct usb_bus *(__cdecl *_usb_get_busses)(void);
typedef usb_dev_handle *(__cdecl *_usb_open)(struct usb_device *dev);
typedef int (__cdecl *_usb_control_msg)(usb_dev_handle *dev, int requesttype, int request, int value, int index, char *bytes, int size, int timeout);

_usb_init pusb_init;
_usb_close pusb_close;
_usb_get_string_simple pusb_get_string_simple;
_usb_open pusb_open;
_usb_find_busses pusb_find_busses;
_usb_find_devices pusb_find_devices;
_usb_get_busses pusb_get_busses;
_usb_control_msg pusb_control_msg;

HMODULE hModLibusb = 0;

void UninitLibUsb() {
	if (hModLibusb) {
		FreeLibrary(hModLibusb);
		hModLibusb = 0;
	}
}

void TryInitDS3(usb_device *dev) {
	while (dev) {
		if (dev->descriptor.idVendor == VID && dev->descriptor.idProduct == PID) {
			usb_dev_handle *handle = pusb_open(dev);
			if (handle) {
				char junk[20];
				// This looks like HidD_GetFeature with a feature report id of 0xF2 to me and a length of 17.
				// That doesn't work, however, and 17 is shorter than the report length.
				pusb_control_msg(handle, 0xa1, 1, 0x03f2, dev->config->interface->altsetting->bInterfaceNumber, junk, 17, 1000);
				pusb_close(handle);
			}
		}
		if (dev->num_children) {
			for (int i=0; i<dev->num_children; i++) {
				TryInitDS3(dev->children[i]);
			}
		}
		dev = dev->next;
	}
}

void DS3Enum(unsigned int time) {
	if (time - lastDS3Enum < DOUBLE_ENUM_DELAY) {
		return;
	}
	lastDS3Enum = time;
	pusb_find_busses();
	pusb_find_devices();
}

void DS3Check(unsigned int time) {
	if (time - lastDS3Check < DOUBLE_CHECK_DELAY) {
		return;
	}
	if (!lastDS3Check) {
		DS3Enum(time);
	}
	lastDS3Check = time;

	usb_bus *bus = pusb_get_busses();
	while (bus) {
		TryInitDS3(bus->devices);
		bus = bus->next;
	}
}

int InitLibUsb() {
	if (hModLibusb) {
		return 1;
	}
	hModLibusb = LoadLibraryA("C:\\windows\\system32\\libusb0.dll");
	if (hModLibusb) {
		if ((pusb_init = (_usb_init) GetProcAddress(hModLibusb, "usb_init")) &&
			(pusb_close = (_usb_close) GetProcAddress(hModLibusb, "usb_close")) &&
			(pusb_get_string_simple = (_usb_get_string_simple) GetProcAddress(hModLibusb, "usb_get_string_simple")) &&
			(pusb_open = (_usb_open) GetProcAddress(hModLibusb, "usb_open")) &&
			(pusb_find_busses = (_usb_find_busses) GetProcAddress(hModLibusb, "usb_find_busses")) &&
			(pusb_find_devices = (_usb_find_devices) GetProcAddress(hModLibusb, "usb_find_devices")) &&
			(pusb_get_busses = (_usb_get_busses) GetProcAddress(hModLibusb, "usb_get_busses")) &&
			(pusb_control_msg = (_usb_control_msg) GetProcAddress(hModLibusb, "usb_control_msg"))) {
				pusb_init();
				return 1;
		}
		UninitLibUsb();
	}
	return 0;
}

int DualShock3Possible() {
	return InitLibUsb();
}

#include <pshpack1.h>

struct MotorState {
	unsigned char duration;
	unsigned char force;
};

struct LightState {
	// 0xFF makes it stay on.
	unsigned char duration;
	// Have to make one or the other non-zero to turn on light.
	unsigned char dunno[2];
	// 0 is fully lit.
	unsigned char dimness;
	// Have to make non-zero to turn on light.
	unsigned char on;
};

// Data sent to DS3 to set state.
struct DS3Command {
	unsigned char id;
	unsigned char unsure;
	// Small is first, then big.
	MotorState motors[2];
	unsigned char noClue[4];
	// 2 is pad 1 light, 4 is pad 2, 8 is pad 3, 16 is pad 4.  No clue about the others.
	unsigned char lightFlags;
	// Lights are in reverse order.  pad 1 is last.
	LightState lights[4];
	unsigned char dunno[18];
};

#include <poppack.h>

int CharToAxis(unsigned char c) {
	int v = (int)c + ((unsigned int)c >> 7);
	return ((c-128) * FULLY_DOWN)>>7;
}

int CharToButton(unsigned char c) {
	int v = (int)c + ((unsigned int)c >> 7);
	return (v * FULLY_DOWN)>>8;
}

class DualShock3Device : public Device {
	// Cached last vibration values by pad and motor.
	// Need this, as only one value is changed at a time.
	int ps2Vibration[2][4][2];
	int vibration[2];
public:
	int index;
	HANDLE hFile;
	DS3Command sendState;
	unsigned char getState[49];
	OVERLAPPED readop;
	OVERLAPPED writeop;
	int writeCount;
	int lastWrite;

	unsigned int dataLastReceived;

	int writeQueued;
	int writing;

	int StartRead() {
		int res = ReadFile(hFile, &getState, sizeof(getState), 0, &readop);
		return (res || GetLastError() == ERROR_IO_PENDING);
	}

	void QueueWrite() {
		// max of 2 queued writes allowed, one for either motor.
		if (writeQueued < 2) {
			writeQueued++;
			StartWrite();
		}
	}

	int StartWrite() {
		if (!writing && writeQueued) {
			lastWrite = GetTickCount();
			writing++;
			writeQueued--;
			sendState.motors[0].duration = 0x50;
			sendState.motors[1].duration = 0x50;

			int bigForce = vibration[0] * 256/FULLY_DOWN;
			if (bigForce > 255) bigForce = 255;
			sendState.motors[1].force = (unsigned char) bigForce;
			sendState.motors[0].force = (unsigned char) (vibration[1] >= FULLY_DOWN/2);
			// Can't seem to have them both non-zero at once.
			if (sendState.motors[writeCount&1].force) {
				sendState.motors[(writeCount&1)^1].force = 0;
				sendState.motors[(writeCount&1)^1].duration = 0;
			}

			writeCount++;
			int res = WriteFile(hFile, &sendState, sizeof(sendState), 0, &writeop);
			return (res || GetLastError() == ERROR_IO_PENDING);
		}
		return 1;
	}

	DualShock3Device(int index, wchar_t *name, wchar_t *path) : Device(DS3, OTHER, name, path, L"DualShock 3") {
		writeCount = 0;
		writing = 0;
		writeQueued = 0;
		memset(&readop, 0, sizeof(readop));
		memset(&writeop, 0, sizeof(writeop));
		memset(&sendState, 0, sizeof(sendState));
		sendState.id = 1;
		int temp = (index&4);
		sendState.lightFlags = (1 << (temp+1));
		sendState.lights[3-temp].duration = 0xFF;
		sendState.lights[3-temp].dunno[0] = 1;
		sendState.lights[3-temp].on = 1;
		memset(ps2Vibration, 0, sizeof(ps2Vibration));
		vibration[0] = vibration[1] = 0;
		this->index = index;
		int i;
		for (i=0; i<16; i++) {
			if (i != 14 && i != 15 && i != 8 && i != 9) {
				AddPhysicalControl(PRESSURE_BTN, i, 0);
			}
			else {
				AddPhysicalControl(PSHBTN, i, 0);
			}
		}
		for (; i<23; i++) {
			AddPhysicalControl(ABSAXIS, i, 0);
		}
		AddFFAxis(L"Big Motor", 0);
		AddFFAxis(L"Small Motor", 1);
		AddFFEffectType(L"Constant Effect", L"Constant", EFFECT_CONSTANT);
		hFile = INVALID_HANDLE_VALUE;
	}

	wchar_t *GetPhysicalControlName(PhysicalControl *c) {
		const static wchar_t *names[] = {
			L"Square",
			L"Cross",
			L"Circle",
			L"Triangle",
			L"R1",
			L"L1",
			L"R2",
			L"L2",
			L"R3",
			L"L3",
			L"Left",
			L"Down",
			L"Right",
			L"Up",
			L"Start",
			L"Select",
			L"L-Stick X",
			L"L-Stick Y",
			L"R-Stick X",
			L"R-Stick Y",
			L"Left/Right Tilt",
			L"Forward/Back Tilt",
			L"???",
		};
		unsigned int i = (unsigned int) (c - physicalControls);
		if (i < sizeof(names)/sizeof(names[0])) {
			return (wchar_t*)names[i];
		}
		return Device::GetPhysicalControlName(c);
	}

	int Activate(InitInfo *initInfo) {
		if (active) Deactivate();
		// Give grace period before get mad.
		lastWrite = dataLastReceived = GetTickCount();
		readop.hEvent = CreateEvent(0, 0, 0, 0);
		writeop.hEvent = CreateEvent(0, 0, 0, 0);
		hFile = CreateFileW(instanceID, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
		if (!readop.hEvent || !writeop.hEvent || hFile == INVALID_HANDLE_VALUE ||
			!StartRead()) {
			Deactivate();
			return 0;
		}
		active = 1;
		AllocState();
		return 1;
	}

	int Update() {
		if (!active) return 0;
		HANDLE h[2] = {
			readop.hEvent,
			writeop.hEvent
		};
		unsigned int time = GetTickCount();
		if (time - lastWrite > UPDATE_INTERVAL) {
			QueueWrite();
		}
		while (1) {
			DWORD res = WaitForMultipleObjects(2, h, 0, 0);
			if (res == WAIT_OBJECT_0) {
				dataLastReceived = time;
				if (!StartRead()) {
					Deactivate();
					return 0;
				}

				physicalControlState[0] = CharToButton(getState[25]);
				physicalControlState[1] = CharToButton(getState[24]);
				physicalControlState[2] = CharToButton(getState[23]);
				physicalControlState[3] = CharToButton(getState[22]);
				physicalControlState[4] = CharToButton(getState[21]);
				physicalControlState[5] = CharToButton(getState[20]);
				physicalControlState[6] = CharToButton(getState[19]);
				physicalControlState[7] = CharToButton(getState[18]);
				physicalControlState[10] = CharToButton(getState[17]);
				physicalControlState[11] = CharToButton(getState[16]);
				physicalControlState[12] = CharToButton(getState[15]);
				physicalControlState[13] = CharToButton(getState[14]);
				physicalControlState[8] = ((getState[2]&4)/4) * FULLY_DOWN;
				physicalControlState[9] = ((getState[2]&2)/2) * FULLY_DOWN;
				physicalControlState[15] = ((getState[2]&1)/1) * FULLY_DOWN;
				physicalControlState[14] = ((getState[2]&8)/8) * FULLY_DOWN;
				physicalControlState[16] = CharToAxis(getState[6]);
				physicalControlState[17] = CharToAxis(getState[7]);
				physicalControlState[18] = CharToAxis(getState[8]);
				physicalControlState[19] = CharToAxis(getState[9]);
				physicalControlState[20] = CharToAxis(getState[42]+128);
				physicalControlState[21] = CharToAxis(getState[44]+128);
				physicalControlState[22] = CharToAxis(getState[46]+128);
				continue;
			}
			else if (res == WAIT_OBJECT_0+1) {
				writing = 0;
				if (!writeQueued && (vibration[0] | vibration[1])) {
					QueueWrite();
				}
				if (!StartWrite()) {
					Deactivate();
					return 0;
				}
			}
			else {
				if (time-dataLastReceived >= DEVICE_CHECK_DELAY) {
					if (time-dataLastReceived >= DEVICE_ENUM_DELAY) {
						DS3Enum(time);
					}
					DS3Check(time);
					QueueWrite();
				}
			}
			break;
		}
		return 1;
	}

	void SetEffects(unsigned char port, unsigned int slot, unsigned char motor, unsigned char force) {
		ps2Vibration[port][slot][motor] = force;
		vibration[0] = vibration[1] = 0;
		for (int p=0; p<2; p++) {
			for (int s=0; s<4; s++) {
				for (int i=0; i<pads[p][s].numFFBindings; i++) {
					// Technically should also be a *65535/BASE_SENSITIVITY, but that's close enough to 1 for me.
					ForceFeedbackBinding *ffb = &pads[p][s].ffBindings[i];
					vibration[0] += (int)((ffb->axes[0].force * (__int64)ps2Vibration[p][s][ffb->motor]) / 255);
					vibration[1] += (int)((ffb->axes[1].force * (__int64)ps2Vibration[p][s][ffb->motor]) / 255);
				}
			}
		}
		// Make sure at least 2 writes are queued, to update both motors.
		QueueWrite();
		QueueWrite();
	}

	void SetEffect(ForceFeedbackBinding *binding, unsigned char force) {
		PadBindings pBackup = pads[0][0];
		pads[0][0].ffBindings = binding;
		pads[0][0].numFFBindings = 1;
		SetEffects(0, 0, binding->motor, 255);
		pads[0][0] = pBackup;
	}

	void Deactivate() {
		if (hFile != INVALID_HANDLE_VALUE) {
			CancelIo(hFile);
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
		}
		if (readop.hEvent) {
			CloseHandle(readop.hEvent);
		}
		if (writeop.hEvent) {
			CloseHandle(writeop.hEvent);
		}
		writing = 0;
		writeQueued = 0;
		memset(ps2Vibration, 0, sizeof(ps2Vibration));
		vibration[0] = vibration[1] = 0;

		FreeState();
		active = 0;
	}

	~DualShock3Device() {
	}
};

void EnumDualShock3s() {
	if (!InitLibUsb()) return;

	HidDeviceInfo *foundDevs = 0;

	int numDevs = FindHids(&foundDevs, VID, PID);
	if (!numDevs) return;
	int index = 0;
	for (int i=0; i<numDevs; i++) {
		if (foundDevs[i].caps.FeatureReportByteLength == 49 &&
			foundDevs[i].caps.InputReportByteLength == 49 &&
			foundDevs[i].caps.OutputReportByteLength == 49) {
				wchar_t temp[100];
				wsprintfW(temp, L"DualShock 3 #%i", index+1);
				dm->AddDevice(new DualShock3Device(index, temp, foundDevs[i].path));
				index++;
		}
		free(foundDevs[i].path);
	}
	free(foundDevs);
}
