//#include "global.h"
#include "VKey.h"

char *GetMouseString(unsigned char button) {
	switch(button) {
	case 0x01:  return "Left button";
	case 0x02:  return "Right button";
	case 0x03:  return "Middle button";
	case 0x04:  return "Button 4";
	case 0x05:  return "Button 5";
	case 0x06:  return "X-axis";
	case 0x07:  return "Y-axis";
	case 0x08:  return "Wheel";
	case 0x09:  return "Wheel 2";
	default:
		break;
	}
	static char t[8];
	sprintf(t, "%i", button);
	return t;
}

#ifndef MAPVK_VK_TO_VSC
#define MAPVK_VK_TO_VSC 0
#endif

wchar_t *GetVKStringW(unsigned char vk) {
	int flag;
	static wchar_t t[20];
	/*
	for (int i=0x90; i<0xFF; i++) {
		int res = MapVirtualKey(i, MAPVK_VK_TO_VSC);
		static char t[20] ={0};
		static char t2[20] ={0};
		if (res && (GetKeyNameText((res<<16) + (0<<24), t, 20) || GetKeyNameText((res<<16) + (0<<24), t2, 20))) {
			t[19] = 0;
		}
	}//*/
	//char *s1, *s2;
	switch(vk) {
	//case 0x01:  return L"Left button";
	//case 0x02:  return L"Right button";
	//case 0x03:  return "Control-break";
	//case 0x04:  return L"Middle button";
	//case 0x08:  return "Backspace";
	//case 0x09:  return "Tab";
	case 0x0C:  return L"Clear";
	//case 0x0D:  return "Enter";
	//case 0x10:  return "Shift";
	//case 0x11:  return "Ctrl";
	//case 0x12:  return "Alt";
	case 0x13:  return L"Pause";
	//case 0x14:  return "Caps Lock";
	//case 0x1B:  return "Esc";
	//case 0x20:  return "Space";

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
	/*
	case 0x30:  return "0";
	case 0x31:  return "1";
	case 0x32:  return "2";
	case 0x33:  return "3";
	case 0x34:  return "4";
	case 0x35:  return "5";
	case 0x36:  return "6";
	case 0x37:  return "7";
	case 0x38:  return "8";
	case 0x39:  return "9";
	case 0x41:  return "A";
	case 0x42:  return "B";
	case 0x43:  return "C";
	case 0x44:  return "D";
	case 0x45:  return "E";
	case 0x46:  return "F";
	case 0x47:  return "G";
	case 0x48:  return "H";
	case 0x49:  return "I";
	case 0x4A:  return "J";
	case 0x4B:  return "K";
	case 0x4C:  return "L";
	case 0x4D:  return "M";
	case 0x4E:  return "N";
	case 0x4F:  return "O";
	case 0x50:  return "P";
	case 0x51:  return "Q";
	case 0x52:  return "R";
	case 0x53:  return "S";
	case 0x54:  return "T";
	case 0x55:  return "U";
	case 0x56:  return "V";
	case 0x57:  return "W";
	case 0x58:  return "X";
	case 0x59:  return "Y";
	case 0x5A:  return "Z";
	//*/
		/*
	case 0x60:  return "Num 0";
	case 0x61:  return "Num 1";
	case 0x62:  return "Num 2";
	case 0x63:  return "Num 3";
	case 0x64:  return "Num 4";
	case 0x65:  return "Num 5";
	case 0x66:  return "Num 6";
	case 0x67:  return "Num 7";
	case 0x68:  return "Num 8";
	case 0x69:  return "Num 9";
	//*/
	//case 0x6A:  return "Num Mul";
	//case 0x6B:  return "Num Add";
	case 0x6C:  return L"|";
	//case 0x6D:  return "Num Sub";
	//case 0x6E:  return "Num Del";
	/*
	case 0x70:  return "F1";
	case 0x71:  return "F2";
	case 0x72:  return "F3";
	case 0x73:  return "F4";
	case 0x74:  return "F5";
	case 0x75:  return "F6";
	case 0x76:  return "F7";
	case 0x77:  return "F8";
	case 0x78:  return "F9";
	case 0x79:  return "F10";
	case 0x7A:  return "F11";
	case 0x7B:  return "F12";
	case 0x7C:  return "F13";
	case 0x7D:  return "F14";
	case 0x7E:  return "F15";
	case 0x7F:  return "F16";
	case 0x80:  return "F17";
	case 0x81:  return "F18";
	case 0x82:  return "F19";
	case 0x83:  return "F20";
	case 0x84:  return "F21";
	case 0x85:  return "F22";
	case 0x86:  return "F23";
	case 0x87:  return "F24";
	//*/
	//case VK_EQUALS: return "=";
	case 0x90:  return L"Num Lock";
	//case 0x91:  return "Scroll Lock";
/*
	case 0xA0:
	case 0xA1:
		s2 = "Shift";
		goto skip;
	case 0xA2:
	case 0xA3:
		s2 = "Ctrl";
		goto skip;
	case 0xA4:
	case 0xA5:
		s2 = "Alt";
skip:
		s1 = "Left";
		if (vk&1) s1 = "Right";
		sprintf(t, "%s %s", s1, s2);
		return t;

/*/
	case 0xA0:  return L"Left Shift";
	case 0xA1:  return L"Right Shift";
	case 0xA2:  return L"Left Ctrl";
	case 0xA3:  return L"Right Ctrl";
	case 0xA4:  return L"Left Alt";
	case 0xA5:  return L"Right Alt";
	//*/
	case 0xA6:  return L"Back";
	case 0xA7: 	return L"Forward";
	case 0xA8: 	return L"Refresh";
	case 0xA9: 	return L"Stop";
	case 0xAA: 	return L"Search";
	case 0xAB: 	return L"Favorites";
	case 0xAC: 	return L"Browser";
	//case 0xBA:  return ":";
	//case 0xBB:  return "=";
	//case 0xBC:  return ",";
	//case 0xBD:  return "-";
	//case 0xBE:  return ".";
	//case 0xBF:  return "/";
	//case 0xC0:  return "`";
	//case 0xDB:  return "[";
	//case 0xDC:  return "\\";
	//case 0xDD:  return "]";
	//case 0xDE:  return "'";
	//case 0xE2:  return "\\";
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

