#include "Global.h"
#include "InputManager.h"
#include "KeyboardQueue.h"

InputDeviceManager *dm = 0;

InputDeviceManager::InputDeviceManager() {
	memset(this, 0, sizeof(*this));
}

void InputDeviceManager::ClearDevices() {
	for (int i=0; i<numDevices; i++) {
		delete devices[i];
	}
	free(devices);
	devices = 0;
	numDevices = 0;
}

InputDeviceManager::~InputDeviceManager() {
	ClearDevices();
}

Device::Device(DeviceAPI api, DeviceType d, const wchar_t *displayName, const wchar_t *instanceID, wchar_t *productID) {
	memset(pads, 0, sizeof(pads));
	this->api = api;
	type = d;
	this->displayName = wcsdup(displayName);
	if (instanceID)
		this->instanceID = wcsdup(instanceID);
	else
		this->instanceID = wcsdup(displayName);
	this->productID = 0;
	if (productID)
		this->productID = wcsdup(productID);
	active = 0;
	attached = 1;
	enabled = 0;

	hWndProc = 0;

	virtualControls = 0;
	numVirtualControls = 0;
	virtualControlState = 0;
	oldVirtualControlState = 0;

	physicalControls = 0;
	numPhysicalControls = 0;
	physicalControlState = 0;

	ffEffectTypes = 0;
	numFFEffectTypes = 0;
	ffAxes = 0;
	numFFAxes = 0;
}

void Device::FreeState() {
	if (virtualControlState) free(virtualControlState);
	virtualControlState = 0;
	oldVirtualControlState = 0;
	physicalControlState = 0;
}

Device::~Device() {
	Deactivate();
	// Generally called by deactivate, but just in case...
	FreeState();
	int i;
	for (int port=0; port<2; port++) {
		for (int slot=0; slot<4; slot++) {
			free(pads[port][slot].bindings);
			for (i=0; i<pads[port][slot].numFFBindings; i++) {
				free(pads[port][slot].ffBindings[i].axes);
			}
			free(pads[port][slot].ffBindings);
		}
	}
	free(virtualControls);

	for (i=numPhysicalControls-1; i>=0; i--) {
		if (physicalControls[i].name) free(physicalControls[i].name);
	}
	free(physicalControls);

	free(displayName);
	free(instanceID);
	free(productID);
	if (ffAxes) {
		for (i=0; i<numFFAxes; i++) {
			free(ffAxes[i].displayName);
		}
		free(ffAxes);
	}
	if (ffEffectTypes) {
		for (i=0; i<numFFEffectTypes; i++) {
			free(ffEffectTypes[i].displayName);
			free(ffEffectTypes[i].effectID);
		}
		free(ffEffectTypes);
	}
}

void Device::AddFFEffectType(const wchar_t *displayName, const wchar_t *effectID, EffectType type) {
	ffEffectTypes = (ForceFeedbackEffectType*) realloc(ffEffectTypes, sizeof(ForceFeedbackEffectType) * (numFFEffectTypes+1));
	ffEffectTypes[numFFEffectTypes].displayName = wcsdup(displayName);
	ffEffectTypes[numFFEffectTypes].effectID = wcsdup(effectID);
	ffEffectTypes[numFFEffectTypes].type = type;
	numFFEffectTypes++;
}

void Device::AddFFAxis(const wchar_t *displayName, int id) {
	ffAxes = (ForceFeedbackAxis*) realloc(ffAxes, sizeof(ForceFeedbackAxis) * (numFFAxes+1));
	ffAxes[numFFAxes].id = id;
	ffAxes[numFFAxes].displayName = wcsdup(displayName);
	numFFAxes++;
	int bindingsExist = 0;
	for (int port=0; port<2; port++) {
		for (int slot=0; slot<4; slot++) {
			for (int i=0; i<pads[port][slot].numFFBindings; i++) {
				ForceFeedbackBinding *b = pads[port][slot].ffBindings+i;
				b->axes = (AxisEffectInfo*) realloc(b->axes, sizeof(AxisEffectInfo) * (numFFAxes));
				memset(b->axes + (numFFAxes-1), 0, sizeof(AxisEffectInfo));
				bindingsExist = 1;
			}
		}
	}
	// Generally the case when not loading a binding file.
	if (!bindingsExist) {
		int i = numFFAxes-1;
		ForceFeedbackAxis temp = ffAxes[i];
		while (i && temp.id < ffAxes[i-1].id) {
			ffAxes[i] = ffAxes[i-1];
			i--;
		}
		ffAxes[i] = temp;
	}
}

