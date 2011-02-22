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

#include "SDL_windowsvideo.h"
#include "SDL_windowsshape.h"
#include "SDL_syswm.h"
#include "SDL_vkeys.h"
#include "../../events/SDL_events_c.h"
#include "../../events/SDL_touch_c.h"



/*#define WMMSG_DEBUG*/
#ifdef WMMSG_DEBUG
#include <stdio.h>	
#include "wmmsg.h"
#endif

/* Masks for processing the windows KEYDOWN and KEYUP messages */
#define REPEATED_KEYMASK    (1<<30)
#define EXTENDED_KEYMASK    (1<<24)

#define VK_ENTER    10          /* Keypad Enter ... no VKEY defined? */

/* Make sure XBUTTON stuff is defined that isn't in older Platform SDKs... */
#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x020B
#endif
#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP 0x020C
#endif
#ifndef GET_XBUTTON_WPARAM
#define GET_XBUTTON_WPARAM(w) (HIWORD(w))
#endif
#ifndef WM_INPUT
#define WM_INPUT 0x00ff
#endif
#ifndef WM_TOUCH
#define WM_TOUCH 0x0240
#endif


static WPARAM
RemapVKEY(WPARAM wParam, LPARAM lParam)
{
    int i;
    BYTE scancode = (BYTE) ((lParam >> 16) & 0xFF);

    /* Windows remaps alphabetic keys based on current layout.
       We try to provide USB scancodes, so undo this mapping.
     */
    if (wParam >= 'A' && wParam <= 'Z') {
        if (scancode != alpha_scancodes[wParam - 'A']) {
            for (i = 0; i < SDL_arraysize(alpha_scancodes); ++i) {
                if (scancode == alpha_scancodes[i]) {
                    wParam = 'A' + i;
                    break;
                }
            }
        }
    }

    /* Keypad keys are a little trickier, we always scan for them.
       Keypad arrow keys have the same scancode as normal arrow keys,
       except they don't have the extended bit (0x1000000) set.
     */
    if (!(lParam & 0x1000000)) {
        if (wParam == VK_DELETE) {
            wParam = VK_DECIMAL;
        } else {
            for (i = 0; i < SDL_arraysize(keypad_scancodes); ++i) {
                if (scancode == keypad_scancodes[i]) {
                    wParam = VK_NUMPAD0 + i;
                    break;
                }
            }
        }
    }

    return wParam;
}

LRESULT CALLBACK
WIN_WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    SDL_WindowData *data;
    LRESULT returnCode = -1;

    /* Send a SDL_SYSWMEVENT if the application wants them */
    if (SDL_GetEventState(SDL_SYSWMEVENT) == SDL_ENABLE) {
        SDL_SysWMmsg wmmsg;

        SDL_VERSION(&wmmsg.version);
        wmmsg.subsystem = SDL_SYSWM_WINDOWS;
        wmmsg.msg.win.hwnd = hwnd;
        wmmsg.msg.win.msg = msg;
        wmmsg.msg.win.wParam = wParam;
        wmmsg.msg.win.lParam = lParam;
        SDL_SendSysWMEvent(&wmmsg);
    }

    /* Get the window data for the window */
    data = (SDL_WindowData *) GetProp(hwnd, TEXT("SDL_WindowData"));
    if (!data) {
        return CallWindowProc(DefWindowProc, hwnd, msg, wParam, lParam);
    }

#ifdef WMMSG_DEBUG
    {		
        FILE *log = fopen("wmmsg.txt", "a");		
        fprintf(log, "Received windows message: %p ", hwnd);
        if (msg > MAX_WMMSG) {
            fprintf(log, "%d", msg);
        } else {
            fprintf(log, "%s", wmtab[msg]);
        }
        fprintf(log, " -- 0x%X, 0x%X\n", wParam, lParam);
        fclose(log);
    }
