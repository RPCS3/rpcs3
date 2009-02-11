// Shouldn't really be here, but not sure where else to put it.
struct InitInfo {
	// 1 when binding key to ignore.
	int bindingIgnore;

	HWND hWndTop;
	HWND hWnd;
	// For config screen, need to eat button's message handling.
	HWND hWndButton;
};

void EnumDevices();

