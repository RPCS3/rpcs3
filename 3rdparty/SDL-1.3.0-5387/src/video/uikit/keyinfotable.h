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

#ifndef _UIKIT_KeyInfo
#define _UIKIT_KeyInfo

#include "SDL_scancode.h"

/*
    This file is used by the keyboard code in SDL_uikitview.m to convert between characters
    passed in from the iPhone's virtual keyboard, and tuples of SDL_Scancode and SDL_keymods.
    For example unicharToUIKeyInfoTable['a'] would give you the scan code and keymod for lower
    case a.
*/

typedef struct
{
    SDL_Scancode code;
    Uint16 mod;
} UIKitKeyInfo;

/* So far only ASCII characters here */
static UIKitKeyInfo unicharToUIKeyInfoTable[] = {
/* *INDENT-OFF* */
/*  0 */  {  SDL_SCANCODE_UNKNOWN, 0 },
/*  1 */  {  SDL_SCANCODE_UNKNOWN, 0 },
/*  2 */  {  SDL_SCANCODE_UNKNOWN, 0 },
/*  3 */  {  SDL_SCANCODE_UNKNOWN, 0 },
/*  4 */  {  SDL_SCANCODE_UNKNOWN, 0 },
/*  5 */  {  SDL_SCANCODE_UNKNOWN, 0 },
/*  6 */  {  SDL_SCANCODE_UNKNOWN, 0 },
/*  7 */  {  SDL_SCANCODE_UNKNOWN, 0 },
/*  8 */  {  SDL_SCANCODE_UNKNOWN, 0 },
/*  9 */  {  SDL_SCANCODE_UNKNOWN, 0 },
/*  10 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  11 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  12 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  13 */ {   SDL_SCANCODE_RETURN, 0 },
/*  14 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  15 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  16 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  17 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  18 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  19 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  20 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  21 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  22 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  23 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  24 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  25 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  26 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  27 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  28 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  29 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  30 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  31 */ {   SDL_SCANCODE_UNKNOWN, 0 },
/*  32 */ {   SDL_SCANCODE_SPACE, 0 },
/*  33 */ {   SDL_SCANCODE_1,    KMOD_SHIFT },        /* plus shift modifier '!' */
/*  34 */ {   SDL_SCANCODE_APOSTROPHE, KMOD_SHIFT },    /* plus shift modifier '"' */
/*  35 */ {   SDL_SCANCODE_3, KMOD_SHIFT },            /* plus shift modifier '#' */
/*  36 */ {   SDL_SCANCODE_4, KMOD_SHIFT },            /* plus shift modifier '$' */
/*  37 */ {   SDL_SCANCODE_5, KMOD_SHIFT },            /* plus shift modifier '%' */
/*  38 */ {   SDL_SCANCODE_7, KMOD_SHIFT },            /* plus shift modifier '&' */
/*  39 */ {   SDL_SCANCODE_APOSTROPHE, 0 },    /* '''                       */
/*  40 */ {   SDL_SCANCODE_9, KMOD_SHIFT },             /* plus shift modifier '(' */
/*  41 */ {   SDL_SCANCODE_0, KMOD_SHIFT },            /* plus shift modifier ')' */
/*  42 */ {   SDL_SCANCODE_8, KMOD_SHIFT },            /* '*' */
/*  43 */ {   SDL_SCANCODE_EQUALS, KMOD_SHIFT },    /* plus shift modifier '+' */
/*  44 */ {   SDL_SCANCODE_COMMA, 0 },        /* ','                       */
/*  45 */ {   SDL_SCANCODE_MINUS, 0 },        /* '-'                       */
/*  46 */ {   SDL_SCANCODE_PERIOD, 0 },        /* '.'                       */
/*  47 */ {   SDL_SCANCODE_SLASH, 0 },        /* '/'                       */
/*  48 */ {   SDL_SCANCODE_0, 0    },        
/*  49 */ {   SDL_SCANCODE_1, 0 },
/*  50 */ {   SDL_SCANCODE_2, 0 },
/*  51 */ {   SDL_SCANCODE_3, 0 },
/*  52 */ {   SDL_SCANCODE_4, 0 },
/*  53 */ {   SDL_SCANCODE_5, 0 },
/*  54 */ {   SDL_SCANCODE_6, 0 },
/*  55 */ {   SDL_SCANCODE_7, 0 },
/*  56 */ {   SDL_SCANCODE_8, 0 },
/*  57 */ {   SDL_SCANCODE_9, 0 },
/*  58 */ {   SDL_SCANCODE_SEMICOLON,  KMOD_SHIFT },    /* plus shift modifier ';' */
/*  59 */ {   SDL_SCANCODE_SEMICOLON, 0 },
/*  60 */ {   SDL_SCANCODE_COMMA,  KMOD_SHIFT },         /* plus shift modifier '<' */
/*  61 */ {   SDL_SCANCODE_EQUALS, 0 },
/*  62 */ {   SDL_SCANCODE_PERIOD,  KMOD_SHIFT },        /* plus shift modifier '>' */
/*  63 */ {   SDL_SCANCODE_SLASH,  KMOD_SHIFT },        /* plus shift modifier '?' */
/*  64 */ {   SDL_SCANCODE_2,  KMOD_SHIFT },            /* plus shift modifier '@' */
/*  65 */ {   SDL_SCANCODE_A,  KMOD_SHIFT },            /* all the following need shift modifiers */
/*  66 */ {   SDL_SCANCODE_B,  KMOD_SHIFT },
/*  67 */ {   SDL_SCANCODE_C,  KMOD_SHIFT },
/*  68 */ {   SDL_SCANCODE_D,  KMOD_SHIFT },
/*  69 */ {   SDL_SCANCODE_E,  KMOD_SHIFT },
/*  70 */ {   SDL_SCANCODE_F,  KMOD_SHIFT },
/*  71 */ {   SDL_SCANCODE_G,  KMOD_SHIFT },
/*  72 */ {   SDL_SCANCODE_H,  KMOD_SHIFT },
/*  73 */ {   SDL_SCANCODE_I,  KMOD_SHIFT },
/*  74 */ {   SDL_SCANCODE_J,  KMOD_SHIFT },
/*  75 */ {   SDL_SCANCODE_K,  KMOD_SHIFT },
/*  76 */ {   SDL_SCANCODE_L,  KMOD_SHIFT },
/*  77 */ {   SDL_SCANCODE_M,  KMOD_SHIFT },
/*  78 */ {   SDL_SCANCODE_N,  KMOD_SHIFT },
/*  79 */ {   SDL_SCANCODE_O,  KMOD_SHIFT },
/*  80 */ {   SDL_SCANCODE_P,  KMOD_SHIFT },
/*  81 */ {   SDL_SCANCODE_Q,  KMOD_SHIFT },
/*  82 */ {   SDL_SCANCODE_R,  KMOD_SHIFT },
/*  83 */ {   SDL_SCANCODE_S,  KMOD_SHIFT },
/*  84 */ {   SDL_SCANCODE_T,  KMOD_SHIFT },
/*  85 */ {   SDL_SCANCODE_U,  KMOD_SHIFT },
/*  86 */ {   SDL_SCANCODE_V,  KMOD_SHIFT },
/*  87 */ {   SDL_SCANCODE_W,  KMOD_SHIFT },
/*  88 */ {   SDL_SCANCODE_X,  KMOD_SHIFT },
/*  89 */ {   SDL_SCANCODE_Y,  KMOD_SHIFT },
/*  90 */ {   SDL_SCANCODE_Z,  KMOD_SHIFT },
/*  91 */ {   SDL_SCANCODE_LEFTBRACKET, 0 },
/*  92 */ {   SDL_SCANCODE_BACKSLASH, 0 },
/*  93 */ {   SDL_SCANCODE_RIGHTBRACKET, 0 },
/*  94 */ {   SDL_SCANCODE_6,  KMOD_SHIFT },            /* plus shift modifier '^' */
/*  95 */ {   SDL_SCANCODE_MINUS,  KMOD_SHIFT },        /* plus shift modifier '_' */
/*  96 */ {   SDL_SCANCODE_GRAVE,  KMOD_SHIFT },        /* '`' */
/*  97 */ {   SDL_SCANCODE_A, 0    },    
/*  98 */ {   SDL_SCANCODE_B, 0 },
/*  99 */ {   SDL_SCANCODE_C, 0 },
/*  100 */{    SDL_SCANCODE_D, 0 },
/*  101 */{    SDL_SCANCODE_E, 0 },
/*  102 */{    SDL_SCANCODE_F, 0 },
/*  103 */{    SDL_SCANCODE_G, 0 },
/*  104 */{    SDL_SCANCODE_H, 0 },
/*  105 */{    SDL_SCANCODE_I, 0 },
/*  106 */{    SDL_SCANCODE_J, 0 },
/*  107 */{    SDL_SCANCODE_K, 0 },
/*  108 */{    SDL_SCANCODE_L, 0 },
/*  109 */{    SDL_SCANCODE_M, 0 },
/*  110 */{    SDL_SCANCODE_N, 0 },
/*  111 */{    SDL_SCANCODE_O, 0 },
/*  112 */{    SDL_SCANCODE_P, 0 },
/*  113 */{    SDL_SCANCODE_Q, 0 },
/*  114 */{    SDL_SCANCODE_R, 0 },
/*  115 */{    SDL_SCANCODE_S, 0 },
/*  116 */{    SDL_SCANCODE_T, 0 },
/*  117 */{    SDL_SCANCODE_U, 0 },
/*  118 */{    SDL_SCANCODE_V, 0 },
/*  119 */{    SDL_SCANCODE_W, 0 },
/*  120 */{    SDL_SCANCODE_X, 0 },
/*  121 */{    SDL_SCANCODE_Y, 0 },
/*  122 */{    SDL_SCANCODE_Z, 0 },
/*  123 */{    SDL_SCANCODE_LEFTBRACKET, KMOD_SHIFT },    /* plus shift modifier '{' */
/*  124 */{    SDL_SCANCODE_BACKSLASH, KMOD_SHIFT },    /* plus shift modifier '|' */
/*  125 */{    SDL_SCANCODE_RIGHTBRACKET, KMOD_SHIFT },    /* plus shift modifier '}' */
/*  126 */{    SDL_SCANCODE_GRAVE, KMOD_SHIFT },         /* plus shift modifier '~' */
/*  127 */{    SDL_SCANCODE_DELETE, KMOD_SHIFT }
/* *INDENT-ON* */
};

#endif /* UIKitKeyInfo */

/* vi: set ts=4 sw=4 expandtab: */
