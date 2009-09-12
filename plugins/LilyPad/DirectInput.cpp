#define DIRECTINPUT_VERSION 0x0800

#include "Global.h"
#include "VKey.h"
#include "DirectInput.h"
#include <dinput.h>
#include "InputManager.h"
#include "DeviceEnumerator.h"
#include "PS2Etypes.h"
#include <stdio.h>

// All for getting GUIDs of XInput devices....
#include <wbemidl.h>
#include <oleauto.h>
// MS's code imports wmsstd.h, thus requiring the entire windows
// media SDK also be installed for a simple macro.  This is
// simpler and less silly.
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

// Aka htons, without the winsock dependency.
inline static u16 flipShort(u16 s) {
	return (s>>8) | (s<<8);
}

// Aka htonl, without the winsock dependency.
inline static u32 flipLong(u32 l) {
	return (((u32)flipShort((u16)l))<<16) | flipShort((u16)(l>>16));
}

static void GUIDtoString(wchar_t *data, const GUID *pg) {
	wsprintfW(data, L"%08X-%04X-%04X-%04X-%04X%08X",
		pg->Data1, (u32)pg->Data2, (u32)pg->Data3,
		flipShort(((u16*)pg->Data4)[0]), 
		flipShort(((u16*)pg->Data4)[1]),
		flipLong(((u32*)pg->Data4)[1]));
}

struct DirectInput8Data {
	IDirectInput8* lpDI8;
	int refCount;
	int deviceCount;
};

DirectInput8Data di8d = {0,0,0};

IDirectInput8* GetDirectInput() {
	if (!di8d.lpDI8) {
		if (FAILED(DirectInput8Create(hInst, 0x800, IID_IDirectInput8, (void**) &di8d.lpDI8, 0))) return 0;
	}
	di8d.refCount++;
	return di8d.lpDI8;
}
void ReleaseDirectInput() {
	if (di8d.refCount) {
		di8d.refCount--;
		if (!di8d.refCount) {
			di8d.lpDI8->Release();
			di8d.lpDI8 = 0;
		}
	}
}

static int StringToGUID(GUID *pg, wchar_t *dataw) {
	char data[100];
	if (wcslen(dataw) > 50) return 0;
	int w = 0;
	while (dataw[w]) {
		data[w] = (char) dataw[w];
		w++;
	}
	data[w] = 0;
	u32 temp[5];
	sscanf(data, "%08X-%04X-%04X-%04X-%04X%08X",
		&pg->Data1, temp, temp+1,
		temp+2, temp+3, temp+4);
	pg->Data2 = (u16) temp[0];
	pg->Data3 = (u16) temp[1];
	((u16*)pg->Data4)[0] = flipShort((u16)temp[2]);
	((u16*)pg->Data4)[1] = flipShort((u16)temp[3]);
	((u32*)pg->Data4)[1] = flipLong(temp[4]);
	return 1;
}


struct DI8Effect {
	IDirectInputEffect *die;
	int scale;
};

BOOL CALLBACK EnumDeviceObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef);
BOOL CALLBACK EnumEffectsCallback(LPCDIEFFECTINFOW pdei, LPVOID pvRef);
class DirectInputDevice : public Device {
public:
	DI8Effect *diEffects;

	IDirectInputDevice8 *did;
	GUID guidInstance;
	DirectInputDevice(DeviceType type, IDirectInputDevice8* did, wchar_t *displayName, wchar_t *instanceID, wchar_t *productID, GUID guid) : Device(DI, type, displayName, instanceID, productID) {
		diEffects = 0;
		guidInstance = guid;
		this->did = 0;
		did->EnumEffects(EnumEffectsCallback, this, DIEFT_ALL);
		did->EnumObjects(EnumDeviceObjectsCallback, this, DIDFT_ALL);
		did->Release();
	}