void Device::AllocState() {
	FreeState();
	virtualControlState = (int*) calloc(numVirtualControls + numVirtualControls + numPhysicalControls, sizeof(int));
	oldVirtualControlState = virtualControlState + numVirtualControls;
	physicalControlState = oldVirtualControlState + numVirtualControls;
}

void Device::FlipState() {
	memcpy(oldVirtualControlState, virtualControlState, sizeof(int)*numVirtualControls);
}

void Device::PostRead() {
	FlipState();
}

void Device::CalcVirtualState() {
	for (int i=0; i<numPhysicalControls; i++) {
		PhysicalControl *c = physicalControls+i;
		int index = c->baseVirtualControlIndex;
		int val = physicalControlState[i];
		if (c->type & BUTTON) {
			virtualControlState[index] = val;
			// DirectInput keyboard events only.
			if (this->api == DI && this->type == KEYBOARD) {
				if (!(virtualControlState[index]>>15) != !(oldVirtualControlState[index]>>15) && c->vkey) {
					// Check for alt-F4 to avoid toggling skip mode incorrectly.
					if (c->vkey == VK_F4) {
						int i;
						for (i=0; i<numPhysicalControls; i++) {
							if (virtualControlState[physicalControls[i].baseVirtualControlIndex] &&
								(physicalControls[i].vkey == VK_MENU ||
								 physicalControls[i].vkey == VK_RMENU ||
								 physicalControls[i].vkey == VK_LMENU)) {
									break;
							}
						}
						if (i<numPhysicalControls) continue;
					}
					int event = KEYPRESS;
					if (!(virtualControlState[index]>>15)) event = KEYRELEASE;
					QueueKeyEvent(c->vkey, event);
				}
			}
		}
		else if (c->type & ABSAXIS) {
			virtualControlState[index] = (val + FULLY_DOWN)/2;
			// Positive.  Overkill.
			virtualControlState[index+1] = (val & ~(val>>31));
			// Negative
			virtualControlState[index+2] = (-val & (val>>31));
		}
		else if (c->type & RELAXIS) {
			int delta = val - oldVirtualControlState[index];
			virtualControlState[index] = val;
			// Positive
			virtualControlState[index+1] = (delta & ~(delta>>31));
			// Negative
			virtualControlState[index+2] = (-delta & (delta>>31));
		}
		else if (c->type & POV) {
			virtualControlState[index] = val;
			int iSouth = 0;
			int iEast = 0;
			if ((unsigned int)val <= 37000) {
				double angle = val * (3.141592653589793/18000.0);
				double East = sin(angle);
				double South = -cos(angle);
				// Normalize so greatest direction is 1.
				double mul = FULLY_DOWN / max(fabs(South), fabs(East));
				iEast = (int) floor(East * mul + 0.5);
				iSouth = (int) floor(South * mul + 0.5);
			}
			// N
			virtualControlState[index+1] = (-iSouth & (iSouth>>31));
			// S
			virtualControlState[index+3] = (iSouth & ~(iSouth>>31));
			// E
			virtualControlState[index+2] = (iEast & ~(iEast>>31));
			// W
			virtualControlState[index+4] = (-iEast & (iEast>>31));
		}
	}
}

VirtualControl *Device::GetVirtualControl(unsigned int uid) {
	for (int i=0; i<numVirtualControls; i++) {
		if (virtualControls[i].uid == uid)
			return virtualControls + i;
	}
	return 0;
}

VirtualControl *Device::AddVirtualControl(unsigned int uid, int physicalControlIndex) {
	// Not really necessary, as always call AllocState when activated, but doesn't hurt.
	FreeState();

	if (numVirtualControls % 16 == 0) {
		virtualControls = (VirtualControl*) realloc(virtualControls, sizeof(VirtualControl)*(numVirtualControls+16));
	}
	VirtualControl *c = virtualControls + numVirtualControls;

	c->uid = uid;
	c->physicalControlIndex = physicalControlIndex;

	numVirtualControls++;
	return c;
}

