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

#import "SDL_uikitview.h"

#include "../../events/SDL_keyboard_c.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_touch_c.h"

#if SDL_IPHONE_KEYBOARD
#import "keyinfotable.h"
#import "SDL_uikitappdelegate.h"
#import "SDL_uikitwindow.h"
#endif

@implementation SDL_uikitview

- (void)dealloc {
    [super dealloc];
}

- (id)initWithFrame:(CGRect)frame {

    self = [super initWithFrame: frame];
    
#if SDL_IPHONE_KEYBOARD
    [self initializeKeyboard];
#endif    

#ifdef FIXED_MULTITOUCH
    SDL_Touch touch;
    touch.id = 0; //TODO: Should be -1?

    //touch.driverdata = SDL_malloc(sizeof(EventTouchData));
    //EventTouchData* data = (EventTouchData*)(touch.driverdata);
    
    touch.x_min = 0;
    touch.x_max = frame.size.width;
    touch.native_xres = touch.x_max - touch.x_min;
    touch.y_min = 0;
    touch.y_max = frame.size.height;
    touch.native_yres = touch.y_max - touch.y_min;
    touch.pressure_min = 0;
    touch.pressure_max = 1;
    touch.native_pressureres = touch.pressure_max - touch.pressure_min;


    touchId = SDL_AddTouch(&touch, "IPHONE SCREEN");
#endif

    return self;

}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {

    NSEnumerator *enumerator = [touches objectEnumerator];
    UITouch *touch = (UITouch*)[enumerator nextObject];
    
    //NSLog("Click");
    
    if (touch) {
        CGPoint locationInView = [touch locationInView: self];
            
        /* send moved event */
        SDL_SendMouseMotion(NULL, 0, locationInView.x, locationInView.y);

        /* send mouse down event */
        SDL_SendMouseButton(NULL, SDL_PRESSED, SDL_BUTTON_LEFT);
    }

#ifdef FIXED_MULTITOUCH
    while(touch) {
      CGPoint locationInView = [touch locationInView: self];


#ifdef IPHONE_TOUCH_EFFICIENT_DANGEROUS
      //FIXME: TODO: Using touch as the fingerId is potentially dangerous
      //It is also much more efficient than storing the UITouch pointer
      //and comparing it to the incoming event.
      SDL_SendFingerDown(touchId,(long)touch,
                 SDL_TRUE,locationInView.x,locationInView.y,
                 1);
#else
      int i;
      for(i = 0;i < MAX_SIMULTANEOUS_TOUCHES;i++) {
        if(finger[i] == NULL) {
          finger[i] = touch;
          SDL_SendFingerDown(touchId,i,
                 SDL_TRUE,locationInView.x,locationInView.y,
                 1);
          break;
        }
      }
#endif
      

      

      touch = (UITouch*)[enumerator nextObject]; 
    }
#endif
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    
    NSEnumerator *enumerator = [touches objectEnumerator];
    UITouch *touch = (UITouch*)[enumerator nextObject];
    
    if (touch) {
        /* send mouse up */
        SDL_SendMouseButton(NULL, SDL_RELEASED, SDL_BUTTON_LEFT);
    }

#ifdef FIXED_MULTITOUCH
    while(touch) {
      CGPoint locationInView = [touch locationInView: self];
      

#ifdef IPHONE_TOUCH_EFFICIENT_DANGEROUS
      SDL_SendFingerDown(touchId,(long)touch,
                 SDL_FALSE,locationInView.x,locationInView.y,
                 1);
#else
      int i;
      for(i = 0;i < MAX_SIMULTANEOUS_TOUCHES;i++) {
        if(finger[i] == touch) {
          SDL_SendFingerDown(touchId,i,
                 SDL_FALSE,locationInView.x,locationInView.y,
                 1);
          break;
        }
      }
#endif

      touch = (UITouch*)[enumerator nextObject]; 
    }
#endif
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    /*
        this can happen if the user puts more than 5 touches on the screen
        at once, or perhaps in other circumstances.  Usually (it seems)
        all active touches are canceled.
    */
    [self touchesEnded: touches withEvent: event];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    
    NSEnumerator *enumerator = [touches objectEnumerator];
    UITouch *touch = (UITouch*)[enumerator nextObject];
    
    if (touch) {
        CGPoint locationInView = [touch locationInView: self];

        /* send moved event */
        SDL_SendMouseMotion(NULL, 0, locationInView.x, locationInView.y);
    }

#ifdef FIXED_MULTITOUCH
    while(touch) {
      CGPoint locationInView = [touch locationInView: self];
      

#ifdef IPHONE_TOUCH_EFFICIENT_DANGEROUS
      SDL_SendTouchMotion(touchId,(long)touch,
                  SDL_FALSE,locationInView.x,locationInView.y,
                  1);
#else
      int i;
      for(i = 0;i < MAX_SIMULTANEOUS_TOUCHES;i++) {
        if(finger[i] == touch) {
          SDL_SendTouchMotion(touchId,i,
                  SDL_FALSE,locationInView.x,locationInView.y,
                  1);
          break;
        }
      }
#endif

      touch = (UITouch*)[enumerator nextObject]; 
    }
#endif
}

/*
    ---- Keyboard related functionality below this line ----
*/
#if SDL_IPHONE_KEYBOARD

/* Is the iPhone virtual keyboard visible onscreen? */
- (BOOL)keyboardVisible {
    return keyboardVisible;
}

