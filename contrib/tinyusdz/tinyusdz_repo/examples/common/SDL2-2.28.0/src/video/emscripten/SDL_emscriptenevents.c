/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/


#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_EMSCRIPTEN

#include <emscripten/html5.h>
#include <emscripten/dom_pk_codes.h>

#include "../../events/SDL_events_c.h"
#include "../../events/SDL_keyboard_c.h"
#include "../../events/SDL_touch_c.h"

#include "SDL_emscriptenevents.h"
#include "SDL_emscriptenvideo.h"

#include "SDL_hints.h"

#define FULLSCREEN_MASK ( SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_FULLSCREEN )

/*
.keyCode to SDL keycode
https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent
https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/keyCode
*/
static const SDL_Keycode emscripten_keycode_table[] = {
    /*  0 */ SDLK_UNKNOWN,
    /*  1 */ SDLK_UNKNOWN,
    /*  2 */ SDLK_UNKNOWN,
    /*  3 */ SDLK_CANCEL,
    /*  4 */ SDLK_UNKNOWN,
    /*  5 */ SDLK_UNKNOWN,
    /*  6 */ SDLK_HELP,
    /*  7 */ SDLK_UNKNOWN,
    /*  8 */ SDLK_BACKSPACE,
    /*  9 */ SDLK_TAB,
    /*  10 */ SDLK_UNKNOWN,
    /*  11 */ SDLK_UNKNOWN,
    /*  12 */ SDLK_KP_5,
    /*  13 */ SDLK_RETURN,
    /*  14 */ SDLK_UNKNOWN,
    /*  15 */ SDLK_UNKNOWN,
    /*  16 */ SDLK_LSHIFT,
    /*  17 */ SDLK_LCTRL,
    /*  18 */ SDLK_LALT,
    /*  19 */ SDLK_PAUSE,
    /*  20 */ SDLK_CAPSLOCK,
    /*  21 */ SDLK_UNKNOWN,
    /*  22 */ SDLK_UNKNOWN,
    /*  23 */ SDLK_UNKNOWN,
    /*  24 */ SDLK_UNKNOWN,
    /*  25 */ SDLK_UNKNOWN,
    /*  26 */ SDLK_UNKNOWN,
    /*  27 */ SDLK_ESCAPE,
    /*  28 */ SDLK_UNKNOWN,
    /*  29 */ SDLK_UNKNOWN,
    /*  30 */ SDLK_UNKNOWN,
    /*  31 */ SDLK_UNKNOWN,
    /*  32 */ SDLK_SPACE,
    /*  33 */ SDLK_PAGEUP,
    /*  34 */ SDLK_PAGEDOWN,
    /*  35 */ SDLK_END,
    /*  36 */ SDLK_HOME,
    /*  37 */ SDLK_LEFT,
    /*  38 */ SDLK_UP,
    /*  39 */ SDLK_RIGHT,
    /*  40 */ SDLK_DOWN,
    /*  41 */ SDLK_UNKNOWN,
    /*  42 */ SDLK_UNKNOWN,
    /*  43 */ SDLK_UNKNOWN,
    /*  44 */ SDLK_UNKNOWN,
    /*  45 */ SDLK_INSERT,
    /*  46 */ SDLK_DELETE,
    /*  47 */ SDLK_UNKNOWN,
    /*  48 */ SDLK_0,
    /*  49 */ SDLK_1,
    /*  50 */ SDLK_2,
    /*  51 */ SDLK_3,
    /*  52 */ SDLK_4,
    /*  53 */ SDLK_5,
    /*  54 */ SDLK_6,
    /*  55 */ SDLK_7,
    /*  56 */ SDLK_8,
    /*  57 */ SDLK_9,
    /*  58 */ SDLK_UNKNOWN,
    /*  59 */ SDLK_SEMICOLON,
    /*  60 */ SDLK_BACKSLASH /*SDL_SCANCODE_NONUSBACKSLASH*/,
    /*  61 */ SDLK_EQUALS,
    /*  62 */ SDLK_UNKNOWN,
    /*  63 */ SDLK_MINUS,
    /*  64 */ SDLK_UNKNOWN,
    /*  65 */ SDLK_a,
    /*  66 */ SDLK_b,
    /*  67 */ SDLK_c,
    /*  68 */ SDLK_d,
    /*  69 */ SDLK_e,
    /*  70 */ SDLK_f,
    /*  71 */ SDLK_g,
    /*  72 */ SDLK_h,
    /*  73 */ SDLK_i,
    /*  74 */ SDLK_j,
    /*  75 */ SDLK_k,
    /*  76 */ SDLK_l,
    /*  77 */ SDLK_m,
    /*  78 */ SDLK_n,
    /*  79 */ SDLK_o,
    /*  80 */ SDLK_p,
    /*  81 */ SDLK_q,
    /*  82 */ SDLK_r,
    /*  83 */ SDLK_s,
    /*  84 */ SDLK_t,
    /*  85 */ SDLK_u,
    /*  86 */ SDLK_v,
    /*  87 */ SDLK_w,
    /*  88 */ SDLK_x,
    /*  89 */ SDLK_y,
    /*  90 */ SDLK_z,
    /*  91 */ SDLK_LGUI,
    /*  92 */ SDLK_UNKNOWN,
    /*  93 */ SDLK_APPLICATION,
    /*  94 */ SDLK_UNKNOWN,
    /*  95 */ SDLK_UNKNOWN,
    /*  96 */ SDLK_KP_0,
    /*  97 */ SDLK_KP_1,
    /*  98 */ SDLK_KP_2,
    /*  99 */ SDLK_KP_3,
    /* 100 */ SDLK_KP_4,
    /* 101 */ SDLK_KP_5,
    /* 102 */ SDLK_KP_6,
    /* 103 */ SDLK_KP_7,
    /* 104 */ SDLK_KP_8,
    /* 105 */ SDLK_KP_9,
    /* 106 */ SDLK_KP_MULTIPLY,
    /* 107 */ SDLK_KP_PLUS,
    /* 108 */ SDLK_UNKNOWN,
    /* 109 */ SDLK_KP_MINUS,
    /* 110 */ SDLK_KP_PERIOD,
    /* 111 */ SDLK_KP_DIVIDE,
    /* 112 */ SDLK_F1,
    /* 113 */ SDLK_F2,
    /* 114 */ SDLK_F3,
    /* 115 */ SDLK_F4,
    /* 116 */ SDLK_F5,
    /* 117 */ SDLK_F6,
    /* 118 */ SDLK_F7,
    /* 119 */ SDLK_F8,
    /* 120 */ SDLK_F9,
    /* 121 */ SDLK_F10,
    /* 122 */ SDLK_F11,
    /* 123 */ SDLK_F12,
    /* 124 */ SDLK_F13,
    /* 125 */ SDLK_F14,
    /* 126 */ SDLK_F15,
    /* 127 */ SDLK_F16,
    /* 128 */ SDLK_F17,
    /* 129 */ SDLK_F18,
    /* 130 */ SDLK_F19,
    /* 131 */ SDLK_F20,
    /* 132 */ SDLK_F21,
    /* 133 */ SDLK_F22,
    /* 134 */ SDLK_F23,
    /* 135 */ SDLK_F24,
    /* 136 */ SDLK_UNKNOWN,
    /* 137 */ SDLK_UNKNOWN,
    /* 138 */ SDLK_UNKNOWN,
    /* 139 */ SDLK_UNKNOWN,
    /* 140 */ SDLK_UNKNOWN,
    /* 141 */ SDLK_UNKNOWN,
    /* 142 */ SDLK_UNKNOWN,
    /* 143 */ SDLK_UNKNOWN,
    /* 144 */ SDLK_NUMLOCKCLEAR,
    /* 145 */ SDLK_SCROLLLOCK,
    /* 146 */ SDLK_UNKNOWN,
    /* 147 */ SDLK_UNKNOWN,
    /* 148 */ SDLK_UNKNOWN,
    /* 149 */ SDLK_UNKNOWN,
    /* 150 */ SDLK_UNKNOWN,
    /* 151 */ SDLK_UNKNOWN,
    /* 152 */ SDLK_UNKNOWN,
    /* 153 */ SDLK_UNKNOWN,
    /* 154 */ SDLK_UNKNOWN,
    /* 155 */ SDLK_UNKNOWN,
    /* 156 */ SDLK_UNKNOWN,
    /* 157 */ SDLK_UNKNOWN,
    /* 158 */ SDLK_UNKNOWN,
    /* 159 */ SDLK_UNKNOWN,
    /* 160 */ SDLK_BACKQUOTE,
    /* 161 */ SDLK_UNKNOWN,
    /* 162 */ SDLK_UNKNOWN,
    /* 163 */ SDLK_KP_HASH, /*KaiOS phone keypad*/
    /* 164 */ SDLK_UNKNOWN,
    /* 165 */ SDLK_UNKNOWN,
    /* 166 */ SDLK_UNKNOWN,
    /* 167 */ SDLK_UNKNOWN,
    /* 168 */ SDLK_UNKNOWN,
    /* 169 */ SDLK_UNKNOWN,
    /* 170 */ SDLK_KP_MULTIPLY, /*KaiOS phone keypad*/
    /* 171 */ SDLK_RIGHTBRACKET,
    /* 172 */ SDLK_UNKNOWN,
    /* 173 */ SDLK_MINUS,      /*FX*/
    /* 174 */ SDLK_VOLUMEDOWN, /*IE, Chrome*/
    /* 175 */ SDLK_VOLUMEUP,   /*IE, Chrome*/
    /* 176 */ SDLK_AUDIONEXT,  /*IE, Chrome*/
    /* 177 */ SDLK_AUDIOPREV,  /*IE, Chrome*/
    /* 178 */ SDLK_UNKNOWN,
    /* 179 */ SDLK_AUDIOPLAY, /*IE, Chrome*/
    /* 180 */ SDLK_UNKNOWN,
    /* 181 */ SDLK_AUDIOMUTE,  /*FX*/
    /* 182 */ SDLK_VOLUMEDOWN, /*FX*/
    /* 183 */ SDLK_VOLUMEUP,   /*FX*/
    /* 184 */ SDLK_UNKNOWN,
    /* 185 */ SDLK_UNKNOWN,
    /* 186 */ SDLK_SEMICOLON, /*IE, Chrome, D3E legacy*/
    /* 187 */ SDLK_EQUALS,    /*IE, Chrome, D3E legacy*/
    /* 188 */ SDLK_COMMA,
    /* 189 */ SDLK_MINUS, /*IE, Chrome, D3E legacy*/
    /* 190 */ SDLK_PERIOD,
    /* 191 */ SDLK_SLASH,
    /* 192 */ SDLK_BACKQUOTE, /*FX, D3E legacy (SDLK_APOSTROPHE in IE/Chrome)*/
    /* 193 */ SDLK_UNKNOWN,
    /* 194 */ SDLK_UNKNOWN,
    /* 195 */ SDLK_UNKNOWN,
    /* 196 */ SDLK_UNKNOWN,
    /* 197 */ SDLK_UNKNOWN,
    /* 198 */ SDLK_UNKNOWN,
    /* 199 */ SDLK_UNKNOWN,
    /* 200 */ SDLK_UNKNOWN,
    /* 201 */ SDLK_UNKNOWN,
    /* 202 */ SDLK_UNKNOWN,
    /* 203 */ SDLK_UNKNOWN,
    /* 204 */ SDLK_UNKNOWN,
    /* 205 */ SDLK_UNKNOWN,
    /* 206 */ SDLK_UNKNOWN,
    /* 207 */ SDLK_UNKNOWN,
    /* 208 */ SDLK_UNKNOWN,
    /* 209 */ SDLK_UNKNOWN,
    /* 210 */ SDLK_UNKNOWN,
    /* 211 */ SDLK_UNKNOWN,
    /* 212 */ SDLK_UNKNOWN,
    /* 213 */ SDLK_UNKNOWN,
    /* 214 */ SDLK_UNKNOWN,
    /* 215 */ SDLK_UNKNOWN,
    /* 216 */ SDLK_UNKNOWN,
    /* 217 */ SDLK_UNKNOWN,
    /* 218 */ SDLK_UNKNOWN,
    /* 219 */ SDLK_LEFTBRACKET,
    /* 220 */ SDLK_BACKSLASH,
    /* 221 */ SDLK_RIGHTBRACKET,
    /* 222 */ SDLK_QUOTE, /*FX, D3E legacy*/
};

