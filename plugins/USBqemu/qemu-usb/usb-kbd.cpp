/*
 * QEMU USB HID devices
 * 
 * Copyright (c) 2005 Fabrice Bellard
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "USBinternal.h"

/* HID interface requests */
#define GET_REPORT   0xa101
#define GET_IDLE     0xa102
#define GET_PROTOCOL 0xa103
#define SET_IDLE     0x210a
#define SET_PROTOCOL 0x210b

#define USB_MOUSE  1
#define USB_TABLET 2

typedef struct USBKeyboardState {
    USBDevice dev;
	int keyboard_grabbed;
} USBKeyboardState;

USBDeviceInfo devinfo;


#define VK_BASED

#ifdef VK_BASED
static const uint8_t vk_to_key_code[] = {
0x00, //FAIL: 0x00
0x00, //FAIL: LMOUSE
0x00, //FAIL: RMOUSE
0x00, //FAIL: Break
0x00, //FAIL: MMOUSE
0x00, //FAIL: X1MOUSE
0x00, //FAIL: X2MOUSE
0x00, //FAIL: 0x00
0x2A, //OK: Backspace
0x2B, //OK: Tab
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x9C, //OK: Clear
0x28, //FAIL: ENTER
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: SHIFT
0x00, //FAIL: CTRL
0x00, //FAIL: ALT
0x48, //OK: Pause
0x39, //OK: Caps Lock
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
//0x00, //FAIL: 0x00
//0x00, //FAIL: 0x00
//0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x29, //FAIL: ESC
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x2C, //OK: Spacebar
#ifdef ENABLE_KEYPAD_Fx
0x4B, //FAIL: PAGE UP
0x4E, //FAIL: PAGE DOWN
0x4D, //OK: End
0x4A, //OK: Home
0x50, //FAIL: LEFT ARROW
0x52, //FAIL: UP ARROW
0x4F, //FAIL: RIGHT ARROW
0x51, //FAIL: DOWN ARROW
0x77, //OK: Select
0x00, //FAIL: PRINT
0x74, //OK: Execute
0x46, //FAIL: PRINT SCREEN
0x49, //FAIL: INS
0x4C, //FAIL: DEL
0x75, //OK: Help VK_HOME
#else
0x00, //FAIL: PAGE UP
0x00, //FAIL: PAGE DOWN
0x00, //OK: End
0x00, //OK: Home
0x00, //FAIL: LEFT ARROW
0x00, //FAIL: UP ARROW
0x00, //FAIL: RIGHT ARROW
0x00, //FAIL: DOWN ARROW
0x00, //OK: Select
0x00, //FAIL: PRINT
0x00, //OK: Execute
0x00, //FAIL: PRINT SCREEN
0x00, //FAIL: INS
0x00, //FAIL: DEL
0x00, //OK: Help VK_HOME
#endif
0x27, //OK: 0
0x1E, //OK: 1
0x1F, //OK: 2
0x20, //OK: 3
0x21, //OK: 4
0x22, //OK: 5
0x23, //OK: 6
0x24, //OK: 7
0x25, //OK: 8
0x26, //OK: 9
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: not found
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x04, //OK: A
0x05, //OK: B
0x06, //OK: C
0x07, //OK: D
0x08, //OK: E
0x09, //OK: F
0x0A, //OK: G
0x0B, //OK: H
0x0C, //OK: I
0x0D, //OK: J
0x0E, //OK: K
0x0F, //OK: L
0x10, //OK: M
0x11, //OK: N
0x12, //OK: O
0x13, //OK: P
0x14, //OK: Q
0x15, //OK: R
0x16, //OK: S
0x17, //OK: T
0x18, //OK: U
0x19, //OK: V
0x1A, //OK: W
0x1B, //OK: X
0x1C, //OK: Y
0x1D, //OK: Z
#ifdef ENABLE_KEYPAD_Fx
0xE3, //OK: LGUI
0xE7, //OK: RGUI
0x65, //OK: Application
0x00, //FAIL: 0x00
0x00, //FAIL: SLEEP
0x62, //OK: Keypad 0
0x59, //OK: Keypad 1
0x5A, //OK: Keypad 2
0x5B, //OK: Keypad 3
0x5C, //OK: Keypad 4
0x5D, //OK: Keypad 5
0x5E, //OK: Keypad 6
0x5F, //OK: Keypad 7
0x60, //OK: Keypad 8
0x61, //OK: Keypad 9
0x55, //OK: Keypad *
0x57, //OK: Keypad +
0x9F, //OK: Separator
0x56, //OK: Keypad -
0x63, //OK: Keypad .
0x54, //OK: Keypad /
0x3A, //OK: F1
0x3B, //OK: F2
0x3C, //OK: F3
0x3D, //OK: F4
0x3E, //OK: F5
0x3F, //OK: F6
0x40, //OK: F7
0x41, //OK: F8
0x42, //OK: F9
0x43, //OK: F10
0x44, //OK: F11
0x45, //OK: F12
0x68, //OK: F13
0x69, //OK: F14
0x6A, //OK: F15
0x6B, //OK: F16
0x6C, //OK: F17
0x6D, //OK: F18
0x6E, //OK: F19
0x6F, //OK: F20
0x70, //OK: F21
0x71, //OK: F22
0x72, //OK: F23
0x73, //OK: F24
#else
0x00, //OK: LGUI
0x00, //OK: RGUI
0x00, //OK: Application
0x00, //FAIL: 0x00
0x00, //FAIL: SLEEP
0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,
#endif
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x83, //OK: NUM LOCK
0x47, //OK: Scroll Lock
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0xE1, //OK: LSHIFT
0xE5, //OK: RSHIFT
0xE0, //OK: LCONTROL
0xE4, //OK: RCONTROL
0xE3, //OK: LGUI
0xE7, //OK: RGUI
0x00, //FAIL: Windows 2000/XP: Browser Back
0x00, //FAIL: Windows 2000/XP: Browser Forward
0x00, //FAIL: Windows 2000/XP: Browser Refresh
0x00, //FAIL: Windows 2000/XP: Browser Stop
0x00, //FAIL: Windows 2000/XP: Browser Search 
0x00, //FAIL: Windows 2000/XP: Browser Favorites
0x00, //FAIL: Windows 2000/XP: Browser Start and Home
0x00, //FAIL: Windows 2000/XP: Volume Mute
0x00, //FAIL: Windows 2000/XP: Volume Down
0x00, //FAIL: Windows 2000/XP: Volume Up
0x00, //FAIL: Windows 2000/XP: Next Track
0x00, //FAIL: Windows 2000/XP: Previous Track
0x00, //FAIL: Windows 2000/XP: Stop Media
0x00, //FAIL: Windows 2000/XP: Play/Pause Media
0x00, //FAIL: Windows 2000/XP: Start Mail
0x00, //FAIL: Windows 2000/XP: Select Media
0x00, //FAIL: Windows 2000/XP: Start Application 1
0x00, //FAIL: Windows 2000/XP: Start Application 2
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x33, //FAIL: Windows 2000/XP: For the US standard keyboard, the ';:' key 
0x2E, //FAIL: Windows 2000/XP: For any country/region, the '+'
0x36, //FAIL: Windows 2000/XP: For any country/region, the ','
0x2D, //FAIL: Windows 2000/XP: For any country/region, the '-'
0x37, //FAIL: Windows 2000/XP: For any country/region, the '.'
0x38, //FAIL: Windows 2000/XP: For the US standard keyboard, the '/?' key 
0x35, //FAIL: Windows 2000/XP: For the US standard keyboard, the '`~' key 
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: not found
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x2F, //FAIL: Windows 2000/XP: For the US standard keyboard, the '[{' key
0x31, //FAIL: Windows 2000/XP: For the US standard keyboard, the '\|' key
0x30, //FAIL: Windows 2000/XP: For the US standard keyboard, the ']}' key
0x34, //FAIL: Windows 2000/XP: For the US standard keyboard, the 'single-quote/double-quote' key
0x00, //FAIL: Used for miscellaneous characters; it can vary byboard.
0x00, //FAIL: Reserved
0x00, //FAIL: OEM specific
0x32, //FAIL: Windows 2000/XP: Either the angle bracket or the backslash key on the RT 102-key keyboard
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: Windows 95/98/Me, Windows NT 4.0, Windows 2000/XP: IME PROCEE6
0x00, //FAIL: OEM specific
0x00, //FAIL: Windows 2000/XP: Used to pass Unicode characters as if they E8
0x00, //FAIL: Unassigned
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: 0x00
0x00, //FAIL: Attn
0xA3, //OK: CrSel
0xA4, //OK: ExSel
0x00, //FAIL: Erase EOF
0x00, //FAIL: Play
0x00, //FAIL: Zoom
0x00, //FAIL: Reserved 
0x00, //FAIL: PA1
0x9C, //OK: Clear
0x00, //FAIL: 0x00
};
#else
#	ifdef ENABLE_KEYPAD_Fx
static const unsigned char scan_to_usb[] = {
0x00,0x29,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x2f,0x30,0x2a,0x2b,
0x14,0x1a,0x08,0x15,0x17,0x1c,0x18,0x0c,0x12,0x13,0x33,0x2e,0x28,0xe0,0x04,0x16,
0x07,0x09,0x0a,0x0b,0x0d,0x0e,0x0f,0x35,0x34,0x31,0xe1,0x38,0x1d,0x1b,0x06,0x19,
0x05,0x11,0x10,0x36,0x37,0x2d,0xe5,0x55,0xe3,0x2c,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,
0x3f,0x40,0x41,0x42,0x43,0x83,0x47,0x5f,0x60,0x61,0x56,0x5c,0x5d,0x5e,0x57,0x59,
0x5a,0x5b,0x62,0x63,0x46,0x00,0x32,0x44,0x45,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x75,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x73,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe4,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x54,0x00,0x00,0xe7,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe3,0xe7,0x65,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x48,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
#	else
static const unsigned char scan_to_usb[] = {
0x00,0x29,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x2f,0x30,0x2a,0x2b,
0x14,0x1a,0x08,0x15,0x17,0x1c,0x18,0x0c,0x12,0x13,0x33,0x2e,0x28,0xe0,0x04,0x16,
0x07,0x09,0x0a,0x0b,0x0d,0x0e,0x0f,0x35,0x34,0x31,0xe1,0x38,0x1d,0x1b,0x06,0x19,
0x05,0x11,0x10,0x36,0x37,0x2d,0xe5,0x00,0xe3,0x2c,0x39,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x83,0x47,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x46,0x00,0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x75,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe4,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe7,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe3,0xe7,0x65,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x48,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
#	endif
#endif

/* mostly the same values as the Bochs USB Keyboard device */
static const uint8_t qemu_keyboard_dev_descriptor[] = {
	0x12,       /*  u8 bLength; */
	0x01,       /*  u8 bDescriptorType; Device */
	0x10, 0x00, /*  u16 bcdUSB; v1.0 */

	0x00,	    /*  u8  bDeviceClass; */
	0x00,	    /*  u8  bDeviceSubClass; */
	0x00,       /*  u8  bDeviceProtocol; [ low/full speeds only ] */
	0x08,       /*  u8  bMaxPacketSize0; 8 Bytes */

	0x27, 0x06, /*  u16 idVendor; */
 	0x01, 0x00, /*  u16 idProduct; */
	0x00, 0x00, /*  u16 bcdDevice */

	0x03,       /*  u8  iManufacturer; */
	0x02,       /*  u8  iProduct; */
	0x01,       /*  u8  iSerialNumber; */
	0x01        /*  u8  bNumConfigurations; */
};

static const uint8_t qemu_keyboard_config_descriptor[] = {
	/* one configuration */
	0x09,       /*  u8  bLength; */
	0x02,       /*  u8  bDescriptorType; Configuration */
	0x22, 0x00, /*  u16 wTotalLength; */
	0x01,       /*  u8  bNumInterfaces; (1) */
	0x01,       /*  u8  bConfigurationValue; */
	0x04,       /*  u8  iConfiguration; */
	0xa0,       /*  u8  bmAttributes; 
				 Bit 7: must be set,
				     6: Self-powered,
				     5: Remote wakeup,
				     4..0: resvd */
	50,         /*  u8  MaxPower; */
      
	/* USB 1.1:
	 * USB 2.0, single TT organization (mandatory):
	 *	one interface, protocol 0
	 *
	 * USB 2.0, multiple TT organization (optional):
	 *	two interfaces, protocols 1 (like single TT)
	 *	and 2 (multiple TT mode) ... config is
	 *	sometimes settable
	 *	NOT IMPLEMENTED
	 */

	/* one interface */
	0x09,       /*  u8  if_bLength; */
	0x04,       /*  u8  if_bDescriptorType; Interface */
	0x00,       /*  u8  if_bInterfaceNumber; */
	0x00,       /*  u8  if_bAlternateSetting; */
	0x01,       /*  u8  if_bNumEndpoints; */
	0x03,       /*  u8  if_bInterfaceClass; */
	0x01,       /*  u8  if_bInterfaceSubClass; */
	0x01,       /*  u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
	0x05,       /*  u8  if_iInterface; */
     
        /* HID descriptor */
        0x09,        /*  u8  bLength; */
        0x21,        /*  u8 bDescriptorType; */
        0x01, 0x00,  /*  u16 HID_class */
        0x00,        /*  u8 country_code */
        0x01,        /*  u8 num_descriptors */
        0x22,        /*  u8 type; Report */
        50, 0,       /*  u16 len */

	/* one endpoint (status change endpoint) */
	0x07,       /*  u8  ep_bLength; */
	0x05,       /*  u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x03,       /*  u8  ep_bmAttributes; Interrupt */
 	0x03, 0x00, /*  u16 ep_wMaxPacketSize; */
	0x0a,       /*  u8  ep_bInterval; (255ms -- usb 2.0 spec) */
};

static const uint8_t qemu_tablet_config_descriptor[] = {
	/* one configuration */
	0x09,       /*  u8  bLength; */
	0x02,       /*  u8  bDescriptorType; Configuration */
	0x22, 0x00, /*  u16 wTotalLength; */
	0x01,       /*  u8  bNumInterfaces; (1) */
	0x01,       /*  u8  bConfigurationValue; */
	0x04,       /*  u8  iConfiguration; */
	0xa0,       /*  u8  bmAttributes; 
				 Bit 7: must be set,
				     6: Self-powered,
				     5: Remote wakeup,
				     4..0: resvd */
	50,         /*  u8  MaxPower; */
      
	/* USB 1.1:
	 * USB 2.0, single TT organization (mandatory):
	 *	one interface, protocol 0
	 *
	 * USB 2.0, multiple TT organization (optional):
	 *	two interfaces, protocols 1 (like single TT)
	 *	and 2 (multiple TT mode) ... config is
	 *	sometimes settable
	 *	NOT IMPLEMENTED
	 */

	/* one interface */
	0x09,       /*  u8  if_bLength; */
	0x04,       /*  u8  if_bDescriptorType; Interface */
	0x00,       /*  u8  if_bInterfaceNumber; */
	0x00,       /*  u8  if_bAlternateSetting; */
	0x01,       /*  u8  if_bNumEndpoints; */
	0x03,       /*  u8  if_bInterfaceClass; */
	0x01,       /*  u8  if_bInterfaceSubClass; */
	0x02,       /*  u8  if_bInterfaceProtocol; [usb1.1 or single tt] */
	0x05,       /*  u8  if_iInterface; */

        /* HID descriptor */
        0x09,        /*  u8  bLength; */
        0x21,        /*  u8 bDescriptorType; */
        0x03, 0x00,  /*  u16 HID_class */
        0x00,        /*  u8 country_code */
        0x01,        /*  u8 num_descriptors */
        0x22,        /*  u8 type; Report */
        74, 0,       /*  u16 len */

	/* one endpoint (status change endpoint) */
	0x07,       /*  u8  ep_bLength; */
	0x05,       /*  u8  ep_bDescriptorType; Endpoint */
	0x81,       /*  u8  ep_bEndpointAddress; IN Endpoint 1 */
 	0x03,       /*  u8  ep_bmAttributes; Interrupt */
 	0x08, 0x00, /*  u16 ep_wMaxPacketSize; */
	0x03,       /*  u8  ep_bInterval; (255ms -- usb 2.0 spec) */
};

static const uint8_t qemu_keyboard_hid_report_descriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard Left Control)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION
};

