/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* General keyboard handling code for SDL */

#include "SDL_timer.h"
#include "SDL_events.h"
#include "SDL_events_c.h"
#include "../video/SDL_sysvideo.h"


/* Global keyboard information */

typedef struct SDL_Keyboard SDL_Keyboard;

struct SDL_Keyboard
{
    /* Data common to all keyboards */
    SDL_Window *focus;
    Uint16 modstate;
    Uint8 keystate[SDL_NUM_SCANCODES];
    SDL_Keycode keymap[SDL_NUM_SCANCODES];
};

static SDL_Keyboard SDL_keyboard;

static const SDL_Keycode SDL_default_keymap[SDL_NUM_SCANCODES] = {
    0, 0, 0, 0,
    'a',
    'b',
    'c',
    'd',
    'e',
    'f',
    'g',
    'h',
    'i',
    'j',
    'k',
    'l',
    'm',
    'n',
    'o',
    'p',
    'q',
    'r',
    's',
    't',
    'u',
    'v',
    'w',
    'x',
    'y',
    'z',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    SDLK_RETURN,
    SDLK_ESCAPE,
    SDLK_BACKSPACE,
    SDLK_TAB,
    SDLK_SPACE,
    '-',
    '=',
    '[',
    ']',
    '\\',
    '#',
    ';',
    '\'',
    '`',
    ',',
    '.',
    '/',
    SDLK_CAPSLOCK,
    SDLK_F1,
    SDLK_F2,
    SDLK_F3,
    SDLK_F4,
    SDLK_F5,
    SDLK_F6,
    SDLK_F7,
    SDLK_F8,
    SDLK_F9,
    SDLK_F10,
    SDLK_F11,
    SDLK_F12,
    SDLK_PRINTSCREEN,
    SDLK_SCROLLLOCK,
    SDLK_PAUSE,
    SDLK_INSERT,
    SDLK_HOME,
    SDLK_PAGEUP,
    SDLK_DELETE,
    SDLK_END,
    SDLK_PAGEDOWN,
    SDLK_RIGHT,
    SDLK_LEFT,
    SDLK_DOWN,
    SDLK_UP,
    SDLK_NUMLOCKCLEAR,
    SDLK_KP_DIVIDE,
    SDLK_KP_MULTIPLY,
    SDLK_KP_MINUS,
    SDLK_KP_PLUS,
    SDLK_KP_ENTER,
    SDLK_KP_1,
    SDLK_KP_2,
    SDLK_KP_3,
    SDLK_KP_4,
    SDLK_KP_5,
    SDLK_KP_6,
    SDLK_KP_7,
    SDLK_KP_8,
    SDLK_KP_9,
    SDLK_KP_0,
    SDLK_KP_PERIOD,
    0,
    SDLK_APPLICATION,
    SDLK_POWER,
    SDLK_KP_EQUALS,
    SDLK_F13,
    SDLK_F14,
    SDLK_F15,
    SDLK_F16,
    SDLK_F17,
    SDLK_F18,
    SDLK_F19,
    SDLK_F20,
    SDLK_F21,
    SDLK_F22,
    SDLK_F23,
    SDLK_F24,
    SDLK_EXECUTE,
    SDLK_HELP,
    SDLK_MENU,
    SDLK_SELECT,
    SDLK_STOP,
    SDLK_AGAIN,
    SDLK_UNDO,
    SDLK_CUT,
    SDLK_COPY,
    SDLK_PASTE,
    SDLK_FIND,
    SDLK_MUTE,
    SDLK_VOLUMEUP,
    SDLK_VOLUMEDOWN,
    0, 0, 0,
    SDLK_KP_COMMA,
    SDLK_KP_EQUALSAS400,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    SDLK_ALTERASE,
    SDLK_SYSREQ,
    SDLK_CANCEL,
    SDLK_CLEAR,
    SDLK_PRIOR,
    SDLK_RETURN2,
    SDLK_SEPARATOR,
    SDLK_OUT,
    SDLK_OPER,
    SDLK_CLEARAGAIN,
    SDLK_CRSEL,
    SDLK_EXSEL,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    SDLK_KP_00,
    SDLK_KP_000,
    SDLK_THOUSANDSSEPARATOR,
    SDLK_DECIMALSEPARATOR,
    SDLK_CURRENCYUNIT,
    SDLK_CURRENCYSUBUNIT,
    SDLK_KP_LEFTPAREN,
    SDLK_KP_RIGHTPAREN,
    SDLK_KP_LEFTBRACE,
    SDLK_KP_RIGHTBRACE,
    SDLK_KP_TAB,
    SDLK_KP_BACKSPACE,
    SDLK_KP_A,
    SDLK_KP_B,
    SDLK_KP_C,
    SDLK_KP_D,
    SDLK_KP_E,
    SDLK_KP_F,
    SDLK_KP_XOR,
    SDLK_KP_POWER,
    SDLK_KP_PERCENT,
    SDLK_KP_LESS,
    SDLK_KP_GREATER,
    SDLK_KP_AMPERSAND,
    SDLK_KP_DBLAMPERSAND,
    SDLK_KP_VERTICALBAR,
    SDLK_KP_DBLVERTICALBAR,
    SDLK_KP_COLON,
    SDLK_KP_HASH,
    SDLK_KP_SPACE,
    SDLK_KP_AT,
    SDLK_KP_EXCLAM,
    SDLK_KP_MEMSTORE,
    SDLK_KP_MEMRECALL,
    SDLK_KP_MEMCLEAR,
    SDLK_KP_MEMADD,
    SDLK_KP_MEMSUBTRACT,
    SDLK_KP_MEMMULTIPLY,
    SDLK_KP_MEMDIVIDE,
    SDLK_KP_PLUSMINUS,
    SDLK_KP_CLEAR,
    SDLK_KP_CLEARENTRY,
    SDLK_KP_BINARY,
    SDLK_KP_OCTAL,
    SDLK_KP_DECIMAL,
    SDLK_KP_HEXADECIMAL,
    0, 0,
    SDLK_LCTRL,
    SDLK_LSHIFT,
    SDLK_LALT,
    SDLK_LGUI,
    SDLK_RCTRL,
    SDLK_RSHIFT,
    SDLK_RALT,
    SDLK_RGUI,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    SDLK_MODE,
    SDLK_AUDIONEXT,
    SDLK_AUDIOPREV,
    SDLK_AUDIOSTOP,
    SDLK_AUDIOPLAY,
    SDLK_AUDIOMUTE,
    SDLK_MEDIASELECT,
    SDLK_WWW,
    SDLK_MAIL,
    SDLK_CALCULATOR,
    SDLK_COMPUTER,
    SDLK_AC_SEARCH,
    SDLK_AC_HOME,
    SDLK_AC_BACK,
    SDLK_AC_FORWARD,
    SDLK_AC_STOP,
    SDLK_AC_REFRESH,
    SDLK_AC_BOOKMARKS,
    SDLK_BRIGHTNESSDOWN,
    SDLK_BRIGHTNESSUP,
    SDLK_DISPLAYSWITCH,
    SDLK_KBDILLUMTOGGLE,
    SDLK_KBDILLUMDOWN,
    SDLK_KBDILLUMUP,
    SDLK_EJECT,
    SDLK_SLEEP,
};

