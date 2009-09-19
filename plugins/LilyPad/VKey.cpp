#include "Global.h"
#include "VKey.h"

wchar_t *GetVKStringW(unsigned char vk) {
	int flag;
	static wchar_t t[20];
	switch(vk) {
		case 0x0C:  return L"Clear";
		case 0x13:  return L"Pause";

		case 0x21://	return "Page Up";
		case 0x22://	return "Page Down";
		case 0x23://	return "End";
		case 0x24://	return "Home";
		case 0x25://	return "Left";
		case 0x26://	return "Up";
		case 0x27://	return "Right";
		case 0x28://	return "Down";
		case 0x2D://	return "Insert";
		case 0x2E://	return "Delete";
		case 0x5B://	return "Left Windows";
		case 0x5C://	return "Right Windows";
		case 0x5D://	return "Application";
		case 0x6F://	return "Num /";
			flag = 1<<24;
			break;

		case 0x29:  return L"Select";
		case 0x2A:  return L"Print";
		case 0x2B:  return L"Execute";
		case 0x2C:  return L"Prnt Scrn";
		case 0x2F:  return L"Help";

		case 0x6C:  return L"|";
		case 0x90:  return L"Num Lock";

		case 0xA0:  return L"Left Shift";
		case 0xA1:  return L"Right Shift";
		case 0xA2:  return L"Left Ctrl";
		case 0xA3:  return L"Right Ctrl";
		case 0xA4:  return L"Left Alt";
		case 0xA5:  return L"Right Alt";

		case 0xA6:  return L"Back";
		case 0xA7: 	return L"Forward";
		case 0xA8: 	return L"Refresh";
		case 0xA9: 	return L"Stop";
		case 0xAA: 	return L"Search";
		case 0xAB: 	return L"Favorites";
		case 0xAC: 	return L"Browser";

		case 0xFA:  return L"Play";
		case 0xFB:  return L"Zoom";
		default:
			flag = 0;
			break;
	}
	int res = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
	if (res && GetKeyNameText((res<<16) | flag, t, 20)) {
		// don't trust windows
		t[19] = 0;
	}
	else {
		wsprintfW(t, L"Key %i", vk);
	}
	return t;
}

