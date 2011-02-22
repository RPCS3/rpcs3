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

#ifndef _SDL_BWin_h
#define _SDL_BWin_h

#include "SDL_config.h"

#include <stdio.h>
#include <AppKit.h>
#include <InterfaceKit.h>
#include <be/game/DirectWindow.h>
#if SDL_VIDEO_OPENGL
#include <be/opengl/GLView.h>
#endif
#include <support/UTF8.h>

#include "../../main/beos/SDL_BeApp.h"
#include "SDL_events.h"
#include "SDL_BView.h"

extern "C"
{
#include "../../events/SDL_events_c.h"
};

class SDL_BWin:public BDirectWindow
{
  public:
    SDL_BWin(BRect bounds):BDirectWindow(bounds, "Untitled",
                                         B_TITLED_WINDOW, 0)
    {
        InitKeyboard();
        last_buttons = 0;

        the_view = NULL;
#if SDL_VIDEO_OPENGL
        SDL_GLView = NULL;
#endif
        SDL_View = NULL;
        Unlock();
        shown = false;
        inhibit_resize = false;
    }

    virtual ~ SDL_BWin()
    {
        Lock();
        if (the_view) {
#if SDL_VIDEO_OPENGL
            if (the_view == SDL_GLView) {
                SDL_GLView->UnlockGL();
            }
#endif
            RemoveChild(the_view);
            the_view = NULL;
        }
        Unlock();
#if SDL_VIDEO_OPENGL
        if (SDL_GLView) {
            delete SDL_GLView;
        }
#endif
        if (SDL_View) {
            delete SDL_View;
        }
    }

