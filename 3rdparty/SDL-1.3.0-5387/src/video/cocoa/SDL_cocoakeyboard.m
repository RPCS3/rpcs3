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

#include "SDL_cocoavideo.h"

#include "../../events/SDL_keyboard_c.h"
#include "../../events/scancodes_darwin.h"

#include <Carbon/Carbon.h>

//#define DEBUG_IME NSLog
#define DEBUG_IME

#ifndef NX_DEVICERCTLKEYMASK
    #define NX_DEVICELCTLKEYMASK    0x00000001
#endif
#ifndef NX_DEVICELSHIFTKEYMASK
    #define NX_DEVICELSHIFTKEYMASK  0x00000002
#endif
#ifndef NX_DEVICERSHIFTKEYMASK
    #define NX_DEVICERSHIFTKEYMASK  0x00000004
#endif
#ifndef NX_DEVICELCMDKEYMASK
    #define NX_DEVICELCMDKEYMASK    0x00000008
#endif
#ifndef NX_DEVICERCMDKEYMASK
    #define NX_DEVICERCMDKEYMASK    0x00000010
#endif
#ifndef NX_DEVICELALTKEYMASK
    #define NX_DEVICELALTKEYMASK    0x00000020
#endif
#ifndef NX_DEVICERALTKEYMASK
    #define NX_DEVICERALTKEYMASK    0x00000040
#endif
#ifndef NX_DEVICERCTLKEYMASK
    #define NX_DEVICERCTLKEYMASK    0x00002000
#endif

@interface SDLTranslatorResponder : NSView <NSTextInput>
{
    NSString *_markedText;
    NSRange   _markedRange;
    NSRange   _selectedRange;
    SDL_Rect  _inputRect;
}
- (void) doCommandBySelector:(SEL)myselector;
- (void) setInputRect:(SDL_Rect *) rect;
@end

@implementation SDLTranslatorResponder

- (void) setInputRect:(SDL_Rect *) rect
{
    _inputRect = *rect;
}

- (void) insertText:(id) aString
{
    const char *str;

    DEBUG_IME(@"insertText: %@", aString);

    /* Could be NSString or NSAttributedString, so we have
     * to test and convert it before return as SDL event */
    if ([aString isKindOfClass: [NSAttributedString class]])
        str = [[aString string] UTF8String];
    else
        str = [aString UTF8String];

    SDL_SendKeyboardText(str);
}

- (void) doCommandBySelector:(SEL) myselector
{
    // No need to do anything since we are not using Cocoa
    // selectors to handle special keys, instead we use SDL
    // key events to do the same job.
}

- (BOOL) hasMarkedText
{
    return _markedText != nil;
}

- (NSRange) markedRange
{
    return _markedRange;
}

- (NSRange) selectedRange
{
    return _selectedRange;
}

- (void) setMarkedText:(id) aString
         selectedRange:(NSRange) selRange
{
    if ([aString isKindOfClass: [NSAttributedString class]])
        aString = [aString string];

    if ([aString length] == 0)
    {
        [self unmarkText];
        return;
    }

    if (_markedText != aString)
    {
        [_markedText release];
        _markedText = [aString retain];
    }

    _selectedRange = selRange;
    _markedRange = NSMakeRange(0, [aString length]);

    SDL_SendEditingText([aString UTF8String],
                        selRange.location, selRange.length);

    DEBUG_IME(@"setMarkedText: %@, (%d, %d)", _markedText,
          selRange.location, selRange.length);
}

- (void) unmarkText
{
    [_markedText release];
    _markedText = nil;

    SDL_SendEditingText("", 0, 0);
}

- (NSRect) firstRectForCharacterRange: (NSRange) theRange
{
    NSWindow *window = [self window];
    NSRect contentRect = [window contentRectForFrameRect: [window frame]];
    float windowHeight = contentRect.size.height;
    NSRect rect = NSMakeRect(_inputRect.x, windowHeight - _inputRect.y - _inputRect.h,
                             _inputRect.w, _inputRect.h);

    DEBUG_IME(@"firstRectForCharacterRange: (%d, %d): windowHeight = %g, rect = %@",
            theRange.location, theRange.length, windowHeight,
            NSStringFromRect(rect));
    rect.origin = [[self window] convertBaseToScreen: rect.origin];

    return rect;
}

