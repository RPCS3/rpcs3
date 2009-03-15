#include "Global.h"
#include <Dbt.h>
#include <commctrl.h>

#include "Config.h"
#include "PS2Edefs.h"
#include "Resource.h"
#include "Diagnostics.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "DeviceEnumerator.h"
#include "InputManager.h"
#include "KeyboardQueue.h"
#include "WndProcEater.h"

// Needed to know if raw input is available.  It requires XP or higher.
#include "RawInput.h"

GeneralConfig config;

// 1 if running inside a PS2 emulator.  Set to 1 on any
// of the PS2-specific functions (PS2EgetLibVersion2, PS2EgetLibType).
// Only affects if I allow read input in GS thread to be set.
u8 ps2e = 0;

HWND hWndProp = 0;

int selected = 0;

HWND hWnds[2][4];
HWND hWndGeneral = 0;

void Populate(int port, int slot);

void SetupLogSlider(HWND hWndSlider) {
	SendMessage(hWndSlider, TBM_SETRANGEMIN, 0, 1);
	SendMessage(hWndSlider, TBM_SETPAGESIZE, 0, 1<<13);
	SendMessage(hWndSlider, TBM_SETRANGEMAX, 1, 1<<22);
	SendMessage(hWndSlider, TBM_SETTIC, 0, 1<<21);
	SendMessage(hWndSlider, TBM_SETTIC, 0, 1<<20);
	SendMessage(hWndSlider, TBM_SETTIC, 0, 3<<20);
}

int GetLogSliderVal(HWND hWnd, int id) {
	HWND hWndSlider = GetDlgItem(hWnd, id);
	int val = SendMessage(hWndSlider, TBM_GETPOS, 0, 0);
	if (val <= (1<<21)) {
		val = (val+16) >> 5;
	}
	else {
		double v = (val - (1<<21))/(double)((1<<22)- (1<<21));
		double max = log((double)(1<<23));
		double min = log((double)BASE_SENSITIVITY);
		v = v*(max-min)+min;
		// exp is not implemented in ntdll, so use pow instead.
		// More than accurate enough for my needs.
		// v = exp(v);
		v = pow(2.7182818284590451, v);
		if (v > (1<<23)) v = (1<<23);
		val = (int)floor(v+0.5);
	}
	if (!val) val = 1;
	if (IsDlgButtonChecked(hWnd, id+1) == BST_CHECKED) {
		val = -val;
	}
	return val;
}

void SetLogSliderVal(HWND hWnd, int id, HWND hWndText, int val) {
	HWND hWndSlider = GetDlgItem(hWnd, id);
	int sliderPos = 0;
	wchar_t temp[30];
	EnableWindow(hWndSlider, val != 0);
	EnableWindow(GetDlgItem(hWnd, id+1), val != 0);
	if (!val) val = BASE_SENSITIVITY;
	CheckDlgButton(hWnd, id+1, BST_CHECKED * (val<0));
	if (val < 0) {
		val = -val;
	}
	if (val <= BASE_SENSITIVITY) {
		sliderPos = val<<5;
	}
	else {
		double max = log((double)(1<<23));
		double min = log((double)BASE_SENSITIVITY);
		double v = log((double)(val));
		v = (v-min)/(max-min);
		v = ((1<<22)- (1<<21)) *v + (1<<21);
		// _ftol
		sliderPos = (int)(floor(v+0.5));
	}
	SendMessage(hWndSlider, TBM_SETPOS, 1, sliderPos);
	int val2 = (int)(1000*(__int64)val/BASE_SENSITIVITY);
	wsprintfW(temp, L"%i.%03i", val2/1000, val2%1000);
	SetWindowTextW(hWndText, temp);
}

void RefreshEnabledDevices(int updateDeviceList) {
	// Clears all device state.
	if (updateDeviceList) EnumDevices();

	for (int i=0; i<dm->numDevices; i++) {
		Device *dev = dm->devices[i];

		if (!dev->attached && dev->displayName[0] != '[') {
			wchar_t *newName = (wchar_t*) malloc(sizeof(wchar_t) * (wcslen(dev->displayName) + 12));
			wsprintfW(newName, L"[Detached] %s", dev->displayName);
			free(dev->displayName);
			dev->displayName = newName;
		}

		if ((dev->type == KEYBOARD && dev->api == IGNORE_KEYBOARD) ||
			(dev->type == KEYBOARD && dev->api == config.keyboardApi) ||
			(dev->type == MOUSE && dev->api == config.mouseApi) ||
			(dev->type == OTHER &&
				((dev->api == DI && config.gameApis.directInput) || 
				 (dev->api == XINPUT && config.gameApis.xInput)))) {
					dm->EnableDevice(i);
		}
		else
					dm->DisableDevice(i);
	}

	// Older code.  Newer version is a bit uglier, but doesn't
	// release devices that are enabled both before and afterwards.
	// So a bit nicer, in theory.
	/*
	dm->DisableAllDevices();
	dm->EnableDevices(KEYBOARD, config.keyboardApi);
	dm->EnableDevices(MOUSE, config.mouseApi);
	if (config.gameApis.directInput) {
		dm->EnableDevices(OTHER, DI);
	}
	//*/
}

// Disables/enables devices as necessary.  Also updates diagnostic list
// and pad1/pad2 bindings lists, depending on params.  Also updates device
// list, if indicated.

// Must be called at some point when entering config mode, as I disable
// (non-keyboard/mice) devices with no active bindings when the emulator's running.
void RefreshEnabledDevicesAndDisplay(int updateDeviceList = 0, HWND hWnd = 0, int populate=0) {
	RefreshEnabledDevices(updateDeviceList);
	if (hWnd) {
		HWND hWndList = GetDlgItem(hWnd, IDC_LIST);
		ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
		int count = ListView_GetItemCount(hWndList);
		LVITEM item;
		item.iItem = 0;
		item.iSubItem = 0;
		item.mask = LVIF_TEXT | LVIF_PARAM;
		for (int j=0; j<dm->numDevices; j++) {
			if (dm->devices[j]->enabled && dm->devices[j]->api != IGNORE_KEYBOARD) {
				item.lParam = j;
				item.pszText = dm->devices[j]->displayName;
				if (count > 0) {
					ListView_SetItem(hWndList, &item);
					count --;
				}
				else {
					ListView_InsertItem(hWndList, &item);
				}
				item.iItem++;
			}
		}
		// This way, won't scroll list to start.
		while(count > 0) {
			ListView_DeleteItem(hWndList, item.iItem);
			count --;
		}
	}
	if (populate) {
		for (int i=0; i<8; i++) {
			Populate(i&1, i>>1);
		}
	}
}

wchar_t *GetCommandStringW(u8 command, int port, int slot) {
	static wchar_t temp[34];
	if (command >= 0x20 && command <= 0x27) {
		if (config.padConfigs[port][slot].type == GuitarPad && (command == 0x20 || command == 0x22)) {
			HWND hWnd = GetDlgItem(hWnds[port][slot], 0x10F0+command);
			int res = GetWindowTextW(hWnd, temp, 20);
			if ((unsigned int)res-1 <= 18) return temp;
		}
		static wchar_t stick[2] = {'L', 'R'};
		static wchar_t *dir[] = {
			L"Up", L"Right",
			L"Down", L"Left"};
		wsprintfW(temp, L"%c-Stick %s", stick[(command-0x20)/4], dir[command & 3]);
		return temp;
	}
	/* Get text from the buttons. */
	if (command >= 0x0C && command <=0x28) {
		HWND hWnd = GetDlgItem(hWnds[port][slot], 0x10F0+command);
		if (!hWnd) {
			wchar_t *strings[] = {
				L"Lock Buttons",
				L"Lock Input",
				L"Lock Direction",
				L"Mouse",
				L"Select",
				L"L3",
				L"R3",
				L"Start",
				L"Up",
				L"Right",
				L"Down",
				L"Left",
				L"L2",
				L"R2",
				L"L1",
				L"R1",
				L"Triangle",
				L"Circle",
				L"Square",
				L"Cross",
				L"L-Stick Up",
				L"L-Stick Right",
				L"L-Stick Down",
				L"L-Stick Left",
				L"R-Stick Up",
				L"R-Stick Right",
				L"R-Stick Down",
				L"R-Stick Left",
				L"Analog",
			};
			return strings[command - 0xC];
		}
		int res = GetWindowTextW(hWnd, temp, 20);
		if ((unsigned int)res-1 <= 18) return temp;
	}
	else if (command == 0x7F) {
		return L"Ignore Key";
	}
	return L"";
}

inline void GetSettingsFileName(wchar_t *out) {
	wcscpy(out, L"inis\\LilyPad.ini");
}

int GetBinding(int port, int slot, int index, Device *&dev, Binding *&b, ForceFeedbackBinding *&ffb);
int BindCommand(Device *dev, unsigned int uid, unsigned int port, unsigned int slot, int command, int sensitivity, int turbo);

int CreateEffectBinding(Device *dev, wchar_t *effectName, unsigned int port, unsigned int slot, unsigned int motor, ForceFeedbackBinding **binding);