static const char *SDL_scancode_names[SDL_NUM_SCANCODES] = {
    NULL, NULL, NULL, NULL,
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "Return",
    "Escape",
    "Backspace",
    "Tab",
    "Space",
    "-",
    "=",
    "[",
    "]",
    "\\",
    "#",
    ";",
    "'",
    "`",
    ",",
    ".",
    "/",
    "CapsLock",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "PrintScreen",
    "ScrollLock",
    "Pause",
    "Insert",
    "Home",
    "PageUp",
    "Delete",
    "End",
    "PageDown",
    "Right",
    "Left",
    "Down",
    "Up",
    "Numlock",
    "Keypad /",
    "Keypad *",
    "Keypad -",
    "Keypad +",
    "Keypad Enter",
    "Keypad 1",
    "Keypad 2",
    "Keypad 3",
    "Keypad 4",
    "Keypad 5",
    "Keypad 6",
    "Keypad 7",
    "Keypad 8",
    "Keypad 9",
    "Keypad 0",
    "Keypad .",
    NULL,
    "Application",
    "Power",
    "Keypad =",
    "F13",
    "F14",
    "F15",
    "F16",
    "F17",
    "F18",
    "F19",
    "F20",
    "F21",
    "F22",
    "F23",
    "F24",
    "Execute",
    "Help",
    "Menu",
    "Select",
    "Stop",
    "Again",
    "Undo",
    "Cut",
    "Copy",
    "Paste",
    "Find",
    "Mute",
    "VolumeUp",
    "VolumeDown",
    NULL, NULL, NULL,
    "Keypad ,",
    "Keypad = (AS400)",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL,
    "AltErase",
    "SysReq",
    "Cancel",
    "Clear",
    "Prior",
    "Return",
    "Separator",
    "Out",
    "Oper",
    "Clear / Again",
    "CrSel",
    "ExSel",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Keypad 00",
    "Keypad 000",
    "ThousandsSeparator",
    "DecimalSeparator",
    "CurrencyUnit",
    "CurrencySubUnit",
    "Keypad (",
    "Keypad )",
    "Keypad {",
    "Keypad }",
    "Keypad Tab",
    "Keypad Backspace",
    "Keypad A",
    "Keypad B",
    "Keypad C",
    "Keypad D",
    "Keypad E",
    "Keypad F",
    "Keypad XOR",
    "Keypad ^",
    "Keypad %",
    "Keypad <",
    "Keypad >",
    "Keypad &",
    "Keypad &&",
    "Keypad |",
    "Keypad ||",
    "Keypad :",
    "Keypad #",
    "Keypad Space",
    "Keypad @",
    "Keypad !",
    "Keypad MemStore",
    "Keypad MemRecall",
    "Keypad MemClear",
    "Keypad MemAdd",
    "Keypad MemSubtract",
    "Keypad MemMultiply",
    "Keypad MemDivide",
    "Keypad +/-",
    "Keypad Clear",
    "Keypad ClearEntry",
    "Keypad Binary",
    "Keypad Octal",
    "Keypad Decimal",
    "Keypad Hexadecimal",
    NULL, NULL,
    "Left Ctrl",
    "Left Shift",
    "Left Alt",
    "Left GUI",
    "Right Ctrl",
    "Right Shift",
    "Right Alt",
    "Right GUI",
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL,
    "ModeSwitch",
    "AudioNext",
    "AudioPrev",
    "AudioStop",
    "AudioPlay",
    "AudioMute",
    "MediaSelect",
    "WWW",
    "Mail",
    "Calculator",
    "Computer",
    "AC Search",
    "AC Home",
    "AC Back",
    "AC Forward",
    "AC Stop",
    "AC Refresh",
    "AC Bookmarks",
    "BrightnessDown",
    "BrightnessUp",
    "DisplaySwitch",
    "KBDIllumToggle",
    "KBDIllumDown",
    "KBDIllumUp",
    "Eject",
    "Sleep",
};