PhysicalControl *Device::AddPhysicalControl(ControlType type, unsigned short id, unsigned short vkey, const wchar_t *name) {
	// Not really necessary, as always call AllocState when activated, but doesn't hurt.
	FreeState();

	if (numPhysicalControls % 16 == 0) {
		physicalControls = (PhysicalControl*) realloc(physicalControls, sizeof(PhysicalControl)*(numPhysicalControls+16));
	}
	PhysicalControl *control = physicalControls + numPhysicalControls;

	memset(control, 0, sizeof(PhysicalControl));
	control->type = type;
	control->id = id;
	if (name) control->name = wcsdup(name);
	control->baseVirtualControlIndex = numVirtualControls;
	unsigned int uid = id | (type<<16);
	if (type & BUTTON) {
		AddVirtualControl(uid, numPhysicalControls);
		control->vkey = vkey;
	}
	else if (type & AXIS) {
		AddVirtualControl(uid | UID_AXIS, numPhysicalControls);
		AddVirtualControl(uid | UID_AXIS_POS, numPhysicalControls);
		AddVirtualControl(uid | UID_AXIS_NEG, numPhysicalControls);
	}
	else if (type & POV) {
		AddVirtualControl(uid | UID_POV, numPhysicalControls);
		AddVirtualControl(uid | UID_POV_N, numPhysicalControls);
		AddVirtualControl(uid | UID_POV_E, numPhysicalControls);
		AddVirtualControl(uid | UID_POV_S, numPhysicalControls);
		AddVirtualControl(uid | UID_POV_W, numPhysicalControls);
	}
	numPhysicalControls++;
	return control;
}

void Device::SetEffects(unsigned char port, unsigned int slot, unsigned char motor, unsigned char force) {
	for (int i=0; i<pads[port][slot].numFFBindings; i++) {
		ForceFeedbackBinding *binding = pads[port][slot].ffBindings+i;
		if (binding->motor == motor) {
			SetEffect(binding, force);
		}
	}
}

wchar_t *GetDefaultControlName(unsigned short id, int type) {
	static wchar_t name[20];
	if (type & BUTTON) {
		wsprintfW(name, L"Button %i", id);
	}
	else if (type & AXIS) {
		wsprintfW(name, L"Axis %i", id);
	}
	else if (type & POV) {
		wsprintfW(name, L"POV %i", id);
	}
	else return L"Unknown";
	return name;
}

wchar_t *Device::GetVirtualControlName(VirtualControl *control) {
	static wchar_t temp[100];
	wchar_t *baseName = 0;
	if (control->physicalControlIndex >= 0) {
		baseName = physicalControls[control->physicalControlIndex].name;
		if (!baseName) baseName = GetPhysicalControlName(&physicalControls[control->physicalControlIndex]);
	}
	unsigned int uid = control->uid;
	if (!baseName) {
		baseName = GetDefaultControlName(uid & 0xFFFF, (uid >> 16)& 0x1F);
	}
	uid &= 0xFF000000;
	int len = (int)wcslen(baseName);
	if (len > 99) len = 99;
	memcpy(temp, baseName, len*sizeof(wchar_t));
	temp[len] = 0;
	if (uid) {
		if (len > 95) len = 95;
		wchar_t *out = temp+len;
		if (uid == UID_AXIS_POS) {
			wcscpy(out, L" +");
		}
		else if (uid == UID_AXIS_NEG) {
			wcscpy(out, L" -");
		}
		else if (uid == UID_POV_N) {
			wcscpy(out, L" N");
		}
		else if (uid == UID_POV_E) {
			wcscpy(out, L" E");
		}
		else if (uid == UID_POV_S) {
			wcscpy(out, L" S");
		}
		else if (uid == UID_POV_W) {
			wcscpy(out, L" W");
		}
	}
	return temp;
}

wchar_t *Device::GetPhysicalControlName(PhysicalControl *control) {
	if (control->name) return control->name;
	return GetDefaultControlName(control->id, control->type);
}

void InputDeviceManager::AddDevice(Device *d) {
	devices = (Device**) realloc(devices, sizeof(Device*) * (numDevices+1));
	devices[numDevices++] = d;
}

void InputDeviceManager::Update(InitInfo *info) {
	for (int i=0; i<numDevices; i++) {
		if (devices[i]->enabled) {
			if (!devices[i]->active) {
				if (!devices[i]->Activate(info) || !devices[i]->Update()) continue;
				devices[i]->CalcVirtualState();
				devices[i]->PostRead();
			}
			if (devices[i]->Update())
				devices[i]->CalcVirtualState();
		}
	}
}

void InputDeviceManager::PostRead() {
	for (int i=0; i<numDevices; i++) {
		if (devices[i]->active)
			devices[i]->PostRead();
	}
}