static int usb_keyboard_poll(USBKeyboardState *s, uint8_t *buf, int len)
{
	static unsigned char keys[256];
    int i,l;

    if (!s->keyboard_grabbed) {
		//qemu_add_keyboard_event_handler(usb_keyboard_event, s, 0);
		s->keyboard_grabbed = 1;
    }

	if(gsWindowHandle != GetForegroundWindow())
	{
		for(int i=0;i<256;i++)
		{
			keys[i] = 0;
		}
	}
	else
	{
		for(int i=0;i<256;i++)
		{
			keys[i] = GetAsyncKeyState(i)>>8;
		}
	}

	l=1;
	l=2;
	buf[0] = 0;
	if(keys[VK_LCONTROL]>>7)	buf[0]|=(1<<0);
	if(keys[VK_LSHIFT]>>7)		buf[0]|=(1<<1);
	if(keys[VK_LMENU]>>7)		buf[0]|=(1<<2); //ALT key
	if(keys[VK_LWIN]>>7)		buf[0]|=(1<<3);
	if(keys[VK_RCONTROL]>>7)	buf[0]|=(1<<4);
	if(keys[VK_RSHIFT]>>7)		buf[0]|=(1<<5);
	if(keys[VK_RMENU]>>7)		buf[0]|=(1<<6);
	if(keys[VK_RWIN]>>7)		buf[0]|=(1<<7);

	buf[1] = 0; //reserved byte

	int k=0;
	for(int i=0;i<256;i++)
	{
		if(keys[i]>>7) //if pressed
		{
			if(l==8)
			{
				buf[1]=buf[1]=buf[1]=buf[1]=buf[1]=buf[1]=buf[1]=buf[1]=1;
			}
#ifdef VK_BASED
			int uc = vk_to_key_code[i];
#else
			int sc = MapVirtualKey(i,MAPVK_VK_TO_VSC_EX);
			if((sc>>8)!=0)
				sc=(sc&0x1FF)+256;
			int uc = scan_to_usb[sc];
			printf("// %08x->%02x ",i,sc);
			k++;
#endif
			if((uc>0)&&(uc<=0x65)) //
				buf[l++]=uc;
		}

	}
	if(k) printf("\n");

	/*if(l>=1)
	{
		while(l<8)
			buf[l++]=0;
		printf("KEYS: %02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
		l=l;
	}*/

	while(l<7)
		buf[l++]=0;
	
    return l;
}