/* Taken from SDL_iconv() */
static char *
SDL_UCS4ToUTF8(Uint32 ch, char *dst)
{
    Uint8 *p = (Uint8 *) dst;
    if (ch <= 0x7F) {
        *p = (Uint8) ch;
        ++dst;
    } else if (ch <= 0x7FF) {
        p[0] = 0xC0 | (Uint8) ((ch >> 6) & 0x1F);
        p[1] = 0x80 | (Uint8) (ch & 0x3F);
        dst += 2;
    } else if (ch <= 0xFFFF) {
        p[0] = 0xE0 | (Uint8) ((ch >> 12) & 0x0F);
        p[1] = 0x80 | (Uint8) ((ch >> 6) & 0x3F);
        p[2] = 0x80 | (Uint8) (ch & 0x3F);
        dst += 3;
    } else if (ch <= 0x1FFFFF) {
        p[0] = 0xF0 | (Uint8) ((ch >> 18) & 0x07);
        p[1] = 0x80 | (Uint8) ((ch >> 12) & 0x3F);
        p[2] = 0x80 | (Uint8) ((ch >> 6) & 0x3F);
        p[3] = 0x80 | (Uint8) (ch & 0x3F);
        dst += 4;
    } else if (ch <= 0x3FFFFFF) {
        p[0] = 0xF8 | (Uint8) ((ch >> 24) & 0x03);
        p[1] = 0x80 | (Uint8) ((ch >> 18) & 0x3F);
        p[2] = 0x80 | (Uint8) ((ch >> 12) & 0x3F);
        p[3] = 0x80 | (Uint8) ((ch >> 6) & 0x3F);
        p[4] = 0x80 | (Uint8) (ch & 0x3F);
        dst += 5;
    } else {
        p[0] = 0xFC | (Uint8) ((ch >> 30) & 0x01);
        p[1] = 0x80 | (Uint8) ((ch >> 24) & 0x3F);
        p[2] = 0x80 | (Uint8) ((ch >> 18) & 0x3F);
        p[3] = 0x80 | (Uint8) ((ch >> 12) & 0x3F);
        p[4] = 0x80 | (Uint8) ((ch >> 6) & 0x3F);
        p[5] = 0x80 | (Uint8) (ch & 0x3F);
        dst += 6;
    }
    return dst;
}