/*
Emscripten PK code to scancode
https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent
https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/code
*/
static const SDL_Scancode emscripten_scancode_table[] = {
    /* 0x00 "Unidentified"   */ SDL_SCANCODE_UNKNOWN,
    /* 0x01 "Escape"         */ SDL_SCANCODE_ESCAPE,
    /* 0x02 "Digit0"         */ SDL_SCANCODE_0,
    /* 0x03 "Digit1"         */ SDL_SCANCODE_1,
    /* 0x04 "Digit2"         */ SDL_SCANCODE_2,
    /* 0x05 "Digit3"         */ SDL_SCANCODE_3,
    /* 0x06 "Digit4"         */ SDL_SCANCODE_4,
    /* 0x07 "Digit5"         */ SDL_SCANCODE_5,
    /* 0x08 "Digit6"         */ SDL_SCANCODE_6,
    /* 0x09 "Digit7"         */ SDL_SCANCODE_7,
    /* 0x0A "Digit8"         */ SDL_SCANCODE_8,
    /* 0x0B "Digit9"         */ SDL_SCANCODE_9,
    /* 0x0C "Minus"          */ SDL_SCANCODE_MINUS,
    /* 0x0D "Equal"          */ SDL_SCANCODE_EQUALS,
    /* 0x0E "Backspace"      */ SDL_SCANCODE_BACKSPACE,
    /* 0x0F "Tab"            */ SDL_SCANCODE_TAB,
    /* 0x10 "KeyQ"           */ SDL_SCANCODE_Q,
    /* 0x11 "KeyW"           */ SDL_SCANCODE_W,
    /* 0x12 "KeyE"           */ SDL_SCANCODE_E,
    /* 0x13 "KeyR"           */ SDL_SCANCODE_R,
    /* 0x14 "KeyT"           */ SDL_SCANCODE_T,
    /* 0x15 "KeyY"           */ SDL_SCANCODE_Y,
    /* 0x16 "KeyU"           */ SDL_SCANCODE_U,
    /* 0x17 "KeyI"           */ SDL_SCANCODE_I,
    /* 0x18 "KeyO"           */ SDL_SCANCODE_O,
    /* 0x19 "KeyP"           */ SDL_SCANCODE_P,
    /* 0x1A "BracketLeft"    */ SDL_SCANCODE_LEFTBRACKET,
    /* 0x1B "BracketRight"   */ SDL_SCANCODE_RIGHTBRACKET,
    /* 0x1C "Enter"          */ SDL_SCANCODE_RETURN,
    /* 0x1D "ControlLeft"    */ SDL_SCANCODE_LCTRL,
    /* 0x1E "KeyA"           */ SDL_SCANCODE_A,
    /* 0x1F "KeyS"           */ SDL_SCANCODE_S,
    /* 0x20 "KeyD"           */ SDL_SCANCODE_D,
    /* 0x21 "KeyF"           */ SDL_SCANCODE_F,
    /* 0x22 "KeyG"           */ SDL_SCANCODE_G,
    /* 0x23 "KeyH"           */ SDL_SCANCODE_H,
    /* 0x24 "KeyJ"           */ SDL_SCANCODE_J,
    /* 0x25 "KeyK"           */ SDL_SCANCODE_K,
    /* 0x26 "KeyL"           */ SDL_SCANCODE_L,
    /* 0x27 "Semicolon"      */ SDL_SCANCODE_SEMICOLON,
    /* 0x28 "Quote"          */ SDL_SCANCODE_APOSTROPHE,
    /* 0x29 "Backquote"      */ SDL_SCANCODE_GRAVE,
    /* 0x2A "ShiftLeft"      */ SDL_SCANCODE_LSHIFT,
    /* 0x2B "Backslash"      */ SDL_SCANCODE_BACKSLASH,
    /* 0x2C "KeyZ"           */ SDL_SCANCODE_Z,
    /* 0x2D "KeyX"           */ SDL_SCANCODE_X,
    /* 0x2E "KeyC"           */ SDL_SCANCODE_C,
    /* 0x2F "KeyV"           */ SDL_SCANCODE_V,
    /* 0x30 "KeyB"           */ SDL_SCANCODE_B,
    /* 0x31 "KeyN"           */ SDL_SCANCODE_N,
    /* 0x32 "KeyM"           */ SDL_SCANCODE_M,
    /* 0x33 "Comma"          */ SDL_SCANCODE_COMMA,
    /* 0x34 "Period"         */ SDL_SCANCODE_PERIOD,
    /* 0x35 "Slash"          */ SDL_SCANCODE_SLASH,
    /* 0x36 "ShiftRight"     */ SDL_SCANCODE_RSHIFT,
    /* 0x37 "NumpadMultiply" */ SDL_SCANCODE_KP_MULTIPLY,
    /* 0x38 "AltLeft"        */ SDL_SCANCODE_LALT,
    /* 0x39 "Space"          */ SDL_SCANCODE_SPACE,
    /* 0x3A "CapsLock"       */ SDL_SCANCODE_CAPSLOCK,
    /* 0x3B "F1"             */ SDL_SCANCODE_F1,
    /* 0x3C "F2"             */ SDL_SCANCODE_F2,
    /* 0x3D "F3"             */ SDL_SCANCODE_F3,
    /* 0x3E "F4"             */ SDL_SCANCODE_F4,
    /* 0x3F "F5"             */ SDL_SCANCODE_F5,
    /* 0x40 "F6"             */ SDL_SCANCODE_F6,
    /* 0x41 "F7"             */ SDL_SCANCODE_F7,
    /* 0x42 "F8"             */ SDL_SCANCODE_F8,
    /* 0x43 "F9"             */ SDL_SCANCODE_F9,
    /* 0x44 "F10"            */ SDL_SCANCODE_F10,
    /* 0x45 "Pause"          */ SDL_SCANCODE_PAUSE,
    /* 0x46 "ScrollLock"     */ SDL_SCANCODE_SCROLLLOCK,
    /* 0x47 "Numpad7"        */ SDL_SCANCODE_KP_7,
    /* 0x48 "Numpad8"        */ SDL_SCANCODE_KP_8,
    /* 0x49 "Numpad9"        */ SDL_SCANCODE_KP_9,
    /* 0x4A "NumpadSubtract" */ SDL_SCANCODE_KP_MINUS,
    /* 0x4B "Numpad4"        */ SDL_SCANCODE_KP_4,
    /* 0x4C "Numpad5"        */ SDL_SCANCODE_KP_5,
    /* 0x4D "Numpad6"        */ SDL_SCANCODE_KP_6,
    /* 0x4E "NumpadAdd"      */ SDL_SCANCODE_KP_PLUS,
    /* 0x4F "Numpad1"        */ SDL_SCANCODE_KP_1,
    /* 0x50 "Numpad2"        */ SDL_SCANCODE_KP_2,
    /* 0x51 "Numpad3"        */ SDL_SCANCODE_KP_3,
    /* 0x52 "Numpad0"        */ SDL_SCANCODE_KP_0,
    /* 0x53 "NumpadDecimal"  */ SDL_SCANCODE_KP_PERIOD,
    /* 0x54 "PrintScreen"    */ SDL_SCANCODE_PRINTSCREEN,
    /* 0x55                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x56 "IntlBackslash"  */ SDL_SCANCODE_NONUSBACKSLASH,
    /* 0x57 "F11"            */ SDL_SCANCODE_F11,
    /* 0x58 "F12"            */ SDL_SCANCODE_F12,
    /* 0x59 "NumpadEqual"    */ SDL_SCANCODE_KP_EQUALS,
    /* 0x5A                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x5B                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x5C                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x5D                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x5E                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x5F                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x60                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x61                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x62                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x63                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x64 "F13"            */ SDL_SCANCODE_F13,
    /* 0x65 "F14"            */ SDL_SCANCODE_F14,
    /* 0x66 "F15"            */ SDL_SCANCODE_F15,
    /* 0x67 "F16"            */ SDL_SCANCODE_F16,
    /* 0x68 "F17"            */ SDL_SCANCODE_F17,
    /* 0x69 "F18"            */ SDL_SCANCODE_F18,
    /* 0x6A "F19"            */ SDL_SCANCODE_F19,
    /* 0x6B "F20"            */ SDL_SCANCODE_F20,
    /* 0x6C "F21"            */ SDL_SCANCODE_F21,
    /* 0x6D "F22"            */ SDL_SCANCODE_F22,
    /* 0x6E "F23"            */ SDL_SCANCODE_F23,
    /* 0x6F                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x70 "KanaMode"       */ SDL_SCANCODE_INTERNATIONAL2,
    /* 0x71 "Lang2"          */ SDL_SCANCODE_LANG2,
    /* 0x72 "Lang1"          */ SDL_SCANCODE_LANG1,
    /* 0x73 "IntlRo"         */ SDL_SCANCODE_INTERNATIONAL1,
    /* 0x74                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x75                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x76 "F24"            */ SDL_SCANCODE_F24,
    /* 0x77                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x78                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x79 "Convert"        */ SDL_SCANCODE_INTERNATIONAL4,
    /* 0x7A                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x7B "NonConvert"     */ SDL_SCANCODE_INTERNATIONAL5,
    /* 0x7C                  */ SDL_SCANCODE_UNKNOWN,
    /* 0x7D "IntlYen"        */ SDL_SCANCODE_INTERNATIONAL3,
    /* 0x7E "NumpadComma"    */ SDL_SCANCODE_KP_COMMA
};