#endif

    if (IME_HandleMessage(hwnd, msg, wParam, &lParam, data->videodata))
        return 0;

    switch (msg) {

    case WM_SHOWWINDOW:
        {
            if (wParam) {
                SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_SHOWN, 0, 0);
            } else {
                SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_HIDDEN, 0, 0);
            }
        }
        break;

    case WM_ACTIVATE:
        {
            BOOL minimized;

            minimized = HIWORD(wParam);
            if (!minimized && (LOWORD(wParam) != WA_INACTIVE)) {
                SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_SHOWN, 0, 0);
                SDL_SendWindowEvent(data->window,
                                    SDL_WINDOWEVENT_RESTORED, 0, 0);
#ifndef _WIN32_WCE              /* WinCE misses IsZoomed() */
                if (IsZoomed(hwnd)) {
                    SDL_SendWindowEvent(data->window,
                                        SDL_WINDOWEVENT_MAXIMIZED, 0, 0);
                }
#endif
                if (SDL_GetKeyboardFocus() != data->window) {
                    SDL_SetKeyboardFocus(data->window);
                }
                /*
                 * FIXME: Update keyboard state
                 */
                WIN_CheckClipboardUpdate(data->videodata);
            } else {
                if (SDL_GetKeyboardFocus() == data->window) {
                    SDL_SetKeyboardFocus(NULL);
                }
                if (minimized) {
                    SDL_SendWindowEvent(data->window,
                                        SDL_WINDOWEVENT_MINIMIZED, 0, 0);
                }
            }
        }
        returnCode = 0;
        break;

	case WM_MOUSEMOVE:
#ifdef _WIN32_WCE
        /* transform coords for VGA, WVGA... */
        {
            SDL_VideoData *videodata = data->videodata;
            if(videodata->CoordTransform) {
                POINT pt;
                pt.x = LOWORD(lParam);
                pt.y = HIWORD(lParam);
                videodata->CoordTransform(data->window, &pt);
                SDL_SendMouseMotion(data->window, 0, pt.x, pt.y);
                break;
            }
        }