- (NSAttributedString *) attributedSubstringFromRange: (NSRange) theRange
{
    DEBUG_IME(@"attributedSubstringFromRange: (%d, %d)", theRange.location, theRange.length);
    return nil;
}

/* Needs long instead of NSInteger for compilation on Mac OS X 10.4 */
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
- (long) conversationIdentifier
#else
- (NSInteger) conversationIdentifier
#endif
{
    return (long) self;
}

// This method returns the index for character that is 
// nearest to thePoint.  thPoint is in screen coordinate system.
- (NSUInteger) characterIndexForPoint:(NSPoint) thePoint
{
    DEBUG_IME(@"characterIndexForPoint: (%g, %g)", thePoint.x, thePoint.y);
    return 0;
}

// This method is the key to attribute extension. 
// We could add new attributes through this method.
// NSInputServer examines the return value of this
// method & constructs appropriate attributed string.
- (NSArray *) validAttributesForMarkedText
{
    return [NSArray array];
}

@end

/* This is the original behavior, before support was added for 
 * differentiating between left and right versions of the keys.
 */
static void
DoUnsidedModifiers(unsigned short scancode,
                   unsigned int oldMods, unsigned int newMods)
{
    const int mapping[] = {
        SDL_SCANCODE_CAPSLOCK,
        SDL_SCANCODE_LSHIFT,
        SDL_SCANCODE_LCTRL,
        SDL_SCANCODE_LALT,
        SDL_SCANCODE_LGUI
    };
    unsigned int i, bit;

    /* Iterate through the bits, testing each against the current modifiers */
    for (i = 0, bit = NSAlphaShiftKeyMask; bit <= NSCommandKeyMask; bit <<= 1, ++i) {
        unsigned int oldMask, newMask;

        oldMask = oldMods & bit;
        newMask = newMods & bit;

        if (oldMask && oldMask != newMask) {        /* modifier up event */
            /* If this was Caps Lock, we need some additional voodoo to make SDL happy */
            if (bit == NSAlphaShiftKeyMask) {
                SDL_SendKeyboardKey(SDL_PRESSED, mapping[i]);
            }
            SDL_SendKeyboardKey(SDL_RELEASED, mapping[i]);
        } else if (newMask && oldMask != newMask) { /* modifier down event */
            SDL_SendKeyboardKey(SDL_PRESSED, mapping[i]);
            /* If this was Caps Lock, we need some additional voodoo to make SDL happy */
            if (bit == NSAlphaShiftKeyMask) {
                SDL_SendKeyboardKey(SDL_RELEASED, mapping[i]);
            }
        }
    }
}

/* This is a helper function for HandleModifierSide. This 
 * function reverts back to behavior before the distinction between
 * sides was made.
 */
static void
HandleNonDeviceModifier(unsigned int device_independent_mask,
                        unsigned int oldMods,
                        unsigned int newMods,
                        SDL_Scancode scancode)
{
    unsigned int oldMask, newMask;
    
    /* Isolate just the bits we care about in the depedent bits so we can 
     * figure out what changed
     */ 
    oldMask = oldMods & device_independent_mask;
    newMask = newMods & device_independent_mask;
    
    if (oldMask && oldMask != newMask) {
        SDL_SendKeyboardKey(SDL_RELEASED, scancode);
    } else if (newMask && oldMask != newMask) {
        SDL_SendKeyboardKey(SDL_PRESSED, scancode);
    }
}

/* This is a helper function for HandleModifierSide. 
 * This function sets the actual SDL_PrivateKeyboard event.
 */
static void
HandleModifierOneSide(unsigned int oldMods, unsigned int newMods,
                      SDL_Scancode scancode, 
                      unsigned int sided_device_dependent_mask)
{
    unsigned int old_dep_mask, new_dep_mask;

    /* Isolate just the bits we care about in the depedent bits so we can 
     * figure out what changed
     */ 
    old_dep_mask = oldMods & sided_device_dependent_mask;
    new_dep_mask = newMods & sided_device_dependent_mask;

    /* We now know that this side bit flipped. But we don't know if
     * it went pressed to released or released to pressed, so we must 
     * find out which it is.
     */
    if (new_dep_mask && old_dep_mask != new_dep_mask) {
        SDL_SendKeyboardKey(SDL_PRESSED, scancode);
    } else {
        SDL_SendKeyboardKey(SDL_RELEASED, scancode);
    }
}