static void usb_keyboard_handle_reset(USBDevice *dev)
{
    USBKeyboardState *s = (USBKeyboardState *)dev;
}

static int usb_keyboard_handle_control(USBDevice *dev, int request, int value,
                                  int index, int length, uint8_t *data)
{
    USBKeyboardState *s = (USBKeyboardState *)dev;
    int ret = 0;

    switch(request) {
    case DeviceRequest | USB_REQ_GET_STATUS:
        data[0] = (1 << USB_DEVICE_SELF_POWERED) |
            (dev->remote_wakeup << USB_DEVICE_REMOTE_WAKEUP);
        data[1] = 0x00;
        ret = 2;
        break;
    case DeviceOutRequest | USB_REQ_CLEAR_FEATURE:
        if (value == USB_DEVICE_REMOTE_WAKEUP) {
            dev->remote_wakeup = 0;
        } else {
            goto fail;
        }
        ret = 0;
        break;
    case DeviceOutRequest | USB_REQ_SET_FEATURE:
        if (value == USB_DEVICE_REMOTE_WAKEUP) {
            dev->remote_wakeup = 1;
        } else {
            goto fail;
        }
        ret = 0;
        break;
    case DeviceOutRequest | USB_REQ_SET_ADDRESS:
        dev->addr = value;
        ret = 0;
        break;
    case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
        switch(value >> 8) {
        case USB_DT_DEVICE:
            memcpy(data, qemu_keyboard_dev_descriptor, 
                   sizeof(qemu_keyboard_dev_descriptor));
            ret = sizeof(qemu_keyboard_dev_descriptor);
            break;
        case USB_DT_CONFIG:
			memcpy(data, qemu_keyboard_config_descriptor, 
				   sizeof(qemu_keyboard_config_descriptor));
			ret = sizeof(qemu_keyboard_config_descriptor);
            break;
        case USB_DT_STRING:
            switch(value & 0xff) {
            case 0:
                /* language ids */
                data[0] = 4;
                data[1] = 3;
                data[2] = 0x09;
                data[3] = 0x04;
                ret = 4;
                break;
            case 1:
                /* serial number */
                ret = set_usb_string(data, "1");
                break;
            case 2:
                /* product description */
			    ret = set_usb_string(data, "Generic USB Keyboard");
                break;
            case 3:
                /* vendor description */
                ret = set_usb_string(data, "PCSX2/QEMU");
                break;
            case 4:
                ret = set_usb_string(data, "HID Keyboard");
                break;
            case 5:
                ret = set_usb_string(data, "Endpoint1 Interrupt Pipe");
                break;
            default:
                goto fail;
            }
            break;
        default:
            goto fail;
        }
        break;
    case DeviceRequest | USB_REQ_GET_CONFIGURATION:
        data[0] = 1;
        ret = 1;
        break;
    case DeviceOutRequest | USB_REQ_SET_CONFIGURATION:
        ret = 0;
        break;
    case DeviceRequest | USB_REQ_GET_INTERFACE:
        data[0] = 0;
        ret = 1;
        break;
    case DeviceOutRequest | USB_REQ_SET_INTERFACE:
        ret = 0;
        break;
        /* hid specific requests */
    case InterfaceRequest | USB_REQ_GET_DESCRIPTOR:
        switch(value >> 8) {
        case 0x22:
			memcpy(data, qemu_keyboard_hid_report_descriptor, 
				   sizeof(qemu_keyboard_hid_report_descriptor));
			ret = sizeof(qemu_keyboard_hid_report_descriptor);
	    break;
        default:
            goto fail;
        }
        break;
    case GET_REPORT:
	    ret = usb_keyboard_poll(s, data, length);
        break;
    case SET_IDLE:
        ret = 0;
        break;
    default:
    fail:
        ret = USB_RET_STALL;
        break;
    }
    return ret;
}

static int usb_keyboard_handle_data(USBDevice *dev, USBPacket* packet)
{
    USBKeyboardState *s = (USBKeyboardState *)dev;
    int ret = 0;

    switch(packet->pid) {
    case USB_TOKEN_IN:
        if (packet->devep == 1) {
			ret = usb_keyboard_poll(s, packet->data, packet->len);
        } else {
            goto fail;
        }
        break;
    case USB_TOKEN_OUT:
    default:
    fail:
        ret = USB_RET_STALL;
        break;
    }
    return ret;
}

static void usb_keyboard_handle_destroy(USBDevice *dev)
{
    USBKeyboardState *s = (USBKeyboardState *)dev;

    //qemu_add_keyboard_event_handler(NULL, NULL, 0);
    free(s);
}

USBDevice *usb_keyboard_init(void)
{
    USBKeyboardState *s;

    s = (USBKeyboardState *)malloc(sizeof(USBKeyboardState));
    if (!s)
        return NULL;
    memset(s,0,sizeof(USBKeyboardState));
    s->dev.speed = USB_SPEED_FULL;
	s->dev.info = &devinfo;
    s->dev.info->handle_packet = usb_generic_handle_packet;
    s->dev.info->handle_reset = usb_keyboard_handle_reset;
    s->dev.info->handle_control = usb_keyboard_handle_control;
    s->dev.info->handle_data = usb_keyboard_handle_data;
    s->dev.info->handle_destroy = usb_keyboard_handle_destroy;
    s->dev.info->product_desc = "Generic USB Keyboard";

    return (USBDevice *)s;
}