Device *InputDeviceManager::GetActiveDevice(InitInfo *info, unsigned int *uid, int *index, int *value) {
	int i, j;
	Update(info);
	int bestDiff = FULLY_DOWN/2;
	Device *bestDevice = 0;
	for (i=0; i<numDevices; i++) {
		if (devices[i]->active) {
			for (j=0; j<devices[i]->numVirtualControls; j++) {
				if (devices[i]->virtualControlState[j] == devices[i]->oldVirtualControlState[j]) continue;
				if (devices[i]->virtualControls[j].uid & UID_POV) continue;
				// Fix for releasing button used to click on bind button
				if (!((devices[i]->virtualControls[j].uid>>16) & (POV|RELAXIS|ABSAXIS))) {
					if (abs(devices[i]->oldVirtualControlState[j]) > abs(devices[i]->virtualControlState[j])) {
						devices[i]->oldVirtualControlState[j] = 0;
					}
				}
				int diff = abs(devices[i]->virtualControlState[j] - devices[i]->oldVirtualControlState[j]);
				// Make it require a bit more work to bind relative axes.
				if (((devices[i]->virtualControls[j].uid>>16) & 0xFF) == RELAXIS) {
					diff = diff/4+1;
				}
				// Less pressure needed to bind DS3 buttons.
				if (devices[i]->api == DS3 && (((devices[i]->virtualControls[j].uid>>16) & 0xFF) & BUTTON)) {
					diff *= 4;
				}
				if (diff > bestDiff) {
					if (devices[i]->virtualControls[j].uid & UID_AXIS) {
						if ((((devices[i]->virtualControls[j].uid>>16)&0xFF) != ABSAXIS)) continue;
						// Very picky when binding entire axes.  Prefer binding half-axes.
						if (!((devices[i]->oldVirtualControlState[j] < FULLY_DOWN/32 && devices[i]->virtualControlState[j] > FULLY_DOWN/8) ||
							  (devices[i]->oldVirtualControlState[j] > 31*FULLY_DOWN/32 && devices[i]->virtualControlState[j] < 7*FULLY_DOWN/8))) {
									continue;
						}
						devices[i]->virtualControls[j].uid = devices[i]->virtualControls[j].uid;
					}
					else if ((((devices[i]->virtualControls[j].uid>>16)&0xFF) == ABSAXIS)) {
						if (devices[i]->oldVirtualControlState[j] > 15*FULLY_DOWN/16)
							continue;
					}
					bestDiff = diff;
					*uid = devices[i]->virtualControls[j].uid;
					*index = j;
					bestDevice =  devices[i];
					if (value) {
						if ((devices[i]->virtualControls[j].uid>>16)& RELAXIS) {
							*value = devices[i]->virtualControlState[j] - devices[i]->oldVirtualControlState[j];
						}
						else {
							*value = devices[i]->virtualControlState[j];
						}
					}
				}
			}
		}
	}
	// Don't call when binding.
	// PostRead();
	return bestDevice;
}

void InputDeviceManager::ReleaseInput() {
	for (int i=0; i<numDevices; i++) {
		if (devices[i]->active) devices[i]->Deactivate();
	}
}

void InputDeviceManager::EnableDevices(DeviceType type, DeviceAPI api) {
	for (int i=0; i<numDevices; i++) {
		if (devices[i]->api == api && devices[i]->type == type) {
			EnableDevice(i);
		}
	}
}

void InputDeviceManager::DisableAllDevices() {
	for (int i=0; i<numDevices; i++) {
		DisableDevice(i);
	}
}

void InputDeviceManager::DisableDevice(int index) {
	devices[index]->enabled = 0;
	if (devices[index]->active) {
		devices[index]->Deactivate();
	}
}

ForceFeedbackEffectType *Device::GetForcefeedbackEffect(wchar_t *id) {
	for (int i=0; i<numFFEffectTypes; i++) {
		if (!wcsicmp(id, ffEffectTypes[i].effectID)) {
			return &ffEffectTypes[i];
		}
	}
	return 0;
}

ForceFeedbackAxis *Device::GetForceFeedbackAxis(int id) {
	for (int i=0; i<numFFAxes; i++) {
		if (ffAxes[i].id == id) return &ffAxes[i];
	}
	return 0;
}