/* This is a helper function for DoSidedModifiers.
 * This function will figure out if the modifier key is the left or right side, 
 * e.g. left-shift vs right-shift. 
 */
static void
HandleModifierSide(int device_independent_mask, 
                   unsigned int oldMods, unsigned int newMods, 
                   SDL_Scancode left_scancode, 
                   SDL_Scancode right_scancode,
                   unsigned int left_device_dependent_mask, 
                   unsigned int right_device_dependent_mask)
{
    unsigned int device_dependent_mask = (left_device_dependent_mask |
                                         right_device_dependent_mask);
    unsigned int diff_mod;
    
    /* On the basis that the device independent mask is set, but there are 
     * no device dependent flags set, we'll assume that we can't detect this 
     * keyboard and revert to the unsided behavior.
     */
    if ((device_dependent_mask & newMods) == 0) {
        /* Revert to the old behavior */
        HandleNonDeviceModifier(device_independent_mask, oldMods, newMods, left_scancode);
        return;
    }

    /* XOR the previous state against the new state to see if there's a change */
    diff_mod = (device_dependent_mask & oldMods) ^
               (device_dependent_mask & newMods);
    if (diff_mod) {
        /* A change in state was found. Isolate the left and right bits 
         * to handle them separately just in case the values can simulataneously
         * change or if the bits don't both exist.
         */
        if (left_device_dependent_mask & diff_mod) {
            HandleModifierOneSide(oldMods, newMods, left_scancode, left_device_dependent_mask);
        }
        if (right_device_dependent_mask & diff_mod) {
            HandleModifierOneSide(oldMods, newMods, right_scancode, right_device_dependent_mask);
        }
    }
}
   
/* This is a helper function for DoSidedModifiers.
 * This function will release a key press in the case that 
 * it is clear that the modifier has been released (i.e. one side 
 * can't still be down).
 */
static void
ReleaseModifierSide(unsigned int device_independent_mask, 
                    unsigned int oldMods, unsigned int newMods,
                    SDL_Scancode left_scancode, 
                    SDL_Scancode right_scancode,
                    unsigned int left_device_dependent_mask, 
                    unsigned int right_device_dependent_mask)
{
    unsigned int device_dependent_mask = (left_device_dependent_mask |
                                          right_device_dependent_mask);

    /* On the basis that the device independent mask is set, but there are 
     * no device dependent flags set, we'll assume that we can't detect this 
     * keyboard and revert to the unsided behavior.
     */
    if ((device_dependent_mask & oldMods) == 0) {
        /* In this case, we can't detect the keyboard, so use the left side 
         * to represent both, and release it. 
         */
        SDL_SendKeyboardKey(SDL_RELEASED, left_scancode);
        return;
    }

    /* 
     * This could have been done in an if-else case because at this point,
     * we know that all keys have been released when calling this function. 
     * But I'm being paranoid so I want to handle each separately,
     * so I hope this doesn't cause other problems.
     */
    if ( left_device_dependent_mask & oldMods ) {
        SDL_SendKeyboardKey(SDL_RELEASED, left_scancode);
    }
    if ( right_device_dependent_mask & oldMods ) {
        SDL_SendKeyboardKey(SDL_RELEASED, right_scancode);
    }
}

/* This is a helper function for DoSidedModifiers.
 * This function handles the CapsLock case.
 */
static void
HandleCapsLock(unsigned short scancode,
               unsigned int oldMods, unsigned int newMods)
{
    unsigned int oldMask, newMask;
    
    oldMask = oldMods & NSAlphaShiftKeyMask;
    newMask = newMods & NSAlphaShiftKeyMask;

    if (oldMask != newMask) {
        SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_CAPSLOCK);
        SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_CAPSLOCK);
    }

    oldMask = oldMods & NSNumericPadKeyMask;
    newMask = newMods & NSNumericPadKeyMask;

    if (oldMask != newMask) {
        SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_NUMLOCKCLEAR);
        SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_NUMLOCKCLEAR);
    }
}