    virtual void InitKeyboard(void)
    {
        for (uint i = 0; i < SDL_TABLESIZE(keymap); ++i)
            keymap[i] = SDLK_UNKNOWN;

        keymap[0x01] = SDLK_ESCAPE;
        keymap[B_F1_KEY] = SDLK_F1;
        keymap[B_F2_KEY] = SDLK_F2;
        keymap[B_F3_KEY] = SDLK_F3;
        keymap[B_F4_KEY] = SDLK_F4;
        keymap[B_F5_KEY] = SDLK_F5;
        keymap[B_F6_KEY] = SDLK_F6;
        keymap[B_F7_KEY] = SDLK_F7;
        keymap[B_F8_KEY] = SDLK_F8;
        keymap[B_F9_KEY] = SDLK_F9;
        keymap[B_F10_KEY] = SDLK_F10;
        keymap[B_F11_KEY] = SDLK_F11;
        keymap[B_F12_KEY] = SDLK_F12;
        keymap[B_PRINT_KEY] = SDLK_PRINTSCREEN;
        keymap[B_SCROLL_KEY] = SDLK_SCROLLLOCK;
        keymap[B_PAUSE_KEY] = SDLK_PAUSE;
        keymap[0x11] = SDLK_BACKQUOTE;
        keymap[0x12] = SDLK_1;
        keymap[0x13] = SDLK_2;
        keymap[0x14] = SDLK_3;
        keymap[0x15] = SDLK_4;
        keymap[0x16] = SDLK_5;
        keymap[0x17] = SDLK_6;
        keymap[0x18] = SDLK_7;
        keymap[0x19] = SDLK_8;
        keymap[0x1a] = SDLK_9;
        keymap[0x1b] = SDLK_0;
        keymap[0x1c] = SDLK_MINUS;
        keymap[0x1d] = SDLK_EQUALS;
        keymap[0x1e] = SDLK_BACKSPACE;
        keymap[0x1f] = SDLK_INSERT;
        keymap[0x20] = SDLK_HOME;
        keymap[0x21] = SDLK_PAGEUP;
        keymap[0x22] = SDLK_NUMLOCKCLEAR;
        keymap[0x23] = SDLK_KP_DIVIDE;
        keymap[0x24] = SDLK_KP_MULTIPLY;
        keymap[0x25] = SDLK_KP_MINUS;
        keymap[0x26] = SDLK_TAB;
        keymap[0x27] = SDLK_q;
        keymap[0x28] = SDLK_w;
        keymap[0x29] = SDLK_e;
        keymap[0x2a] = SDLK_r;
        keymap[0x2b] = SDLK_t;
        keymap[0x2c] = SDLK_y;
        keymap[0x2d] = SDLK_u;
        keymap[0x2e] = SDLK_i;
        keymap[0x2f] = SDLK_o;
        keymap[0x30] = SDLK_p;
        keymap[0x31] = SDLK_LEFTBRACKET;
        keymap[0x32] = SDLK_RIGHTBRACKET;
        keymap[0x33] = SDLK_BACKSLASH;
        keymap[0x34] = SDLK_DELETE;
        keymap[0x35] = SDLK_END;
        keymap[0x36] = SDLK_PAGEDOWN;
        keymap[0x37] = SDLK_KP_7;
        keymap[0x38] = SDLK_KP_8;
        keymap[0x39] = SDLK_KP_9;
        keymap[0x3a] = SDLK_KP_PLUS;
        keymap[0x3b] = SDLK_CAPSLOCK;
        keymap[0x3c] = SDLK_a;
        keymap[0x3d] = SDLK_s;
        keymap[0x3e] = SDLK_d;
        keymap[0x3f] = SDLK_f;
        keymap[0x40] = SDLK_g;
        keymap[0x41] = SDLK_h;
        keymap[0x42] = SDLK_j;
        keymap[0x43] = SDLK_k;
        keymap[0x44] = SDLK_l;
        keymap[0x45] = SDLK_SEMICOLON;
        keymap[0x46] = SDLK_QUOTE;
        keymap[0x47] = SDLK_RETURN;
        keymap[0x48] = SDLK_KP_4;
        keymap[0x49] = SDLK_KP_5;
        keymap[0x4a] = SDLK_KP_6;
        keymap[0x4b] = SDLK_LSHIFT;
        keymap[0x4c] = SDLK_z;
        keymap[0x4d] = SDLK_x;
        keymap[0x4e] = SDLK_c;
        keymap[0x4f] = SDLK_v;
        keymap[0x50] = SDLK_b;
        keymap[0x51] = SDLK_n;
        keymap[0x52] = SDLK_m;
        keymap[0x53] = SDLK_COMMA;
        keymap[0x54] = SDLK_PERIOD;
        keymap[0x55] = SDLK_SLASH;
        keymap[0x56] = SDLK_RSHIFT;
        keymap[0x57] = SDLK_UP;
        keymap[0x58] = SDLK_KP_1;
        keymap[0x59] = SDLK_KP_2;
        keymap[0x5a] = SDLK_KP_3;
        keymap[0x5b] = SDLK_KP_ENTER;
        keymap[0x5c] = SDLK_LCTRL;
        keymap[0x5d] = SDLK_LALT;
        keymap[0x5e] = SDLK_SPACE;
        keymap[0x5f] = SDLK_RALT;
        keymap[0x60] = SDLK_RCTRL;
        keymap[0x61] = SDLK_LEFT;
        keymap[0x62] = SDLK_DOWN;
        keymap[0x63] = SDLK_RIGHT;
        keymap[0x64] = SDLK_KP_0;
        keymap[0x65] = SDLK_KP_PERIOD;
        keymap[0x66] = SDLK_LGUI;
        keymap[0x67] = SDLK_RGUI;
        keymap[0x68] = SDLK_MENU;
        keymap[0x69] = SDLK_2; //SDLK_EURO;
        keymap[0x6a] = SDLK_KP_EQUALS;
        keymap[0x6b] = SDLK_POWER;
    }