#endif
        SDL_SendMouseMotion(data->window, 0, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_LBUTTONDOWN:
        SDL_SendMouseButton(data->window, SDL_PRESSED, SDL_BUTTON_LEFT);
        break;

    case WM_LBUTTONUP:
        SDL_SendMouseButton(data->window, SDL_RELEASED, SDL_BUTTON_LEFT);
        break;

    case WM_RBUTTONDOWN:
        SDL_SendMouseButton(data->window, SDL_PRESSED, SDL_BUTTON_RIGHT);
        break;

    case WM_RBUTTONUP:
        SDL_SendMouseButton(data->window, SDL_RELEASED, SDL_BUTTON_RIGHT);
        break;

    case WM_MBUTTONDOWN:
        SDL_SendMouseButton(data->window, SDL_PRESSED, SDL_BUTTON_MIDDLE);
        break;

    case WM_MBUTTONUP:
        SDL_SendMouseButton(data->window, SDL_RELEASED, SDL_BUTTON_MIDDLE);
        break;

    case WM_XBUTTONDOWN:
        SDL_SendMouseButton(data->window, SDL_PRESSED, SDL_BUTTON_X1 + GET_XBUTTON_WPARAM(wParam) - 1);
        returnCode = TRUE;
        break;

    case WM_XBUTTONUP:
        SDL_SendMouseButton(data->window, SDL_RELEASED, SDL_BUTTON_X1 + GET_XBUTTON_WPARAM(wParam) - 1);
        returnCode = TRUE;
        break;

    case WM_MOUSEWHEEL:
        {
            int motion = (short) HIWORD(wParam);

            SDL_SendMouseWheel(data->window, 0, motion);
            break;
        }

#ifdef WM_MOUSELEAVE
    /* FIXME: Do we need the SDL 1.2 hack to generate WM_MOUSELEAVE now? */
    case WM_MOUSELEAVE:
        if (SDL_GetMouseFocus() == data->window) {
            SDL_SetMouseFocus(NULL);
        }
        returnCode = 0;
        break;
#endif /* WM_MOUSELEAVE */

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        {
            wParam = RemapVKEY(wParam, lParam);
            switch (wParam) {
            case VK_CONTROL:
                if (lParam & EXTENDED_KEYMASK)
                    wParam = VK_RCONTROL;
                else
                    wParam = VK_LCONTROL;
                break;
            case VK_SHIFT:
                /* EXTENDED trick doesn't work here */
                {
                    Uint8 *state = SDL_GetKeyboardState(NULL);
                    if (state[SDL_SCANCODE_LSHIFT] == SDL_RELEASED
                        && (GetKeyState(VK_LSHIFT) & 0x8000)) {
                        wParam = VK_LSHIFT;
                    } else if (state[SDL_SCANCODE_RSHIFT] == SDL_RELEASED
                               && (GetKeyState(VK_RSHIFT) & 0x8000)) {
                        wParam = VK_RSHIFT;
                    } else {
                        /* Probably a key repeat */
                        wParam = 256;
                    }
                }
                break;
            case VK_MENU:
                if (lParam & EXTENDED_KEYMASK)
                    wParam = VK_RMENU;
                else
                    wParam = VK_LMENU;
                break;
            case VK_RETURN:
                if (lParam & EXTENDED_KEYMASK)
                    wParam = VK_ENTER;
                break;
            }
            if (wParam < 256) {
                SDL_SendKeyboardKey(SDL_PRESSED,
                                    data->videodata->key_layout[wParam]);
            }
        }
        returnCode = 0;
        break;

    case WM_SYSKEYUP:
    case WM_KEYUP:
        {
            wParam = RemapVKEY(wParam, lParam);
            switch (wParam) {
            case VK_CONTROL:
                if (lParam & EXTENDED_KEYMASK)
                    wParam = VK_RCONTROL;
                else
                    wParam = VK_LCONTROL;
                break;
            case VK_SHIFT:
                /* EXTENDED trick doesn't work here */
                {
                    Uint8 *state = SDL_GetKeyboardState(NULL);
                    if (state[SDL_SCANCODE_LSHIFT] == SDL_PRESSED
                        && !(GetKeyState(VK_LSHIFT) & 0x8000)) {
                        wParam = VK_LSHIFT;
                    } else if (state[SDL_SCANCODE_RSHIFT] == SDL_PRESSED
                               && !(GetKeyState(VK_RSHIFT) & 0x8000)) {
                        wParam = VK_RSHIFT;
                    } else {
                        /* Probably a key repeat */
                        wParam = 256;
                    }
                }
                break;
            case VK_MENU:
                if (lParam & EXTENDED_KEYMASK)
                    wParam = VK_RMENU;
                else
                    wParam = VK_LMENU;
                break;
            case VK_RETURN:
                if (lParam & EXTENDED_KEYMASK)
                    wParam = VK_ENTER;
                break;
            }

            /* Windows only reports keyup for print screen */
            if (wParam == VK_SNAPSHOT
                && SDL_GetKeyboardState(NULL)[SDL_SCANCODE_PRINTSCREEN] ==
                SDL_RELEASED) {
                SDL_SendKeyboardKey(SDL_PRESSED,
                                    data->videodata->key_layout[wParam]);
            }
            if (wParam < 256) {
                SDL_SendKeyboardKey(SDL_RELEASED,
                                    data->videodata->key_layout[wParam]);
            }
        }
        returnCode = 0;
        break;

    case WM_CHAR:
        {
            char text[4];

            /* Convert to UTF-8 and send it on... */
            if (wParam <= 0x7F) {
                text[0] = (char) wParam;
                text[1] = '\0';
            } else if (wParam <= 0x7FF) {
                text[0] = 0xC0 | (char) ((wParam >> 6) & 0x1F);
                text[1] = 0x80 | (char) (wParam & 0x3F);
                text[2] = '\0';
            } else {
                text[0] = 0xE0 | (char) ((wParam >> 12) & 0x0F);
                text[1] = 0x80 | (char) ((wParam >> 6) & 0x3F);
                text[2] = 0x80 | (char) (wParam & 0x3F);
                text[3] = '\0';
            }
            SDL_SendKeyboardText(text);
        }
        returnCode = 0;
        break;

#ifdef WM_INPUTLANGCHANGE
    case WM_INPUTLANGCHANGE:
        {
            WIN_UpdateKeymap();
        }
        returnCode = 1;
        break;
#endif /* WM_INPUTLANGCHANGE */

#ifdef WM_GETMINMAXINFO
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO *info;
            RECT size;
            int x, y;
            int w, h;
            int style;
            BOOL menu;

            /* If we allow resizing, let the resize happen naturally */
            if(SDL_IsShapedWindow(data->window))
                Win32_ResizeWindowShape(data->window);
            if (SDL_GetWindowFlags(data->window) & SDL_WINDOW_RESIZABLE) {
                returnCode = 0;
                break;
            }

            /* Get the current position of our window */
            GetWindowRect(hwnd, &size);
            x = size.left;
            y = size.top;

            /* Calculate current size of our window */
            SDL_GetWindowSize(data->window, &w, &h);
            size.top = 0;
            size.left = 0;
            size.bottom = h;
            size.right = w;


            style = GetWindowLong(hwnd, GWL_STYLE);
#ifdef _WIN32_WCE
            menu = FALSE;
#else
            /* DJM - according to the docs for GetMenu(), the
               return value is undefined if hwnd is a child window.
               Aparently it's too difficult for MS to check
               inside their function, so I have to do it here.
             */
            menu = (style & WS_CHILDWINDOW) ? FALSE : (GetMenu(hwnd) != NULL);
#endif
            AdjustWindowRectEx(&size, style, menu, 0);
            w = size.right - size.left;
            h = size.bottom - size.top;

            /* Fix our size to the current size */
            info = (MINMAXINFO *) lParam;
            info->ptMaxSize.x = w;
            info->ptMaxSize.y = h;
            info->ptMaxPosition.x = x;
            info->ptMaxPosition.y = y;
            info->ptMinTrackSize.x = w;
            info->ptMinTrackSize.y = h;
            info->ptMaxTrackSize.x = w;
            info->ptMaxTrackSize.y = h;
        }
        returnCode = 0;
        break;
