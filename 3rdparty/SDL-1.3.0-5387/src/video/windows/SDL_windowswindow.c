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

#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_keyboard_c.h"

#include "SDL_windowsvideo.h"
#include "SDL_windowswindow.h"

/* This is included after SDL_windowsvideo.h, which includes windows.h */
#include "SDL_syswm.h"

/* Windows CE compatibility */
#ifndef SWP_NOCOPYBITS
#define SWP_NOCOPYBITS 0
#endif

/* Fake window to help with DirectInput events. */
HWND SDL_HelperWindow = NULL;
static WCHAR *SDL_HelperWindowClassName = TEXT("SDLHelperWindowInputCatcher");
static WCHAR *SDL_HelperWindowName = TEXT("SDLHelperWindowInputMsgWindow");
static ATOM SDL_HelperWindowClass = 0;

#define STYLE_BASIC         (WS_CLIPSIBLINGS | WS_CLIPCHILDREN)
#define STYLE_FULLSCREEN    (WS_POPUP)
#define STYLE_BORDERLESS    (WS_POPUP)
#define STYLE_NORMAL        (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)
#define STYLE_RESIZABLE     (WS_THICKFRAME | WS_MAXIMIZEBOX)
#define STYLE_MASK          (STYLE_FULLSCREEN | STYLE_BORDERLESS | STYLE_NORMAL | STYLE_RESIZABLE)

static DWORD
GetWindowStyle(SDL_Window * window)
{
    DWORD style = 0;

	if (window->flags & SDL_WINDOW_FULLSCREEN) {
        style |= STYLE_FULLSCREEN;
	} else {
		if (window->flags & SDL_WINDOW_BORDERLESS) {
            style |= STYLE_BORDERLESS;
		} else {
            style |= STYLE_NORMAL;
		}
		if (window->flags & SDL_WINDOW_RESIZABLE) {
            style |= STYLE_RESIZABLE;
		}
	}
    return style;
}

static int
SetupWindowData(_THIS, SDL_Window * window, HWND hwnd, SDL_bool created)
{
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    SDL_WindowData *data;

    /* Allocate the window data */
    data = (SDL_WindowData *) SDL_malloc(sizeof(*data));
    if (!data) {
        SDL_OutOfMemory();
        return -1;
    }
    data->window = window;
    data->hwnd = hwnd;
    data->hdc = GetDC(hwnd);
    data->created = created;
    data->mouse_pressed = SDL_FALSE;
    data->videodata = videodata;

    /* Associate the data with the window */
    if (!SetProp(hwnd, TEXT("SDL_WindowData"), data)) {
        ReleaseDC(hwnd, data->hdc);
        SDL_free(data);
        WIN_SetError("SetProp() failed");
        return -1;
    }

    /* Set up the window proc function */
#ifdef GWLP_WNDPROC
    data->wndproc = (WNDPROC) GetWindowLongPtr(hwnd, GWLP_WNDPROC);
    if (data->wndproc == WIN_WindowProc) {
        data->wndproc = NULL;
    } else {
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) WIN_WindowProc);
    }
#else
    data->wndproc = (WNDPROC) GetWindowLong(hwnd, GWL_WNDPROC);
    if (data->wndproc == WIN_WindowProc) {
        data->wndproc = NULL;
    } else {
        SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR) WIN_WindowProc);
    }