#if 0
#define PS2KBD_VERSION 0x100

#define USB_SUBCLASS_BOOT 1
#define USB_HIDPROTO_KEYBOARD 1

#define PS2KBD_MAXDEV 2
#define PS2KBD_MAXKEYS 6

#define PS2KBD_DEFLINELEN 4096
#define PS2KBD_DEFREPEATRATE 100
/* Default repeat rate in milliseconds */
#define PS2KBD_REPEATWAIT 1000
/* Number of milliseconds to wait before starting key repeat */
#define USB_KEYB_NUMLOCK 0x53
#define USB_KEYB_CAPSLOCK 0x39
#define USB_KEYB_SCRLOCK 0x47

#define USB_KEYB_NUMPAD_START 0x54
#define USB_KEYB_NUMPAD_END 0x63

#define SEMA_ZERO -419
#define SEMA_DELETED -425

int ps2kbd_init();
void ps2kbd_config_set(int resultCode, int bytes, void *arg);
void ps2kbd_idlemode_set(int resultCode, int bytes, void *arg);
void ps2kbd_data_recv(int resultCode, int bytes, void *arg);
int ps2kbd_probe(int devId);
int ps2kbd_connect(int devId);
int ps2kbd_disconnect(int devId);
void usb_getstring(int endp, int index, char *desc);

typedef struct _kbd_data_recv

{
  u8 mod_keys;
  u8 reserved;
  u8 keycodes[PS2KBD_MAXKEYS];
} kbd_data_recv;

typedef struct _keyb_dev

{
  int configEndp;
  int dataEndp;
  int packetSize;
  int devId;
  int interfaceNo;    /* Holds the interface number selected on this device */
  char repeatkeys[2];
  u32 eventmask;
  u8 ledStatus;     /* Maintains state on the led status */
  kbd_data_recv oldData;
  kbd_data_recv data; /* Holds the data for the transfers */
} kbd_dev;

/* Global Variables */

int kbd_readmode;
int kbd_blocking;
u32 kbd_repeatrate;
kbd_dev *devices[PS2KBD_MAXDEV]; /* Holds a list of current devices */
int dev_count;
UsbDriver kbd_driver = { NULL, NULL, "PS2Kbd", ps2kbd_probe, ps2kbd_connect, ps2kbd_disconnect };
u8 *lineBuffer;
u32 lineStartP, lineEndP;
int lineSema;
int bufferSema;
u32 lineSize;
u8 keymap[PS2KBD_KEYMAP_SIZE];         /* Normal key map */
u8 shiftkeymap[PS2KBD_KEYMAP_SIZE];  /* Shifted key map */
u8 keycap[PS2KBD_KEYMAP_SIZE];          /* Does this key get shifted by capslock ? */
u8 special_keys[PS2KBD_KEYMAP_SIZE];
u8 control_map[PS2KBD_KEYMAP_SIZE];
u8 alt_map[PS2KBD_KEYMAP_SIZE];
//static struct fileio_driver kbd_fdriver;
iop_device_t kbd_filedrv;
u8 keyModValue[8] = { 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7 };
int repeat_tid;
int eventid;   /* Id of the repeat event */

int _start ()
{
  FlushDcache();

  ps2kbd_init();

  printf("PS2KBD - USB Keyboard Library\n");

  return 0;

}

int ps2kbd_probe(int devId)

{
  UsbDeviceDescriptor *dev;
  UsbConfigDescriptor *conf;
  UsbInterfaceDescriptor *intf;
  UsbEndpointDescriptor *endp;
  //UsbStringDescriptor *str;

  if(dev_count >= PS2KBD_MAXDEV)
    {
      printf("ERROR: Maximum keyboard devices reached\n");
      return 0;
    }

  //printf("PS2Kbd_probe devId %d\n", devId);

  dev = UsbGetDeviceStaticDescriptor(devId, NULL, USB_DT_DEVICE); /* Get device descriptor */
  if(!dev) 
    {
      printf("ERROR: Couldn't get device descriptor\n");
      return 0;
    }

  //printf("Device class %d, Size %d, Man %d, Product %d Cpnfigurations %d\n", dev->bDeviceClass, dev->bMaxPacketSize0, dev->iManufacturer, dev->iProduct, dev->bNumConfigurations);
  /* Check that the device class is specified in the interfaces and it has at least one configuration */
  if((dev->bDeviceClass != USB_CLASS_PER_INTERFACE) || (dev->bNumConfigurations < 1))
    {
      //printf("This is not the droid you're looking for\n");
      return 0;
    }

  conf = UsbGetDeviceStaticDescriptor(devId, dev, USB_DT_CONFIG);
  if(!conf)
    {
      printf("ERROR: Couldn't get configuration descriptor\n");
      return 0;
    }
  //printf("Config Length %d Total %d Interfaces %d\n", conf->bLength, conf->wTotalLength, conf->bNumInterfaces);

  if((conf->bNumInterfaces < 1) || (conf->wTotalLength < (sizeof(UsbConfigDescriptor) + sizeof(UsbInterfaceDescriptor))))
    {
      printf("ERROR: No interfaces available\n");
      return 0;
    }
     
  intf = (UsbInterfaceDescriptor *) ((char *) conf + conf->bLength); /* Get first interface */
/*   printf("Interface Length %d Endpoints %d Class %d Sub %d Proto %d\n", intf->bLength, */
/* 	 intf->bNumEndpoints, intf->bInterfaceClass, intf->bInterfaceSubClass, */
/* 	 intf->bInterfaceProtocol); */

  if((intf->bInterfaceClass != USB_CLASS_HID) || (intf->bInterfaceSubClass != USB_SUBCLASS_BOOT) ||
     (intf->bInterfaceProtocol != USB_HIDPROTO_KEYBOARD) || (intf->bNumEndpoints < 1))

    {
      //printf("We came, we saw, we told it to fuck off\n");
      return 0;
    }

  endp = (UsbEndpointDescriptor *) ((char *) intf + intf->bLength);
  endp = (UsbEndpointDescriptor *) ((char *) endp + endp->bLength); /* Go to the data endpoint */

  //printf("Endpoint 1 Addr %d, Attr %d, MaxPacket %d\n", endp->bEndpointAddress, endp->bmAttributes, endp->wMaxPacketSizeLB);
  
  if(((endp->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) != USB_ENDPOINT_XFER_INT) ||
     ((endp->bEndpointAddress & USB_ENDPOINT_DIR_MASK) != USB_DIR_IN))
    {
      printf("ERROR: Endpoint not interrupt type and/or an input\n");
      return 0;
    }

  printf("PS2KBD: Found a keyboard device\n");

  return 1;
}

int ps2kbd_connect(int devId)