static SDL_Scancode Emscripten_MapScanCode(const char *code)
{
    const DOM_PK_CODE_TYPE pk_code = emscripten_compute_dom_pk_code(code);
    if (pk_code < SDL_arraysize(emscripten_scancode_table)) {
        return emscripten_scancode_table[pk_code];
    }

    switch (pk_code) {
    case DOM_PK_PASTE:
        return SDL_SCANCODE_PASTE;
    case DOM_PK_MEDIA_TRACK_PREVIOUS:
        return SDL_SCANCODE_AUDIOPREV;
    case DOM_PK_CUT:
        return SDL_SCANCODE_CUT;
    case DOM_PK_COPY:
        return SDL_SCANCODE_COPY;
    case DOM_PK_MEDIA_TRACK_NEXT:
        return SDL_SCANCODE_AUDIONEXT;
    case DOM_PK_NUMPAD_ENTER:
        return SDL_SCANCODE_KP_ENTER;
    case DOM_PK_CONTROL_RIGHT:
        return SDL_SCANCODE_RCTRL;
    case DOM_PK_AUDIO_VOLUME_MUTE:
        return SDL_SCANCODE_AUDIOMUTE;
    case DOM_PK_LAUNCH_APP_2:
        return SDL_SCANCODE_CALCULATOR;
    case DOM_PK_MEDIA_PLAY_PAUSE:
        return SDL_SCANCODE_AUDIOPLAY;
    case DOM_PK_MEDIA_STOP:
        return SDL_SCANCODE_AUDIOSTOP;
    case DOM_PK_EJECT:
        return SDL_SCANCODE_EJECT;
    case DOM_PK_AUDIO_VOLUME_DOWN:
        return SDL_SCANCODE_VOLUMEDOWN;
    case DOM_PK_AUDIO_VOLUME_UP:
        return SDL_SCANCODE_VOLUMEUP;
    case DOM_PK_BROWSER_HOME:
        return SDL_SCANCODE_AC_HOME;
    case DOM_PK_NUMPAD_DIVIDE:
        return SDL_SCANCODE_KP_DIVIDE;
    case DOM_PK_ALT_RIGHT:
        return SDL_SCANCODE_RALT;
    case DOM_PK_HELP:
        return SDL_SCANCODE_HELP;
    case DOM_PK_NUM_LOCK:
        return SDL_SCANCODE_NUMLOCKCLEAR;
    case DOM_PK_HOME:
        return SDL_SCANCODE_HOME;
    case DOM_PK_ARROW_UP:
        return SDL_SCANCODE_UP;
    case DOM_PK_PAGE_UP:
        return SDL_SCANCODE_PAGEUP;
    case DOM_PK_ARROW_LEFT:
        return SDL_SCANCODE_LEFT;
    case DOM_PK_ARROW_RIGHT:
        return SDL_SCANCODE_RIGHT;
    case DOM_PK_END:
        return SDL_SCANCODE_END;
    case DOM_PK_ARROW_DOWN:
        return SDL_SCANCODE_DOWN;
    case DOM_PK_PAGE_DOWN:
        return SDL_SCANCODE_PAGEDOWN;
    case DOM_PK_INSERT:
        return SDL_SCANCODE_INSERT;
    case DOM_PK_DELETE:
        return SDL_SCANCODE_DELETE;
    case DOM_PK_META_LEFT:
        return SDL_SCANCODE_LGUI;
    case DOM_PK_META_RIGHT:
        return SDL_SCANCODE_RGUI;
    case DOM_PK_CONTEXT_MENU:
        return SDL_SCANCODE_APPLICATION;
    case DOM_PK_POWER:
        return SDL_SCANCODE_POWER;
    case DOM_PK_BROWSER_SEARCH:
        return SDL_SCANCODE_AC_SEARCH;
    case DOM_PK_BROWSER_FAVORITES:
        return SDL_SCANCODE_AC_BOOKMARKS;
    case DOM_PK_BROWSER_REFRESH:
        return SDL_SCANCODE_AC_REFRESH;
    case DOM_PK_BROWSER_STOP:
        return SDL_SCANCODE_AC_STOP;
    case DOM_PK_BROWSER_FORWARD:
        return SDL_SCANCODE_AC_FORWARD;
    case DOM_PK_BROWSER_BACK:
        return SDL_SCANCODE_AC_BACK;
    case DOM_PK_LAUNCH_APP_1:
        return SDL_SCANCODE_COMPUTER;
    case DOM_PK_LAUNCH_MAIL:
        return SDL_SCANCODE_MAIL;
    case DOM_PK_MEDIA_SELECT:
        return SDL_SCANCODE_MEDIASELECT;
    }

    return SDL_SCANCODE_UNKNOWN;
}