#endif

    /* Fill in the SDL window with the window data */
    {
        POINT point;
        point.x = 0;
        point.y = 0;
        if (ClientToScreen(hwnd, &point)) {
            window->x = point.x;
            window->y = point.y;
        }
    }
    {
        RECT rect;
        if (GetClientRect(hwnd, &rect)) {
            window->w = rect.right;
            window->h = rect.bottom;
        }
    }
    {
        DWORD style = GetWindowLong(hwnd, GWL_STYLE);
        if (style & WS_VISIBLE) {
            window->flags |= SDL_WINDOW_SHOWN;
        } else {
            window->flags &= ~SDL_WINDOW_SHOWN;
        }
        if (style & (WS_BORDER | WS_THICKFRAME)) {
            window->flags &= ~SDL_WINDOW_BORDERLESS;
        } else {
            window->flags |= SDL_WINDOW_BORDERLESS;
        }
        if (style & WS_THICKFRAME) {
            window->flags |= SDL_WINDOW_RESIZABLE;
        } else {
            window->flags &= ~SDL_WINDOW_RESIZABLE;
        }
#ifdef WS_MAXIMIZE
        if (style & WS_MAXIMIZE) {
            window->flags |= SDL_WINDOW_MAXIMIZED;
        } else
#endif
        {
            window->flags &= ~SDL_WINDOW_MAXIMIZED;
        }
#ifdef WS_MINIMIZE
        if (style & WS_MINIMIZE) {
            window->flags |= SDL_WINDOW_MINIMIZED;
        } else
#endif
        {
            window->flags &= ~SDL_WINDOW_MINIMIZED;
        }
    }
    if (GetFocus() == hwnd) {
        window->flags |= SDL_WINDOW_INPUT_FOCUS;
        SDL_SetKeyboardFocus(data->window);

        if (window->flags & SDL_WINDOW_INPUT_GRABBED) {
            RECT rect;
            GetClientRect(hwnd, &rect);
            ClientToScreen(hwnd, (LPPOINT) & rect);
            ClientToScreen(hwnd, (LPPOINT) & rect + 1);
            ClipCursor(&rect);
        }
    }

	/* Enable multi-touch */
    if (videodata->RegisterTouchWindow) {
        videodata->RegisterTouchWindow(hwnd, (TWF_FINETOUCH|TWF_WANTPALM));
    }

    /* All done! */
    window->driverdata = data;
    return 0;
}

int
WIN_CreateWindow(_THIS, SDL_Window * window)
{
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    HWND hwnd;
    RECT rect;
    DWORD style = STYLE_BASIC;
    int x, y;
    int w, h;
    
    style |= GetWindowStyle(window);

    /* Figure out what the window area will be */
    rect.left = window->x;
    rect.top = window->y;
    rect.right = window->x + window->w;
    rect.bottom = window->y + window->h;
    AdjustWindowRectEx(&rect, style, FALSE, 0);
    x = rect.left;
    y = rect.top;
    w = (rect.right - rect.left);
    h = (rect.bottom - rect.top);

    hwnd =
        CreateWindow(SDL_Appname, TEXT(""), style, x, y, w, h, NULL, NULL,
                     SDL_Instance, NULL);
    if (!hwnd) {
        WIN_SetError("Couldn't create window");
        return -1;
    }

    WIN_PumpEvents(_this);

    if (SetupWindowData(_this, window, hwnd, SDL_TRUE) < 0) {
        DestroyWindow(hwnd);
        return -1;
    }
#if SDL_VIDEO_OPENGL_WGL
    if (window->flags & SDL_WINDOW_OPENGL) {
        if (WIN_GL_SetupWindow(_this, window) < 0) {
            WIN_DestroyWindow(_this, window);
            return -1;
        }
    }
#endif
    return 0;
}

int
WIN_CreateWindowFrom(_THIS, SDL_Window * window, const void *data)
{
    HWND hwnd = (HWND) data;
    LPTSTR title;
    int titleLen;

    /* Query the title from the existing window */
    titleLen = GetWindowTextLength(hwnd);
    title = SDL_stack_alloc(TCHAR, titleLen + 1);
    if (title) {
        titleLen = GetWindowText(hwnd, title, titleLen);
    } else {
        titleLen = 0;
    }
    if (titleLen > 0) {
        window->title = WIN_StringToUTF8(title);
    }
    if (title) {
        SDL_stack_free(title);
    }

    if (SetupWindowData(_this, window, hwnd, SDL_FALSE) < 0) {
        return -1;
    }
    return 0;
}

void
WIN_SetWindowTitle(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    LPTSTR title;

    if (window->title) {
        title = WIN_UTF8ToString(window->title);
    } else {
        title = NULL;
    }
    SetWindowText(hwnd, title ? title : TEXT(""));
    if (title) {
        SDL_free(title);
    }
}