void SelChanged(int port, int slot) {
	HWND hWnd = hWnds[port][slot];
	if (!hWnd) return;
	HWND hWndTemp, hWndList = GetDlgItem(hWnd, IDC_LIST);
	int j, i = ListView_GetSelectedCount(hWndList);
	wchar_t *devName = L"N/A";
	wchar_t *key = L"N/A";
	wchar_t *command = L"N/A";
	// Second value is now turbo.
	int turbo = -1;
	int sensitivity = 0;
	// Set if sensitivity != 0, but need to disable flip anyways.
	// Only used to relative axes.
	int disableFlip = 0;
	wchar_t temp[4][1000];
	Device *dev;
	int bFound = 0;
	int ffbFound = 0;
	ForceFeedbackBinding *ffb = 0;
	Binding *b = 0;
	if (i >= 1) {
		int index = -1;
		int flipped = 0;
		while (1) {
			index = ListView_GetNextItem(hWndList, index, LVNI_SELECTED);
			if (index < 0) break;
			LVITEMW item;
			item.iItem = index;
			item.mask = LVIF_TEXT;
			for (j=0; j<3; j++) {
				item.pszText = temp[3];
				item.iSubItem = j;
				item.cchTextMax = sizeof(temp[0])/sizeof(temp[3][0]);
				if (!ListView_GetItem(hWndList, &item)) break;
				if (!bFound && !ffbFound)
					wcscpy(temp[j], temp[3]);
				else if (wcsicmp(temp[j], temp[3])) {
					int q = 0;
					while (temp[j][q] == temp[3][q]) q++;
					if (q && temp[j][q-1] == ' ' && temp[j][q] && temp[j][q+1] == 0) q--;
					if (j == 1) {
						// Really ugly, but merges labels for multiple directions for same axis.
						if ((temp[j][q] == 0 || (temp[j][q] == ' ' && temp[j][q+2] == 0)) &&
							(temp[3][q] == 0 || (temp[3][q] == ' ' && temp[3][q+2] == 0))) {
								temp[j][q] = 0;
								continue;
						}
					}
					// Merge different directions for same stick.
					else if (j == 2 && q > 4) {
						temp[j][q] = 0;
						continue;
					}
					wcscpy(temp[j], L"*");
				}
			}
			if (j == 3) {
				devName = temp[0];
				key = temp[1];
				command = temp[2];
				if (GetBinding(port, slot, index, dev, b, ffb)) {
					if (b) {
						bFound ++;
						VirtualControl *control = &dev->virtualControls[b->controlIndex];
						// Ignore
						if (b->command != 0x7F) {
							// Only relative axes can't have negative sensitivity.
							if (((control->uid >> 16) & 0xFF) == RELAXIS) {
								disableFlip = 1;
							}
							turbo += b->turbo;
							if (b->sensitivity < 0) {
								flipped ++;
								sensitivity -= b->sensitivity;
							}
							else {
								sensitivity += b->sensitivity;
							}
						}
						else disableFlip = 1;
					}
					else ffbFound++;
				}
			}
		}
		if ((bFound && ffbFound) || ffbFound > 1) {
			ffb = 0;
			turbo = -1;
			sensitivity = 0;
			disableFlip = 1;
			bFound = ffbFound = 0;
		}
		else if (bFound) {
			turbo++;
			sensitivity /= bFound;
			if (bFound > 1) disableFlip = 1;
			else if (flipped) {
				sensitivity = -sensitivity;
			}
		}
	}

	for (i=IDC_SLIDER1; i<ID_DELETE; i++) {
		hWndTemp = GetDlgItem(hWnd, i);
		if (hWndTemp)
			ShowWindow(hWndTemp, ffb == 0);
	}
	for (i=0x1300; i<0x1400; i++) {
		hWndTemp = GetDlgItem(hWnd, i);
		if (hWndTemp) {
			int enable = ffb != 0;
			if (ffb && i >= IDC_FF_AXIS1_ENABLED) {
				int index = (i - IDC_FF_AXIS1_ENABLED) / 16;
				if (index >= dev->numFFAxes) {
					enable = 0;
				}
				else {
					int type = i - index*16;
					AxisEffectInfo *info = 0;
					if (dev->numFFAxes > index) {
						info = ffb->axes+index;
					}
					if (type == IDC_FF_AXIS1) {
						// takes care of IDC_FF_AXIS1_FLIP as well.
						SetupLogSlider(hWndTemp);
						int force = 0;
						if (info) {
							force = info->force;
						}
						SetLogSliderVal(hWnd, i, GetDlgItem(hWnd, i-IDC_FF_AXIS1+IDC_FF_AXIS1_SCALE), force);
					}
					else if (type == IDC_FF_AXIS1_ENABLED) {
						CheckDlgButton(hWnd, i, BST_CHECKED * (info->force!=0));
						wsprintfW(temp[0], L"Axis %i", index+1);
						if (dev->ffAxes[index].displayName && wcslen(dev->ffAxes[index].displayName) < 50) {
							wchar_t *end = wcschr(temp[0], 0);
							wsprintfW(end, L": %s", dev->ffAxes[index].displayName);
						}
						SetWindowText(hWndTemp, temp[0]);
					}
				}
			}
			ShowWindow(hWndTemp, enable);
		}
	}
	if (!ffb) { 
		SetLogSliderVal(hWnd, IDC_SLIDER1, GetDlgItem(hWnd, IDC_AXIS_SENSITIVITY1), sensitivity);

		if (disableFlip) EnableWindow(GetDlgItem(hWnd, IDC_FLIP1), 0);

		EnableWindow(GetDlgItem(hWnd, IDC_TURBO), turbo >= 0);
		if (turbo > 0 && turbo < bFound) {
			SendMessage(GetDlgItem(hWnd, IDC_TURBO), BM_SETSTYLE, BS_AUTO3STATE, 0);
			CheckDlgButton(hWnd, IDC_TURBO, BST_INDETERMINATE);
		}
		else {
			SendMessage(GetDlgItem(hWnd, IDC_TURBO), BM_SETSTYLE, BS_AUTOCHECKBOX, 0);
			CheckDlgButton(hWnd, IDC_TURBO, BST_CHECKED * (bFound && turbo == bFound));
		}
		HWND hWndCombo = GetDlgItem(hWnd, IDC_AXIS_DIRECTION);
		int enableCombo = 0;
		SendMessage(hWndCombo, CB_RESETCONTENT, 0, 0);
		if (b && bFound == 1) {
			VirtualControl *control = &dev->virtualControls[b->controlIndex];
			unsigned int uid = control->uid;
			if (((uid>>16) & 0xFF) == ABSAXIS) {
				enableCombo = 1;
				wchar_t *endings[3] = {L" -", L" +", L""};
				wchar_t *string = temp[3];
				wcscpy(string, key);
				wchar_t *end = wcschr(string, 0);
				int sel = 2;
				if (!(uid & UID_AXIS)) {
					end[-2] = 0;
					sel = (end[-1] == '+');
					end -= 2;
				}
				for (int i=0; i<3; i++) {
					wcscpy(end, endings[i]);
					SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) string);
					if (i == sel)
						SendMessage(hWndCombo, CB_SETCURSEL, i, 0);
				}
			}
		}
		EnableWindow(hWndCombo, enableCombo);
		if (!enableCombo) {
			SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) key);
			SendMessage(hWndCombo, CB_SELECTSTRING, i, (LPARAM) key);
		}

		SetDlgItemText(hWnd, IDC_AXIS_DEVICE1, devName);
		SetDlgItemText(hWnd, IDC_AXIS_CONTROL1, command);
	}
	else {
		wchar_t temp2[2000];
		wsprintfW(temp2, L"%s / %s", devName, command);
		SetDlgItemText(hWnd, ID_FF, temp2);

		hWndTemp = GetDlgItem(hWnd, IDC_FF_EFFECT);
		SendMessage(hWndTemp, CB_RESETCONTENT, 0, 0);
		for (i=0; i<dev->numFFEffectTypes; i++) {
			SendMessage(hWndTemp, CB_INSERTSTRING, i, (LPARAM)dev->ffEffectTypes[i].displayName);
		}
		SendMessage(hWndTemp, CB_SETCURSEL, ffb->effectIndex, 0);
		EnableWindow(hWndTemp, dev->numFFEffectTypes>1);
	}
}

void UnselectAll(HWND hWnd) {
	int i = ListView_GetSelectedCount(hWnd);
	while (i-- > 0) {
		int index = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);
		ListView_SetItemState(hWnd, index, 0, LVIS_SELECTED);
	}
}


int GetItemIndex(int port, int slot, Device *dev, ForceFeedbackBinding *binding) {
	int count = 0;
	for (int i = 0; i<dm->numDevices; i++) {
		Device *dev2 = dm->devices[i];
		if (!dev2->enabled) continue;
		if (dev2 != dev) {
			count += dev2->pads[port][slot].numBindings + dev2->pads[port][slot].numFFBindings;
			continue;
		}
		return count += dev2->pads[port][slot].numBindings + (binding - dev2->pads[port][slot].ffBindings);
	}
	return -1;
}
int GetItemIndex(int port, int slot, Device *dev, Binding *binding) {
	int count = 0;
	for (int i = 0; i<dm->numDevices; i++) {
		Device *dev2 = dm->devices[i];
		if (!dev2->enabled) continue;
		if (dev2 != dev) {
			count += dev2->pads[port][slot].numBindings + dev2->pads[port][slot].numFFBindings;
			continue;
		}
		return count += binding - dev->pads[port][slot].bindings;
	}
	return -1;
}

// Doesn't check if already displayed.
int ListBoundCommand(int port, int slot, Device *dev, Binding *b) {
	if (!hWnds[port][slot]) return -1;
	HWND hWndList = GetDlgItem(hWnds[port][slot], IDC_LIST);
	int index = -1;
	if (hWndList) {
		index = GetItemIndex(port, slot, dev, b);
		if (index >= 0) {
			LVITEM item;
			item.mask = LVIF_TEXT;
			item.pszText = dev->displayName;
			item.iItem = index;
			item.iSubItem = 0;
			SendMessage(hWndList, LVM_INSERTITEM, 0, (LPARAM)&item);
			item.mask = LVIF_TEXT;
			item.iSubItem = 1;
			item.pszText = dev->GetVirtualControlName(&dev->virtualControls[b->controlIndex]);
			SendMessage(hWndList, LVM_SETITEM, 0, (LPARAM)&item);
			item.iSubItem = 2;
			item.pszText = GetCommandStringW(b->command, port, slot);
			SendMessage(hWndList, LVM_SETITEM, 0, (LPARAM)&item);
		}
	}
	return index;
}