static SDL_Keycode Emscripten_MapKeyCode(const EmscriptenKeyboardEvent *keyEvent)
{
    SDL_Keycode keycode = SDLK_UNKNOWN;
    if (keyEvent->keyCode < SDL_arraysize(emscripten_keycode_table)) {
        keycode = emscripten_keycode_table[keyEvent->keyCode];
        if (keycode != SDLK_UNKNOWN) {
            if (keyEvent->location == DOM_KEY_LOCATION_RIGHT) {
                switch (keycode) {
                case SDLK_LSHIFT:
                    keycode = SDLK_RSHIFT;
                    break;
                case SDLK_LCTRL:
                    keycode = SDLK_RCTRL;
                    break;
                case SDLK_LALT:
                    keycode = SDLK_RALT;
                    break;
                case SDLK_LGUI:
                    keycode = SDLK_RGUI;
                    break;
                }
            } else if (keyEvent->location == DOM_KEY_LOCATION_NUMPAD) {
                switch (keycode) {
                case SDLK_0:
                case SDLK_INSERT:
                    keycode = SDLK_KP_0;
                    break;
                case SDLK_1:
                case SDLK_END:
                    keycode = SDLK_KP_1;
                    break;
                case SDLK_2:
                case SDLK_DOWN:
                    keycode = SDLK_KP_2;
                    break;
                case SDLK_3:
                case SDLK_PAGEDOWN:
                    keycode = SDLK_KP_3;
                    break;
                case SDLK_4:
                case SDLK_LEFT:
                    keycode = SDLK_KP_4;
                    break;
                case SDLK_5:
                    keycode = SDLK_KP_5;
                    break;
                case SDLK_6:
                case SDLK_RIGHT:
                    keycode = SDLK_KP_6;
                    break;
                case SDLK_7:
                case SDLK_HOME:
                    keycode = SDLK_KP_7;
                    break;
                case SDLK_8:
                case SDLK_UP:
                    keycode = SDLK_KP_8;
                    break;
                case SDLK_9:
                case SDLK_PAGEUP:
                    keycode = SDLK_KP_9;
                    break;
                case SDLK_RETURN:
                    keycode = SDLK_KP_ENTER;
                    break;
                case SDLK_DELETE:
                    keycode = SDLK_KP_PERIOD;
                    break;
                }
            }
        }
    }

    return keycode;
}