{
  /* Assume we can only get here if we have already checked the device is kosher */

  UsbDeviceDescriptor *dev;
  UsbConfigDescriptor *conf;
  UsbInterfaceDescriptor *intf;
  UsbEndpointDescriptor *endp;
  kbd_dev *currDev;
  int devLoop;

  //printf("PS2Kbd_connect devId %d\n", devId);

  dev = UsbGetDeviceStaticDescriptor(devId, NULL, USB_DT_DEVICE); /* Get device descriptor */
  if(!dev) 
    {
      printf("ERROR: Couldn't get device descriptor\n");
      return 1;
    }

  conf = UsbGetDeviceStaticDescriptor(devId, dev, USB_DT_CONFIG);
  if(!conf)
    {
      printf("ERROR: Couldn't get configuration descriptor\n");
      return 1;
    }
     
  intf = (UsbInterfaceDescriptor *) ((char *) conf + conf->bLength); /* Get first interface */
  endp = (UsbEndpointDescriptor *) ((char *) intf + intf->bLength);
  endp = (UsbEndpointDescriptor *) ((char *) endp + endp->bLength); /* Go to the data endpoint */

  for(devLoop = 0; devLoop < PS2KBD_MAXDEV; devLoop++)
    {
      if(devices[devLoop] == NULL)
	{
	  break;
	}
    }

  if(devLoop == PS2KBD_MAXDEV)
    {
      /* How the f*** did we end up here ??? */
      printf("ERROR: Device Weirdness!!\n");
      return 1;
    }

  currDev = (kbd_dev *) AllocSysMemory(0, sizeof(kbd_dev), NULL);
  if(!currDev)
    {
      printf("ERROR: Couldn't allocate a device point for the kbd\n");
      return 1;
    }

  devices[devLoop] = currDev;
  memset(currDev, 0, sizeof(kbd_dev));
  currDev->configEndp = UsbOpenEndpoint(devId, NULL);
  currDev->dataEndp = UsbOpenEndpoint(devId, endp);
  currDev->packetSize = endp->wMaxPacketSizeLB | ((int) endp->wMaxPacketSizeHB << 8);
  currDev->eventmask = (1 << devLoop);
  if(currDev->packetSize > sizeof(kbd_data_recv))
    {
      currDev->packetSize = sizeof(kbd_data_recv);
    }

  if(dev->iManufacturer != 0)
    {
      usb_getstring(currDev->configEndp, dev->iManufacturer, "Keyboard Manufacturer");
    }

  if(dev->iProduct != 0)
    {
      usb_getstring(currDev->configEndp, dev->iProduct, "Keyboard Product");
    }

  currDev->devId = devId;
  currDev->interfaceNo = intf->bInterfaceNumber;
  currDev->ledStatus = 0;

  UsbSetDevicePrivateData(devId, currDev); /* Set the index for the device data */

  //printf("Configuration value %d\n", conf->bConfigurationValue);
  UsbSetDeviceConfiguration(currDev->configEndp, conf->bConfigurationValue, ps2kbd_config_set, currDev);

  dev_count++; /* Increment device count */
  printf("PS2KBD: Connected device\n");

  return 0;
}

int ps2kbd_disconnect(int devId)

{
  int devLoop;
  //printf("PS2Kbd_disconnect devId %d\n", devId);

  for(devLoop = 0; devLoop < PS2KBD_MAXDEV; devLoop++)
    {
      if((devices[devLoop]) && (devices[devLoop]->devId == devId))
	{
	  dev_count--;
	  FreeSysMemory(devices[devLoop]);
	  devices[devLoop] = NULL;
	  printf("PS2KBD: Disconnected device\n");
	  break;
	}
    }

  return 0;
}

typedef struct _string_descriptor

{
  u8 buf[200];
  char *desc;
} string_descriptor; 

void ps2kbd_getstring_set(int resultCode, int bytes, void *arg)

{
  UsbStringDescriptor *str = (UsbStringDescriptor *) arg;
  string_descriptor *strBuf = (string_descriptor *) arg;
  char string[50];
  int strLoop;

/*   printf("=========getstring=========\n"); */

/*   printf("PS2KEYBOARD: GET_DESCRIPTOR res %d, bytes %d, arg %p\n", resultCode, bytes, arg); */

  if(resultCode == USB_RC_OK)
    {
      memset(string, 0, 50);
      for(strLoop = 0; strLoop < ((bytes - 2) / 2); strLoop++)
	{
	  string[strLoop] = str->wData[strLoop] & 0xFF;
	}
      printf("PS2KBD %s: %s\n", strBuf->desc, string);
    }
  
  FreeSysMemory(arg);
}

void usb_getstring(int endp, int index, char *desc)

{
  u8 *data;
  string_descriptor *str;
  int ret; 

  data = (u8 *) AllocSysMemory(0, sizeof(string_descriptor), NULL);
  str = (string_descriptor *) data;

  if(data != NULL)
    {
      str->desc = desc;
      ret = UsbControlTransfer(endp, 0x80, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) | index, 
			       0, sizeof(string_descriptor) - 4, data, ps2kbd_getstring_set, data);
      if(ret != USB_RC_OK)
	{
	  printf("PS2KBD: Error sending string descriptor request\n");
	  FreeSysMemory(data);
	}
    }
}

void ps2kbd_config_set(int resultCode, int bytes, void *arg)
     /* Called when we have finished choosing our configuration */

{
  kbd_dev *dev;

  if(resultCode != USB_RC_OK)
    {
      printf("PS2KEYBOARD: Configuration set error res %d, bytes %d, arg %p\n", resultCode, bytes, arg);
      return;
    }

  //printf("PS2KEYBOARD: Configuration set res %d, bytes %d, arg %p\n", resultCode, bytes, arg);
  /* Do a interrupt data transfer */

  dev = (kbd_dev *) arg;
  if(dev != NULL)
    {
      int ret;
      
      ret = UsbControlTransfer(dev->configEndp, 0x21, USB_REQ_SET_IDLE, 0, dev->interfaceNo, 0, NULL, ps2kbd_idlemode_set, arg);
    }
}

void ps2kbd_idlemode_set(int resultCode, int bytes, void *arg)

{
  kbd_dev *dev;



  if(resultCode != USB_RC_OK)
    {
      printf("PS2KBD: Idlemode set error res %d, bytes %d, arg %p\n", resultCode, bytes, arg);
      return;
    }
  
  dev = (kbd_dev *) arg;
  if(dev != NULL)
    {
      int ret;
      
      ret = UsbInterruptTransfer(dev->dataEndp, &dev->data, dev->packetSize, ps2kbd_data_recv, arg);
    }
}

void ps2kbd_led_set(int resultCode, int bytes, void *arg)

{
  //printf("LED Set\n");
}

void ps2kbd_build_uniquekeys(u8 *res, const u8 *new, const u8 *old)

     /* Builds a list of unique keys */

{
  int loopNew, loopOld;
  int loopRes = 0;
  int foundKey;

  for(loopNew = 0; loopNew < PS2KBD_MAXKEYS; loopNew++)
    {
      if(new[loopNew] != 0)
	{
	  foundKey = 0;
	  for(loopOld = 0; loopOld < PS2KBD_MAXKEYS; loopOld++)
	    {
	      if(new[loopNew] == old[loopOld])
		{
		  foundKey = 1;
		  break;
		}
	    }
	  if(!foundKey)
	    {
	      res[loopRes++] = new[loopNew];
	    }
	}
    }
}

u32 ps2kbd_repeathandler(void *arg)

{
  kbd_dev *dev = arg;
  iop_sys_clock_t t;
  //printf("Repeat handler\n");

  iSetEventFlag(eventid, dev->eventmask);

  USec2SysClock(kbd_repeatrate * 1000, &t);
  iSetAlarm(&t, (void *)ps2kbd_repeathandler, arg);

  return t.hi;
}

void ps2kbd_getkeys(u8 keyMods, u8 ledStatus, const u8 *keys, kbd_dev *dev)