#endif /* WM_GETMINMAXINFO */

    case WM_WINDOWPOSCHANGED:
        {
            RECT rect;
            int x, y;
            int w, h;
            Uint32 window_flags;

            if (!GetClientRect(hwnd, &rect) ||
                (rect.right == rect.left && rect.bottom == rect.top)) {
                break;
            }
            ClientToScreen(hwnd, (LPPOINT) & rect);
            ClientToScreen(hwnd, (LPPOINT) & rect + 1);

            window_flags = SDL_GetWindowFlags(data->window);
            if ((window_flags & SDL_WINDOW_INPUT_GRABBED) &&
                (window_flags & SDL_WINDOW_INPUT_FOCUS)) {
                ClipCursor(&rect);
            }

            x = rect.left;
            y = rect.top;
            SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_MOVED, x, y);

            w = rect.right - rect.left;
            h = rect.bottom - rect.top;
            SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_RESIZED, w,
                                h);
        }
        break;

    case WM_SETCURSOR:
        {
            Uint16 hittest;

            hittest = LOWORD(lParam);
            if (hittest == HTCLIENT) {
                /* FIXME: Implement the cursor API */
                static HCURSOR cursor;
                if (!cursor) {
                    cursor = LoadCursor(NULL, IDC_ARROW);
                }
                SetCursor(cursor);
                returnCode = TRUE;
            }
        }
        break;

        /* We were occluded, refresh our display */
    case WM_PAINT:
        {
            RECT rect;
            if (GetUpdateRect(hwnd, &rect, FALSE)) {
                ValidateRect(hwnd, &rect);
                SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_EXPOSED,
                                    0, 0);
            }
        }
        returnCode = 0;
        break;

        /* We'll do our own drawing, prevent flicker */
    case WM_ERASEBKGND:
        {
        }
        return (1);

#if defined(SC_SCREENSAVE) || defined(SC_MONITORPOWER)
    case WM_SYSCOMMAND:
        {
            /* Don't start the screensaver or blank the monitor in fullscreen apps */
            if ((wParam & 0xFFF0) == SC_SCREENSAVE ||
                (wParam & 0xFFF0) == SC_MONITORPOWER) {
                if (SDL_GetVideoDevice()->suspend_screensaver) {
                    return (0);
                }
            }
        }
        break;
