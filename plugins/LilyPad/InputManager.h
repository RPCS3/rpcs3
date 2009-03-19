#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

// Both of these are hard coded in a lot of places, so don't modify them.
// Base sensitivity means that a sensitivity of that corresponds to a factor of 1.
// Fully down means that value corresponds to a button being fully down (255).
// a value of 128 or more corresponds to that button being pressed, for binary
// values.
#define BASE_SENSITIVITY (1<<16)
#define FULLY_DOWN (1<<16)


/* Idea is for this file and the associated cpp file to be Windows independent.
 * Still more effort than it's worth to port to Linux, however.
 */

// Match DirectInput8 values.
enum ControlType {
	NO_CONTROL = 0,
	// Axes are ints.  Relative axes are for mice, mice wheels, etc,
	// and are always reported relative to their last value.
	// Absolute axes range from -65536 to 65536 and are absolute positions,
	// like for joysticks and pressure sensitive buttons.
	RELAXIS = 1,
	ABSAXIS = 2,

	// Buttons range from 0 to 65536.
	PSHBTN = 4,
	TGLBTN = 8,

	// POV controls are ints, values range from -1 to 36000.
	// -1 means not pressed, otherwise it's an angle.
	// For easy DirectInput compatibility, anything outside.
	// that range is treated as -1 (Though 36000-37000 is treated
	// like 0 to 1000, just in case).
	POV = 16,
};

// Masks to determine button type.  Don't need one for POV.
#define BUTTON 12
#define AXIS 3

struct Binding {
	int controlIndex;
	int command;
	int sensitivity;
	unsigned char turbo;
};

#define UID_AXIS (1<<31)
#define UID_POV  (1<<30)

#define UID_AXIS_POS (1<<24)
#define UID_AXIS_NEG (2<<24)
#define UID_POV_N    (3<<24)
#define UID_POV_E    (4<<24)
#define UID_POV_S    (5<<24)
#define UID_POV_W    (6<<24)

// One of these exists for each bindable object.
// Bindable objects consist of buttons, axis, pov controls,
// and individual axis/pov directions.  Not that pov controls
// cannot actually be bound, but when trying to bind as an axis,
// all directions are assigned individually.
struct VirtualControl {
	// Unique id for control, given device.  Based on source control's id,
	// source control type, axis/pov flags if it's a pov/axis (Rather than
	// a button or a pov/axis control's individual button), and an index,
	// if the control is split.
	unsigned int uid;
	// virtual key code.  0 if none.
	int physicalControlIndex;
};

// Need one for each button, axis, and pov control.
// API-specific code creates the PhysicalControls and
// updates their state, standard function then populates
// the VirtualControls and queues the keyboard messages, if
// needed.
struct PhysicalControl {
	// index of the first virtual control corresponding to this.
	// Buttons have 1 virtual control, axes 3, and povs 5, all
	// in a row.
	int baseVirtualControlIndex;
	ControlType type;
	// id.  Must be unique for control type.
	// short so can be combined with other values to get
	// uid for virtual controls.
	unsigned short id;
	unsigned short vkey;
	wchar_t *name;
};

enum DeviceAPI {
	NO_API = 0,
	DI = 1,
	WM = 2,
	RAW = 3,
	XINPUT = 4,
	// Not currently used.
	LLHOOK = 5,
	// Not a real API, obviously.  Only used with keyboards,
	// to ignore individual buttons.  Wrapper itself takes care
	// of ignoring bound keys.  Otherwise, works normally.
	IGNORE_KEYBOARD = 6,
};

enum DeviceType {
	NO_DEVICE = 0,
	KEYBOARD =	1,
	MOUSE =		2,
	OTHER =	3
};

enum EffectType {
	EFFECT_CONSTANT,
	EFFECT_PERIODIC,
	EFFECT_RAMP
};

// force range sfrom -BASE_SENSITIVITY to BASE_SENSITIVITY.
// Order matches ForceFeedbackAxis order.  force of 0 means to
// ignore that axis completely.  Force of 1 or -1 means to initialize
// the axis with minimum force (Possibly 0 force), if applicable.
struct AxisEffectInfo {
	int force;
};

struct ForceFeedbackBinding {
	AxisEffectInfo *axes;
	int effectIndex;
	unsigned char motor;
};

// Bindings listed by effect, so I don't have to bother with
// indexing effects.
struct ForceFeedbackEffectType {
	wchar_t *displayName;
	// Because I'm lazy, can only have ASCII characters and no spaces.
	wchar_t *effectID;
	// constant, ramp, or periodic
	EffectType type;
};


struct ForceFeedbackAxis {
	wchar_t *displayName;
	int id;
};

// Note:  Unbound controls don't need to be listed, though they can be.
// Nonexistent controls can be listed as well (Like listing 5 buttons
// even for three button mice, as some APIs make it impossible to figure
// out which buttons you really have).

// Used both for active devices and for sets of settings for devices.
// Way things work:
// LoadSettings() will delete all device info, then load settings to get
// one set of generic devices.  Then I enumerate all devices.  Then I merge
// them, moving settings from the generic devices to the enumerated ones.