int ListBoundEffect(int port, int slot, Device *dev, ForceFeedbackBinding *b) {
	if (!hWnds[port][slot]) return -1;
	HWND hWndList = GetDlgItem(hWnds[port][slot], IDC_LIST);
	int index = -1;
	if (hWndList) {
		index = GetItemIndex(port, slot, dev, b);
		if (index >= 0) {
			LVITEM item;
			item.mask = LVIF_TEXT;
			item.pszText = dev->displayName;
			item.iItem = index;
			item.iSubItem = 0;
			SendMessage(hWndList, LVM_INSERTITEM, 0, (LPARAM)&item);
			item.mask = LVIF_TEXT;
			item.iSubItem = 1;
			item.pszText = dev->ffEffectTypes[b->effectIndex].displayName;
			SendMessage(hWndList, LVM_SETITEM, 0, (LPARAM)&item);
			item.iSubItem = 2;
			wchar_t *ps2Motors[2] = {L"Big Motor", L"Small Motor"};
			item.pszText = ps2Motors[b->motor];
			SendMessage(hWndList, LVM_SETITEM, 0, (LPARAM)&item);
		}
	}
	return index;
}

// Only for use with control bindings.  Affects all highlighted bindings.
void ChangeValue(int port, int slot, int *newSensitivity, int *turbo) {
	if (!hWnds[port][slot]) return;
	HWND hWndList = GetDlgItem(hWnds[port][slot], IDC_LIST);
	int count = ListView_GetSelectedCount(hWndList);
	if (count < 1) return;
	int index = -1;
	while (1) {
		index = ListView_GetNextItem(hWndList, index, LVNI_SELECTED);
		if (index < 0) break;
		Device *dev;
		Binding *b;
		ForceFeedbackBinding *ffb;
		if (!GetBinding(port, slot, index, dev, b, ffb) || ffb) return;
		if (newSensitivity) {
			// Don't change flip state when modifying multiple controls.
			if (count > 1 && b->sensitivity < 0)
				b->sensitivity = -*newSensitivity;
			else
				b->sensitivity = *newSensitivity;
		}
		if (turbo) {
			b->turbo = *turbo;
		}
	}
	PropSheet_Changed(hWndProp, hWnds[port][slot]);
	SelChanged(port, slot);
}

// Only for use with effect bindings.
void ChangeEffect(int port, int slot, int id, int *newForce, unsigned int *newEffectType) {
	if (!hWnds[port][slot]) return;
	HWND hWndList = GetDlgItem(hWnds[port][slot], IDC_LIST);
	int i = ListView_GetSelectedCount(hWndList);
	if (i != 1) return;
	int index = ListView_GetNextItem(hWndList, -1, LVNI_SELECTED);
	Device *dev;
	Binding *b;
	ForceFeedbackBinding *ffb;
	if (!GetBinding(port, slot, index, dev, b, ffb) || b) return;
	if (newForce) {
		unsigned int axisIndex = (id - IDC_FF_AXIS1_ENABLED)/16;
		if (axisIndex < (unsigned int)dev->numFFAxes) {
			ffb->axes[axisIndex].force = *newForce;
		}
	}
	if (newEffectType && *newEffectType < (unsigned int)dev->numFFEffectTypes) {
		ffb->effectIndex = *newEffectType;
		ListView_DeleteItem(hWndList, index);
		index = ListBoundEffect(port, slot, dev, ffb);
		ListView_SetItemState(hWndList, index, LVIS_SELECTED, LVIS_SELECTED);
	}
	PropSheet_Changed(hWndProp, hWnds[port][slot]);
	SelChanged(port, slot);
}



void Populate(int port, int slot) {
	if (!hWnds[port][slot]) return;
	HWND hWnd = GetDlgItem(hWnds[port][slot], IDC_LIST);
	ListView_DeleteAllItems(hWnd);
	int i, j;

	int multipleBinding = config.multipleBinding;
	config.multipleBinding = 1;
	for (j=0; j<dm->numDevices; j++) {
		Device *dev = dm->devices[j];
		if (!dev->enabled) continue;
		for (i=0; i<dev->pads[port][slot].numBindings; i++) {
			ListBoundCommand(port, slot, dev, dev->pads[port][slot].bindings+i);
		}
		for (i=0; i<dev->pads[port][slot].numFFBindings; i++) {
			ListBoundEffect(port, slot, dev, dev->pads[port][slot].ffBindings+i);
		}
	}
	config.multipleBinding = multipleBinding;

	hWnd = GetDlgItem(hWnds[port][slot], IDC_FORCEFEEDBACK);
	SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
	int added = 0;
	for (i=0; i<dm->numDevices; i++) {
		Device *dev = dm->devices[i];
		if (dev->enabled && dev->numFFAxes && dev->numFFEffectTypes) {
			SendMessage(hWnd, CB_INSERTSTRING, added, (LPARAM)dev->displayName);
			SendMessage(hWnd, CB_SETITEMDATA, added, i);
			added++;
		}
	}
	SendMessage(hWnd, CB_SETCURSEL, 0, 0);
	EnableWindow(hWnd, added!=0);
	EnableWindow(GetDlgItem(hWnds[port][slot], ID_BIG_MOTOR), added!=0);
	EnableWindow(GetDlgItem(hWnds[port][slot], ID_SMALL_MOTOR), added!=0);

	SelChanged(port, slot);
}

int WritePrivateProfileInt(wchar_t *s1, wchar_t *s2, int v, wchar_t *ini) {
	wchar_t temp[100];
	_itow(v, temp, 10);
	return WritePrivateProfileStringW(s1, s2, temp, ini);
}

void SetVolume(int volume) {
	if (volume > 100) volume = 100;
	if (volume < 0) volume = 0;
	config.volume = volume;
	unsigned int val = 0xFFFF * volume/100;
	val = val | (val<<16);
	for (int i=waveOutGetNumDevs()-1; i>=0; i--) {
		waveOutSetVolume((HWAVEOUT)i, val);
	}
	wchar_t ini[MAX_PATH+20];
	GetSettingsFileName(ini);
	WritePrivateProfileInt(L"General Settings", L"Volume", config.volume, ini);
}

int SaveSettings(wchar_t *file=0) {
	wchar_t ini[MAX_PATH+20];

	// Need this either way for saving path.
	GetSettingsFileName(ini);
	if (!file) {
		file = ini;
	}
	else {
		wchar_t *c = wcsrchr(file, '\\');
		if (*c) {
			*c = 0;
			wcscpy(config.lastSaveConfigPath, file);
			wcscpy(config.lastSaveConfigFileName, c+1);
			*c = '\\'; 
		}
	}
	DeleteFileW(file);

	WritePrivateProfileStringW(L"General Settings", L"Last Config Path", config.lastSaveConfigPath, ini);
	WritePrivateProfileStringW(L"General Settings", L"Last Config Name", config.lastSaveConfigFileName, ini);

	// Just check first, last, and all pad bindings.  Should be more than enough.  No real need to check
	// config path.
	int noError = WritePrivateProfileInt(L"General Settings", L"Force Cursor Hide", config.forceHide, file);
	WritePrivateProfileInt(L"General Settings", L"Close Hacks", config.closeHacks, file);
	WritePrivateProfileInt(L"General Settings", L"Background", config.background, file);

	WritePrivateProfileInt(L"General Settings", L"GS Thread Updates", config.GSThreadUpdates, file);
	WritePrivateProfileInt(L"General Settings", L"Escape Fullscreen Hack", config.escapeFullscreenHack, file);

	WritePrivateProfileInt(L"General Settings", L"Disable Screen Saver", config.disableScreenSaver, file);
	WritePrivateProfileInt(L"General Settings", L"GH2", config.GH2, file);
	WritePrivateProfileInt(L"General Settings", L"Mouse Unfocus", config.mouseUnfocus, file);
	WritePrivateProfileInt(L"General Settings", L"Logging", config.debug, file);
	WritePrivateProfileInt(L"General Settings", L"Keyboard Mode", config.keyboardApi, file);
	WritePrivateProfileInt(L"General Settings", L"Mouse Mode", config.mouseApi, file);
	WritePrivateProfileInt(L"General Settings", L"DirectInput Game Devices", config.gameApis.directInput, file);
	WritePrivateProfileInt(L"General Settings", L"XInput", config.gameApis.xInput, file);
	WritePrivateProfileInt(L"General Settings", L"Multiple Bindings", config.multipleBinding, file);

	WritePrivateProfileInt(L"General Settings", L"Save State in Title", config.saveStateTitle, file);

	WritePrivateProfileInt(L"General Settings", L"Vista Volume", config.vistaVolume, file);
	WritePrivateProfileInt(L"General Settings", L"Volume", config.volume, file);

	for (int port=0; port<2; port++) {
		for (int slot=0; slot<4; slot++) {
			wchar_t temp[50];
			wsprintf(temp, L"Pad %i %i", port, slot);
			WritePrivateProfileInt(temp, L"Mode", config.padConfigs[port][slot].type, file);
			noError &= WritePrivateProfileInt(temp, L"Auto Analog", config.padConfigs[port][slot].autoAnalog, file);
		}
	}

	for (int i=0; i<dm->numDevices; i++) {
		wchar_t id[50];
		wchar_t temp[50], temp2[1000];
		wsprintfW(id, L"Device %i", i);
		Device *dev = dm->devices[i];
		wchar_t *name = dev->displayName;
		while (name[0] == '[') {
			wchar_t *name2 = wcschr(name, ']');
			if (!name2) break;
			name = name2+1;
			while (iswspace(name[0])) name++;
		}
		WritePrivateProfileStringW(id, L"Display Name", name, file);
		WritePrivateProfileStringW(id, L"Instance ID", dev->instanceID, file);
		if (dev->productID) {
			WritePrivateProfileStringW(id, L"Product ID", dev->productID, file);
		}
		WritePrivateProfileInt(id, L"API", dev->api, file);
		WritePrivateProfileInt(id, L"Type", dev->type, file);
		int ffBindingCount = 0;
		int bindingCount = 0;
		for (int port=0; port<2; port++) {
			for (int slot=0; slot<4; slot++) {
				for (int j=0; j<dev->pads[port][slot].numBindings; j++) {
					Binding *b = dev->pads[port][slot].bindings+j;
					VirtualControl *c = &dev->virtualControls[b->controlIndex];
					wsprintfW(temp, L"Binding %i", bindingCount++);
					wsprintfW(temp2, L"0x%08X, %i, %i, %i, %i, %i", c->uid, port, b->command, b->sensitivity, b->turbo, slot);
					noError &= WritePrivateProfileStringW(id, temp, temp2, file);
				}
				for (int j=0; j<dev->pads[port][slot].numFFBindings; j++) {
					ForceFeedbackBinding *b = dev->pads[port][slot].ffBindings+j;
					ForceFeedbackEffectType *eff = &dev->ffEffectTypes[b->effectIndex];
					wsprintfW(temp, L"FF Binding %i", ffBindingCount++);
					wsprintfW(temp2, L"%s %i, %i, %i", eff->effectID, port, b->motor, slot);
					for (int k=0; k<dev->numFFAxes; k++) {
						ForceFeedbackAxis *axis = dev->ffAxes + k;
						AxisEffectInfo *info = b->axes + k;
						wsprintfW(wcschr(temp2,0), L", %i, %i", axis->id, info->force);
					}
					noError &= WritePrivateProfileStringW(id, temp, temp2, file);
				}
			}
		}
	}
	if (!noError) {
		MessageBoxA(hWndProp, "Unable to save settings.  Make sure the disk is not full or write protected, the file isn't write protected, and that the app has permissions to write to the directory.  On Vista, try running in administrator mode.", "Error Writing Configuration File", MB_OK | MB_ICONERROR);
	}
	return !noError;
}