/* Public functions */
int
SDL_KeyboardInit(void)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;

    /* Set the default keymap */
    SDL_memcpy(keyboard->keymap, SDL_default_keymap, sizeof(SDL_default_keymap));
    return (0);
}

void
SDL_ResetKeyboard(void)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;
    SDL_Scancode scancode;

    for (scancode = 0; scancode < SDL_NUM_SCANCODES; ++scancode) {
        if (keyboard->keystate[scancode] == SDL_PRESSED) {
            SDL_SendKeyboardKey(SDL_RELEASED, scancode);
        }
    }
}

void
SDL_GetDefaultKeymap(SDL_Keycode * keymap)
{
    SDL_memcpy(keymap, SDL_default_keymap, sizeof(SDL_default_keymap));
}

void
SDL_SetKeymap(int start, SDL_Keycode * keys, int length)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;

    if (start < 0 || start + length > SDL_NUM_SCANCODES) {
        return;
    }

    SDL_memcpy(&keyboard->keymap[start], keys, sizeof(*keys) * length);
}

void
SDL_SetScancodeName(SDL_Scancode scancode, const char *name)
{
    SDL_scancode_names[scancode] = name;
}

SDL_Window *
SDL_GetKeyboardFocus(void)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;

    return keyboard->focus;
}

void
SDL_SetKeyboardFocus(SDL_Window * window)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;

    /* See if the current window has lost focus */
    if (keyboard->focus && keyboard->focus != window) {
        SDL_SendWindowEvent(keyboard->focus, SDL_WINDOWEVENT_FOCUS_LOST,
                            0, 0);

        /* Ensures IME compositions are committed */
        if (SDL_EventState(SDL_TEXTINPUT, SDL_QUERY)) {
            SDL_VideoDevice *video = SDL_GetVideoDevice();
            if (video && video->StopTextInput) {
                video->StopTextInput(video);
            }
        }
    }

    keyboard->focus = window;

    if (keyboard->focus) {
        SDL_SendWindowEvent(keyboard->focus, SDL_WINDOWEVENT_FOCUS_GAINED,
                            0, 0);

        if (SDL_EventState(SDL_TEXTINPUT, SDL_QUERY)) {
            SDL_VideoDevice *video = SDL_GetVideoDevice();
            if (video && video->StartTextInput) {
                video->StartTextInput(video);
            }
        }
    }
}

int
SDL_SendKeyboardKey(Uint8 state, SDL_Scancode scancode)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;
    int posted;
    Uint16 modstate;
    Uint32 type;
    Uint8 repeat;

    if (!scancode) {
        return 0;
    }
#if 0
    printf("The '%s' key has been %s\n", SDL_GetScancodeName(scancode),
           state == SDL_PRESSED ? "pressed" : "released");