/* "borrowed" from SDL_windowsevents.c */
static int Emscripten_ConvertUTF32toUTF8(Uint32 codepoint, char *text)
{
    if (codepoint <= 0x7F) {
        text[0] = (char)codepoint;
        text[1] = '\0';
    } else if (codepoint <= 0x7FF) {
        text[0] = 0xC0 | (char)((codepoint >> 6) & 0x1F);
        text[1] = 0x80 | (char)(codepoint & 0x3F);
        text[2] = '\0';
    } else if (codepoint <= 0xFFFF) {
        text[0] = 0xE0 | (char)((codepoint >> 12) & 0x0F);
        text[1] = 0x80 | (char)((codepoint >> 6) & 0x3F);
        text[2] = 0x80 | (char)(codepoint & 0x3F);
        text[3] = '\0';
    } else if (codepoint <= 0x10FFFF) {
        text[0] = 0xF0 | (char)((codepoint >> 18) & 0x0F);
        text[1] = 0x80 | (char)((codepoint >> 12) & 0x3F);
        text[2] = 0x80 | (char)((codepoint >> 6) & 0x3F);
        text[3] = 0x80 | (char)(codepoint & 0x3F);
        text[4] = '\0';
    } else {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}

static EM_BOOL Emscripten_HandlePointerLockChange(int eventType, const EmscriptenPointerlockChangeEvent *changeEvent, void *userData)
{
    SDL_WindowData *window_data = (SDL_WindowData *)userData;
    /* keep track of lock losses, so we can regrab if/when appropriate. */
    window_data->has_pointer_lock = changeEvent->isActive;
    return 0;
}

static EM_BOOL Emscripten_HandleMouseMove(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData)
{
    SDL_WindowData *window_data = userData;
    const int isPointerLocked = window_data->has_pointer_lock;
    int mx, my;
    static double residualx = 0, residualy = 0;

    /* rescale (in case canvas is being scaled)*/
    double client_w, client_h, xscale, yscale;
    emscripten_get_element_css_size(window_data->canvas_id, &client_w, &client_h);
    xscale = window_data->window->w / client_w;
    yscale = window_data->window->h / client_h;

    if (isPointerLocked) {
        residualx += mouseEvent->movementX * xscale;
        residualy += mouseEvent->movementY * yscale;
        /* Let slow sub-pixel motion accumulate. Don't lose it. */
        mx = residualx;
        residualx -= mx;
        my = residualy;
        residualy -= my;
    } else {
        mx = mouseEvent->targetX * xscale;
        my = mouseEvent->targetY * yscale;
    }

    SDL_SendMouseMotion(window_data->window, 0, isPointerLocked, mx, my);
    return 0;
}

static EM_BOOL Emscripten_HandleMouseButton(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData)
{
    SDL_WindowData *window_data = userData;
    Uint8 sdl_button;
    Uint8 sdl_button_state;
    SDL_EventType sdl_event_type;
    double css_w, css_h;
    SDL_bool prevent_default = SDL_FALSE; /* needed for iframe implementation in Chrome-based browsers. */

    switch (mouseEvent->button) {
    case 0:
        sdl_button = SDL_BUTTON_LEFT;
        break;
    case 1:
        sdl_button = SDL_BUTTON_MIDDLE;
        break;
    case 2:
        sdl_button = SDL_BUTTON_RIGHT;
        break;
    default:
        return 0;
    }

    if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN) {
        if (SDL_GetMouse()->relative_mode && !window_data->has_pointer_lock) {
            emscripten_request_pointerlock(window_data->canvas_id, 0); /* try to regrab lost pointer lock. */
        }
        sdl_button_state = SDL_PRESSED;
        sdl_event_type = SDL_MOUSEBUTTONDOWN;
    } else {
        sdl_button_state = SDL_RELEASED;
        sdl_event_type = SDL_MOUSEBUTTONUP;
        prevent_default = SDL_GetEventState(sdl_event_type) == SDL_ENABLE;
    }
    SDL_SendMouseButton(window_data->window, 0, sdl_button_state, sdl_button);

    /* Do not consume the event if the mouse is outside of the canvas. */
    emscripten_get_element_css_size(window_data->canvas_id, &css_w, &css_h);
    if (mouseEvent->targetX < 0 || mouseEvent->targetX >= css_w ||
        mouseEvent->targetY < 0 || mouseEvent->targetY >= css_h) {
        return 0;
    }

    return prevent_default;
}