#endif /* System has screensaver support */

    case WM_CLOSE:
        {
            SDL_SendWindowEvent(data->window, SDL_WINDOWEVENT_CLOSE, 0, 0);
        }
        returnCode = 0;
        break;

	case WM_TOUCH:
		{
			UINT i, num_inputs = LOWORD(wParam);
			PTOUCHINPUT inputs = SDL_stack_alloc(TOUCHINPUT, num_inputs);
			if (data->videodata->GetTouchInputInfo((HTOUCHINPUT)lParam, num_inputs, inputs, sizeof(TOUCHINPUT))) {
				RECT rect;
				float x, y;

				if (!GetClientRect(hwnd, &rect) ||
				    (rect.right == rect.left && rect.bottom == rect.top)) {
					break;
				}
				ClientToScreen(hwnd, (LPPOINT) & rect);
				ClientToScreen(hwnd, (LPPOINT) & rect + 1);
				rect.top *= 100;
				rect.left *= 100;
				rect.bottom *= 100;
				rect.right *= 100;

				for (i = 0; i < num_inputs; ++i) {
					PTOUCHINPUT input = &inputs[i];

					SDL_TouchID touchId = (SDL_TouchID)input->hSource;
					if (!SDL_GetTouch(touchId)) {
						SDL_Touch touch;

						touch.id = touchId;
						touch.x_min = 0;
						touch.x_max = 1;
						touch.native_xres = touch.x_max - touch.x_min;
						touch.y_min = 0;
						touch.y_max = 1;
						touch.native_yres = touch.y_max - touch.y_min;
						touch.pressure_min = 0;
						touch.pressure_max = 1;
						touch.native_pressureres = touch.pressure_max - touch.pressure_min;

						if (SDL_AddTouch(&touch, "") < 0) {
							continue;
						}
					}

					// Get the normalized coordinates for the window
					x = (float)(input->x - rect.left)/(rect.right - rect.left);
					y = (float)(input->y - rect.top)/(rect.bottom - rect.top);

					if (input->dwFlags & TOUCHEVENTF_DOWN) {
						SDL_SendFingerDown(touchId, input->dwID, SDL_TRUE, x, y, 1);
					}
					if (input->dwFlags & TOUCHEVENTF_MOVE) {
						SDL_SendTouchMotion(touchId, input->dwID, SDL_FALSE, x, y, 1);
					}
					if (input->dwFlags & TOUCHEVENTF_UP) {
						SDL_SendFingerDown(touchId, input->dwID, SDL_FALSE, x, y, 1);
					}
				}
			}
			SDL_stack_free(inputs);

			data->videodata->CloseTouchInputHandle((HTOUCHINPUT)lParam);
			return 0;
		}
		break;
	}

    /* If there's a window proc, assume it's going to handle messages */
    if (data->wndproc) {
        return CallWindowProc(data->wndproc, hwnd, msg, wParam, lParam);
    } else if (returnCode >= 0) {
        return returnCode;
    } else {
        return CallWindowProc(DefWindowProc, hwnd, msg, wParam, lParam);
    }
}

void
WIN_PumpEvents(_THIS)
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static int app_registered = 0;
LPTSTR SDL_Appname = NULL;
Uint32 SDL_Appstyle = 0;
HINSTANCE SDL_Instance = NULL;

/* Register the class for this application */
int
SDL_RegisterApp(char *name, Uint32 style, void *hInst)
{
    WNDCLASS class;

    /* Only do this once... */
    if (app_registered) {
        ++app_registered;
        return (0);
    }
    if (!name && !SDL_Appname) {
        name = "SDL_app";
#if defined(CS_BYTEALIGNCLIENT) || defined(CS_OWNDC)
        SDL_Appstyle = (CS_BYTEALIGNCLIENT | CS_OWNDC);
#endif
        SDL_Instance = hInst ? hInst : GetModuleHandle(NULL);
    }

    if (name) {
        SDL_Appname = WIN_UTF8ToString(name);
        SDL_Appstyle = style;
        SDL_Instance = hInst ? hInst : GetModuleHandle(NULL);
    }

    /* Register the application class */
    class.hCursor = NULL;
    class.hIcon =
        LoadImage(SDL_Instance, SDL_Appname, IMAGE_ICON, 0, 0,
                  LR_DEFAULTCOLOR);
    class.lpszMenuName = NULL;
    class.lpszClassName = SDL_Appname;
    class.hbrBackground = NULL;
    class.hInstance = SDL_Instance;
    class.style = SDL_Appstyle;
    class.lpfnWndProc = WIN_WindowProc;
    class.cbWndExtra = 0;
    class.cbClsExtra = 0;
    if (!RegisterClass(&class)) {
        SDL_SetError("Couldn't register application class");
        return (-1);
    }

    app_registered = 1;
    return (0);
}

/* Unregisters the windowclass registered in SDL_RegisterApp above. */
void
SDL_UnregisterApp()
{
    WNDCLASS class;

    /* SDL_RegisterApp might not have been called before */
    if (!app_registered) {
        return;
    }
    --app_registered;
    if (app_registered == 0) {
        /* Check for any registered window classes. */
        if (GetClassInfo(SDL_Instance, SDL_Appname, &class)) {
            UnregisterClass(SDL_Appname, SDL_Instance);
        }
        SDL_free(SDL_Appname);
        SDL_Appname = NULL;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