{
  int loopKey;
  int tempPos = 0;
  int byteCount = 0;
  u8 currChars[2];

  if(lineStartP < lineEndP)
    {
      tempPos = lineStartP + lineSize;
    }
  else
    {
      tempPos = lineStartP;
    }
  
  for(loopKey = 0; loopKey < PS2KBD_MAXKEYS; loopKey++)
    {
      u8 currKey = keys[loopKey];

      currChars[0] = 0;
      currChars[1] = 0;

      if(lineEndP == (tempPos - 1))
	{
	  break;
	}

      if(currKey) /* If this is a valid key */
	{
	  if((currKey >= USB_KEYB_NUMPAD_START) && (currKey <= USB_KEYB_NUMPAD_END))
	    /* Handle numpad specially */
	    {
	      if(ledStatus & PS2KBD_LED_NUMLOCK)
		{
		  if(keymap[currKey])
		    {
		      currChars[0] = keymap[currKey];
		    }
		}
	      else
		{
		  if(special_keys[currKey])
		    {
		      currChars[0] = PS2KBD_ESCAPE_KEY;
		      currChars[1] = special_keys[currKey];
		    }
		  else if(keymap[currKey] != '5') /* Make sure this isnt a 5 key :) */
		    {
		      currChars[0] = keymap[currKey];
		    }
		}
	    }
	  else if(special_keys[currKey]) /* This is a special key */
	    {
	      currChars[0] = PS2KBD_ESCAPE_KEY;
	      currChars[1] = special_keys[currKey];
	    }
	  else if(keyMods & PS2KBD_CTRL) /* CTRL */
	    {
	      if(control_map[currKey])
		{
		  currChars[0] = control_map[currKey];
		}
	    }
	  else if(keyMods & PS2KBD_ALT) /* ALT */
	    {
	      if(alt_map[currKey])
		{
		  currChars[0] = alt_map[currKey];
		}
	    }
	  else if(keyMods & PS2KBD_SHIFT) /* SHIFT */
	    {
	      if((ledStatus & PS2KBD_LED_CAPSLOCK) && (keycap[currKey]))
		{
		  currChars[0] = keymap[currKey];
		}
	      else
		{
		  currChars[0] = shiftkeymap[currKey];
		}
	    }
	  else /* Normal key */
	    {
	      if(keymap[keys[loopKey]])
		{
		  if((ledStatus & PS2KBD_LED_CAPSLOCK) && (keycap[currKey]))
		    {
		      currChars[0] = shiftkeymap[currKey];
		    }
		  else
		    {
		      currChars[0] = keymap[currKey];
		    }
		}
	    }
	}

      if((currChars[0] == PS2KBD_ESCAPE_KEY) && (currChars[1] != 0))
	{
	  if(lineEndP != (tempPos - 2))
	    {
	      lineBuffer[lineEndP++] = currChars[0];
	      lineEndP %= lineSize;
	      lineBuffer[lineEndP++] = currChars[1];
	      lineEndP %= lineSize;
	      byteCount += 2;
	    }
	  dev->repeatkeys[0] = currChars[0];
	  dev->repeatkeys[1] = currChars[1];
	}
      else if(currChars[0] != 0)
	{
	  lineBuffer[lineEndP++] = currChars[0];
	  lineEndP %= lineSize;
	  byteCount++;
	  dev->repeatkeys[0] = currChars[0];
	  dev->repeatkeys[1] = 0;
	}
    }

  if(byteCount > 0) 
    {
      iop_sys_clock_t t;
      /* Set alarm to do repeat rate */
      //printf("repeatkeys %d %d\n", kbd_repeatkeys[0], kbd_repeatkeys[1]);
      USec2SysClock(PS2KBD_REPEATWAIT * 1000, &t);
      SetAlarm(&t, (void *)ps2kbd_repeathandler, dev);
    }

  for(loopKey = 0; loopKey < byteCount; loopKey++) /* Signal the sema to indicate data */
    {
      SignalSema(bufferSema);
    }

/*   lineBuffer[PS2KBD_DEFLINELEN - 1] = 0; */
/*   printf(lineBuffer); */
  //printf("lineStart %d, lineEnd %d\n", lineStartP, lineEndP);
}


void ps2kbd_getkeys_raw(u8 newKeyMods, u8 oldKeyMods, u8 *new, const u8 *old)

{
  int loopKey;
  u8 currKey;
  u8 keyMods = newKeyMods ^ oldKeyMods;
  u8 keyModsMap = newKeyMods & keyMods;
  int tempPos = 0;
  int byteCount = 0;

  if(lineStartP < lineEndP)
    {
      tempPos = lineStartP + lineSize;
    }
  else
    {
      tempPos = lineStartP;
    }
  
  for(loopKey = 0; loopKey < 8; loopKey++)
    {
      int currMod = (1 << loopKey);
      if(keyMods & currMod) 
	{
	  if(lineEndP == (tempPos - 2))
	    {
	      return;
	    }

	  currKey = keyModValue[loopKey];

	  if(keyModsMap & currMod) /* If key pressed */
	    {
	      lineBuffer[lineEndP++] = PS2KBD_RAWKEY_DOWN;
	      //printf("Key down\n");
	    }
	  else
	    {
	      lineBuffer[lineEndP++] = PS2KBD_RAWKEY_UP;
	      //printf("Key up\n");
	    }

	  lineEndP %= lineSize;
	  lineBuffer[lineEndP++] = currKey;
	  lineEndP %= lineSize;
	  byteCount += 2;
	  //printf("Key %d\n", currKey);
	}
    }

  for(loopKey = 0; loopKey < PS2KBD_MAXKEYS; loopKey++)
    {
      if(lineEndP == (tempPos - 2))
	{
	  return;
	}

      if(new[loopKey] != 0)
	{
	  lineBuffer[lineEndP++] = PS2KBD_RAWKEY_DOWN;
	  lineEndP %= lineSize;
	  lineBuffer[lineEndP++] = new[loopKey];
	  lineEndP %= lineSize;
	  byteCount += 2;
	  //printf("Key down\nKey %d\n", new[loopKey]);
	}

    }

  for(loopKey = 0; loopKey < PS2KBD_MAXKEYS; loopKey++)
    {
      if(lineEndP == (tempPos - 2))
	{
	  return;
	}

      if(old[loopKey] != 0)
	{
	  lineBuffer[lineEndP++] = PS2KBD_RAWKEY_UP;
	  lineEndP %= lineSize;
	  lineBuffer[lineEndP++] = old[loopKey];
	  lineEndP %= lineSize;
	  byteCount += 2;
	  //printf("Key up\nKey %d\n", old[loopKey]);
	}

    }

  for(loopKey = 0; loopKey < byteCount; loopKey++) /* Signal the sema for the number of bytes read */
    {
      SignalSema(bufferSema);
    }
}

void ps2kbd_data_recv(int resultCode, int bytes, void *arg)

{
  kbd_dev *dev;
  int ret;
  int phantom;
  int loop;

  if(resultCode != USB_RC_OK)
    {
      printf("PS2KEYBOARD: Data Recv set res %d, bytes %d, arg %p\n", resultCode, bytes, arg);
      return;
    }
  
  //printf("PS2KBD: Data Recv set res %d, bytes %d, arg %p\n", resultCode, bytes, arg);

  dev = (kbd_dev *) arg;
  if(dev == NULL)
    {
      printf("PS2KBD: dev == NULL\n");
      return;
    }
    
/*       printf("PS2KBD Modifiers %02X, Keys ", dev->data.mod_keys); */
/*       for(loop = 0; loop < PS2KBD_MAXKEYS; loop++) */
/* 	{ */
/* 	  printf("%02X ", dev->data.keycodes[loop]); */
/* 	} */
/*       printf("\n"); */

  CancelAlarm((void *)ps2kbd_repeathandler, dev); /* Make sure repeat alarm is cancelled */

  /* Check for phantom states */
  phantom = 1;
  for(loop = 0; loop < PS2KBD_MAXKEYS; loop++)
    {
      if(dev->data.keycodes[loop] != 1)
	{
	  phantom = 0;
	  break;
	}
    }
  
  if(!phantom) /* If not in a phantom state */
    {
      u8 uniqueKeys[PS2KBD_MAXKEYS];
      u8 missingKeys[PS2KBD_MAXKEYS];
      int loopKey;

      memset(uniqueKeys, 0, PS2KBD_MAXKEYS);
      memset(missingKeys, 0, PS2KBD_MAXKEYS);
      ps2kbd_build_uniquekeys(uniqueKeys, dev->data.keycodes, dev->oldData.keycodes);
      ps2kbd_build_uniquekeys(missingKeys, dev->oldData.keycodes, dev->data.keycodes);
      /* Build new and missing key lists */

/*       printf("Unique keys : "); */
/*       for(loopKey = 0; loopKey < PS2KBD_MAXKEYS; loopKey++) */
/* 	{ */
/* 	  printf("%02X ", uniqueKeys[loopKey]); */
/* 	} */
/*       printf("\n"); */

/*       printf("Missing keys : "); */
/*       for(loopKey = 0; loopKey < PS2KBD_MAXKEYS; loopKey++) */
/* 	{ */
/* 	  printf("%02X ", missingKeys[loopKey]); */
/* 	} */
/*       printf("\n"); */
      
      if(kbd_readmode == PS2KBD_READMODE_NORMAL)
	{
	  u8 ledStatus;

	  ledStatus = dev->ledStatus;
	  //printf("ledStatus %02X\n", ledStatus);
	  
	  for(loopKey = 0; loopKey < PS2KBD_MAXKEYS; loopKey++) /* Process key codes */
	    {
	      switch(uniqueKeys[loopKey])
		{
		case USB_KEYB_NUMLOCK : 
		  ledStatus ^= PS2KBD_LED_NUMLOCK;
		  uniqueKeys[loopKey] = 0;
		  break;
		case USB_KEYB_CAPSLOCK : 
		  ledStatus ^= PS2KBD_LED_CAPSLOCK;
		  uniqueKeys[loopKey] = 0;
		  break;
		case USB_KEYB_SCRLOCK :
		  ledStatus ^= PS2KBD_LED_SCRLOCK;
		  uniqueKeys[loopKey] = 0;
		  break;
		}
	    }
	  
	  if(ledStatus != dev->ledStatus)
	    {
	      dev->ledStatus = ledStatus & PS2KBD_LED_MASK;
	      //printf("LEDS %02X\n", dev->ledStatus);
	      /* Call Set LEDS */
	      UsbControlTransfer(dev->configEndp, 0x21, USB_REQ_SET_REPORT, 0x200, 
				 dev->interfaceNo, 1, &dev->ledStatus, ps2kbd_led_set, arg);
	    }
	  
	  WaitSema(lineSema); /* Make sure no other thread is going to manipulate the buffer */
	  ps2kbd_getkeys(dev->data.mod_keys, dev->ledStatus, uniqueKeys, dev); /* read in remaining keys */
	  SignalSema(lineSema);
	}
      else /* RAW Mode */
	{
	  WaitSema(lineSema);
	  ps2kbd_getkeys_raw(dev->data.mod_keys, dev->oldData.mod_keys, uniqueKeys, missingKeys);
	  SignalSema(lineSema);
	}

      memcpy(&dev->oldData, &dev->data, sizeof(kbd_data_recv));
    }
  
  ret = UsbInterruptTransfer(dev->dataEndp, &dev->data, dev->packetSize, ps2kbd_data_recv, arg);
}