static EM_BOOL Emscripten_HandleMouseFocus(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData)
{
    SDL_WindowData *window_data = userData;

    int mx = mouseEvent->targetX, my = mouseEvent->targetY;
    const int isPointerLocked = window_data->has_pointer_lock;

    if (!isPointerLocked) {
        /* rescale (in case canvas is being scaled)*/
        double client_w, client_h;
        emscripten_get_element_css_size(window_data->canvas_id, &client_w, &client_h);

        mx = mx * (window_data->window->w / client_w);
        my = my * (window_data->window->h / client_h);
        SDL_SendMouseMotion(window_data->window, 0, isPointerLocked, mx, my);
    }

    SDL_SetMouseFocus(eventType == EMSCRIPTEN_EVENT_MOUSEENTER ? window_data->window : NULL);
    return SDL_GetEventState(SDL_WINDOWEVENT) == SDL_ENABLE;
}

static EM_BOOL Emscripten_HandleWheel(int eventType, const EmscriptenWheelEvent *wheelEvent, void *userData)
{
    SDL_WindowData *window_data = userData;

    float deltaY = wheelEvent->deltaY;

    switch (wheelEvent->deltaMode) {
    case DOM_DELTA_PIXEL:
        deltaY /= 100; /* 100 pixels make up a step */
        break;
    case DOM_DELTA_LINE:
        deltaY /= 3; /* 3 lines make up a step */
        break;
    case DOM_DELTA_PAGE:
        deltaY *= 80; /* A page makes up 80 steps */
        break;
    }

    SDL_SendMouseWheel(window_data->window, 0, (float)wheelEvent->deltaX, -deltaY, SDL_MOUSEWHEEL_NORMAL);
    return SDL_GetEventState(SDL_MOUSEWHEEL) == SDL_ENABLE;
}

static EM_BOOL Emscripten_HandleFocus(int eventType, const EmscriptenFocusEvent *wheelEvent, void *userData)
{
    SDL_WindowData *window_data = userData;
    /* If the user switches away while keys are pressed (such as
     * via Alt+Tab), key release events won't be received. */
    if (eventType == EMSCRIPTEN_EVENT_BLUR) {
        SDL_ResetKeyboard();
    }

    SDL_SendWindowEvent(window_data->window, eventType == EMSCRIPTEN_EVENT_FOCUS ? SDL_WINDOWEVENT_FOCUS_GAINED : SDL_WINDOWEVENT_FOCUS_LOST, 0, 0);
    return SDL_GetEventState(SDL_WINDOWEVENT) == SDL_ENABLE;
}

