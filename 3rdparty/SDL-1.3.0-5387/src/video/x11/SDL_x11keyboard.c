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

#include "SDL_x11video.h"

#include "../../events/SDL_keyboard_c.h"
#include "../../events/scancodes_darwin.h"
#include "../../events/scancodes_xfree86.h"

#include <X11/keysym.h>

#include "imKStoUCS.h"

/* *INDENT-OFF* */
static const struct {
    KeySym keysym;
    SDL_Keycode sdlkey;
} KeySymToSDLKey[] = {
    { XK_Return, SDLK_RETURN },
    { XK_Escape, SDLK_ESCAPE },
    { XK_BackSpace, SDLK_BACKSPACE },
    { XK_Tab, SDLK_TAB },
    { XK_Caps_Lock, SDLK_CAPSLOCK },
    { XK_F1, SDLK_F1 },
    { XK_F2, SDLK_F2 },
    { XK_F3, SDLK_F3 },
    { XK_F4, SDLK_F4 },
    { XK_F5, SDLK_F5 },
    { XK_F6, SDLK_F6 },
    { XK_F7, SDLK_F7 },
    { XK_F8, SDLK_F8 },
    { XK_F9, SDLK_F9 },
    { XK_F10, SDLK_F10 },
    { XK_F11, SDLK_F11 },
    { XK_F12, SDLK_F12 },
    { XK_Print, SDLK_PRINTSCREEN },
    { XK_Scroll_Lock, SDLK_SCROLLLOCK },
    { XK_Pause, SDLK_PAUSE },
    { XK_Insert, SDLK_INSERT },
    { XK_Home, SDLK_HOME },
    { XK_Prior, SDLK_PAGEUP },
    { XK_Delete, SDLK_DELETE },
    { XK_End, SDLK_END },
    { XK_Next, SDLK_PAGEDOWN },
    { XK_Right, SDLK_RIGHT },
    { XK_Left, SDLK_LEFT },
    { XK_Down, SDLK_DOWN },
    { XK_Up, SDLK_UP },
    { XK_Num_Lock, SDLK_NUMLOCKCLEAR },
    { XK_KP_Divide, SDLK_KP_DIVIDE },
    { XK_KP_Multiply, SDLK_KP_MULTIPLY },
    { XK_KP_Subtract, SDLK_KP_MINUS },
    { XK_KP_Add, SDLK_KP_PLUS },
    { XK_KP_Enter, SDLK_KP_ENTER },
    { XK_KP_Delete, SDLK_KP_PERIOD },
    { XK_KP_End, SDLK_KP_1 },
    { XK_KP_Down, SDLK_KP_2 },
    { XK_KP_Next, SDLK_KP_3 },
    { XK_KP_Left, SDLK_KP_4 },
    { XK_KP_Begin, SDLK_KP_5 },
    { XK_KP_Right, SDLK_KP_6 },
    { XK_KP_Home, SDLK_KP_7 },
    { XK_KP_Up, SDLK_KP_8 },
    { XK_KP_Prior, SDLK_KP_9 },
    { XK_KP_Insert, SDLK_KP_0 },
    { XK_KP_Decimal, SDLK_KP_PERIOD },
    { XK_KP_1, SDLK_KP_1 },
    { XK_KP_2, SDLK_KP_2 },
    { XK_KP_3, SDLK_KP_3 },
    { XK_KP_4, SDLK_KP_4 },
    { XK_KP_5, SDLK_KP_5 },
    { XK_KP_6, SDLK_KP_6 },
    { XK_KP_7, SDLK_KP_7 },
    { XK_KP_8, SDLK_KP_8 },
    { XK_KP_9, SDLK_KP_9 },
    { XK_KP_0, SDLK_KP_0 },
    { XK_KP_Decimal, SDLK_KP_PERIOD },
    { XK_Hyper_R, SDLK_APPLICATION },
    { XK_KP_Equal, SDLK_KP_EQUALS },
    { XK_F13, SDLK_F13 },
    { XK_F14, SDLK_F14 },
    { XK_F15, SDLK_F15 },
    { XK_F16, SDLK_F16 },
    { XK_F17, SDLK_F17 },
    { XK_F18, SDLK_F18 },
    { XK_F19, SDLK_F19 },
    { XK_F20, SDLK_F20 },
    { XK_F21, SDLK_F21 },
    { XK_F22, SDLK_F22 },
    { XK_F23, SDLK_F23 },
    { XK_F24, SDLK_F24 },
    { XK_Execute, SDLK_EXECUTE },
    { XK_Help, SDLK_HELP },
    { XK_Menu, SDLK_MENU },
    { XK_Select, SDLK_SELECT },
    { XK_Cancel, SDLK_STOP },
    { XK_Redo, SDLK_AGAIN },
    { XK_Undo, SDLK_UNDO },
    { XK_Find, SDLK_FIND },
    { XK_KP_Separator, SDLK_KP_COMMA },
    { XK_Sys_Req, SDLK_SYSREQ },
    { XK_Control_L, SDLK_LCTRL },
    { XK_Shift_L, SDLK_LSHIFT },
    { XK_Alt_L, SDLK_LALT },
    { XK_Meta_L, SDLK_LGUI },
    { XK_Super_L, SDLK_LGUI },
    { XK_Control_R, SDLK_RCTRL },
    { XK_Shift_R, SDLK_RSHIFT },
    { XK_Alt_R, SDLK_RALT },
    { XK_Meta_R, SDLK_RGUI },
    { XK_Super_R, SDLK_RGUI },
    { XK_Mode_switch, SDLK_MODE },
};