	void SetEffect(ForceFeedbackBinding *binding, unsigned char force) {
		int index = 0;
		for (int port=0; port<2; port++) {
			for (int slot=0; slot<4; slot++) {
				unsigned int diff = binding - pads[port][slot].ffBindings;
				if (diff < (unsigned int)pads[port][slot].numFFBindings) {
					index += diff;
					port = 2;
					break;
				}
				index += pads[port][slot].numFFBindings;
			}
		}
		IDirectInputEffect *die = diEffects[index].die;
		if (die) {
			DIEFFECT dieffect;
			memset(&dieffect, 0, sizeof(dieffect));
			union {
				DIPERIODIC periodic;
				DIRAMPFORCE ramp;
				DICONSTANTFORCE constant;
			};

			dieffect.dwSize = sizeof(dieffect);
			dieffect.lpvTypeSpecificParams = &periodic;
			int magnitude = abs((int)((force*10000*(__int64)diEffects[index].scale)/BASE_SENSITIVITY/255));
			if (magnitude > 10000) magnitude = 10000;
			int type = ffEffectTypes[binding->effectIndex].type;
			if (type == EFFECT_CONSTANT) {
				dieffect.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
				constant.lMagnitude = magnitude;
			}
			else if (type == EFFECT_PERIODIC) {
				dieffect.cbTypeSpecificParams = sizeof(DIPERIODIC);
				periodic.dwMagnitude = 0;
				periodic.lOffset = magnitude;
				periodic.dwPhase = 0;
				periodic.dwPeriod = 2000000;
			}
			else if (type == EFFECT_RAMP) {
				dieffect.cbTypeSpecificParams = sizeof(DIRAMPFORCE);
				ramp.lEnd = ramp.lStart = magnitude;
			}
			dieffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTIDS;
			dieffect.dwDuration = 2000000;
			die->SetParameters(&dieffect, DIEP_TYPESPECIFICPARAMS | DIEP_START);
		}
	}