static EM_BOOL Emscripten_HandleTouch(int eventType, const EmscriptenTouchEvent *touchEvent, void *userData)
{
    SDL_WindowData *window_data = (SDL_WindowData *)userData;
    int i;
    double client_w, client_h;
    int preventDefault = 0;

    SDL_TouchID deviceId = 1;
    if (SDL_AddTouch(deviceId, SDL_TOUCH_DEVICE_DIRECT, "") < 0) {
        return 0;
    }

    emscripten_get_element_css_size(window_data->canvas_id, &client_w, &client_h);

    for (i = 0; i < touchEvent->numTouches; i++) {
        SDL_FingerID id;
        float x, y;

        if (!touchEvent->touches[i].isChanged) {
            continue;
        }

        id = touchEvent->touches[i].identifier;
        x = touchEvent->touches[i].targetX / client_w;
        y = touchEvent->touches[i].targetY / client_h;

        if (eventType == EMSCRIPTEN_EVENT_TOUCHSTART) {
            SDL_SendTouch(deviceId, id, window_data->window, SDL_TRUE, x, y, 1.0f);

            /* disable browser scrolling/pinch-to-zoom if app handles touch events */
            if (!preventDefault && SDL_GetEventState(SDL_FINGERDOWN) == SDL_ENABLE) {
                preventDefault = 1;
            }
        } else if (eventType == EMSCRIPTEN_EVENT_TOUCHMOVE) {
            SDL_SendTouchMotion(deviceId, id, window_data->window, x, y, 1.0f);
        } else {
            SDL_SendTouch(deviceId, id, window_data->window, SDL_FALSE, x, y, 1.0f);

            /* block browser's simulated mousedown/mouseup on touchscreen devices */
            preventDefault = 1;
        }
    }

    return preventDefault;
}

static EM_BOOL Emscripten_HandleKey(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData)
{
    const SDL_Keycode keycode = Emscripten_MapKeyCode(keyEvent);
    SDL_Scancode scancode = Emscripten_MapScanCode(keyEvent->code);
    SDL_bool prevent_default = SDL_TRUE;
    SDL_bool is_nav_key = SDL_FALSE;

    if (scancode == SDL_SCANCODE_UNKNOWN) {
        /* KaiOS Left Soft Key and Right Soft Key, they act as OK/Next/Menu and Cancel/Back/Clear */
        if (SDL_strncmp(keyEvent->key, "SoftLeft", 9) == 0) {
            scancode = SDL_SCANCODE_AC_FORWARD;
        } else if (SDL_strncmp(keyEvent->key, "SoftRight", 10) == 0) {
            scancode = SDL_SCANCODE_AC_BACK;
        }
    }

    if (scancode != SDL_SCANCODE_UNKNOWN) {
        SDL_SendKeyboardKeyAndKeycode(eventType == EMSCRIPTEN_EVENT_KEYDOWN ? SDL_PRESSED : SDL_RELEASED, scancode, keycode);
    }

    /* if TEXTINPUT events are enabled we can't prevent keydown or we won't get keypress
     * we need to ALWAYS prevent backspace and tab otherwise chrome takes action and does bad navigation UX
     */
    if ((scancode == SDL_SCANCODE_BACKSPACE) ||
        (scancode == SDL_SCANCODE_TAB) ||
        (scancode == SDL_SCANCODE_LEFT) ||
        (scancode == SDL_SCANCODE_UP) ||
        (scancode == SDL_SCANCODE_RIGHT) ||
        (scancode == SDL_SCANCODE_DOWN) ||
        ((scancode >= SDL_SCANCODE_F1) && (scancode <= SDL_SCANCODE_F15)) ||
        keyEvent->ctrlKey) {
        is_nav_key = SDL_TRUE;
    }

    if ((eventType == EMSCRIPTEN_EVENT_KEYDOWN) && (SDL_GetEventState(SDL_TEXTINPUT) == SDL_ENABLE) && !is_nav_key) {
        prevent_default = SDL_FALSE;
    }

    return prevent_default;
}

static EM_BOOL Emscripten_HandleKeyPress(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData)
{
    char text[5];
    if (Emscripten_ConvertUTF32toUTF8(keyEvent->charCode, text)) {
        SDL_SendKeyboardText(text);
    }
    return SDL_GetEventState(SDL_TEXTINPUT) == SDL_ENABLE;
}

static EM_BOOL Emscripten_HandleFullscreenChange(int eventType, const EmscriptenFullscreenChangeEvent *fullscreenChangeEvent, void *userData)
{
    SDL_WindowData *window_data = userData;
    SDL_VideoDisplay *display;

    if (fullscreenChangeEvent->isFullscreen) {
        window_data->window->flags |= window_data->requested_fullscreen_mode;

        window_data->requested_fullscreen_mode = 0;
    } else {
        window_data->window->flags &= ~FULLSCREEN_MASK;

        /* reset fullscreen window if the browser left fullscreen */
        display = SDL_GetDisplayForWindow(window_data->window);

        if (display->fullscreen_window == window_data->window) {
            display->fullscreen_window = NULL;
        }
    }

    return 0;
}

static EM_BOOL Emscripten_HandleResize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
{
    SDL_WindowData *window_data = userData;
    SDL_bool force = SDL_FALSE;

    /* update pixel ratio */
    if (window_data->window->flags & SDL_WINDOW_ALLOW_HIGHDPI) {
        if (window_data->pixel_ratio != emscripten_get_device_pixel_ratio()) {
            window_data->pixel_ratio = emscripten_get_device_pixel_ratio();
            force = SDL_TRUE;
        }
    }

    if (!(window_data->window->flags & FULLSCREEN_MASK)) {
        /* this will only work if the canvas size is set through css */
        if (window_data->window->flags & SDL_WINDOW_RESIZABLE) {
            double w = window_data->window->w;
            double h = window_data->window->h;

            if (window_data->external_size) {
                emscripten_get_element_css_size(window_data->canvas_id, &w, &h);
            }

            emscripten_set_canvas_element_size(window_data->canvas_id, w * window_data->pixel_ratio, h * window_data->pixel_ratio);

            /* set_canvas_size unsets this */
            if (!window_data->external_size && window_data->pixel_ratio != 1.0f) {
                emscripten_set_element_css_size(window_data->canvas_id, w, h);
            }

            if (force) {
                /* force the event to trigger, so pixel ratio changes can be handled */
                window_data->window->w = 0;
                window_data->window->h = 0;
            }

            SDL_SendWindowEvent(window_data->window, SDL_WINDOWEVENT_RESIZED, w, h);
        }
    }

    return 0;
}