    /* Override the Show() method so we can tell when we've been shown */
    virtual void Show(void)
    {
        BWindow::Show();
        shown = true;
    }
    virtual bool Shown(void)
    {
        return (shown);
    }
    /* If called, the next resize event will not be forwarded to SDL. */
    virtual void InhibitResize(void)
    {
        inhibit_resize = true;
    }
    /* Handle resizing of the window */
    virtual void FrameResized(float width, float height)
    {
        if (inhibit_resize)
            inhibit_resize = false;
        else
            SDL_PrivateResize((int) width, (int) height);
    }
    virtual int CreateView(Uint32 flags, Uint32 gl_flags)
    {
        int retval;

        retval = 0;
        Lock();
        if (flags & SDL_INTERNALOPENGL) {
#if SDL_VIDEO_OPENGL
            if (SDL_GLView == NULL) {
                SDL_GLView = new BGLView(Bounds(), "SDL GLView",
                                         B_FOLLOW_ALL_SIDES,
                                         (B_WILL_DRAW | B_FRAME_EVENTS),
                                         gl_flags);
            }
            if (the_view != SDL_GLView) {
                if (the_view) {
                    RemoveChild(the_view);
                }
                AddChild(SDL_GLView);
                SDL_GLView->LockGL();
                the_view = SDL_GLView;
            }
#else
            SDL_SetError("OpenGL support not enabled");
            retval = -1;
#endif
        } else {
            if (SDL_View == NULL) {
                SDL_View = new SDL_BView(Bounds());
            }
            if (the_view != SDL_View) {
                if (the_view) {
#if SDL_VIDEO_OPENGL
                    if (the_view == SDL_GLView) {
                        SDL_GLView->UnlockGL();
                    }
#endif
                    RemoveChild(the_view);
                }
                AddChild(SDL_View);
                the_view = SDL_View;
            }
        }
        Unlock();
        return (retval);
    }
    virtual void SetBitmap(BBitmap * bitmap)
    {
        SDL_View->SetBitmap(bitmap);
    }
    virtual void SetXYOffset(int x, int y)
    {
#if SDL_VIDEO_OPENGL
        if (the_view == SDL_GLView) {
            return;
        }
#endif
        SDL_View->SetXYOffset(x, y);
    }
    virtual void GetXYOffset(int &x, int &y)
    {
#if SDL_VIDEO_OPENGL
        if (the_view == SDL_GLView) {
            x = 0;
            y = 0;
            return;
        }
#endif
        SDL_View->GetXYOffset(x, y);
    }
    virtual bool BeginDraw(void)
    {
        return (Lock());
    }
    virtual void DrawAsync(BRect updateRect)
    {
        SDL_View->DrawAsync(updateRect);
    }
    virtual void EndDraw(void)
    {
        SDL_View->Sync();
        Unlock();
    }
#if SDL_VIDEO_OPENGL
    virtual void SwapBuffers(void)
    {
        SDL_GLView->UnlockGL();
        SDL_GLView->LockGL();
        SDL_GLView->SwapBuffers();
    }
#endif
    virtual BView *View(void)
    {
        return (the_view);
    }

    /* Hook functions -- overridden */
    virtual void Minimize(bool minimize)
    {
        /* This is only called when mimimized, not when restored */
        //SDL_PrivateAppActive(minimize, SDL_APPACTIVE);
        BWindow::Minimize(minimize);
    }
    virtual void WindowActivated(bool active)
    {
        SDL_PrivateAppActive(active, SDL_APPINPUTFOCUS);
    }
    virtual bool QuitRequested(void)
    {
        if (SDL_BeAppActive > 0) {
            SDL_PrivateQuit();
            /* We don't ever actually close the window here because
               the application should respond to the quit request,
               or ignore it as desired.
             */
            return (false);
        }
        return (true);          /* Close the app window */
    }
    virtual void Quit()
    {
        if (!IsLocked())
            Lock();
        BDirectWindow::Quit();
    }

    virtual int16 Translate2Unicode(const char *buf)
    {
        int32 state, srclen, dstlen;
        unsigned char destbuf[2];
        Uint16 unicode = 0;

        if ((uchar) buf[0] > 127) {
            state = 0;
            srclen = SDL_strlen(buf);
            dstlen = sizeof(destbuf);
            convert_from_utf8(B_UNICODE_CONVERSION, buf, &srclen,
                              (char *) destbuf, &dstlen, &state);
            unicode = destbuf[0];
            unicode <<= 8;
            unicode |= destbuf[1];
        } else
            unicode = buf[0];

        /* For some reason function keys map to control characters */
# define CTRL(X)	((X)-'@')
        switch (unicode) {
        case CTRL('A'):
        case CTRL('B'):
        case CTRL('C'):
        case CTRL('D'):
        case CTRL('E'):
        case CTRL('K'):
        case CTRL('L'):
        case CTRL('P'):
            if (!(SDL_GetModState() & KMOD_CTRL))
                unicode = 0;
            break;
            /* Keyboard input maps newline to carriage return */
        case '\n':
            unicode = '\r';
            break;
        default:
            break;
        }

        return unicode;
    }