void
WIN_SetWindowIcon(_THIS, SDL_Window * window, SDL_Surface * icon)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    HICON hicon = NULL;

    if (icon) {
        BYTE *icon_bmp;
        int icon_len;
        SDL_RWops *dst;
        SDL_PixelFormat format;
        SDL_Surface *surface;

        /* Create temporary bitmap buffer */
        icon_len = 40 + icon->h * icon->w * 4;
        icon_bmp = SDL_stack_alloc(BYTE, icon_len);
        dst = SDL_RWFromMem(icon_bmp, icon_len);
        if (!dst) {
            SDL_stack_free(icon_bmp);
            return;
        }

        /* Write the BITMAPINFO header */
        SDL_WriteLE32(dst, 40);
        SDL_WriteLE32(dst, icon->w);
        SDL_WriteLE32(dst, icon->h * 2);
        SDL_WriteLE16(dst, 1);
        SDL_WriteLE16(dst, 32);
        SDL_WriteLE32(dst, BI_RGB);
        SDL_WriteLE32(dst, icon->h * icon->w * 4);
        SDL_WriteLE32(dst, 0);
        SDL_WriteLE32(dst, 0);
        SDL_WriteLE32(dst, 0);
        SDL_WriteLE32(dst, 0);

        /* Convert the icon to a 32-bit surface with alpha channel */
        SDL_InitFormat(&format, SDL_PIXELFORMAT_ARGB8888);
        surface = SDL_ConvertSurface(icon, &format, 0);
        if (surface) {
            /* Write the pixels upside down into the bitmap buffer */
            int y = surface->h;
            while (y--) {
                Uint8 *src = (Uint8 *) surface->pixels + y * surface->pitch;
                SDL_RWwrite(dst, src, surface->pitch, 1);
            }
            SDL_FreeSurface(surface);

/* TODO: create the icon in WinCE (CreateIconFromResource isn't available) */
#ifndef _WIN32_WCE
            hicon =
                CreateIconFromResource(icon_bmp, icon_len, TRUE, 0x00030000);
#endif
        }
        SDL_RWclose(dst);
        SDL_stack_free(icon_bmp);
    }

    /* Set the icon for the window */
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM) hicon);

    /* Set the icon in the task manager (should we do this?) */
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM) hicon);
}

void
WIN_SetWindowPosition(_THIS, SDL_Window * window)
{
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    RECT rect;
    SDL_Rect bounds;
    DWORD style;
    HWND top;
    BOOL menu;
    int x, y;
    int w, h;

    /* Figure out what the window area will be */
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        top = HWND_TOPMOST;
    } else {
        top = HWND_NOTOPMOST;
    }
    style = GetWindowLong(hwnd, GWL_STYLE);
    rect.left = 0;
    rect.top = 0;
    rect.right = window->w;
    rect.bottom = window->h;
#ifdef _WIN32_WCE
    menu = FALSE;
#else
    menu = (style & WS_CHILDWINDOW) ? FALSE : (GetMenu(hwnd) != NULL);
#endif
    AdjustWindowRectEx(&rect, style, menu, 0);
    w = (rect.right - rect.left);
    h = (rect.bottom - rect.top);

    WIN_GetDisplayBounds(_this, display, &bounds);
    if (SDL_WINDOWPOS_ISCENTERED(window->x)) {
        x = bounds.x + (bounds.w - w) / 2;
    } else {
        x = window->x + rect.left;
    }
    if (SDL_WINDOWPOS_ISCENTERED(window->y)) {
        y = bounds.y + (bounds.h - h) / 2;
    } else {
        y = window->y + rect.top;
    }

    SetWindowPos(hwnd, top, x, y, 0, 0, (SWP_NOCOPYBITS | SWP_NOSIZE));
}

void
WIN_SetWindowSize(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    RECT rect;
    DWORD style;
    HWND top;
    BOOL menu;
    int w, h;

    /* Figure out what the window area will be */
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        top = HWND_TOPMOST;
    } else {
        top = HWND_NOTOPMOST;
    }
    style = GetWindowLong(hwnd, GWL_STYLE);
    rect.left = 0;
    rect.top = 0;
    rect.right = window->w;
    rect.bottom = window->h;
#ifdef _WIN32_WCE
    menu = FALSE;
#else
    menu = (style & WS_CHILDWINDOW) ? FALSE : (GetMenu(hwnd) != NULL);