/* Set ourselves up as a UITextFieldDelegate */
- (void)initializeKeyboard {
        
    textField = [[UITextField alloc] initWithFrame: CGRectZero];
    textField.delegate = self;
    /* placeholder so there is something to delete! */
    textField.text = @" ";    
    
    /* set UITextInputTrait properties, mostly to defaults */
    textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
    textField.autocorrectionType = UITextAutocorrectionTypeNo;
    textField.enablesReturnKeyAutomatically = NO;
    textField.keyboardAppearance = UIKeyboardAppearanceDefault;
    textField.keyboardType = UIKeyboardTypeDefault;
    textField.returnKeyType = UIReturnKeyDefault;
    textField.secureTextEntry = NO;    
    
    textField.hidden = YES;
    keyboardVisible = NO;
    /* add the UITextField (hidden) to our view */
    [self addSubview: textField];
    [textField release];
}

/* reveal onscreen virtual keyboard */
- (void)showKeyboard {
    keyboardVisible = YES;
    [textField becomeFirstResponder];
}

/* hide onscreen virtual keyboard */
- (void)hideKeyboard {
    keyboardVisible = NO;
    [textField resignFirstResponder];
}

/* UITextFieldDelegate method.  Invoked when user types something. */
- (BOOL)textField:(UITextField *)_textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
    
    if ([string length] == 0) {
        /* it wants to replace text with nothing, ie a delete */
        SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_DELETE);
        SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_DELETE);
    }
    else {
        /* go through all the characters in the string we've been sent
           and convert them to key presses */
        int i;
        for (i=0; i<[string length]; i++) {
            
            unichar c = [string characterAtIndex: i];
            
            Uint16 mod = 0;
            SDL_Scancode code;
            
            if (c < 127) {
                /* figure out the SDL_Scancode and SDL_keymod for this unichar */
                code = unicharToUIKeyInfoTable[c].code;
                mod  = unicharToUIKeyInfoTable[c].mod;
            }
            else {
                /* we only deal with ASCII right now */
                code = SDL_SCANCODE_UNKNOWN;
                mod = 0;
            }
            
            if (mod & KMOD_SHIFT) {
                /* If character uses shift, press shift down */
                SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_LSHIFT);
            }
            /* send a keydown and keyup even for the character */
            SDL_SendKeyboardKey(SDL_PRESSED, code);
            SDL_SendKeyboardKey(SDL_RELEASED, code);
            if (mod & KMOD_SHIFT) {
                /* If character uses shift, press shift back up */
                SDL_SendKeyboardKey(SDL_RELEASED, SDL_SCANCODE_LSHIFT);
            }            
        }
    }
    return NO; /* don't allow the edit! (keep placeholder text there) */
}

/* Terminates the editing session */
- (BOOL)textFieldShouldReturn:(UITextField*)_textField {
    SDL_SendKeyboardKey(SDL_PRESSED, SDL_SCANCODE_RETURN);
    [self hideKeyboard];
    return YES;
}

#endif

@end

/* iPhone keyboard addition functions */
#if SDL_IPHONE_KEYBOARD

int SDL_iPhoneKeyboardShow(SDL_Window * window) {
    
    SDL_WindowData *data;
    SDL_uikitview *view;
    
    if (NULL == window) {
        SDL_SetError("Window does not exist");
        return -1;
    }
    
    data = (SDL_WindowData *)window->driverdata;
    view = data->view;
    
    if (nil == view) {
        SDL_SetError("Window has no view");
        return -1;
    }
    else {
        [view showKeyboard];
        return 0;
    }
}

int SDL_iPhoneKeyboardHide(SDL_Window * window) {
    
    SDL_WindowData *data;
    SDL_uikitview *view;
    
    if (NULL == window) {
        SDL_SetError("Window does not exist");
        return -1;
    }    
    
    data = (SDL_WindowData *)window->driverdata;
    view = data->view;
    
    if (NULL == view) {
        SDL_SetError("Window has no view");
        return -1;
    }
    else {
        [view hideKeyboard];
        return 0;
    }
}

SDL_bool SDL_iPhoneKeyboardIsShown(SDL_Window * window) {
    
    SDL_WindowData *data;
    SDL_uikitview *view;
    
    if (NULL == window) {
        SDL_SetError("Window does not exist");
        return -1;
    }    
    
    data = (SDL_WindowData *)window->driverdata;
    view = data->view;
    
    if (NULL == view) {
        SDL_SetError("Window has no view");
        return 0;
    }
    else {
        return view.keyboardVisible;
    }
}

int SDL_iPhoneKeyboardToggle(SDL_Window * window) {
    
    SDL_WindowData *data;
    SDL_uikitview *view;
    
    if (NULL == window) {
        SDL_SetError("Window does not exist");
        return -1;
    }    
    
    data = (SDL_WindowData *)window->driverdata;
    view = data->view;
    
    if (NULL == view) {
        SDL_SetError("Window has no view");
        return -1;
    }
    else {
        if (SDL_iPhoneKeyboardIsShown(window)) {
            SDL_iPhoneKeyboardHide(window);
        }
        else {
            SDL_iPhoneKeyboardShow(window);
        }
        return 0;
    }
}

#else

/* stubs, used if compiled without keyboard support */

int SDL_iPhoneKeyboardShow(SDL_Window * window) {
    SDL_SetError("Not compiled with keyboard support");
    return -1;
}

int SDL_iPhoneKeyboardHide(SDL_Window * window) {
    SDL_SetError("Not compiled with keyboard support");
    return -1;
}

SDL_bool SDL_iPhoneKeyboardIsShown(SDL_Window * window) {
    return 0;
}

int SDL_iPhoneKeyboardToggle(SDL_Window * window) {
    SDL_SetError("Not compiled with keyboard support");
    return -1;
}

#endif /* SDL_IPHONE_KEYBOARD */

/* vi: set ts=4 sw=4 expandtab: */
