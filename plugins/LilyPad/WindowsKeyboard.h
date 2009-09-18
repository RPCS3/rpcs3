// Shared functionality for WM and RAW keyboards.
class WindowsKeyboard : public Device {
public:
	WindowsKeyboard(DeviceAPI api, wchar_t *displayName, wchar_t *instanceID=0, wchar_t *deviceID=0);
	wchar_t *GetPhysicalControlName(PhysicalControl *control);
	void UpdateKey(int vkey, int state);
	// Calls AllocState() and initializes to current keyboard state using
	// GetAsyncKeyState().
	void InitState();
};