void InputDeviceManager::CopyBindings(int numOldDevices, Device **oldDevices) {
	int *oldMatches = (int*) malloc(sizeof(int) * numOldDevices);
	int *matches = (int*) malloc(sizeof(int) * numDevices);
	int i, j, port, slot;
	Device *old, *dev;
	for (i=0; i<numDevices; i++) {
		matches[i] = -1;
	}
	for (i=0; i<numOldDevices; i++) {
		oldMatches[i] = -2;
		old = oldDevices[i];
		for (port=0; port<2; port++) {
			for (slot=0; slot<4; slot++) {
				if (old->pads[port][slot].numBindings + old->pads[port][slot].numFFBindings) {
					// Means that there are bindings.
					oldMatches[i] = -1;
				}
			}
		}
	}
	// Loops through ids looking for match, from most specific to most general.
	for (int id=0; id<3; id++) {
		for (i=0; i<numOldDevices; i++) {
			if (oldMatches[i] >= 0) continue;
			for (j=0; j<numDevices; j++) {
				if (matches[j] >= 0) {
					continue;
				}
				wchar_t *id1 = devices[j]->IDs[id];
				wchar_t *id2 = oldDevices[i]->IDs[id];
				if (!id1 || !id2) {
					continue;
				}
				if (!wcsicmp(id1, id2)) {
					matches[j] = i;
					oldMatches[i] = j;
					break;
				}
			}
		}
	}

	for (i=0; i<numOldDevices; i++) {
		if (oldMatches[i] == -2) continue;
		old = oldDevices[i];
		if (oldMatches[i] < 0) {
			dev = new Device(old->api, old->type, old->displayName, old->instanceID, old->productID);
			dev->attached = 0;
			AddDevice(dev);
			for (j=0; j<old->numVirtualControls; j++) {
				VirtualControl *c = old->virtualControls+j;
				dev->AddVirtualControl(c->uid, -1);
			}
			for (j=0; j<old->numFFEffectTypes; j++) {
				ForceFeedbackEffectType * effect = old->ffEffectTypes + j;
				dev->AddFFEffectType(effect->displayName, effect->effectID, effect->type);
			}
			for (j=0; j<old->numFFAxes; j++) {
				ForceFeedbackAxis * axis = old->ffAxes + j;
				dev->AddFFAxis(axis->displayName, axis->id);
			}
			// Just steal the old bindings directly when there's no matching device.
			// Indices will be the same.
			memcpy(dev->pads, old->pads, sizeof(old->pads));
			memset(old->pads, 0, sizeof(old->pads));
		}
		else {
			dev = devices[oldMatches[i]];
			for (port=0; port<2; port++) {
				for (slot=0; slot<4; slot++) {
					if (old->pads[port][slot].numBindings) {
						dev->pads[port][slot].bindings = (Binding*) malloc(old->pads[port][slot].numBindings * sizeof(Binding));
						for (int j=0; j<old->pads[port][slot].numBindings; j++) {
							Binding *bo = old->pads[port][slot].bindings + j;
							Binding *bn = dev->pads[port][slot].bindings + dev->pads[port][slot].numBindings;
							VirtualControl *cn = dev->GetVirtualControl(old->virtualControls[bo->controlIndex].uid);
							if (cn) {
								*bn = *bo;
								bn->controlIndex = cn - dev->virtualControls;
								dev->pads[port][slot].numBindings++;
							}
						}
					}
					if (old->pads[port][slot].numFFBindings) {
						dev->pads[port][slot].ffBindings = (ForceFeedbackBinding*) malloc(old->pads[port][slot].numFFBindings * sizeof(ForceFeedbackBinding));
						for (int j=0; j<old->pads[port][slot].numFFBindings; j++) {
							ForceFeedbackBinding *bo = old->pads[port][slot].ffBindings + j;
							ForceFeedbackBinding *bn = dev->pads[port][slot].ffBindings + dev->pads[port][slot].numFFBindings;
							ForceFeedbackEffectType *en = dev->GetForcefeedbackEffect(old->ffEffectTypes[bo->effectIndex].effectID);
							if (en) {
								*bn = *bo;
								bn->effectIndex = en - dev->ffEffectTypes;
								bn->axes = (AxisEffectInfo*)calloc(dev->numFFAxes, sizeof(AxisEffectInfo));
								for (int k=0; k<old->numFFAxes; k++) {
									ForceFeedbackAxis *newAxis = dev->GetForceFeedbackAxis(old->ffAxes[k].id);
									if (newAxis) {
										bn->axes[newAxis - dev->ffAxes] = bo->axes[k];
									}
								}
								dev->pads[port][slot].numFFBindings++;
							}
						}
					}
				}
			}
		}
	}
	free(oldMatches);
	free(matches);
}

void InputDeviceManager::SetEffect(unsigned char port, unsigned int slot, unsigned char motor, unsigned char force) {
	for (int i=0; i<numDevices; i++) {
		Device *dev = devices[i];
		if (dev->enabled && dev->numFFEffectTypes) {
			dev->SetEffects(port, slot, motor, force);
		}
	}
}