    virtual void DispatchMessage(BMessage * msg, BHandler * target)
    {
        switch (msg->what) {
        case B_MOUSE_MOVED:
            {
                BPoint where;
                int32 transit;
                if (msg->FindPoint("where", &where) == B_OK
                    && msg->FindInt32("be:transit", &transit) == B_OK) {
                    if (transit == B_EXITED_VIEW) {
                        if (SDL_GetAppState() & SDL_APPMOUSEFOCUS) {
                            SDL_PrivateAppActive(0, SDL_APPMOUSEFOCUS);
                            be_app->SetCursor(B_HAND_CURSOR);
                        }
                    } else {
                        int x, y;
                        if (!(SDL_GetAppState() & SDL_APPMOUSEFOCUS)) {
                            SDL_PrivateAppActive(1, SDL_APPMOUSEFOCUS);
                            SDL_SetCursor(NULL);
                        }
                        GetXYOffset(x, y);
                        x = (int) where.x - x;
                        y = (int) where.y - y;
                        SDL_PrivateMouseMotion(0, 0, x, y);
                    }
                }
                break;
            }

        case B_MOUSE_DOWN:
            {
                /*      it looks like mouse down is send only for first clicked
                   button, each next is not send while last one is holded */
                int32 buttons;
                int sdl_buttons = 0;
                if (msg->FindInt32("buttons", &buttons) == B_OK) {
                    /* Add any mouse button events */
                    if (buttons & B_PRIMARY_MOUSE_BUTTON) {
                        sdl_buttons |= SDL_BUTTON_LEFT;
                    }
                    if (buttons & B_SECONDARY_MOUSE_BUTTON) {
                        sdl_buttons |= SDL_BUTTON_RIGHT;
                    }
                    if (buttons & B_TERTIARY_MOUSE_BUTTON) {
                        sdl_buttons |= SDL_BUTTON_MIDDLE;
                    }
                    SDL_PrivateMouseButton(SDL_PRESSED, sdl_buttons, 0, 0);

                    last_buttons = buttons;
                }
                break;
            }

        case B_MOUSE_UP:
            {
                /*      mouse up doesn't give which button was released,
                   only state of buttons (after release, so it's always = 0),
                   which is not what we need ;]
                   So we need to store button in mouse down, and restore
                   in mouse up :(
                   mouse up is (similarly to mouse down) send only for
                   first button down (ie. it's no send if we click another button
                   without releasing previous one first) - but that's probably
                   because of how drivers are written?, not BeOS itself. */
                int32 buttons;
                int sdl_buttons = 0;
                if (msg->FindInt32("buttons", &buttons) == B_OK) {
                    /* Add any mouse button events */
                    if ((buttons ^ B_PRIMARY_MOUSE_BUTTON) & last_buttons) {
                        sdl_buttons |= SDL_BUTTON_LEFT;
                    }
                    if ((buttons ^ B_SECONDARY_MOUSE_BUTTON) & last_buttons) {
                        sdl_buttons |= SDL_BUTTON_RIGHT;
                    }
                    if ((buttons ^ B_TERTIARY_MOUSE_BUTTON) & last_buttons) {
                        sdl_buttons |= SDL_BUTTON_MIDDLE;
                    }
                    SDL_PrivateMouseButton(SDL_RELEASED, sdl_buttons, 0, 0);

                    last_buttons = buttons;
                }
                break;
            }

        case B_MOUSE_WHEEL_CHANGED:
            {
                float x, y;
                x = y = 0;
                if (msg->FindFloat("be:wheel_delta_x", &x) == B_OK
                    && msg->FindFloat("be:wheel_delta_y", &y) == B_OK) {
                    if (x < 0 || y < 0) {
                        SDL_PrivateMouseButton(SDL_PRESSED,
                                               SDL_BUTTON_WHEELDOWN, 0, 0);
                        SDL_PrivateMouseButton(SDL_RELEASED,
                                               SDL_BUTTON_WHEELDOWN, 0, 0);
                    } else if (x > 0 || y > 0) {
                        SDL_PrivateMouseButton(SDL_PRESSED,
                                               SDL_BUTTON_WHEELUP, 0, 0);
                        SDL_PrivateMouseButton(SDL_RELEASED,
                                               SDL_BUTTON_WHEELUP, 0, 0);
                    }
                }
                break;
            }

        case B_KEY_DOWN:
        case B_UNMAPPED_KEY_DOWN:      /* modifier keys are unmapped */
            {
                int32 key;
                int32 modifiers;
                int32 key_repeat;
                /* Workaround for SDL message queue being filled too fast because of BeOS own key-repeat mechanism */
                if (msg->FindInt32("be:key_repeat", &key_repeat) == B_OK
                    && key_repeat > 0)
                    break;

                if (msg->FindInt32("key", &key) == B_OK
                    && msg->FindInt32("modifiers", &modifiers) == B_OK) {
                    SDL_Keysym keysym;
                    keysym.scancode = key;
                    if ((key > 0) && (key < 128)) {
                        keysym.sym = keymap[key];
                    } else {
                        keysym.sym = SDLK_UNKNOWN;
                    }
                    /*      FIX THIS?
                       it seems SDL_PrivateKeyboard() changes mod value
                       anyway, and doesn't care about what we setup here */
                    keysym.mod = KMOD_NONE;
                    keysym.unicode = 0;
                    if (SDL_TranslateUNICODE) {
                        const char *bytes;
                        if (msg->FindString("bytes", &bytes) == B_OK) {
                            /*      FIX THIS?
                               this cares only about first "letter",
                               so if someone maps some key to print
                               "BeOS rulez!" only "B" will be used. */
                            keysym.unicode = Translate2Unicode(bytes);
                        }
                    }
                    SDL_PrivateKeyboard(SDL_PRESSED, &keysym);
                }
                break;
            }

        case B_KEY_UP:
        case B_UNMAPPED_KEY_UP:        /* modifier keys are unmapped */
            {
                int32 key;
                int32 modifiers;
                if (msg->FindInt32("key", &key) == B_OK
                    && msg->FindInt32("modifiers", &modifiers) == B_OK) {
                    SDL_Keysym keysym;
                    keysym.scancode = key;
                    if ((key > 0) && (key < 128)) {
                        keysym.sym = keymap[key];
                    } else {
                        keysym.sym = SDLK_UNKNOWN;
                    }
                    keysym.mod = KMOD_NONE;     /* FIX THIS? */
                    keysym.unicode = 0;
                    if (SDL_TranslateUNICODE) {
                        const char *bytes;
                        if (msg->FindString("bytes", &bytes) == B_OK) {
                            keysym.unicode = Translate2Unicode(bytes);
                        }
                    }
                    SDL_PrivateKeyboard(SDL_RELEASED, &keysym);
                }
                break;
            }

        default:
            /* move it after switch{} so it's always handled
               that way we keep BeOS feautures like:
               - CTRL+Q to close window (and other shortcuts)
               - PrintScreen to make screenshot into /boot/home
               - etc.. */
            //BDirectWindow::DispatchMessage(msg, target);
            break;
        }
        BDirectWindow::DispatchMessage(msg, target);
    }

  private:
#if SDL_VIDEO_OPENGL
    BGLView * SDL_GLView;
#endif
    SDL_BView *SDL_View;
    BView *the_view;

    bool shown;
    bool inhibit_resize;

    int32 last_buttons;
    SDL_Keycode keymap[128];
};

#endif /* _SDL_BWin_h */
/* vi: set ts=4 sw=4 expandtab: */