static int loaded = 0;

u8 GetPrivateProfileBool(wchar_t *s1, wchar_t *s2, int def, wchar_t *ini) {
	return (0!=GetPrivateProfileIntW(s1, s2, def, ini));
}

int LoadSettings(int force, wchar_t *file) {
	if (loaded && !force) return 0;
	CreateDirectory(L"inis", 0);
	// Could just do ClearDevices() instead, but if I ever add any extra stuff,
	// this will still work.
	UnloadConfigs();
	dm = new InputDeviceManager();

	wchar_t ini[MAX_PATH+20];
	GetSettingsFileName(ini);
	if (!file) {
		file = ini;
		GetPrivateProfileStringW(L"General Settings", L"Last Config Path", L"inis", config.lastSaveConfigPath, sizeof(config.lastSaveConfigPath), file);
		GetPrivateProfileStringW(L"General Settings", L"Last Config Name", L"LilyPad.lily", config.lastSaveConfigFileName, sizeof(config.lastSaveConfigFileName), file);
	}
	else {
		wchar_t *c = wcsrchr(file, '\\');
		if (c) {
			*c = 0;
			wcscpy(config.lastSaveConfigPath, file);
			wcscpy(config.lastSaveConfigFileName, c+1);
			*c = '\\';
			WritePrivateProfileStringW(L"General Settings", L"Last Config Path", config.lastSaveConfigPath, ini);
			WritePrivateProfileStringW(L"General Settings", L"Last Config Name", config.lastSaveConfigFileName, ini);
		}
	}

	config.GSThreadUpdates = GetPrivateProfileBool(L"General Settings", L"GS Thread Updates", 1, file);

	if (!ps2e) config.GSThreadUpdates = 0;

	config.escapeFullscreenHack = GetPrivateProfileBool(L"General Settings", L"Escape Fullscreen Hack", 1, file);

	config.disableScreenSaver = GetPrivateProfileBool(L"General Settings", L"Disable Screen Saver", 0, file);
	config.GH2 = GetPrivateProfileBool(L"General Settings", L"GH2", 0, file);

	config.mouseUnfocus = GetPrivateProfileBool(L"General Settings", L"Mouse Unfocus", 0, file);

	config.background = (u8)GetPrivateProfileIntW(L"General Settings", L"Background", 0, file);

	config.closeHacks = (u8)GetPrivateProfileIntW(L"General Settings", L"Close Hacks", 0, file);
	if (config.closeHacks&1) config.closeHacks &= ~2;
	config.debug = GetPrivateProfileBool(L"General Settings", L"Logging", 0, file);
	config.multipleBinding = GetPrivateProfileBool(L"General Settings", L"Multiple Bindings", 0, file);
	config.forceHide = GetPrivateProfileBool(L"General Settings", L"Force Cursor Hide", 0, file);

	config.keyboardApi = (DeviceAPI)GetPrivateProfileIntW(L"General Settings", L"Keyboard Mode", WM, file);
	config.mouseApi = (DeviceAPI) GetPrivateProfileIntW(L"General Settings", L"Mouse Mode", 0, file);
	config.gameApis.directInput = GetPrivateProfileBool(L"General Settings", L"DirectInput Game Devices", 1, file);
	config.gameApis.xInput = GetPrivateProfileBool(L"General Settings", L"XInput", 1, file);

	config.saveStateTitle = GetPrivateProfileBool(L"General Settings", L"Save State in Title", 1, file);

	config.vistaVolume = GetPrivateProfileBool(L"General Settings", L"Vista Volume", 1, file);
	config.volume = GetPrivateProfileInt(L"General Settings", L"Volume", 100, file);
	OSVERSIONINFO os;
	os.dwOSVersionInfoSize = sizeof(os);
	config.osVersion = 0;
	if (GetVersionEx(&os)) {
		config.osVersion = os.dwMajorVersion;
	}
	if (config.osVersion < 6) config.vistaVolume = 0;
	if (!config.vistaVolume) config.volume = 100;
	if (config.vistaVolume) SetVolume(config.volume);

	if (!InitializeRawInput()) {
		if (config.keyboardApi == RAW) config.keyboardApi = WM;
		if (config.mouseApi == RAW) config.mouseApi = WM;
	}

	if (config.debug) {
		HANDLE hFile = CreateFileA("logs\\padLog.txt", GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
		if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
		else CreateDirectory(L"logs", 0);
	}


	for (int port=0; port<2; port++) {
		for (int slot=0; slot<4; slot++) {
			wchar_t temp[50];
			wsprintf(temp, L"Pad %i %i", port, slot);
			config.padConfigs[port][slot].type = (PadType) GetPrivateProfileInt(temp, L"Mode", Dualshock2Pad, file);
			config.padConfigs[port][slot].autoAnalog = GetPrivateProfileBool(temp, L"Auto Analog", 0, file);
		}
	}

	loaded = 1;

	int i=0;
	int multipleBinding = config.multipleBinding;
	// Disabling multiple binding only prevents new multiple bindings.
	config.multipleBinding = 1;
	while (1) {
		wchar_t id[50];
		wchar_t temp[50], temp2[1000], temp3[1000], temp4[1000];
		wsprintfW(id, L"Device %i", i++);
		if (!GetPrivateProfileStringW(id, L"Display Name", 0, temp2, 1000, file) || !temp2[0] ||
			!GetPrivateProfileStringW(id, L"Instance ID", 0, temp3, 1000, file) || !temp3[0]) {
			if (i >= 100) break;
			continue;
		}
		wchar_t *id2 = 0;
		if (GetPrivateProfileStringW(id, L"Product ID", 0, temp4, 1000, file) && temp4[0])
			id2 = temp4;

		int api = GetPrivateProfileIntW(id, L"API", 0, file);
		int type = GetPrivateProfileIntW(id, L"Type", 0, file);
		if (!api || !type) continue;

		Device *dev = new Device((DeviceAPI)api, (DeviceType)type, temp2, temp3, id2);
		dev->attached = 0;
		dm->AddDevice(dev);
		int j = 0;
		int last = 0;
		while (1) {
			wsprintfW(temp, L"Binding %i", j++);
			if (!GetPrivateProfileStringW(id, temp, 0, temp2, 1000, file)) {
				if (j >= 100) {
					if (!last) break;
					last = 0;
				}
				continue;
			}
			last = 1;
			unsigned int uid;
			int port, command, sensitivity, turbo, slot = 0;
			int w = 0;
			char string[1000];
			while (temp2[w]) {
				string[w] = (char)temp2[w];
				w++;
			}
			string[w] = 0;
			int len = sscanf(string, " %i , %i , %i , %i , %i , %i", &uid, &port, &command, &sensitivity, &turbo, &slot);
			if (len >= 5 && type) {
				VirtualControl *c = dev->GetVirtualControl(uid);
				if (!c) c = dev->AddVirtualControl(uid, -1);
				if (c) {
					BindCommand(dev, uid, port, slot, command, sensitivity, turbo);
				}
			}
		}
		j = 0;
		while (1) {
			wsprintfW(temp, L"FF Binding %i", j++);
			if (!GetPrivateProfileStringW(id, temp, 0, temp2, 1000, file)) {
				if (j >= 10) {
					if (!last) break;
					last = 0;
				}
				continue;
			}
			last = 1;
			int port, slot, motor;
			int w = 0;
			char string[1000];
			char effect[1000];
			while (temp2[w]) {
				string[w] = (char)temp2[w];
				w++;
			}
			string[w] = 0;
			// wcstok not in ntdll.  More effore than its worth to shave off
			// whitespace without it.
			if (sscanf(string, " %s %i , %i , %i", effect, &port, &motor, &slot) == 4) {
				char *s = strchr(strchr(strchr(string, ',')+1, ',')+1, ',');
				if (!s) continue;
				s++;
				w = 0;
				while (effect[w]) {
					temp2[w] = effect[w];
					w++;
				}
				temp2[w] = 0;
				ForceFeedbackEffectType *eff = dev->GetForcefeedbackEffect(temp2);
				if (!eff) {
					// At the moment, don't record effect types.
					// Only used internally, anyways, so not an issue.
					dev->AddFFEffectType(temp2, temp2, EFFECT_CONSTANT);
					// eff = &dev->ffEffectTypes[dev->numFFEffectTypes-1];
				}
				ForceFeedbackBinding *b;
				CreateEffectBinding(dev, temp2, port, slot, motor, &b);
				if (b) {
					while (1) {
						int axisID = atoi(s);
						if (!(s = strchr(s, ','))) break;
						s++;
						int force = atoi(s);
						int i;
						for (i=0; i<dev->numFFAxes; i++) {
							if (axisID == dev->ffAxes[i].id) break;
						}
						if (i == dev->numFFAxes) {
							dev->AddFFAxis(L"?", axisID);
						}
						b->axes[i].force = force;
						if (!(s = strchr(s, ','))) break;
						s++;
					}
				}
			}
		}
	}
	config.multipleBinding = multipleBinding;

	RefreshEnabledDevicesAndDisplay(1);

	return 0;
}

inline int GetPort(HWND hWnd, int *slot) {
	for (int i=0; i<sizeof(hWnds)/sizeof(hWnds[0][0]); i++) {
		if (hWnds[i&1][i>>1] == hWnd) {
			*slot = i>>1;
			return i&1;
		}
	}
	*slot = 0;
	return 0;
}

void Diagnostics(HWND hWnd) {
	HWND hWndList = GetDlgItem(hWnd, IDC_LIST);
	if (!hWndList) return;
	int index = ListView_GetNextItem(hWndList, -1, LVNI_SELECTED);
	if (index < 0) return;
	LVITEM item;
	memset(&item, 0, sizeof(item));
	item.mask = LVIF_PARAM;
	item.iSubItem = 0;
	item.iItem = index;
	if (!ListView_GetItem(hWndList, &item)) return;
	Diagnose(item.lParam, hWnd);
	RefreshEnabledDevicesAndDisplay(0, hWnd, 1);
}

int GetBinding(int port, int slot, int index, Device *&dev, Binding *&b, ForceFeedbackBinding *&ffb) {
	ffb = 0;
	b = 0;
	for (int i = 0; i<dm->numDevices; i++) {
		dev = dm->devices[i];
		if (!dev->enabled) continue;
		if (index < dev->pads[port][slot].numBindings) {
			b = dev->pads[port][slot].bindings + index;
			return 1;
		}
		index -= dev->pads[port][slot].numBindings;

		if (index < dev->pads[port][slot].numFFBindings) {
			ffb = dev->pads[port][slot].ffBindings + index;
			return 1;
		}
		index -= dev->pads[port][slot].numFFBindings;
	}
	return 0;
}

// Only used when deleting things from ListView. Will remove from listview if needed.
void DeleteBinding(int port, int slot, Device *dev, Binding *b) {
	if (dev->enabled && hWnds[port][slot]) {
		int count = GetItemIndex(port, slot, dev, b);
		if (count >= 0) {
			HWND hWndList = GetDlgItem(hWnds[port][slot], IDC_LIST);
			if (hWndList) {
				ListView_DeleteItem(hWndList, count);
			}
		}
	}
	Binding *bindings = dev->pads[port][slot].bindings;
	int i = b - bindings;
	memmove(bindings+i, bindings+i+1, sizeof(Binding) * (dev->pads[port][slot].numBindings - i - 1));
	dev->pads[port][slot].numBindings--;
}

void DeleteBinding(int port, int slot, Device *dev, ForceFeedbackBinding *b) {
	if (dev->enabled && hWnds[port][slot]) {
		int count = GetItemIndex(port, slot, dev, b);
		if (count >= 0) {
			HWND hWndList = GetDlgItem(hWnds[port][slot], IDC_LIST);
			if (hWndList) {
				ListView_DeleteItem(hWndList, count);
			}
		}
	}
	ForceFeedbackBinding *bindings = dev->pads[port][slot].ffBindings;
	int i = b - bindings;
	memmove(bindings+i, bindings+i+1, sizeof(Binding) * (dev->pads[port][slot].numFFBindings - i - 1));
	dev->pads[port][slot].numFFBindings--;
}

int DeleteByIndex(int port, int slot, int index) {
	ForceFeedbackBinding *ffb;
	Binding *b;
	Device *dev;
	if (GetBinding(port, slot, index, dev, b, ffb)) {
		if (b) {
			DeleteBinding(port, slot, dev, b);
		}
		else {
			DeleteBinding(port, slot, dev, ffb);
		}
		return 1;
	}
	return 0;
}

int DeleteSelected(int port, int slot) {
	if (!hWnds[port][slot]) return 0;
	HWND hWnd = GetDlgItem(hWnds[port][slot], IDC_LIST);
	int changes = 0;
	while (1) {
		int index = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);
		if (index < 0) break;
		changes += DeleteByIndex(port, slot, index);
	}
	//ShowScrollBar(hWnd, SB_VERT, 1);
	return changes;
}