// Can't refresh without loading settings.

struct PadBindings {
	Binding *bindings;
	int numBindings;
	ForceFeedbackBinding *ffBindings;
	int numFFBindings;
};

// Mostly self-contained, but bindings are modified by config.cpp, to make
// updating the ListView simpler.
class Device {
public:
	DeviceAPI api;
	DeviceType type;
	char active;
	char attached;
	// Based on input modes.
	char enabled;
	union {
		// Allows for one loop to compare all 3 in order.
		wchar_t *IDs[3];
		struct {
			// Same as DisplayName, when not given.  Absolutely must be unique.
			// Used for loading/saving controls.  If matches, all other strings
			// are ignored, so must be unique.
			wchar_t *instanceID;
			// Not required.  Used when a device's instance id changes, doesn't have to
			// be unique.  For devices that can only have one instance, not needed.
			wchar_t *productID;

			wchar_t *displayName;
		};
	};

	PadBindings pads[2][4];

	// Virtual controls.  All basically act like pressure sensitivity buttons, with
	// values between 0 and 2^16.  2^16 is fully down, 0 is up.  Larger values
	// are allowed, but *only* for absolute axes (Which don't support the flip checkbox).
	// Each control on a device must have a unique id, used for binding.
	VirtualControl *virtualControls;
	int numVirtualControls;

	int *virtualControlState;
	int *oldVirtualControlState;

	PhysicalControl *physicalControls;
	int numPhysicalControls;
	int *physicalControlState;

	ForceFeedbackEffectType *ffEffectTypes;
	int numFFEffectTypes;
	ForceFeedbackAxis *ffAxes;
	int numFFAxes;
	void AddFFAxis(const wchar_t *displayName, int id);
	void AddFFEffectType(const wchar_t *displayName, const wchar_t *effectID, EffectType type);

	Device(DeviceAPI, DeviceType, const wchar_t *displayName, const wchar_t *instanceID = 0, wchar_t *deviceID = 0);
	virtual ~Device();

	// Allocates memory for old and new state, sets everything to 0.
	// all old states are in one array, buttons, axes, and then POVs.
	// start of each section is int aligned.  This makes it DirectInput
	// compatible.
	void AllocState();

	// Doesn't actually flip.  Copies current state to old state.
	void FlipState();

	// Frees state variables.
	void FreeState();

	ForceFeedbackEffectType *GetForcefeedbackEffect(wchar_t *id);
	ForceFeedbackAxis *GetForceFeedbackAxis(int id);

	VirtualControl *GetVirtualControl(unsigned int uid);

	PhysicalControl *AddPhysicalControl(ControlType type, unsigned short id, unsigned short vkey, const wchar_t *name = 0);
	VirtualControl *AddVirtualControl(unsigned int uid, int physicalControlIndex);

	virtual wchar_t *GetVirtualControlName(VirtualControl *c);
	virtual wchar_t *GetPhysicalControlName(PhysicalControl *c);

	void CalcVirtualState();

	// args are OS dependent.
	inline virtual int Activate(void *args) {
		return 0;
	}

	inline virtual void Deactivate() {
		FreeState();
		active = 0;
	}

	// Default update proc.  All that's needed for post-based APIs.
	inline virtual int Update() {
		return active;
	}

	// force is from -FULLY_DOWN to FULLY_DOWN.
	// Either function can be overridden.  Second one by default calls the first
	// for every bound effect that's affected.

	// Note:  Only used externally for binding, so if override the other one, can assume
	// all other forces are currently 0.
	inline virtual void SetEffect(ForceFeedbackBinding *binding, unsigned char force) {}
	inline virtual void SetEffects(unsigned char port, unsigned int slot, unsigned char motor, unsigned char force);

	// Called after reading.  Basically calls FlipState().
	// Some device types (Those that don't incrementally update)
	// could call FlipState elsewhere, but this makes it simpler to ignore
	// while binding.
	virtual void PostRead();
};

class InputDeviceManager {
public:
	Device **devices;
	int numDevices;

	void ClearDevices();

	// When refreshing devices, back up old devices, then
	// populate this with new devices, then call copy bindings.
	// All old bindings are copied to matching devices.

	// When old devices are missing, I do a slightly more careful search
	// using productIDs and then (in desperation) displayName.
	// Finally create new dummy devices if no matches found.
	void CopyBindings(int numDevices, Device **devices);


	InputDeviceManager();
	~InputDeviceManager();

	void AddDevice(Device *d);
	Device *GetActiveDevice(void *info, unsigned int *uid, int *index, int *value);
	void Update(void *attachInfo);

	// Called after reading state, after Update().
	void PostRead();

	void SetEffect(unsigned char port, unsigned int slot, unsigned char motor, unsigned char force);

	// Update does this as needed.
	// void GetInput(void *v);
	void ReleaseInput();

	void DisableDevice(int index);
	inline void EnableDevice(int i) {
		devices[i]->enabled = 1;
	}

	void EnableDevices(DeviceType type, DeviceAPI api);
	void DisableAllDevices();
};

extern InputDeviceManager *dm;

#endif