void flushbuffer()

{
  iop_sema_t s;

  lineStartP = 0;
  lineEndP = 0;
  memset(lineBuffer, 0, lineSize);
  
  DeleteSema(bufferSema);
  s.initial = 0;
  s.max = lineSize;
  s.option = 0;
  s.attr = 0;
  bufferSema = CreateSema(&s); /* Create a sema to maintain status of readable data */
  
  if(bufferSema <= 0)
    {
      printf("PS2KBD: Error creating buffer sema\n");
    }
}

void ps2kbd_ioctl_setreadmode(u32 readmode)

{
  int devLoop;

  if(readmode == kbd_readmode) return; 

  if((readmode == PS2KBD_READMODE_NORMAL) || (readmode == PS2KBD_READMODE_RAW))
    {
      /* Reset line buffer */
      //printf("ioctl_setreadmode %d\n", readmode);
      for(devLoop = 0; devLoop < PS2KBD_MAXDEV; devLoop++)
	{
	  CancelAlarm((void *)ps2kbd_repeathandler, devices[devLoop]);
	}

      WaitSema(lineSema);
      kbd_readmode = readmode;
      flushbuffer();
      SignalSema(lineSema);
    }
}

void ps2kbd_ioctl_setkeymap(kbd_keymap *keymaps)

{
  //printf("ioctl_setkeymap %p\n", keymaps);
  WaitSema(lineSema);   /* Lock the input so you dont end up with weird results */
  memcpy(keymap, keymaps->keymap, PS2KBD_KEYMAP_SIZE);
  memcpy(shiftkeymap, keymaps->shiftkeymap, PS2KBD_KEYMAP_SIZE);
  memcpy(keycap, keymaps->keycap, PS2KBD_KEYMAP_SIZE);
  SignalSema(lineSema);
}

void ps2kbd_ioctl_setctrlmap(u8 *ctrlmap)

{
  //printf("ioctl_setctrlmap %p\n", ctrlmap);
  WaitSema(lineSema);
  memcpy(control_map, ctrlmap, PS2KBD_KEYMAP_SIZE);
  SignalSema(lineSema);
}

void ps2kbd_ioctl_setaltmap(u8 *altmap)

{
  //printf("ioctl_setaltmap %p\n", altmap);
  WaitSema(lineSema);
  memcpy(alt_map, altmap, PS2KBD_KEYMAP_SIZE);
  SignalSema(lineSema);
}

void ps2kbd_ioctl_setspecialmap(u8 *special)

{
  //printf("ioctl_setspecialmap %p\n", special);
  WaitSema(lineSema);
  memcpy(special_keys, special, PS2KBD_KEYMAP_SIZE);
  SignalSema(lineSema);
}

void ps2kbd_ioctl_resetkeymap()
     /* Reset keymap to default US variety */

{
  //printf("ioctl_resetkeymap()\n");
  WaitSema(lineSema);
  memcpy(keymap, us_keymap, PS2KBD_KEYMAP_SIZE);
  memcpy(shiftkeymap, us_shiftkeymap, PS2KBD_KEYMAP_SIZE);
  memcpy(keycap, us_keycap, PS2KBD_KEYMAP_SIZE);
  memcpy(special_keys, us_special_keys, PS2KBD_KEYMAP_SIZE);
  memcpy(control_map, us_control_map, PS2KBD_KEYMAP_SIZE);
  memcpy(alt_map, us_alt_map, PS2KBD_KEYMAP_SIZE);
  SignalSema(lineSema);
}

void ps2kbd_ioctl_flushbuffer()
     /* Flush the internal buffer */

{
  //printf("ioctl_flushbuffer()\n");
  WaitSema(lineSema);
  flushbuffer();
  SignalSema(lineSema);
}

void ps2kbd_ioctl_setleds(u8 ledStatus)

{
  int devLoop;
  kbd_dev *dev;

  //printf("ioctl_setleds %d\n", ledStatus);
  ledStatus &= PS2KBD_LED_MASK;
  for(devLoop = 0; devLoop < PS2KBD_MAXDEV; devLoop++)
    {
      dev = devices[devLoop];
      if(dev)
	{
	  if(ledStatus != dev->ledStatus)
	    {
	      dev->ledStatus = ledStatus & PS2KBD_LED_MASK;
	      UsbControlTransfer(dev->configEndp, 0x21, USB_REQ_SET_REPORT, 0x200,
				 dev->interfaceNo, 1, &dev->ledStatus, ps2kbd_led_set, dev);
	    }
	}
    }
}

void ps2kbd_ioctl_setblockmode(u32 blockmode)

{
  if((blockmode == PS2KBD_BLOCKING) || (blockmode == PS2KBD_NONBLOCKING))
    {
      kbd_blocking = blockmode;
      //printf("ioctl_setblockmode %d\n", blockmode);
    }
}

void ps2kbd_ioctl_setrepeatrate(u32 rate)

{
  kbd_repeatrate = rate;
}

int fio_dummy()
{
  //printf("fio_dummy()\n");
  return -5;
}

int fio_init(iop_device_t *driver)
{
  //printf("fio_init()\n");
  return 0;
}

int fio_format(iop_file_t *f)
{
  //printf("fio_format()\n");
  return 0;
}

int fio_open(iop_file_t *f, const char *name, int mode)
{
  //printf("fio_open() %s %d\n", name, mode);
  if(strcmp(name, PS2KBD_KBDFILE)) /* If not the keyboard file */
    {
      return -1;
    }

  return 0;
}

int fio_read(iop_file_t *f, void *buf, int size)