#endif
    if (state == SDL_PRESSED) {
        modstate = keyboard->modstate;
        switch (scancode) {
        case SDL_SCANCODE_NUMLOCKCLEAR:
            keyboard->modstate ^= KMOD_NUM;
            break;
        case SDL_SCANCODE_CAPSLOCK:
            keyboard->modstate ^= KMOD_CAPS;
            break;
        case SDL_SCANCODE_LCTRL:
            keyboard->modstate |= KMOD_LCTRL;
            break;
        case SDL_SCANCODE_RCTRL:
            keyboard->modstate |= KMOD_RCTRL;
            break;
        case SDL_SCANCODE_LSHIFT:
            keyboard->modstate |= KMOD_LSHIFT;
            break;
        case SDL_SCANCODE_RSHIFT:
            keyboard->modstate |= KMOD_RSHIFT;
            break;
        case SDL_SCANCODE_LALT:
            keyboard->modstate |= KMOD_LALT;
            break;
        case SDL_SCANCODE_RALT:
            keyboard->modstate |= KMOD_RALT;
            break;
        case SDL_SCANCODE_LGUI:
            keyboard->modstate |= KMOD_LGUI;
            break;
        case SDL_SCANCODE_RGUI:
            keyboard->modstate |= KMOD_RGUI;
            break;
        case SDL_SCANCODE_MODE:
            keyboard->modstate |= KMOD_MODE;
            break;
        default:
            break;
        }
    } else {
        switch (scancode) {
        case SDL_SCANCODE_NUMLOCKCLEAR:
        case SDL_SCANCODE_CAPSLOCK:
            break;
        case SDL_SCANCODE_LCTRL:
            keyboard->modstate &= ~KMOD_LCTRL;
            break;
        case SDL_SCANCODE_RCTRL:
            keyboard->modstate &= ~KMOD_RCTRL;
            break;
        case SDL_SCANCODE_LSHIFT:
            keyboard->modstate &= ~KMOD_LSHIFT;
            break;
        case SDL_SCANCODE_RSHIFT:
            keyboard->modstate &= ~KMOD_RSHIFT;
            break;
        case SDL_SCANCODE_LALT:
            keyboard->modstate &= ~KMOD_LALT;
            break;
        case SDL_SCANCODE_RALT:
            keyboard->modstate &= ~KMOD_RALT;
            break;
        case SDL_SCANCODE_LGUI:
            keyboard->modstate &= ~KMOD_LGUI;
            break;
        case SDL_SCANCODE_RGUI:
            keyboard->modstate &= ~KMOD_RGUI;
            break;
        case SDL_SCANCODE_MODE:
            keyboard->modstate &= ~KMOD_MODE;
            break;
        default:
            break;
        }
        modstate = keyboard->modstate;
    }

    /* Figure out what type of event this is */
    switch (state) {
    case SDL_PRESSED:
        type = SDL_KEYDOWN;
        break;
    case SDL_RELEASED:
        type = SDL_KEYUP;
        break;
    default:
        /* Invalid state -- bail */
        return 0;
    }

    /* Drop events that don't change state */
    repeat = (state && keyboard->keystate[scancode]);
    if (keyboard->keystate[scancode] == state && !repeat) {
#if 0
        printf("Keyboard event didn't change state - dropped!\n");
#endif
        return 0;
    }

    /* Update internal keyboard state */
    keyboard->keystate[scancode] = state;

    /* Post the event, if desired */
    posted = 0;
    if (SDL_GetEventState(type) == SDL_ENABLE) {
        SDL_Event event;
        event.key.type = type;
        event.key.state = state;
        event.key.repeat = repeat;
        event.key.keysym.scancode = scancode;
        event.key.keysym.sym = keyboard->keymap[scancode];
        event.key.keysym.mod = modstate;
        event.key.keysym.unicode = 0;
        event.key.windowID = keyboard->focus ? keyboard->focus->id : 0;
        posted = (SDL_PushEvent(&event) > 0);
    }
    return (posted);
}