int CreateEffectBinding(Device *dev, wchar_t *effectID, unsigned int port, unsigned int slot, unsigned int motor, ForceFeedbackBinding **binding) {
	// Checks needed because I use this directly when loading bindings.
	// Note: dev->numFFAxes *can* be 0, for loading from file.
	*binding = 0;
	if (port > 1 || slot>3 || motor > 1 || !dev->numFFEffectTypes) {
		return -1;
	}
	if (!effectID) {
		effectID = dev->ffEffectTypes[0].effectID;
	}
	ForceFeedbackEffectType *eff = dev->GetForcefeedbackEffect(effectID);
	if (!eff) return -1;
	int effectIndex = eff - dev->ffEffectTypes;
	dev->pads[port][slot].ffBindings = (ForceFeedbackBinding*) realloc(dev->pads[port][slot].ffBindings, (dev->pads[port][slot].numFFBindings+1) * sizeof(ForceFeedbackBinding));
	int newIndex = dev->pads[port][slot].numFFBindings;
	while (newIndex && dev->pads[port][slot].ffBindings[newIndex-1].motor >= motor) {
		dev->pads[port][slot].ffBindings[newIndex] = dev->pads[port][slot].ffBindings[newIndex-1];
		newIndex--;
	}
	ForceFeedbackBinding *b = dev->pads[port][slot].ffBindings + newIndex;
	b->axes = (AxisEffectInfo*) calloc(dev->numFFAxes, sizeof(AxisEffectInfo));
	b->motor = motor;
	b->effectIndex = effectIndex;
	dev->pads[port][slot].numFFBindings++;
	if (binding) *binding = b;
	return ListBoundEffect(port, slot, dev, b);
}

int BindCommand(Device *dev, unsigned int uid, unsigned int port, unsigned int slot, int command, int sensitivity, int turbo) {
	// Checks needed because I use this directly when loading bindings.
	if (port > 1 || slot>3) {
		return -1;
	}
	if (!sensitivity) sensitivity = BASE_SENSITIVITY;
	// Relative axes can have negative sensitivity.
	else if (((uid>>16) & 0xFF) == RELAXIS) {
		sensitivity = abs(sensitivity);
	}
	VirtualControl *c = dev->GetVirtualControl(uid);
	if (!c) return -1;
	// Add before deleting.  Means I won't scroll up one line when scrolled down to bottom.
	int controlIndex = c - dev->virtualControls;
	int index = 0;
	PadBindings *p = dev->pads[port]+slot;
	p->bindings = (Binding*) realloc(p->bindings, (p->numBindings+1) * sizeof(Binding));
	for (index = p->numBindings; index > 0; index--) {
		if (p->bindings[index-1].controlIndex < controlIndex) break;
		p->bindings[index] = p->bindings[index-1];
	}
	Binding *b = p->bindings+index;
	p->numBindings++;
	b->command = command;
	b->controlIndex = controlIndex;
	b->turbo = turbo;
	b->sensitivity = sensitivity;
	// Where it appears in listview.
	int count = ListBoundCommand(port, slot, dev, b);

	int newBindingIndex = index;
	index = 0;
	while (index < p->numBindings) {
		if (index == newBindingIndex) {
			index ++;
			continue;
		}
		b = p->bindings + index;
		int nuke = 0;
		if (config.multipleBinding) {
			if (b->controlIndex == controlIndex && b->command == command)
				nuke = 1;
		}
		else {
			int uid2 = dev->virtualControls[b->controlIndex].uid;
			if (b->controlIndex == controlIndex || (!((uid2^uid) & 0xFFFFFF) && ((uid|uid2) & (UID_POV | UID_AXIS))))
				nuke = 1;
		}
		if (!nuke) {
			index++;
			continue;
		}
		if (index < newBindingIndex) {
			newBindingIndex--;
			count --;
		}
		DeleteBinding(port, slot, dev, b);
	}

	return count;
}

// Does nothing, but makes sure I'm overriding the dialog's window proc, to block
// default key handling.
ExtraWndProcResult DoNothingWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *output) {
	return CONTINUE_BLISSFULLY;
}

void EndBinding(HWND hWnd) {
	if (selected) {
		KillTimer(hWnd, 1);

		int needRefreshDevices = 0;
		// If binding ignore keyboard, this will disable it and enable other devices.
		// 0xFF is used for testing force feedback.  I disable all other devices first
		// just so I don't needlessly steal the mouse.
		if (selected == 0x7F || selected == 0xFF) {
			needRefreshDevices = 1;
		}
		selected = 0;

		dm->ReleaseInput();
		ClearKeyQueue();
		ReleaseEatenProc();

		// Safest to do this last.
		if (needRefreshDevices) {
			RefreshEnabledDevices();
		}
	}
}