	int Activate(void *d) {
		int i;
		IDirectInput8 *di8 = GetDirectInput();
		Deactivate();
		if (!di8) return 0;
		if (DI_OK != di8->CreateDevice(guidInstance, &did, 0)) {
			ReleaseDirectInput();
			did = 0;
			return 0;
		}

		{
			DIOBJECTDATAFORMAT *formats = (DIOBJECTDATAFORMAT*)calloc(numPhysicalControls, sizeof(DIOBJECTDATAFORMAT));
			for (i=0; i<numPhysicalControls; i++) {
				formats[i].dwType = physicalControls[i].type | DIDFT_MAKEINSTANCE(physicalControls[i].id);
				formats[i].dwOfs = 4*i;
			}
			DIDATAFORMAT format;
			format.dwSize = sizeof(format);
			format.dwDataSize = 4 * numPhysicalControls;
			format.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
			format.dwFlags = 0;
			format.dwNumObjs = numPhysicalControls;
			format.rgodf = formats;
			int res = did->SetDataFormat(&format);
			for (i=0; i<numPhysicalControls; i++) {
				if (physicalControls[i].type == ABSAXIS) {
					DIPROPRANGE prop;
					prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
					prop.diph.dwSize = sizeof(DIPROPRANGE);
					prop.diph.dwObj = formats[i].dwType;
					prop.diph.dwHow = DIPH_BYID;
					prop.lMin = -FULLY_DOWN;
					prop.lMax = FULLY_DOWN;
					did->SetProperty(DIPROP_RANGE, &prop.diph);

					// May do something like this again, if there's any need.
					/*
					if (FAILED(DI->did->SetProperty(DIPROP_RANGE, &prop.diph))) {
						if (FAILED(DI->did->GetProperty(DIPROP_RANGE, &prop.diph))) {
							// ????
							DI->objects[i].min = prop.lMin;
							DI->objects[i].max = prop.lMax;
							continue;
						}
					}
					DI->objects[i].min = prop.lMin;
					DI->objects[i].max = prop.lMax;
					//*/
				}
			}
			free(formats);
		}
		InitInfo *info = (InitInfo*)d;
		// Note:  Have to use hWndTop to properly hide cursor for mouse device.
		if (type == OTHER) {
			did->SetCooperativeLevel(info->hWndTop, DISCL_BACKGROUND | DISCL_EXCLUSIVE);
		}
		else if (type == KEYBOARD) {
			did->SetCooperativeLevel(info->hWndTop, DISCL_FOREGROUND);
		}
		else {
			did->SetCooperativeLevel(info->hWndTop, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
		}
		if (did->Acquire() != DI_OK) {
			did->Release();
			did = 0;
			ReleaseDirectInput();
			return 0;
		}
		AllocState();
		int count = GetFFBindingCount();
		diEffects = (DI8Effect*) calloc(count, sizeof(DI8Effect));
		i = 0;
		for (int port=0; port<2; port++) {
			for (int slot=0; slot<4; slot++) {
				int subIndex = i;
				for (int j=0; j<pads[port][slot].numFFBindings; j++) {
					ForceFeedbackBinding *b = 0;
					b = &pads[port][slot].ffBindings[i-subIndex];
					ForceFeedbackEffectType *eff = ffEffectTypes + b->effectIndex;
					GUID guid;
					if (!StringToGUID(&guid, eff->effectID)) continue;

					DIEFFECT dieffect;
					memset(&dieffect, 0, sizeof(dieffect));
					dieffect.dwSize = sizeof(dieffect);
					dieffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTIDS;
					dieffect.dwDuration = 1000000;
					dieffect.dwGain = 10000;
					dieffect.dwTriggerButton = DIEB_NOTRIGGER;
					union {
						DIPERIODIC pediodic;
						DIRAMPFORCE ramp;
						DICONSTANTFORCE constant;
					} stuff = {0,0,0,0};

					if (eff->type == EFFECT_CONSTANT) {
						dieffect.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
					}
					else if (eff->type == EFFECT_PERIODIC) {
						dieffect.cbTypeSpecificParams = sizeof(DIPERIODIC);
					}
					else if (eff->type == EFFECT_RAMP) {
						dieffect.cbTypeSpecificParams = sizeof(DIRAMPFORCE);
					}
					dieffect.lpvTypeSpecificParams = &stuff;

					int maxForce = 0;
					int numAxes = 0;
					int *axes = (int*) malloc(sizeof(int) * 3 * numFFAxes);
					DWORD *axisIDs = (DWORD*)(axes + numFFAxes);
					LONG *dirList = (LONG*)(axisIDs + numFFAxes);
					dieffect.rgdwAxes = axisIDs;
					dieffect.rglDirection = dirList;
					for (int k=0; k<numFFAxes; k++) {
						if (b->axes[k].force) {
							int force = abs(b->axes[k].force);
							if (force > maxForce) {
								maxForce = force;
							}
							axes[numAxes] = k;
							axisIDs[numAxes] = ffAxes[k].id;
							dirList[numAxes] = b->axes[k].force;
							numAxes++;
						}
					}
					if (!numAxes) {
						free(axes);
						continue;
					}
					dieffect.cAxes = numAxes;
					diEffects[i].scale = maxForce;
					if (!SUCCEEDED(did->CreateEffect(guid, &dieffect, &diEffects[i].die, 0))) {
						diEffects[i].die = 0;
						diEffects[i].scale = 0;
					}

					free(axes);
					axes = 0;
					i++;
				}
			}
		}
		active = 1;
		return 1;
	}

	int Update() {
		if (!active) return 0;
		if (numPhysicalControls) {
			HRESULT res = did->Poll();
			// ??
			if ((res != DI_OK && res != DI_NOEFFECT) ||
				DI_OK != did->GetDeviceState(4*numPhysicalControls, physicalControlState)) {
					Deactivate();
					return 0;
			}
			for (int i=0; i<numPhysicalControls; i++) {
				if (physicalControls[i].type & RELAXIS) {
					physicalControlState[i] *= (FULLY_DOWN/3);
				}
				else if (physicalControls[i].type & BUTTON) {
					physicalControlState[i] = (physicalControlState[i]&0x80) * FULLY_DOWN / 128;
				}
			}
		}
		return 1;
	}

	int GetFFBindingCount() {
		int count = 0;
		for (int port = 0; port<2; port++) {
			for (int slot = 0; slot<4; slot++) {
				count += pads[port][slot].numFFBindings;
			}
		}
		return count;
	}

	void Deactivate() {
		FreeState();
		if (diEffects) {
			int count = GetFFBindingCount();
			for (int i=0; i<count; i++) {
				if (diEffects[i].die) {
					diEffects[i].die->Stop();
					diEffects[i].die->Release();
				}
			}
			free(diEffects);
			diEffects = 0;
		}
		if (active) {
			did->Unacquire();
			did->Release();
			ReleaseDirectInput();
			did = 0;
			active = 0;
		}
	}

	~DirectInputDevice() {
	}
};

BOOL CALLBACK EnumEffectsCallback(LPCDIEFFECTINFOW pdei, LPVOID pvRef) {
	DirectInputDevice * did = (DirectInputDevice*) pvRef;
	EffectType type;
	int diType = DIEFT_GETTYPE(pdei->dwEffType);
	if (diType == DIEFT_CONSTANTFORCE) {
		type = EFFECT_CONSTANT;
	}
	else if (diType == DIEFT_RAMPFORCE) {
		type = EFFECT_RAMP;
	}
	else if (diType == DIEFT_PERIODIC) {
		type = EFFECT_PERIODIC;
	}
	else {
		return DIENUM_CONTINUE;
	}
	wchar_t guidString[50];
	GUIDtoString(guidString, &pdei->guid);
	did->AddFFEffectType(pdei->tszName, guidString, type);

	return DIENUM_CONTINUE;
}

BOOL CALLBACK EnumDeviceObjectsCallback (LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvRef) {
	DirectInputDevice * did = (DirectInputDevice*) pvRef;
	if (lpddoi->dwType & DIDFT_FFACTUATOR) {
		did->AddFFAxis(lpddoi->tszName, lpddoi->dwType);
	}

	ControlType type;

	if (lpddoi->dwType & DIDFT_POV)
		type = POV;
	else if (lpddoi->dwType & DIDFT_ABSAXIS)
		type = ABSAXIS;
	else if (lpddoi->dwType & DIDFT_RELAXIS)
		type = RELAXIS;
	else if (lpddoi->dwType & DIDFT_PSHBUTTON)
		type = PSHBTN;
	else if (lpddoi->dwType & DIDFT_TGLBUTTON)
		type = TGLBTN;
	else
		return DIENUM_CONTINUE;
	// If too many objects, ignore extra buttons.
	if ((did->numPhysicalControls > 255 && DIDFT_GETINSTANCE(lpddoi->dwType) > 255) && (type & (DIDFT_PSHBUTTON | DIDFT_TGLBUTTON))) {
		int i;
		for (i = did->numPhysicalControls-1; i>did->numPhysicalControls-4; i--) {
			if (!lpddoi->tszName[0]) break;
			const wchar_t *s1 = lpddoi->tszName;
			const wchar_t *s2 = did->physicalControls[i].name;
			while (*s1 && *s1 == *s2) {
				s1++;
				s2++;
			}
			// If perfect match with one of last 4 names, break.
			if (!*s1 && !*s2) break;

			while (s1 != lpddoi->tszName && (s1[-1] >= '0' && s1[-1] <= '9')) s1--;
			int check = 0;
			while (*s1 >= '0' && *s1 <= '9') {
				check = check*10 + *s1 - '0';
				s1++;
			}
			while (*s2 >= '0' && *s2 <= '9') {
				s2++;
			}
			// If perfect match other than final number > 30, then break.
			// takes care of "button xx" case without causing issues with F keys.
			if (!*s1 && !*s2 && check > 30) break;
		}
		if (i != did->numPhysicalControls-4) {
			return DIENUM_CONTINUE;
		}
	}

	int vkey = 0;
	if (lpddoi->tszName[0] && did->type == KEYBOARD) {
		for (u32 i = 0; i<256; i++) {
			wchar_t *t = GetVKStringW((u8)i);
			if (!wcsicmp(lpddoi->tszName, t)) {
				vkey = i;
				break;
			}
		}
	}
	did->AddPhysicalControl(type, DIDFT_GETINSTANCE(lpddoi->dwType), vkey, lpddoi->tszName);
	return DIENUM_CONTINUE;
}

// Evil code from MS's site.  If only they'd just made a way to get
// an XInput device's GUID directly in the first place...
BOOL IsXInputDevice( const GUID* pGuidProductFromDirectInput )
{
    IWbemLocator*           pIWbemLocator  = NULL;
    IEnumWbemClassObject*   pEnumDevices   = NULL;
    IWbemClassObject*       pDevices[20]   = {0};
    IWbemServices*          pIWbemServices = NULL;
    BSTR                    bstrNamespace  = NULL;
    BSTR                    bstrDeviceID   = NULL;
    BSTR                    bstrClassName  = NULL;
    DWORD                   uReturned      = 0;
    bool                    bIsXinputDevice= false;
    UINT                    iDevice        = 0;
    VARIANT                 var;
    HRESULT                 hr;

    // CoInit if needed
    hr = CoInitialize(NULL);
    bool bCleanupCOM = SUCCEEDED(hr);

    // Create WMI
    hr = CoCreateInstance( __uuidof(WbemLocator),
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           __uuidof(IWbemLocator),
                           (LPVOID*) &pIWbemLocator);
    if( FAILED(hr) || pIWbemLocator == NULL )
        goto LCleanup;

    bstrNamespace = SysAllocString( L"\\\\.\\root\\cimv2" );if( bstrNamespace == NULL ) goto LCleanup;        
    bstrClassName = SysAllocString( L"Win32_PNPEntity" );   if( bstrClassName == NULL ) goto LCleanup;        
    bstrDeviceID  = SysAllocString( L"DeviceID" );          if( bstrDeviceID == NULL )  goto LCleanup;        
    
    // Connect to WMI 
    hr = pIWbemLocator->ConnectServer( bstrNamespace, NULL, NULL, 0L, 
                                       0L, NULL, NULL, &pIWbemServices );
    if( FAILED(hr) || pIWbemServices == NULL )
        goto LCleanup;

    // Switch security level to IMPERSONATE. 
    CoSetProxyBlanket( pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, 
                       RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );                    

    hr = pIWbemServices->CreateInstanceEnum( bstrClassName, 0, NULL, &pEnumDevices ); 
    if( FAILED(hr) || pEnumDevices == NULL )
        goto LCleanup;

    // Loop over all devices
    for( ;; )
    {
        // Get 20 at a time
        hr = pEnumDevices->Next( 10000, 20, pDevices, &uReturned );
        if( FAILED(hr) )
            goto LCleanup;
        if( uReturned == 0 )
            break;

        for( iDevice=0; iDevice<uReturned; iDevice++ )
        {
            // For each device, get its device ID
            hr = pDevices[iDevice]->Get( bstrDeviceID, 0L, &var, NULL, NULL );
            if( SUCCEEDED( hr ) && var.vt == VT_BSTR && var.bstrVal != NULL )
            {
                // Check if the device ID contains "IG_".  If it does, then it's an XInput device
				    // This information can not be found from DirectInput 
                if( wcsstr( var.bstrVal, L"IG_" ) )
                {
                    // If it does, then get the VID/PID from var.bstrVal
                    DWORD dwPid = 0, dwVid = 0;
                    WCHAR* strVid = wcsstr( var.bstrVal, L"VID_" );
                    if( strVid && swscanf( strVid, L"VID_%4X", &dwVid ) != 1 )
                        dwVid = 0;
                    WCHAR* strPid = wcsstr( var.bstrVal, L"PID_" );
                    if( strPid && swscanf( strPid, L"PID_%4X", &dwPid ) != 1 )
                        dwPid = 0;

                    // Compare the VID/PID to the DInput device
                    DWORD dwVidPid = MAKELONG( dwVid, dwPid );
                    if( dwVidPid == pGuidProductFromDirectInput->Data1 )
                    {
                        bIsXinputDevice = true;
                        goto LCleanup;
                    }
                }
            }   
            SAFE_RELEASE( pDevices[iDevice] );
        }
    }

LCleanup:
    if(bstrNamespace)
        SysFreeString(bstrNamespace);
    if(bstrDeviceID)
        SysFreeString(bstrDeviceID);
    if(bstrClassName)
        SysFreeString(bstrClassName);
    for( iDevice=0; iDevice<20; iDevice++ )
        SAFE_RELEASE( pDevices[iDevice] );
    SAFE_RELEASE( pEnumDevices );
    SAFE_RELEASE( pIWbemLocator );
    SAFE_RELEASE( pIWbemServices );

    if( bCleanupCOM )
        CoUninitialize();

    return bIsXinputDevice;
}


struct DeviceEnumInfo {
	IDirectInput8 *di8;
	int ignoreXInput;
};

BOOL CALLBACK EnumCallback (LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
	IDirectInput8* di8 = ((DeviceEnumInfo*)pvRef)->di8;
	const wchar_t *name;
	wchar_t temp[40];
	//if (((DeviceEnumInfo*)pvRef)->ignoreXInput && lpddi->
	if (lpddi->tszInstanceName[0]) {
		name = lpddi->tszInstanceName;
	}
	else if (lpddi->tszProductName[0]) {
		name = lpddi->tszProductName;
	}
	else {
		wsprintfW (temp, L"Device %i", di8d.deviceCount);
		name = temp;
	}
	di8d.deviceCount++;
	wchar_t *fullName = (wchar_t *) malloc((wcslen(name) + 4) * sizeof(wchar_t));
	wsprintf(fullName, L"DX %s", name);
	wchar_t instanceID[100];
	wchar_t productID[100];
	GUIDtoString(instanceID, &lpddi->guidInstance);
	GUIDtoString(productID, &lpddi->guidProduct);
	DeviceType type = OTHER;
	if ((lpddi->dwDevType & 0xFF) == DI8DEVTYPE_KEYBOARD) {
		type = KEYBOARD;
	}
	else if ((lpddi->dwDevType & 0xFF) == DI8DEVTYPE_MOUSE) {
		type = MOUSE;
	}
	IDirectInputDevice8 *did;
	if (DI_OK == di8->CreateDevice(lpddi->guidInstance, &did, 0)) {
		dm->AddDevice(new DirectInputDevice(type, did, fullName, instanceID, productID, lpddi->guidInstance));
	}
	free(fullName);
	return DIENUM_CONTINUE;
}

void EnumDirectInputDevices(int ignoreXInput) {
	DeviceEnumInfo enumInfo;
	enumInfo.di8 = GetDirectInput();
	if (!enumInfo.di8) return;
	enumInfo.ignoreXInput = ignoreXInput;
	di8d.deviceCount = 0;
	enumInfo.di8->EnumDevices(DI8DEVCLASS_ALL, EnumCallback, &enumInfo, DIEDFL_ATTACHEDONLY);
	ReleaseDirectInput();
}

