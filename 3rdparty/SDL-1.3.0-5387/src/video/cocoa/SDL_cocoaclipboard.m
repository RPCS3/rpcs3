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
#include "../../events/SDL_clipboardevents_c.h"

static NSString *
GetTextFormat(_THIS)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;

    if (data->osversion >= 0x1060) {
        return NSPasteboardTypeString;
    } else {
        return NSStringPboardType;
    }
#else
    return NSStringPboardType;
#endif
}

int
Cocoa_SetClipboardText(_THIS, const char *text)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    NSAutoreleasePool *pool;
    NSPasteboard *pasteboard;
    NSString *format = GetTextFormat(_this);

    pool = [[NSAutoreleasePool alloc] init];

    pasteboard = [NSPasteboard generalPasteboard];
    data->clipboard_count = [pasteboard declareTypes:[NSArray arrayWithObject:format] owner:nil];
    [pasteboard setString:[NSString stringWithUTF8String:text] forType:format];

    [pool release];

    return 0;
}

char *
Cocoa_GetClipboardText(_THIS)
{
    NSAutoreleasePool *pool;
    NSPasteboard *pasteboard;
    NSString *format = GetTextFormat(_this);
    NSString *available;
    char *text;

    pool = [[NSAutoreleasePool alloc] init];

    pasteboard = [NSPasteboard generalPasteboard];
    available = [pasteboard availableTypeFromArray: [NSArray arrayWithObject:format]];
    if ([available isEqualToString:format]) {
        NSString* string;
        const char *utf8;

        string = [pasteboard stringForType:format];
        if (string == nil) {
            utf8 = "";
        } else {
            utf8 = [string UTF8String];
        }
        text = SDL_strdup(utf8);
    } else {
        text = SDL_strdup("");
    }

    [pool release];

    return text;
}

SDL_bool
Cocoa_HasClipboardText(_THIS)
{
    NSAutoreleasePool *pool;
    NSPasteboard *pasteboard;
    NSString *format = GetTextFormat(_this);
    NSString *available;
    SDL_bool result;

    pool = [[NSAutoreleasePool alloc] init];

    pasteboard = [NSPasteboard generalPasteboard];
    available = [pasteboard availableTypeFromArray: [NSArray arrayWithObject:format]];
    if ([available isEqualToString:format]) {
        result = SDL_TRUE;
    } else {
        result = SDL_FALSE;
    }

    [pool release];

    return result;
}

void
Cocoa_CheckClipboardUpdate(struct SDL_VideoData * data)
{
    NSAutoreleasePool *pool;
    NSPasteboard *pasteboard;
    NSInteger count;

    pool = [[NSAutoreleasePool alloc] init];

    pasteboard = [NSPasteboard generalPasteboard];
    count = [pasteboard changeCount];
    if (count != data->clipboard_count) {
        if (data->clipboard_count) {
            SDL_SendClipboardUpdate();
        }
        data->clipboard_count = count;
    }

    [pool release];
}

/* vi: set ts=4 sw=4 expandtab: */