/* This function will handle the modifier keys and also determine the 
 * correct side of the key.
 */
static void
DoSidedModifiers(unsigned short scancode,
                 unsigned int oldMods, unsigned int newMods)
{
	/* Set up arrays for the key syms for the left and right side. */
    const SDL_Scancode left_mapping[]  = {
        SDL_SCANCODE_LSHIFT,
        SDL_SCANCODE_LCTRL,
        SDL_SCANCODE_LALT,
        SDL_SCANCODE_LGUI
    };
    const SDL_Scancode right_mapping[] = {
        SDL_SCANCODE_RSHIFT,
        SDL_SCANCODE_RCTRL,
        SDL_SCANCODE_RALT,
        SDL_SCANCODE_RGUI
    };
	/* Set up arrays for the device dependent masks with indices that 
     * correspond to the _mapping arrays 
     */
    const unsigned int left_device_mapping[]  = { NX_DEVICELSHIFTKEYMASK, NX_DEVICELCTLKEYMASK, NX_DEVICELALTKEYMASK, NX_DEVICELCMDKEYMASK };
    const unsigned int right_device_mapping[] = { NX_DEVICERSHIFTKEYMASK, NX_DEVICERCTLKEYMASK, NX_DEVICERALTKEYMASK, NX_DEVICERCMDKEYMASK };

    unsigned int i, bit;

    /* Handle CAPSLOCK separately because it doesn't have a left/right side */
    HandleCapsLock(scancode, oldMods, newMods);

    /* Iterate through the bits, testing each against the old modifiers */
    for (i = 0, bit = NSShiftKeyMask; bit <= NSCommandKeyMask; bit <<= 1, ++i) {
        unsigned int oldMask, newMask;
		
        oldMask = oldMods & bit;
        newMask = newMods & bit;
		
        /* If the bit is set, we must always examine it because the left
         * and right side keys may alternate or both may be pressed.
         */
        if (newMask) {
            HandleModifierSide(bit, oldMods, newMods,
                               left_mapping[i], right_mapping[i],
                               left_device_mapping[i], right_device_mapping[i]);
        }
        /* If the state changed from pressed to unpressed, we must examine
            * the device dependent bits to release the correct keys.
            */
        else if (oldMask && oldMask != newMask) {
            ReleaseModifierSide(bit, oldMods, newMods,
                              left_mapping[i], right_mapping[i],
                              left_device_mapping[i], right_device_mapping[i]);
        }
    }
}

static void
HandleModifiers(_THIS, unsigned short scancode, unsigned int modifierFlags)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    if (modifierFlags == data->modifierFlags) {
    	return;
    }

    /* 
     * Starting with Panther (10.3.0), the ability to distinguish between 
     * left side and right side modifiers is available.
     */
    if (data->osversion >= 0x1030) {
        DoSidedModifiers(scancode, data->modifierFlags, modifierFlags);
    } else {
        DoUnsidedModifiers(scancode, data->modifierFlags, modifierFlags);
    }
    data->modifierFlags = modifierFlags;
}

static void
UpdateKeymap(SDL_VideoData *data)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
    TISInputSourceRef key_layout;
#else
    KeyboardLayoutRef key_layout;
#endif
    const void *chr_data;
    int i;
    SDL_Scancode scancode;
    SDL_Keycode keymap[SDL_NUM_SCANCODES];

    /* See if the keymap needs to be updated */
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
    key_layout = TISCopyCurrentKeyboardLayoutInputSource();
#else
    KLGetCurrentKeyboardLayout(&key_layout);
#endif
    if (key_layout == data->key_layout) {
        return;
    }
    data->key_layout = key_layout;

    SDL_GetDefaultKeymap(keymap);

    /* Try Unicode data first (preferred as of Mac OS X 10.5) */
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
    CFDataRef uchrDataRef = TISGetInputSourceProperty(key_layout, kTISPropertyUnicodeKeyLayoutData);
    if (uchrDataRef)
        chr_data = CFDataGetBytePtr(uchrDataRef);
    else
        goto cleanup;
#else
    KLGetKeyboardLayoutProperty(key_layout, kKLuchrData, &chr_data);