#endif
    AdjustWindowRectEx(&rect, style, menu, 0);
    w = (rect.right - rect.left);
    h = (rect.bottom - rect.top);

    SetWindowPos(hwnd, top, 0, 0, w, h, (SWP_NOCOPYBITS | SWP_NOMOVE));
}

#ifdef _WIN32_WCE
void WINCE_ShowWindow(_THIS, SDL_Window* window, int visible)
{
    SDL_WindowData* windowdata = (SDL_WindowData*) window->driverdata;
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;

    if(visible) {
        if(window->flags & SDL_WINDOW_FULLSCREEN) {
            if(videodata->SHFullScreen)
                videodata->SHFullScreen(windowdata->hwnd, SHFS_HIDETASKBAR | SHFS_HIDESTARTICON | SHFS_HIDESIPBUTTON);

            ShowWindow(FindWindow(TEXT("HHTaskBar"), NULL), SW_HIDE);
        }

        ShowWindow(windowdata->hwnd, SW_SHOW);
        SetForegroundWindow(windowdata->hwnd);
    } else {
        ShowWindow(windowdata->hwnd, SW_HIDE);

        if(window->flags & SDL_WINDOW_FULLSCREEN) {
            if(videodata->SHFullScreen)
                videodata->SHFullScreen(windowdata->hwnd, SHFS_SHOWTASKBAR | SHFS_SHOWSTARTICON | SHFS_SHOWSIPBUTTON);

            ShowWindow(FindWindow(TEXT("HHTaskBar"), NULL), SW_SHOW);

        }
    }
}
#endif /* _WIN32_WCE */

void
WIN_ShowWindow(_THIS, SDL_Window * window)
{
#ifdef _WIN32_WCE
    WINCE_ShowWindow(_this, window, 1);
#else
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    ShowWindow(hwnd, SW_SHOW);
#endif
}

void
WIN_HideWindow(_THIS, SDL_Window * window)
{
#ifdef _WIN32_WCE
    WINCE_ShowWindow(_this, window, 0);
#else
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    ShowWindow(hwnd, SW_HIDE);
#endif
}

void
WIN_RaiseWindow(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    HWND top;

    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        top = HWND_TOPMOST;
    } else {
        top = HWND_NOTOPMOST;
    }
    SetWindowPos(hwnd, top, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE));
}

void
WIN_MaximizeWindow(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;

#ifdef _WIN32_WCE
    if((window->flags & SDL_WINDOW_FULLSCREEN) && videodata->SHFullScreen)
        videodata->SHFullScreen(hwnd, SHFS_HIDETASKBAR | SHFS_HIDESTARTICON | SHFS_HIDESIPBUTTON);
#endif

    ShowWindow(hwnd, SW_MAXIMIZE);
}

void
WIN_MinimizeWindow(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    SDL_VideoData *videodata = (SDL_VideoData *) _this->driverdata;

    ShowWindow(hwnd, SW_MINIMIZE);

#ifdef _WIN32_WCE
    if((window->flags & SDL_WINDOW_FULLSCREEN) && videodata->SHFullScreen)
        videodata->SHFullScreen(hwnd, SHFS_SHOWTASKBAR | SHFS_SHOWSTARTICON | SHFS_SHOWSIPBUTTON);
#endif
}

void
WIN_RestoreWindow(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;

    ShowWindow(hwnd, SW_RESTORE);
}

void
WIN_SetWindowFullscreen(_THIS, SDL_Window * window, SDL_VideoDisplay * display, SDL_bool fullscreen)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
    HWND hwnd = data->hwnd;
    RECT rect;
    SDL_Rect bounds;
    DWORD style;
    HWND top;
    BOOL menu;
    int x, y;
    int w, h;

    if (fullscreen) {
        top = HWND_TOPMOST;
    } else {
        top = HWND_NOTOPMOST;
    }
    style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~STYLE_MASK;
    style |= GetWindowStyle(window);

    WIN_GetDisplayBounds(_this, display, &bounds);

    if (fullscreen) {
        /* Save the windowed position */
        data->windowed_x = window->x;
        data->windowed_y = window->y;

        x = bounds.x;
        y = bounds.y;
        w = bounds.w;
        h = bounds.h;
    } else {
        rect.left = 0;
        rect.top = 0;
        rect.right = window->w;
        rect.bottom = window->h;
#ifdef _WIN32_WCE
        menu = FALSE;
#else
        menu = (style & WS_CHILDWINDOW) ? FALSE : (GetMenu(hwnd) != NULL);
#endif
        AdjustWindowRectEx(&rect, style, menu, 0);
        w = (rect.right - rect.left);
        h = (rect.bottom - rect.top);
        x = data->windowed_x + rect.left;
        y = data->windowed_y + rect.top;
    }
    SetWindowLong(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, top, x, y, w, h, SWP_NOCOPYBITS);
}