INT_PTR CALLBACK DialogProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam) {
	int index = (hWnd == PropSheet_IndexToHwnd(hWndProp, 1));
	int slot;
	int port = GetPort(hWnd, &slot);
	HWND hWndList = GetDlgItem(hWnd, IDC_LIST);
	switch (msg) {
	case WM_INITDIALOG:
		{
			ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
			LVCOLUMN c;
			c.mask = LVCF_TEXT | LVCF_WIDTH;
			c.cx = 101;
			c.pszText = L"Device";
			ListView_InsertColumn(hWndList, 0, &c);
			c.cx = 70;
			c.pszText = L"PC Control";
			ListView_InsertColumn(hWndList, 1, &c);
			c.cx = 84;
			c.pszText = L"PS2 Control";
			ListView_InsertColumn(hWndList, 2, &c);
			selected = 0;
			port = (int)((PROPSHEETPAGE *)lParam)->lParam & 1;
			slot = (int)((PROPSHEETPAGE *)lParam)->lParam >> 1;
			hWnds[port][slot] = hWnd;
			SendMessage(hWndList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
			HWND hWndSlider = GetDlgItem(hWnd, IDC_SLIDER1);
			SetupLogSlider(hWndSlider);
			if (port || slot)
				EnableWindow(GetDlgItem(hWnd, ID_IGNORE), 0);

			Populate(port, slot);
		}
		break;
	case WM_DEVICECHANGE:
		if (wParam == DBT_DEVNODES_CHANGED) {
			EndBinding(hWnd);
			RefreshEnabledDevicesAndDisplay(1, hWndGeneral, 1);
		}
		break;
	case WM_TIMER:
		if (!selected || selected == 0xFF) {
			// !selected is mostly for device added/removed when binding.
			selected = 0xFF;
			EndBinding(hWnd);
		}
		else {
			unsigned int uid;
			int value;
			InitInfo info = {selected==0x7F, hWndProp, hWnd, GetDlgItem(hWnd, selected)};
			Device *dev = dm->GetActiveDevice(&info, &uid, &index, &value);
			if (dev) {
				int command = selected;
				// Good idea to do this first, as BindCommand modifies the ListView, which will
				// call it anyways, which is a bit funky.
				EndBinding(hWnd);
				UnselectAll(hWndList);
				int index = -1;
				if (command == 0x7F) {
					if (dev->api == IGNORE_KEYBOARD) {
						index = BindCommand(dev, uid, 0, 0, command, BASE_SENSITIVITY, 0);
					}
				}
				else if (command < 0x30) {
					if (!(uid & UID_POV)) {
						index = BindCommand(dev, uid, port, slot, command, BASE_SENSITIVITY, 0);
					}
				}
				if (index >= 0) {
					PropSheet_Changed(hWndProp, hWnds[port][slot]);
					if (index >= 0)
						ListView_SetItemState(hWndList, index, LVIS_SELECTED, LVIS_SELECTED);
				}
			}
		}
		break;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		EndBinding(hWnd);
		break;
	case PSM_CANCELTOCLOSE:
		// Load();
		break;
	case WM_ACTIVATE:
		EndBinding(hWnd);
		break;
	case WM_NOTIFY:
		{
			PSHNOTIFY* n = (PSHNOTIFY*) lParam;
			if (n->hdr.hwndFrom == hWndProp) {
				switch(n->hdr.code) {
				case PSN_QUERYCANCEL:
				case PSN_KILLACTIVE:
					EndBinding(hWnd);
					return 0;
				case PSN_SETACTIVE:
					return 0;
				case PSN_APPLY:
					SetWindowLong(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
					return 1;
				}
			}
			else if (n->hdr.idFrom == IDC_LIST) {
				static int NeedUpdate = 0;
				if (n->hdr.code == LVN_KEYDOWN) {
					NMLVKEYDOWN *key = (NMLVKEYDOWN *) n;
					if (key->wVKey == VK_DELETE ||
						key->wVKey == VK_BACK) {

						if (DeleteSelected(port, slot))
							PropSheet_Changed(hWndProp, hWnds[0]);
					}
				}
				// Update sensitivity and motor/binding display on redraw
				// rather than on itemchanged.  This reduces blinking, as
				// I get 3 LVN_ITEMCHANGED messages, and first is sent before
				// the new item is set as being selected.
				else if (n->hdr.code == LVN_ITEMCHANGED) {
					NeedUpdate = 1;
				}
				else if (n->hdr.code == NM_CUSTOMDRAW && NeedUpdate) {
					NeedUpdate = 0;
					SelChanged(port, slot);
				}
				EndBinding(hWnd);
			}
			else {
				EndBinding(hWnd);
			}
		}
		break;
	case WM_HSCROLL:
		{
			int id = GetDlgCtrlID((HWND)lParam);
			int val = GetLogSliderVal(hWnd, id);
			if (id == IDC_SLIDER1) {
				ChangeValue(port, slot, &val, 0);
			}
			else {
				ChangeEffect(port, slot, id, &val, 0);
			}
		}
		break;
	case WM_COMMAND:
		if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam) == IDC_AXIS_DIRECTION) {
			int index = ListView_GetNextItem(hWndList, -1, LVNI_SELECTED);
			if (index >= 0) {
				int cbsel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				if (cbsel >= 0) {
					ForceFeedbackBinding *ffb;
					Binding *b;
					Device *dev;
					if (GetBinding(port, slot, index, dev, b, ffb)) {
						int uid = dev->virtualControls[b->controlIndex].uid;
						const static unsigned int axisUIDs[3] = {UID_AXIS_NEG, UID_AXIS_POS, UID_AXIS};
						uid = (uid&0x00FFFFFF) | axisUIDs[cbsel];
						Binding backup = *b;
						DeleteSelected(port, slot);
						int index = BindCommand(dev, uid, port, slot, backup.command, backup.sensitivity, backup.turbo);
						ListView_SetItemState(hWndList, index, LVIS_SELECTED, LVIS_SELECTED);
					}
				}
			}
		}
		else if (HIWORD(wParam)==CBN_SELCHANGE && LOWORD(wParam) == IDC_FF_EFFECT) {
			unsigned int typeIndex = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
			if (typeIndex >= 0)
				ChangeEffect(port, slot, 0, 0, &typeIndex);
		}
		else if (HIWORD(wParam)==BN_CLICKED) {
			EndBinding(hWnd);
			int cmd = LOWORD(wParam);
			if (cmd == ID_DELETE) {
				if (DeleteSelected(port, slot))
					PropSheet_Changed(hWndProp, hWnd);
			}
			else if (cmd == ID_CLEAR) {
				int changed=0;
				while (DeleteByIndex(port, slot, 0)) changed++;
				if (changed)
					PropSheet_Changed(hWndProp, hWnd);
			}
			else if (cmd == ID_BIG_MOTOR || cmd == ID_SMALL_MOTOR) {
				int i = (int)SendMessage(GetDlgItem(hWnd, IDC_FORCEFEEDBACK), CB_GETCURSEL, 0, 0);
				if (i >= 0) {
					unsigned int index = (unsigned int)SendMessage(GetDlgItem(hWnd, IDC_FORCEFEEDBACK), CB_GETITEMDATA, i, 0);
					if (index < (unsigned int) dm->numDevices) {
						ForceFeedbackBinding *b;
						int count = CreateEffectBinding(dm->devices[index], 0, port, slot, cmd-ID_BIG_MOTOR, &b);
						if (b) {
							for (int j=0; j<2 && j <dm->devices[index]->numFFAxes; j++) {
								b->axes[j].force = BASE_SENSITIVITY;
							}
						}
						if (count >= 0) {
							PropSheet_Changed(hWndProp, hWnd);
							UnselectAll(hWndList);
							ListView_SetItemState(hWndList, count, LVIS_SELECTED, LVIS_SELECTED);
						}
					}
				}
			}
			else if (cmd == ID_CONTROLS) {
				UnselectAll(hWndList);
			}
			else if (cmd == ID_TEST) {
				Device *dev;
				Binding *b;
				ForceFeedbackBinding *ffb = 0;
				int selIndex = ListView_GetNextItem(hWndList, -1, LVNI_SELECTED);
				if (selIndex >= 0) {
					if (GetBinding(port, slot, selIndex, dev, b, ffb)) {
						selected = 0xFF;
						InitInfo info = {0, hWndProp, hWnd, GetDlgItem(hWnd, cmd)};
						EatWndProc(info.hWndButton, DoNothingWndProc);
						for (int i=0; i<dm->numDevices; i++) {
							if (dm->devices[i] != dev) {
								dm->DisableDevice(i);
							}
						}
						dm->Update(&info);
						dm->PostRead();
						dev->SetEffect(ffb, 255);
						SetTimer(hWnd, 1, 3000, 0);
					}
				}
			}
			else if ((cmd >= ID_GUITAR_HERO && cmd <= ID_ANALOG) || cmd == ID_IGNORE) {// || cmd == ID_FORCE_FEEDBACK) {
				// Messes up things, unfortunately.
				// End binding on a bunch of notification messages, and
				// this will send a bunch.
				// UnselectAll(hWndList);
				EndBinding(hWnd);
				if (cmd != ID_IGNORE) {
					selected = cmd-(ID_SELECT-0x10);
				}
				else {
					selected = 0x7F;
					for (int i=0; i<dm->numDevices; i++) {
						if (dm->devices[i]->api != IGNORE_KEYBOARD) {
							dm->DisableDevice(i);
						}
						else {
							dm->EnableDevice(i);
						}
					}
				}

				InitInfo info = {selected==0x7F, hWndProp, hWnd, GetDlgItem(hWnd, cmd)};
				EatWndProc(info.hWndButton, DoNothingWndProc);
				int w = timeGetTime();
				dm->Update(&info);
				dm->PostRead();
				// Workaround for things that return 0 on first poll and something else ever after.
				Sleep(40);
				dm->Update(&info);
				dm->PostRead();
				int w2 = timeGetTime();
				if (dm->devices[0xe]->oldVirtualControlState[6] != 0x8000) {
					dm->devices[0xe]->oldVirtualControlState[6]=dm->devices[0xe]->oldVirtualControlState[6];
				}
				SetTimer(hWnd, 1, 30, 0);
			}
			if (cmd == IDC_TURBO) {
				// Don't allow setting it back to indeterminate.
				SendMessage(GetDlgItem(hWnd, IDC_TURBO), BM_SETSTYLE, BS_AUTOCHECKBOX, 0);
				int turbo = (IsDlgButtonChecked(hWnd, IDC_TURBO) == BST_CHECKED);
				ChangeValue(port, slot, 0, &turbo);
			}
			else if (cmd == IDC_FLIP1 || cmd == IDC_TURBO) {
				int val = GetLogSliderVal(hWnd, IDC_SLIDER1);
				ChangeValue(port, slot, &val, 0);
			}
			else if (cmd >= IDC_FF_AXIS1_ENABLED && cmd < IDC_FF_AXIS8_ENABLED + 16) {
				int index = (cmd - IDC_FF_AXIS1_ENABLED)/16;
				int val = GetLogSliderVal(hWnd, 16*index + IDC_FF_AXIS1);
				if (IsDlgButtonChecked(hWnd, 16*index + IDC_FF_AXIS1_ENABLED) != BST_CHECKED) {
					val = 0;
				}
				ChangeEffect(port, slot, cmd, &val, 0);
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

void UpdatePadPages() {
	HPROPSHEETPAGE pages[10];
	int count = 0;
	memset(hWnds, 0, sizeof(hWnds));
	int slot = 0;
	for (int port=0; port<2; port++) {
		if (config.padConfigs[port][slot].type == DisabledPad) continue;
		PROPSHEETPAGE psp;
		ZeroMemory(&psp, sizeof(psp));
		psp.dwSize = sizeof(psp);
		psp.dwFlags = PSP_USETITLE | PSP_PREMATURE;
		psp.hInstance = hInst;
		psp.pfnDlgProc = (DLGPROC) DialogProc;
		psp.lParam = port | (slot<<1);
		if (port == 0)
			psp.pszTitle = L"Pad 1";
		else
			psp.pszTitle = L"Pad 2";
		if (config.padConfigs[port][slot].type != GuitarPad)
			psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG);
		else
			psp.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG_GUITAR);
		pages[count] = CreatePropertySheetPage(&psp);
		if (pages[count]) count++;
	}
	while (SendMessage(hWndProp, PSM_INDEXTOPAGE, 1, 0)) {
		PropSheet_RemovePage(hWndProp, 1, 0);
	}
	for (int i=0; i<count; i++) {
		PropSheet_AddPage(hWndProp, pages[i]);
	}
}

INT_PTR CALLBACK GeneralDialogProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam);

HPROPSHEETPAGE CreateGeneralPage() {
	PROPSHEETPAGE psp;
	ZeroMemory(&psp, sizeof(psp));
	psp.dwSize = sizeof(psp);
	psp.dwFlags = PSP_USETITLE | PSP_PREMATURE;
	psp.hInstance = hInst;
	psp.pfnDlgProc = (DLGPROC) GeneralDialogProc;
	psp.pszTitle = L"General";
	psp.pszTemplate = MAKEINTRESOURCE(IDD_GENERAL);
	return CreatePropertySheetPage(&psp);
}

void UpdatePadList(HWND hWnd) {
	HWND hWndList = GetDlgItem(hWnd, IDC_PAD_LIST);
	HWND hWndCombo = GetDlgItem(hWnd, IDC_PAD_TYPE);
	HWND hWndAnalog = GetDlgItem(hWnd, IDC_ANALOG_START1);
	int slot = 0;
	int index = 0;
	for (int port=0; port<2; port++) {
		wchar_t text[100];
		wsprintf(text, L"Pad %i", port);
		LVITEM item;
		item.iItem = index;
		item.iSubItem = 0;
		item.mask = LVIF_TEXT;
		item.pszText = text;
		if (SendMessage(hWndList, LVM_GETITEMCOUNT, 0, 0) <= index) {
			ListView_InsertItem(hWndList, &item);
		}
		else {
			ListView_SetItem(hWndList, &item);
		}

		item.iSubItem = 1;
		wchar_t *strings[] = {L"Disabled", L"Dualshock 2", L"Guitar"};
		if (2 < (unsigned int)config.padConfigs[port][slot].type) config.padConfigs[port][slot].type = Dualshock2Pad;
		item.pszText = strings[config.padConfigs[port][slot].type];
		ListView_SetItem(hWndList, &item);

		item.iSubItem = 2;
		int count = 0;
		for (int i = 0; i<dm->numDevices; i++) {
			Device *dev = dm->devices[i];
			if (!dev->enabled) continue;
			count += dev->pads[port][slot].numBindings + dev->pads[port][slot].numFFBindings;
		}
		wsprintf(text, L"%i", count);
		item.pszText = text;
		ListView_SetItem(hWndList, &item);
		index++;
	}
	int sel = ListView_GetNextItem(hWndList, -1, LVNI_SELECTED);

	EnableWindow(hWndCombo, sel>=0);
	EnableWindow(hWndAnalog, sel>=0);
	if (sel < 0) {
		SendMessage(hWndCombo, CB_SETCURSEL, -1, 0);
		CheckDlgButton(hWnd, IDC_ANALOG_START1, BST_UNCHECKED);
	}
	else {
		SendMessage(hWndCombo, CB_SETCURSEL, config.padConfigs[sel][0].type, 0);
		CheckDlgButton(hWnd, IDC_ANALOG_START1, BST_CHECKED*config.padConfigs[sel][0].autoAnalog);
	}
}

INT_PTR CALLBACK GeneralDialogProc(HWND hWnd, unsigned int msg, WPARAM wParam, LPARAM lParam) {
	int i;
	HWND hWndList = GetDlgItem(hWnd, IDC_PAD_LIST);
	switch (msg) {
	case WM_INITDIALOG:
		{
			HWND hWndCombo = GetDlgItem(hWnd, IDC_PAD_TYPE);
			if (SendMessage(hWndCombo, CB_GETCOUNT, 0, 0) == 0) {
				ListView_SetExtendedListViewStyleEx(hWndList, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
				LVCOLUMN c;
				c.mask = LVCF_TEXT | LVCF_WIDTH;
				c.cx = 50;
				c.pszText = L"Pad";
				ListView_InsertColumn(hWndList, 0, &c);
				c.cx = 90;
				c.pszText = L"Type";
				ListView_InsertColumn(hWndList, 1, &c);
				c.cx = 70;
				c.pszText = L"Bindings";
				ListView_InsertColumn(hWndList, 2, &c);
				selected = 0;
				SendMessage(hWndList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
				SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) L"Disabled");
				SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) L"Dualshock 2");
				SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) L"Guitar");
			}
		}
		UpdatePadList(hWnd);
		UpdatePadPages();
		hWndGeneral = hWnd;
		RefreshEnabledDevicesAndDisplay(0, hWnd, 0);

		CheckDlgButton(hWnd, IDC_BACKGROUND, BST_CHECKED * config.background);
		CheckDlgButton(hWnd, IDC_FORCE_HIDE, BST_CHECKED * config.forceHide);
		CheckDlgButton(hWnd, IDC_CLOSE_HACK1, BST_CHECKED * (config.closeHacks&1));
		CheckDlgButton(hWnd, IDC_CLOSE_HACK2, BST_CHECKED * ((config.closeHacks&2)>>1));
		CheckDlgButton(hWnd, IDC_CLOSE_HACK3, BST_CHECKED * ((config.closeHacks&4)>>2));
		CheckDlgButton(hWnd, IDC_MOUSE_UNFOCUS, BST_CHECKED * config.mouseUnfocus);

		CheckDlgButton(hWnd, IDC_GS_THREAD_INPUT, BST_CHECKED * config.GSThreadUpdates);
		if (!ps2e) {
			EnableWindow(GetDlgItem(hWnd, IDC_GS_THREAD_INPUT), 0);
		}
		CheckDlgButton(hWnd, IDC_ESCAPE_FULLSCREEN_HACK, BST_CHECKED * config.escapeFullscreenHack);

		CheckDlgButton(hWnd, IDC_DISABLE_SCREENSAVER, BST_CHECKED * config.disableScreenSaver);
		CheckDlgButton(hWnd, IDC_GH2_HACK, BST_CHECKED * config.GH2);
		CheckDlgButton(hWnd, IDC_SAVE_STATE_TITLE, BST_CHECKED * config.saveStateTitle);

		CheckDlgButton(hWnd, IDC_VISTA_VOLUME, BST_CHECKED * config.vistaVolume);
		if (config.osVersion < 6) EnableWindow(GetDlgItem(hWnd, IDC_VISTA_VOLUME), 0);

		CheckDlgButton(hWnd, IDC_DEBUG_FILE, BST_CHECKED * config.debug);
		CheckDlgButton(hWnd, IDC_MULTIPLE_BINDING, BST_CHECKED * config.multipleBinding);


		if (config.keyboardApi < 0 || config.keyboardApi > 3) config.keyboardApi = NO_API;
		CheckRadioButton(hWnd, IDC_KB_DISABLE, IDC_KB_RAW, IDC_KB_DISABLE + config.keyboardApi);
		if (config.mouseApi < 0 || config.mouseApi > 3) config.mouseApi = NO_API;
		CheckRadioButton(hWnd, IDC_M_DISABLE, IDC_M_RAW, IDC_M_DISABLE + config.mouseApi);
		CheckDlgButton(hWnd, IDC_G_DI, BST_CHECKED * config.gameApis.directInput);
		CheckDlgButton(hWnd, IDC_G_XI, BST_CHECKED * config.gameApis.xInput);


		if (!InitializeRawInput()) {
			EnableWindow(GetDlgItem(hWnd, IDC_KB_RAW), 0);
			EnableWindow(GetDlgItem(hWnd, IDC_M_RAW), 0);
		}
		break;
	case WM_DEVICECHANGE:
		if (wParam == DBT_DEVNODES_CHANGED) {
			RefreshEnabledDevicesAndDisplay(1, hWndGeneral, 1);
		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_PAD_TYPE) {
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				int pad = ListView_GetNextItem(hWndList, -1, LVNI_SELECTED);
				HWND hWndCombo = GetDlgItem(hWnd, IDC_PAD_TYPE);
				int sel = SendMessage(hWndCombo, CB_GETCURSEL, 0, 0);
				if (pad >= 0 && sel >= 0 && sel != config.padConfigs[sel][0].type) {
					config.padConfigs[sel][0].type = (PadType)sel;
					PropSheet_Changed(hWndProp, hWnd);
				}
			}
		}
		else if (HIWORD(wParam)==BN_CLICKED && (LOWORD(wParam) == ID_LOAD || LOWORD(wParam) == ID_SAVE)) {
			OPENFILENAMEW ofn;
			memset (&ofn, 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hWnd;
			ofn.lpstrFilter = L"LilyPad Config Files\0*.lily\0All Files\0*.*\0\0";
			wchar_t file[MAX_PATH+1];
			ofn.lpstrFile = file;
			ofn.nMaxFile = MAX_PATH;
			wcscpy(file, config.lastSaveConfigFileName);
			ofn.lpstrInitialDir = config.lastSaveConfigPath;
			ofn.Flags = OFN_DONTADDTORECENT | OFN_LONGNAMES | OFN_NOCHANGEDIR;
			if (LOWORD(wParam) == ID_LOAD) {
				ofn.lpstrTitle = L"Load LilyPad Configuration";
				ofn.Flags |= OFN_FILEMUSTEXIST;
				if (GetOpenFileNameW(&ofn)) {
					LoadSettings(1, ofn.lpstrFile);
					GeneralDialogProc(hWnd, WM_INITDIALOG, 0, 0);
					PropSheet_Changed(hWndProp, hWnd);
				}
			}
			else {
				ofn.lpstrTitle = L"Save LilyPad Configuration";
				ofn.Flags |= OFN_OVERWRITEPROMPT;
				if (GetSaveFileNameW(&ofn)) {
					if (SaveSettings(ofn.lpstrFile) == -1) {
						MessageBox(hWnd, L"Save fail", L"Couldn't save file.", MB_OK);
					}
				}
			}
			break;
		}

		else if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam) == ID_TEST) {
			Diagnostics(hWnd);
			RefreshEnabledDevices();
		}
		else if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam) == ID_REFRESH) {
			RefreshEnabledDevicesAndDisplay(1, hWnd, 1);
		}
		else if (HIWORD(wParam)==BN_CLICKED && LOWORD(wParam) == IDC_ANALOG_START1) {
			int pad = ListView_GetNextItem(hWndList, -1, LVNI_SELECTED);
			if (pad >= 0) {
				config.padConfigs[pad][0].autoAnalog = (IsDlgButtonChecked(hWnd, IDC_ANALOG_START1) == BST_CHECKED);
				PropSheet_Changed(hWndProp, hWnd);
			}
		}
		else {
			int t = IDC_CLOSE_HACK1;
			int test = LOWORD(wParam);
			if (test == IDC_CLOSE_HACK1) {
				CheckDlgButton(hWnd, IDC_CLOSE_HACK2, BST_UNCHECKED);
			}
			else if (test == IDC_CLOSE_HACK2) {
				CheckDlgButton(hWnd, IDC_CLOSE_HACK1, BST_UNCHECKED);
			}

			config.forceHide = (IsDlgButtonChecked(hWnd, IDC_FORCE_HIDE) == BST_CHECKED);
			config.closeHacks = (IsDlgButtonChecked(hWnd, IDC_CLOSE_HACK1) == BST_CHECKED) |
				((IsDlgButtonChecked(hWnd, IDC_CLOSE_HACK2) == BST_CHECKED)<<1) |
				((IsDlgButtonChecked(hWnd, IDC_CLOSE_HACK3) == BST_CHECKED)<<2);
			config.background = (IsDlgButtonChecked(hWnd, IDC_BACKGROUND) == BST_CHECKED);
			config.mouseUnfocus = (IsDlgButtonChecked(hWnd, IDC_MOUSE_UNFOCUS) == BST_CHECKED);

			config.GSThreadUpdates = (IsDlgButtonChecked(hWnd, IDC_GS_THREAD_INPUT) == BST_CHECKED);
			config.escapeFullscreenHack = (IsDlgButtonChecked(hWnd, IDC_ESCAPE_FULLSCREEN_HACK) == BST_CHECKED);

			config.disableScreenSaver = (IsDlgButtonChecked(hWnd, IDC_DISABLE_SCREENSAVER) == BST_CHECKED);
			config.GH2 = (IsDlgButtonChecked(hWnd, IDC_GH2_HACK) == BST_CHECKED);
			config.saveStateTitle = (IsDlgButtonChecked(hWnd, IDC_SAVE_STATE_TITLE) == BST_CHECKED);

			u8 newVistaVolume = (IsDlgButtonChecked(hWnd, IDC_VISTA_VOLUME) == BST_CHECKED);
			if (config.vistaVolume != newVistaVolume) {
				config.vistaVolume = newVistaVolume;
				SetVolume(100);
			}


			config.debug = (IsDlgButtonChecked(hWnd, IDC_DEBUG_FILE) == BST_CHECKED);
			config.multipleBinding = (IsDlgButtonChecked(hWnd, IDC_MULTIPLE_BINDING) == BST_CHECKED);

			for (i=0; i<4; i++) {
				if (IsDlgButtonChecked(hWnd, IDC_KB_DISABLE+i) == BST_CHECKED) {
					if (i != NO_API || config.keyboardApi == NO_API || IDOK == MessageBoxA(hWnd, 
						"Disabling keyboard input will prevent LilyPad from passing any\n"
						"keyboard input on to PCSX2.\n"
						"\n"
						"This is only meant to be used if you're using two different\n"
						"pad plugins. If both pads are set to LilyPad, then GS and PCSX2   \n"
						"keyboard shortcuts will not work.\n"
						"\n"
						"Are you sure you want to do this?", "Warning", MB_OKCANCEL | MB_ICONWARNING)) {
						
							config.keyboardApi = (DeviceAPI)i;
					}
					CheckRadioButton(hWnd, IDC_KB_DISABLE, IDC_KB_RAW, IDC_KB_DISABLE + config.keyboardApi);
				}
				if (IsDlgButtonChecked(hWnd, IDC_M_DISABLE+i) == BST_CHECKED) {
					config.mouseApi = (DeviceAPI)i;
				}
			}
			config.gameApis.directInput = (IsDlgButtonChecked(hWnd, IDC_G_DI) == BST_CHECKED);
			config.gameApis.xInput = (IsDlgButtonChecked(hWnd, IDC_G_XI) == BST_CHECKED);

			config.forceHide = (IsDlgButtonChecked(hWnd, IDC_FORCE_HIDE) == BST_CHECKED);

			RefreshEnabledDevicesAndDisplay(0, hWnd, 1);

			PropSheet_Changed(hWndProp, hWnd);
			/*
			if (needUpdate) {
				UpdatePadPages();
			}//*/
		}
		break;
	case WM_NOTIFY:
		{
			PSHNOTIFY* n = (PSHNOTIFY*) lParam;
			if (n->hdr.hwndFrom == hWndProp) {
				switch(n->hdr.code) {
				case PSN_QUERYCANCEL:
				case PSN_KILLACTIVE:
					EndBinding(hWnd);
					return 0;
				case PSN_SETACTIVE:
					//selected = 0;
					return 0;
				case PSN_APPLY:
					selected = 0;
					if (SaveSettings()) {
						SetWindowLong(hWnd, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
						return 0;
					}
					SetWindowLong(hWnd, DWL_MSGRESULT, PSNRET_NOERROR);
					return 1;
				}
			}
			else if (n->hdr.idFrom == IDC_LIST && n->hdr.code == NM_DBLCLK) {
				Diagnostics(hWnd);
			}
			else if (n->hdr.idFrom == IDC_PAD_LIST) {
				if (n->hdr.code == NM_CLICK) {
					UpdatePadList(hWnd);
				}
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

int CALLBACK PropSheetProc(HWND hWnd, UINT msg, LPARAM lParam) {
	hWndProp = hWnd;
	return 0;
}

void CALLBACK PADconfigure() {
	// Can end up here without PadConfigure() being called first.
	LoadSettings();
	memset(hWnds, 0, sizeof(hWnds));

	HPROPSHEETPAGE page;
	page = CreateGeneralPage();
	if (!page) return;

	PROPSHEETHEADER psh;
	ZeroMemory(&psh, sizeof(psh));
	psh.dwFlags = PSH_DEFAULT | PSH_USECALLBACK | PSH_NOCONTEXTHELP;
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.pfnCallback = PropSheetProc;
	psh.hwndParent = GetActiveWindow();
	psh.nPages = 1;
	psh.phpage = &page;
	wchar_t title[200];
	GetNameAndVersionString(title);
	wcscat(title, L" Settings");
	psh.pszCaption = title;
	PropertySheet(&psh);
	LoadSettings(1);
	memset(hWnds, 0, sizeof(hWnds));
}

void UnloadConfigs() {
	loaded = 0;
	if (dm) {
		delete dm;
		dm = 0;
	}
}