#endif
    if (chr_data) {
        UInt32 keyboard_type = LMGetKbdType();
        OSStatus err;

        for (i = 0; i < SDL_arraysize(darwin_scancode_table); i++) {
            UniChar s[8];
            UniCharCount len;
            UInt32 dead_key_state;

            /* Make sure this scancode is a valid character scancode */
            scancode = darwin_scancode_table[i];
            if (scancode == SDL_SCANCODE_UNKNOWN ||
                (keymap[scancode] & SDLK_SCANCODE_MASK)) {
                continue;
            }

            dead_key_state = 0;
            err = UCKeyTranslate ((UCKeyboardLayout *) chr_data,
                                  i, kUCKeyActionDown,
                                  0, keyboard_type,
                                  kUCKeyTranslateNoDeadKeysMask,
                                  &dead_key_state, 8, &len, s);
            if (err != noErr)
                continue;

            if (len > 0 && s[0] != 0x10) {
                keymap[scancode] = s[0];
            }
        }
        SDL_SetKeymap(0, keymap, SDL_NUM_SCANCODES);
        return;
    }

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
cleanup:
    CFRelease(key_layout);
#else
    /* Fall back to older style key map data */
    KLGetKeyboardLayoutProperty(key_layout, kKLKCHRData, &chr_data);
    if (chr_data) {
        for (i = 0; i < 128; i++) {
            UInt32 c, state = 0;

            /* Make sure this scancode is a valid character scancode */
            scancode = darwin_scancode_table[i];
            if (scancode == SDL_SCANCODE_UNKNOWN ||
                (keymap[scancode] & SDLK_SCANCODE_MASK)) {
                continue;
            }

            c = KeyTranslate (chr_data, i, &state) & 255;
            if (state) {
                /* Dead key, process key up */
                c = KeyTranslate (chr_data, i | 128, &state) & 255;
            }

            if (c != 0 && c != 0x10) {
                /* MacRoman to Unicode table, taken from X.org sources */
                static const unsigned short macroman_table[128] = {
                    0xc4, 0xc5, 0xc7, 0xc9, 0xd1, 0xd6, 0xdc, 0xe1,
                    0xe0, 0xe2, 0xe4, 0xe3, 0xe5, 0xe7, 0xe9, 0xe8,
                    0xea, 0xeb, 0xed, 0xec, 0xee, 0xef, 0xf1, 0xf3,
                    0xf2, 0xf4, 0xf6, 0xf5, 0xfa, 0xf9, 0xfb, 0xfc,
                    0x2020, 0xb0, 0xa2, 0xa3, 0xa7, 0x2022, 0xb6, 0xdf,
                    0xae, 0xa9, 0x2122, 0xb4, 0xa8, 0x2260, 0xc6, 0xd8,
                    0x221e, 0xb1, 0x2264, 0x2265, 0xa5, 0xb5, 0x2202, 0x2211,
                    0x220f, 0x3c0, 0x222b, 0xaa, 0xba, 0x3a9, 0xe6, 0xf8,
                    0xbf, 0xa1, 0xac, 0x221a, 0x192, 0x2248, 0x2206, 0xab,
                    0xbb, 0x2026, 0xa0, 0xc0, 0xc3, 0xd5, 0x152, 0x153,
                    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0xf7, 0x25ca,
                    0xff, 0x178, 0x2044, 0x20ac, 0x2039, 0x203a, 0xfb01, 0xfb02,
                    0x2021, 0xb7, 0x201a, 0x201e, 0x2030, 0xc2, 0xca, 0xc1,
                    0xcb, 0xc8, 0xcd, 0xce, 0xcf, 0xcc, 0xd3, 0xd4,
                    0xf8ff, 0xd2, 0xda, 0xdb, 0xd9, 0x131, 0x2c6, 0x2dc,
                    0xaf, 0x2d8, 0x2d9, 0x2da, 0xb8, 0x2dd, 0x2db, 0x2c7,
                };

                if (c >= 128) {
                    c = macroman_table[c - 128];
                }
                keymap[scancode] = c;
            }
        }
        SDL_SetKeymap(0, keymap, SDL_NUM_SCANCODES);
        return;
    }
#endif
}