void
WIN_SetWindowGrab(_THIS, SDL_Window * window)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;

    if ((window->flags & (SDL_WINDOW_INPUT_GRABBED | SDL_WINDOW_FULLSCREEN))
        && (window->flags & SDL_WINDOW_INPUT_FOCUS)) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        ClientToScreen(hwnd, (LPPOINT) & rect);
        ClientToScreen(hwnd, (LPPOINT) & rect + 1);
        ClipCursor(&rect);
    } else {
        ClipCursor(NULL);
    }
}

void
WIN_DestroyWindow(_THIS, SDL_Window * window)
{
    SDL_WindowData *data = (SDL_WindowData *) window->driverdata;

    if (data) {
#ifdef _WIN32_WCE
	WINCE_ShowWindow(_this, window, 0);
#endif
        ReleaseDC(data->hwnd, data->hdc);
        if (data->created) {
            DestroyWindow(data->hwnd);
        }
        SDL_free(data);
    }
}

SDL_bool
WIN_GetWindowWMInfo(_THIS, SDL_Window * window, SDL_SysWMinfo * info)
{
    HWND hwnd = ((SDL_WindowData *) window->driverdata)->hwnd;
    if (info->version.major <= SDL_MAJOR_VERSION) {
        info->subsystem = SDL_SYSWM_WINDOWS;
        info->info.win.window = hwnd;
        return SDL_TRUE;
    } else {
        SDL_SetError("Application not compiled with SDL %d.%d\n",
                     SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
        return SDL_FALSE;
    }
}


/*
 * Creates a HelperWindow used for DirectInput events.
 */
int
SDL_HelperWindowCreate(void)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS wce;
    HWND hWndParent = NULL;

    /* Make sure window isn't created twice. */
    if (SDL_HelperWindow != NULL) {
        return 0;
    }

    /* Create the class. */
    SDL_zero(wce);
    wce.lpfnWndProc = DefWindowProc;
    wce.lpszClassName = (LPCWSTR) SDL_HelperWindowClassName;
    wce.hInstance = hInstance;

    /* Register the class. */
    SDL_HelperWindowClass = RegisterClass(&wce);
    if (SDL_HelperWindowClass == 0) {
        WIN_SetError("Unable to create Helper Window Class");
        return -1;
    }

#ifndef _WIN32_WCE
    /* WinCE doesn't have HWND_MESSAGE */
    hWndParent = HWND_MESSAGE;
#endif

    /* Create the window. */
    SDL_HelperWindow = CreateWindowEx(0, SDL_HelperWindowClassName,
                                      SDL_HelperWindowName,
                                      WS_OVERLAPPED, CW_USEDEFAULT,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      CW_USEDEFAULT, hWndParent, NULL,
                                      hInstance, NULL);
    if (SDL_HelperWindow == NULL) {
        UnregisterClass(SDL_HelperWindowClassName, hInstance);
        WIN_SetError("Unable to create Helper Window");
        return -1;
    }

    return 0;
}


/*
 * Destroys the HelperWindow previously created with SDL_HelperWindowCreate.
 */
void
SDL_HelperWindowDestroy(void)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    /* Destroy the window. */
    if (SDL_HelperWindow != NULL) {
        if (DestroyWindow(SDL_HelperWindow) == 0) {
            WIN_SetError("Unable to destroy Helper Window");
            return;
        }
        SDL_HelperWindow = NULL;
    }

    /* Unregister the class. */
    if (SDL_HelperWindowClass != 0) {
        if ((UnregisterClass(SDL_HelperWindowClassName, hInstance)) == 0) {
            WIN_SetError("Unable to destroy Helper Window Class");
            return;
        }
        SDL_HelperWindowClass = 0;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
