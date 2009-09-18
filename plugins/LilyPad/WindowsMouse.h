// Shared functionality for WM and RAW keyboards.
class WindowsMouse : public Device {
public:
	// Used by GetMouseCapture()/ReleaseMouseCapture()
	// Static because can have multiple raw mice active at once,
	// and only get/release capture once.
	static POINT origCursorPos;
	static POINT center;

	static void GetMouseCapture(HWND hWnd);
	static void WindowResized(HWND hWnd);
	static void ReleaseMouseCapture();

	// hWheel variable lets me display no horizontal wheel for raw input, just to make it clear
	// that it's not supported.
	WindowsMouse(DeviceAPI api, int hWheel, wchar_t *displayName, wchar_t *instanceID=0, wchar_t *deviceID=0);
	wchar_t *GetPhysicalControlName(PhysicalControl *control);
	// State is 0 for up, 1 for down.
	void UpdateButton(unsigned int button, int state);
	// 0/1 are x/y. 2 is vert wheel, 3 is horiz wheel.
	// Delta is in my micro units.  change of (1<<16) is 1 full unit, with
	// the default sensitivity.
	void UpdateAxis(unsigned int axis, int delta);
};