EM_BOOL
Emscripten_HandleCanvasResize(int eventType, const void *reserved, void *userData)
{
    /*this is used during fullscreen changes*/
    SDL_WindowData *window_data = userData;

    if (window_data->fullscreen_resize) {
        double css_w, css_h;
        emscripten_get_element_css_size(window_data->canvas_id, &css_w, &css_h);
        SDL_SendWindowEvent(window_data->window, SDL_WINDOWEVENT_RESIZED, css_w, css_h);
    }

    return 0;
}

static EM_BOOL Emscripten_HandleVisibilityChange(int eventType, const EmscriptenVisibilityChangeEvent *visEvent, void *userData)
{
    SDL_WindowData *window_data = userData;
    SDL_SendWindowEvent(window_data->window, visEvent->hidden ? SDL_WINDOWEVENT_HIDDEN : SDL_WINDOWEVENT_SHOWN, 0, 0);
    return 0;
}

static const char *Emscripten_HandleBeforeUnload(int eventType, const void *reserved, void *userData)
{
    /* This event will need to be handled synchronously, e.g. using
       SDL_AddEventWatch, as the page is being closed *now*. */
    /* No need to send a SDL_QUIT, the app won't get control again. */
    SDL_SendAppEvent(SDL_APP_TERMINATING);
    return ""; /* don't trigger confirmation dialog */
}

void Emscripten_RegisterEventHandlers(SDL_WindowData *data)
{
    const char *keyElement;

    /* There is only one window and that window is the canvas */
    emscripten_set_mousemove_callback(data->canvas_id, data, 0, Emscripten_HandleMouseMove);

    emscripten_set_mousedown_callback(data->canvas_id, data, 0, Emscripten_HandleMouseButton);
    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, data, 0, Emscripten_HandleMouseButton);

    emscripten_set_mouseenter_callback(data->canvas_id, data, 0, Emscripten_HandleMouseFocus);
    emscripten_set_mouseleave_callback(data->canvas_id, data, 0, Emscripten_HandleMouseFocus);

    emscripten_set_wheel_callback(data->canvas_id, data, 0, Emscripten_HandleWheel);

    emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, data, 0, Emscripten_HandleFocus);
    emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, data, 0, Emscripten_HandleFocus);

    emscripten_set_touchstart_callback(data->canvas_id, data, 0, Emscripten_HandleTouch);
    emscripten_set_touchend_callback(data->canvas_id, data, 0, Emscripten_HandleTouch);
    emscripten_set_touchmove_callback(data->canvas_id, data, 0, Emscripten_HandleTouch);
    emscripten_set_touchcancel_callback(data->canvas_id, data, 0, Emscripten_HandleTouch);

    emscripten_set_pointerlockchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, data, 0, Emscripten_HandlePointerLockChange);

    /* Keyboard events are awkward */
    keyElement = SDL_GetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT);
    if (keyElement == NULL) {
        keyElement = EMSCRIPTEN_EVENT_TARGET_WINDOW;
    }

    emscripten_set_keydown_callback(keyElement, data, 0, Emscripten_HandleKey);
    emscripten_set_keyup_callback(keyElement, data, 0, Emscripten_HandleKey);
    emscripten_set_keypress_callback(keyElement, data, 0, Emscripten_HandleKeyPress);

    emscripten_set_fullscreenchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, data, 0, Emscripten_HandleFullscreenChange);

    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, data, 0, Emscripten_HandleResize);

    emscripten_set_visibilitychange_callback(data, 0, Emscripten_HandleVisibilityChange);

    emscripten_set_beforeunload_callback(data, Emscripten_HandleBeforeUnload);
}

void Emscripten_UnregisterEventHandlers(SDL_WindowData *data)
{
    const char *target;

    /* only works due to having one window */
    emscripten_set_mousemove_callback(data->canvas_id, NULL, 0, NULL);

    emscripten_set_mousedown_callback(data->canvas_id, NULL, 0, NULL);
    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, 0, NULL);

    emscripten_set_mouseenter_callback(data->canvas_id, NULL, 0, NULL);
    emscripten_set_mouseleave_callback(data->canvas_id, NULL, 0, NULL);

    emscripten_set_wheel_callback(data->canvas_id, NULL, 0, NULL);

    emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 0, NULL);
    emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 0, NULL);

    emscripten_set_touchstart_callback(data->canvas_id, NULL, 0, NULL);
    emscripten_set_touchend_callback(data->canvas_id, NULL, 0, NULL);
    emscripten_set_touchmove_callback(data->canvas_id, NULL, 0, NULL);
    emscripten_set_touchcancel_callback(data->canvas_id, NULL, 0, NULL);

    emscripten_set_pointerlockchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, 0, NULL);

    target = SDL_GetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT);
    if (target == NULL) {
        target = EMSCRIPTEN_EVENT_TARGET_WINDOW;
    }

    emscripten_set_keydown_callback(target, NULL, 0, NULL);
    emscripten_set_keyup_callback(target, NULL, 0, NULL);
    emscripten_set_keypress_callback(target, NULL, 0, NULL);

    emscripten_set_fullscreenchange_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, 0, NULL);

    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 0, NULL);

    emscripten_set_visibilitychange_callback(NULL, 0, NULL);

    emscripten_set_beforeunload_callback(NULL, NULL);
}

#endif /* SDL_VIDEO_DRIVER_EMSCRIPTEN */

/* vi: set ts=4 sw=4 expandtab: */