int
SDL_SendKeyboardText(const char *text)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;
    int posted;

    /* Don't post text events for unprintable characters */
    if ((unsigned char)*text < ' ' || *text == 127) {
        return 0;
    }

    /* Post the event, if desired */
    posted = 0;
    if (SDL_GetEventState(SDL_TEXTINPUT) == SDL_ENABLE) {
        SDL_Event event;
        event.text.type = SDL_TEXTINPUT;
        event.text.windowID = keyboard->focus ? keyboard->focus->id : 0;
        SDL_utf8strlcpy(event.text.text, text, SDL_arraysize(event.text.text));
        event.text.windowID = keyboard->focus ? keyboard->focus->id : 0;
        posted = (SDL_PushEvent(&event) > 0);
    }
    return (posted);
}

int
SDL_SendEditingText(const char *text, int start, int length)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;
    int posted;

    /* Post the event, if desired */
    posted = 0;
    if (SDL_GetEventState(SDL_TEXTEDITING) == SDL_ENABLE) {
        SDL_Event event;
        event.edit.type = SDL_TEXTEDITING;
        event.edit.windowID = keyboard->focus ? keyboard->focus->id : 0;
        event.edit.start = start;
        event.edit.length = length;
        SDL_utf8strlcpy(event.edit.text, text, SDL_arraysize(event.edit.text));
        posted = (SDL_PushEvent(&event) > 0);
    }
    return (posted);
}

void
SDL_KeyboardQuit(void)
{
}

Uint8 *
SDL_GetKeyboardState(int *numkeys)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;

    if (numkeys != (int *) 0) {
        *numkeys = SDL_NUM_SCANCODES;
    }
    return keyboard->keystate;
}

SDL_Keymod
SDL_GetModState(void)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;

    return keyboard->modstate;
}

void
SDL_SetModState(SDL_Keymod modstate)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;

    keyboard->modstate = modstate;
}

SDL_Keycode
SDL_GetKeyFromScancode(SDL_Scancode scancode)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;

    return keyboard->keymap[scancode];
}

SDL_Scancode
SDL_GetScancodeFromKey(SDL_Keycode key)
{
    SDL_Keyboard *keyboard = &SDL_keyboard;
    SDL_Scancode scancode;

    for (scancode = SDL_SCANCODE_UNKNOWN; scancode < SDL_NUM_SCANCODES;
         ++scancode) {
        if (keyboard->keymap[scancode] == key) {
            return scancode;
        }
    }
    return SDL_SCANCODE_UNKNOWN;
}

const char *
SDL_GetScancodeName(SDL_Scancode scancode)
{
    const char *name = SDL_scancode_names[scancode];

    if (name)
        return name;
    else
        return "";
}

const char *
SDL_GetKeyName(SDL_Keycode key)
{
    static char name[8];
    char *end;

    if (key & SDLK_SCANCODE_MASK) {
        return
            SDL_GetScancodeName((SDL_Scancode) (key & ~SDLK_SCANCODE_MASK));
    }

    switch (key) {
    case SDLK_RETURN:
        return SDL_GetScancodeName(SDL_SCANCODE_RETURN);
    case SDLK_ESCAPE:
        return SDL_GetScancodeName(SDL_SCANCODE_ESCAPE);
    case SDLK_BACKSPACE:
        return SDL_GetScancodeName(SDL_SCANCODE_BACKSPACE);
    case SDLK_TAB:
        return SDL_GetScancodeName(SDL_SCANCODE_TAB);
    case SDLK_SPACE:
        return SDL_GetScancodeName(SDL_SCANCODE_SPACE);
    case SDLK_DELETE:
        return SDL_GetScancodeName(SDL_SCANCODE_DELETE);
    default:
        /* Unaccented letter keys on latin keyboards are normally
           labeled in upper case (and probably on others like Greek or
           Cyrillic too, so if you happen to know for sure, please
           adapt this). */
        if (key >= 'a' && key <= 'z') {
            key -= 32;
        }

        end = SDL_UCS4ToUTF8((Uint32) key, name);
        *end = '\0';
        return name;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
