//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//


#include "doomtype.h"
#include "m_variable.h"


static default_t doom_defaults_list[] = {
    //
    // Mouse sensitivity.  This value is used to multiply input mouse
    // movement to control the effect of moving the mouse.
    //
    // The "normal" maximum value available for this through the
    // in-game options menu is 9. A value of 31 or greater will cause
    // the game to crash when entering the options menu.
    //
    CONFIG_VARIABLE_INT(mouse_sensitivity),

    //
    // Volume of sound effects, range 0-15.
    //
    CONFIG_VARIABLE_INT(sfx_volume),

    //
    // Volume of in-game music, range 0-15.
    //
    CONFIG_VARIABLE_INT(music_volume),

    //
    //
    // If non-zero, messages are displayed on the heads-up display
    // in the game ("picked up a clip", etc).  If zero, these messages
    // are not displayed.
    //
    CONFIG_VARIABLE_INT(show_messages),

    //
    // Keyboard key to turn right.
    //
    CONFIG_VARIABLE_KEY(key_right),

    //
    // Keyboard key to turn left.
    //
    CONFIG_VARIABLE_KEY(key_left),

    //
    // Keyboard key to move forward.
    //
    CONFIG_VARIABLE_KEY(key_up),

    //
    // Keyboard key to move backward.
    //
    CONFIG_VARIABLE_KEY(key_down),

    //
    // Keyboard key to strafe left.
    //
    CONFIG_VARIABLE_KEY(key_strafeleft),

    //
    // Keyboard key to strafe right.
    //
    CONFIG_VARIABLE_KEY(key_straferight),

    //
    // Keyboard key to fire the currently selected weapon.
    //
    CONFIG_VARIABLE_KEY(key_fire),

    //
    // Keyboard key to "use" an object, eg. a door or switch.
    //
    CONFIG_VARIABLE_KEY(key_use),

    //
    // Keyboard key to turn on strafing.  When held down, pressing the
    // key to turn left or right causes the player to strafe left or
    // right instead.
    //
    CONFIG_VARIABLE_KEY(key_strafe),

    //
    // Keyboard key to make the player run.
    //
    CONFIG_VARIABLE_KEY(key_speed),

    //
    // If non-zero, mouse input is enabled.  If zero, mouse input is
    // disabled.
    //
    CONFIG_VARIABLE_INT(use_mouse),

    //
    // Mouse button to fire the currently selected weapon.
    //
    CONFIG_VARIABLE_INT(mouseb_fire),

    //
    // Mouse button to turn on strafing.  When held down, the player
    // will strafe left and right instead of turning left and right.
    //
    CONFIG_VARIABLE_INT(mouseb_strafe),

    //
    // Mouse button to move forward.
    //
    CONFIG_VARIABLE_INT(mouseb_forward),

    //
    // Mouse button to turn on running.  When held down, the player
    // will run while moving.
    //
    CONFIG_VARIABLE_INT(mouseb_speed),

    //
    // If non-zero, joystick input is enabled.
    //
    CONFIG_VARIABLE_INT(use_joystick),

    //
    // Joystick virtual button that fires the current weapon.
    //
    CONFIG_VARIABLE_INT(joyb_fire),

    //
    // Joystick virtual button that makes the player strafe while
    // held down.
    //
    CONFIG_VARIABLE_INT(joyb_strafe),

    //
    // Joystick virtual button to "use" an object, eg. a door or switch.
    //
    CONFIG_VARIABLE_INT(joyb_use),

    //
    // Joystick virtual button that makes the player run while held
    // down.
    //
    // If this has a value of 20 or greater, the player will always run,
    // even if use_joystick is 0.
    //
    CONFIG_VARIABLE_INT(joyb_speed),

    //
    //
    // Screen size, range 3-11.
    //
    // A value of 11 gives a full-screen view with the status bar not
    // displayed.  A value of 10 gives a full-screen view with the
    // status bar displayed.
    //
    CONFIG_VARIABLE_INT(screenblocks),

    //
    //
    // Screen detail.  Zero gives normal "high detail" mode, while
    // a non-zero value gives "low detail" mode.
    //
    CONFIG_VARIABLE_INT(detaillevel),

    //
    // Number of sounds that will be played simultaneously.
    //
    CONFIG_VARIABLE_INT(snd_channels),

    //
    // Music output device.  A non-zero value gives MIDI sound output,
    // while a value of zero disables music.
    //
    CONFIG_VARIABLE_INT(snd_musicdevice),

    //
    // Sound effects device.  A value of zero disables in-game sound
    // effects, a value of 1 enables PC speaker sound effects, while
    // a value in the range 2-9 enables the "normal" digital sound
    // effects.
    //
    CONFIG_VARIABLE_INT(snd_sfxdevice),

    //
    // SoundBlaster I/O port. Unused.
    //
    CONFIG_VARIABLE_INT(snd_sbport),

    //
    // SoundBlaster IRQ.  Unused.
    //
    CONFIG_VARIABLE_INT(snd_sbirq),

    //
    // SoundBlaster DMA channel.  Unused.
    //
    CONFIG_VARIABLE_INT(snd_sbdma),

    //
    // Output port to use for OPL MIDI playback.  Unused.
    //
    CONFIG_VARIABLE_INT(snd_mport),

    //
    // Gamma correction level.  A value of zero disables gamma
    // correction, while a value in the range 1-4 gives increasing
    // levels of gamma correction.
    //
    CONFIG_VARIABLE_INT(usegamma),

    //
    // Multiplayer chat macro: message to send when alt+0 is pressed.
    //
    CONFIG_VARIABLE_STRING(chatmacro0),

    //
    // Multiplayer chat macro: message to send when alt+1 is pressed.
    //
    CONFIG_VARIABLE_STRING(chatmacro1),

    //
    // Multiplayer chat macro: message to send when alt+2 is pressed.
    //
    CONFIG_VARIABLE_STRING(chatmacro2),

    //
    // Multiplayer chat macro: message to send when alt+3 is pressed.
    //
    CONFIG_VARIABLE_STRING(chatmacro3),

    //
    // Multiplayer chat macro: message to send when alt+4 is pressed.
    //
    CONFIG_VARIABLE_STRING(chatmacro4),

    //
    // Multiplayer chat macro: message to send when alt+5 is pressed.
    //
    CONFIG_VARIABLE_STRING(chatmacro5),

    //
    // Multiplayer chat macro: message to send when alt+6 is pressed.
    //
    CONFIG_VARIABLE_STRING(chatmacro6),

    //
    // Multiplayer chat macro: message to send when alt+7 is pressed.
    //
    CONFIG_VARIABLE_STRING(chatmacro7),

    //
    // Multiplayer chat macro: message to send when alt+8 is pressed.
    //
    CONFIG_VARIABLE_STRING(chatmacro8),

    //
    // Multiplayer chat macro: message to send when alt+9 is pressed.
    //
    CONFIG_VARIABLE_STRING(chatmacro9),
};

default_collection_t doom_defaults = {
    doom_defaults_list,
    arrlen(doom_defaults_list),
    NULL,
};