{
  int count = 0;
  char *data = (char *) buf;
  int ret;

  //printf("fio_read() %p %d\n", buf, size);

  if(kbd_readmode == PS2KBD_READMODE_RAW)
    {
      size &= ~1; /* Ensure size of a multiple of 2 */
    }

  ret = PollSema(bufferSema);
  if(ret < 0) 
    {
      if((ret == SEMA_ZERO) && (kbd_blocking == PS2KBD_BLOCKING))
	{
	  //printf("Blocking\n");
	  ret = WaitSema(bufferSema);
	  if(ret < 0) /* Means either an error or the sema was deleted from under us */
	    {
	      return 0;
	    }
	}
      else
	{
	  return 0;
	}
    }
  
  SignalSema(bufferSema);
  ret = WaitSema(lineSema);
  if(ret < 0) return 0;

  while((count < size) && (lineStartP != lineEndP))
    {
      data[count] = lineBuffer[lineStartP++];
      lineStartP %= lineSize;
      count++;
      PollSema(bufferSema); /* Take off one count from the sema */
    }

  SignalSema(lineSema);
/*   //printf("read %d bytes\n", count); */
/*   { */
/*     struct t_sema_status s; */

/*     ReferSemaStatus(bufferSema, &s); */
/*     //printf("Sema count %d\n", s.curr_count); */
/*   } */
  
  return count;
}

int fio_ioctl(iop_file_t *f, unsigned long arg, void *param)

{
  //printf("fio_ioctl() %ld %d\n", arg, *((u32 *) param));
  switch(arg) 
    {
    case PS2KBD_IOCTL_SETREADMODE: ps2kbd_ioctl_setreadmode(*((u32 *) param));
      break;
    case PS2KBD_IOCTL_SETKEYMAP: ps2kbd_ioctl_setkeymap((kbd_keymap *) param);
      break;
    case PS2KBD_IOCTL_SETALTMAP: ps2kbd_ioctl_setaltmap((u8 *) param);
      break;
    case PS2KBD_IOCTL_SETCTRLMAP: ps2kbd_ioctl_setctrlmap((u8 *) param);
      break;
    case PS2KBD_IOCTL_SETSPECIALMAP: ps2kbd_ioctl_setspecialmap((u8 *) param);
      break;
    case PS2KBD_IOCTL_FLUSHBUFFER: ps2kbd_ioctl_flushbuffer();
      break;
    case PS2KBD_IOCTL_SETLEDS: ps2kbd_ioctl_setleds(*(u8*) param);
      break;
    case PS2KBD_IOCTL_SETBLOCKMODE: ps2kbd_ioctl_setblockmode(*(u32 *) param);
      break;
    case PS2KBD_IOCTL_RESETKEYMAP: ps2kbd_ioctl_resetkeymap();
      break;
    case PS2KBD_IOCTL_SETREPEATRATE: ps2kbd_ioctl_setrepeatrate(*(u32 *) param);
      break;
    default : return -1;
    }

  return 0;
}

int fio_close(iop_file_t *f)

{
  //printf("fio_close()\n");
  return 0;
}

iop_device_ops_t fio_ops = 

  {
    fio_init,
    fio_dummy,
    fio_format,
    fio_open,
    fio_close,
    fio_read,
    fio_dummy,
    fio_dummy,
    fio_ioctl,
    fio_dummy,
    fio_dummy,
    fio_dummy,
    fio_dummy,
    fio_dummy,
    fio_dummy,
    fio_dummy,
    fio_dummy
  };

int init_fio()

{
  kbd_filedrv.name = PS2KBD_FSNAME;
  kbd_filedrv.type = IOP_DT_CHAR;
  kbd_filedrv.version = 0;
  kbd_filedrv.desc = "USB Keyboard FIO driver";
  kbd_filedrv.ops = &fio_ops;
  return AddDrv(&kbd_filedrv);
}

void repeat_thread(void *arg)

{
  u32 eventmask;
  int devLoop;

  for(;;)
    {
      WaitEventFlag(eventid, 0xFFFFFFFF, 0x01 | 0x10, &eventmask);
      //printf("Recieved event %08X\n", eventmask);
      for(devLoop = 0; devLoop < PS2KBD_MAXDEV; devLoop++)
	{
	  if((eventmask & (1 << devLoop)) && (devices[devLoop]))
	    {
	      int tempPos = 0;

	      WaitSema(lineSema);
	      if(lineStartP < lineEndP)
		{
		  tempPos = lineStartP + lineSize;
		}
	      else
		{
		  tempPos = lineStartP;
		}

	      if((devices[devLoop]->repeatkeys[0]) && (devices[devLoop]->repeatkeys[1]))
		{
		  if(lineEndP != (tempPos - 2))
		    {
		      lineBuffer[lineEndP++] = devices[devLoop]->repeatkeys[0];
		      lineEndP %= lineSize;	      
		      lineBuffer[lineEndP++] = devices[devLoop]->repeatkeys[1];
		      lineEndP %= lineSize;
		      SignalSema(bufferSema);
		      SignalSema(bufferSema);
		    }
		}
	      else if(devices[devLoop]->repeatkeys[0])
		{
		  if(lineEndP != (tempPos - 1))
		    {
		      lineBuffer[lineEndP++] = devices[devLoop]->repeatkeys[0];
		      lineEndP %= lineSize;	      
		      SignalSema(bufferSema);
		    }
		}

	      SignalSema(lineSema);
	    }
	}
    }
}

int init_repeatthread()
     /* Creates a thread to handle key repeats */

{
  iop_thread_t param;
  iop_event_t event;

  event.attr = 0;
  event.option = 0;
  event.bits = 0;
  eventid = CreateEventFlag(&event);

  param.attr         = TH_C;
  param.thread    = repeat_thread;
  param.priority     = 40;
  param.stacksize    = 0x800;
  param.option       = 0;

  repeat_tid = CreateThread(&param);
  if (repeat_tid > 0) {
    StartThread(repeat_tid, 0);
    return 0;
  }
  else 
    {
      return 1;
    }
}

int ps2kbd_init()

{
  int ret;
  iop_sema_t s;

  s.initial = 1;
  s.max = 1;
  s.option = 0;
  s.attr = 0;
  lineSema = CreateSema(&s);
  if(lineSema <= 0)
    {
      printf("PS2KBD: Error creating sema\n");
      return 1;
    }

  s.initial = 0;
  s.max = PS2KBD_DEFLINELEN;
  s.option = 0;
  s.attr = 0;
  bufferSema = CreateSema(&s); /* Create a sema to maintain status of readable data */
  if(bufferSema <= 0)
    {
      printf("PS2KBD: Error creating buffer sema\n");
      return 1;
    }

  lineBuffer = (u8 *) AllocSysMemory(0, PS2KBD_DEFLINELEN, NULL);
  if(lineBuffer == NULL)
    {
      printf("PS2KBD: Error allocating line buffer\n");
      return 1;
    }
  lineStartP = 0;
  lineEndP = 0;
  lineSize = PS2KBD_DEFLINELEN;
  memset(lineBuffer, 0, PS2KBD_DEFLINELEN);
  
  memset(devices, 0, sizeof(kbd_dev *) * PS2KBD_MAXDEV);
  dev_count = 0;
  kbd_blocking = PS2KBD_NONBLOCKING;
  kbd_readmode = PS2KBD_READMODE_NORMAL;
  kbd_repeatrate = PS2KBD_DEFREPEATRATE;
  memcpy(keymap, us_keymap, PS2KBD_KEYMAP_SIZE);
  memcpy(shiftkeymap, us_shiftkeymap, PS2KBD_KEYMAP_SIZE);
  memcpy(keycap, us_keycap, PS2KBD_KEYMAP_SIZE);
  memcpy(special_keys, us_special_keys, PS2KBD_KEYMAP_SIZE);
  memcpy(control_map, us_control_map, PS2KBD_KEYMAP_SIZE);
  memcpy(alt_map, us_alt_map, PS2KBD_KEYMAP_SIZE);

  ret = init_fio();
  //printf("ps2kbd AddDrv [%d]\n", ret);
  init_repeatthread();

  ret = UsbRegisterDriver(&kbd_driver);
  if(ret != USB_RC_OK)
    {
      printf("PS2KBD: Error registering USB devices\n");
      return 1;
    }

  //printf("UsbRegisterDriver %d\n", ret);

  return 0;
}
#endif