void
Cocoa_InitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    UpdateKeymap(data);
    
    /* Set our own names for the platform-dependent but layout-independent keys */
    /* This key is NumLock on the MacBook keyboard. :) */
    /*SDL_SetScancodeName(SDL_SCANCODE_NUMLOCKCLEAR, "Clear");*/
    SDL_SetScancodeName(SDL_SCANCODE_LALT, "Left Option");
    SDL_SetScancodeName(SDL_SCANCODE_LGUI, "Left Command");
    SDL_SetScancodeName(SDL_SCANCODE_RALT, "Right Option");
    SDL_SetScancodeName(SDL_SCANCODE_RGUI, "Right Command");
}

void
Cocoa_StartTextInput(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSView *parentView = [[NSApp keyWindow] contentView];

    /* We only keep one field editor per process, since only the front most
     * window can receive text input events, so it make no sense to keep more
     * than one copy. When we switched to another window and requesting for
     * text input, simply remove the field editor from its superview then add
     * it to the front most window's content view */
    if (!data->fieldEdit) {
        data->fieldEdit =
            [[SDLTranslatorResponder alloc] initWithFrame: NSMakeRect(0.0, 0.0, 0.0, 0.0)];
    }

    if (![[data->fieldEdit superview] isEqual: parentView])
    {
        // DEBUG_IME(@"add fieldEdit to window contentView");
        [data->fieldEdit removeFromSuperview];
        [parentView addSubview: data->fieldEdit];
        [[NSApp keyWindow] makeFirstResponder: data->fieldEdit];
    }

    [pool release];
}

void
Cocoa_StopTextInput(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    if (data && data->fieldEdit) {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        [data->fieldEdit removeFromSuperview];
        [data->fieldEdit release];
        data->fieldEdit = nil;
        [pool release];
    }
}

void
Cocoa_SetTextInputRect(_THIS, SDL_Rect *rect)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    [data->fieldEdit setInputRect: rect];
}

void
Cocoa_HandleKeyEvent(_THIS, NSEvent *event)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    unsigned short scancode = [event keyCode];
    SDL_Scancode code;
#if 0
    const char *text;
#endif

    if ((scancode == 10 || scancode == 50) && KBGetLayoutType(LMGetKbdType()) == kKeyboardISO) {
        /* see comments in SDL_cocoakeys.h */
        scancode = 60 - scancode;
    }
    if (scancode < SDL_arraysize(darwin_scancode_table)) {
        code = darwin_scancode_table[scancode];
    }
    else {
        /* Hmm, does this ever happen?  If so, need to extend the keymap... */
        code = SDL_SCANCODE_UNKNOWN;
    }

    switch ([event type]) {
    case NSKeyDown:
        if (![event isARepeat]) {
            /* See if we need to rebuild the keyboard layout */
            UpdateKeymap(data);
        }

        SDL_SendKeyboardKey(SDL_PRESSED, code);
#if 1
        if (code == SDL_SCANCODE_UNKNOWN) {
            fprintf(stderr, "The key you just pressed is not recognized by SDL. To help get this fixed, report this to the SDL mailing list <sdl@libsdl.org> or to Christian Walther <cwalther@gmx.ch>. Mac virtual key code is %d.\n", scancode);
        }
#endif
        if (SDL_EventState(SDL_TEXTINPUT, SDL_QUERY)) {
            /* FIXME CW 2007-08-16: only send those events to the field editor for which we actually want text events, not e.g. esc or function keys. Arrow keys in particular seem to produce crashes sometimes. */
            [data->fieldEdit interpretKeyEvents:[NSArray arrayWithObject:event]];
#if 0
            text = [[event characters] UTF8String];
            if(text && *text) {
                SDL_SendKeyboardText(text);
                [data->fieldEdit setString:@""];
            }
#endif
        }
        break;
    case NSKeyUp:
        SDL_SendKeyboardKey(SDL_RELEASED, code);
        break;
    case NSFlagsChanged:
        /* FIXME CW 2007-08-14: check if this whole mess that takes up half of this file is really necessary */
        HandleModifiers(_this, scancode, [event modifierFlags]);
        break;
    default: /* just to avoid compiler warnings */
        break;
    }
}

void
Cocoa_QuitKeyboard(_THIS)
{
}

/* vi: set ts=4 sw=4 expandtab: */