static const struct
{
    const SDL_Scancode const *table;
    int table_size;
} scancode_set[] = {
    { darwin_scancode_table, SDL_arraysize(darwin_scancode_table) },
    { xfree86_scancode_table, SDL_arraysize(xfree86_scancode_table) },
    { xfree86_scancode_table2, SDL_arraysize(xfree86_scancode_table2) },
};
/* *INDENT-OFF* */

static SDL_Keycode
X11_KeyCodeToSDLKey(Display *display, KeyCode keycode)
{
    KeySym keysym;
    unsigned int ucs4;
    int i;

    keysym = XKeycodeToKeysym(display, keycode, 0);
    if (keysym == NoSymbol) {
        return SDLK_UNKNOWN;
    }

    ucs4 = X11_KeySymToUcs4(keysym);
    if (ucs4) {
        return (SDL_Keycode) ucs4;
    }

    for (i = 0; i < SDL_arraysize(KeySymToSDLKey); ++i) {
        if (keysym == KeySymToSDLKey[i].keysym) {
            return KeySymToSDLKey[i].sdlkey;
        }
    }
    return SDLK_UNKNOWN;
}

int
X11_InitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int i, j;
    int min_keycode, max_keycode;
    struct {
        SDL_Scancode scancode;
        KeySym keysym;
        int value;
    } fingerprint[] = {
        { SDL_SCANCODE_HOME, XK_Home, 0 },
        { SDL_SCANCODE_PAGEUP, XK_Prior, 0 },
        { SDL_SCANCODE_PAGEDOWN, XK_Next, 0 },
    };
    SDL_bool fingerprint_detected;

    XAutoRepeatOn(data->display);

    /* Try to determine which scancodes are being used based on fingerprint */
    fingerprint_detected = SDL_FALSE;
    XDisplayKeycodes(data->display, &min_keycode, &max_keycode);
    for (i = 0; i < SDL_arraysize(fingerprint); ++i) {
        fingerprint[i].value =
            XKeysymToKeycode(data->display, fingerprint[i].keysym) -
            min_keycode;
    }
    for (i = 0; i < SDL_arraysize(scancode_set); ++i) {
        /* Make sure the scancode set isn't too big */
        if ((max_keycode - min_keycode + 1) <= scancode_set[i].table_size) {
            continue;
        }
        for (j = 0; j < SDL_arraysize(fingerprint); ++j) {
            if (fingerprint[j].value < 0
                || fingerprint[j].value >= scancode_set[i].table_size) {
                break;
            }
            if (scancode_set[i].table[fingerprint[j].value] !=
                fingerprint[j].scancode) {
                break;
            }
        }
        if (j == SDL_arraysize(fingerprint)) {
#ifdef DEBUG_KEYBOARD
            printf("Using scancode set %d, min_keycode = %d, max_keycode = %d, table_size = %d\n", i, min_keycode, max_keycode, scancode_set[i].table_size);
#endif
            SDL_memcpy(&data->key_layout[min_keycode], scancode_set[i].table,
                       sizeof(SDL_Scancode) * scancode_set[i].table_size);
            fingerprint_detected = SDL_TRUE;
            break;
        }
    }

    if (!fingerprint_detected) {
        SDL_Keycode keymap[SDL_NUM_SCANCODES];

        printf
            ("Keyboard layout unknown, please send the following to the SDL mailing list (sdl@libsdl.org):\n");

        /* Determine key_layout - only works on US QWERTY layout */
        SDL_GetDefaultKeymap(keymap);
        for (i = min_keycode; i <= max_keycode; ++i) {
            KeySym sym;
            sym = XKeycodeToKeysym(data->display, i, 0);
            if (sym != NoSymbol) {
                SDL_Keycode key;
                printf("code = %d, sym = 0x%X (%s) ", i - min_keycode,
                       (unsigned int) sym, XKeysymToString(sym));
                key = X11_KeyCodeToSDLKey(data->display, i);
                for (j = 0; j < SDL_arraysize(keymap); ++j) {
                    if (keymap[j] == key) {
                        data->key_layout[i] = (SDL_Scancode) j;
                        break;
                    }
                }
                if (j == SDL_arraysize(keymap)) {
                    printf("scancode not found\n");
                } else {
                    printf("scancode = %d (%s)\n", j, SDL_GetScancodeName(j));
                }
            }
        }
    }

    X11_UpdateKeymap(_this);

    SDL_SetScancodeName(SDL_SCANCODE_APPLICATION, "Menu");

    return 0;
}

void
X11_UpdateKeymap(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int i;
    SDL_Scancode scancode;
    SDL_Keycode keymap[SDL_NUM_SCANCODES];

    SDL_zero(keymap);

    for (i = 0; i < SDL_arraysize(data->key_layout); i++) {

        /* Make sure this is a valid scancode */
        scancode = data->key_layout[i];
        if (scancode == SDL_SCANCODE_UNKNOWN) {
            continue;
        }

        keymap[scancode] = X11_KeyCodeToSDLKey(data->display, (KeyCode)i);
    }
    SDL_SetKeymap(0, keymap, SDL_NUM_SCANCODES);
}

void
X11_QuitKeyboard(_THIS)
{
}

/* vi: set ts=4 sw=4 expandtab: */
