/*
 * i_sound.c
 *
 * Sound system support code
 *
 * Copyright (C) 2023 Chin Yik Ming
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "i_sound.h"

/* Sound */
/* ----- */

void
I_InitSound()
{
    int request_type = INIT_SOUND;
    
    register int a0 asm("a0") = request_type;
    register int a7 asm("a7") = 0xBABE;

    asm volatile("scall" : "+r"(a0) : "r"(a7));
}

void
I_UpdateSound(void)
{
}

void
I_SubmitSound(void)
{
}

void
I_ShutdownSound(void)
{
    int request_type = SHUTDOWN_SOUND;
    
    register int a0 asm("a0") = request_type;
    register int a7 asm("a7") = 0xBABE;

    asm volatile("scall" : "+r"(a0) : "r"(a7));
}

void I_SetChannels(void)
{
}

int
I_GetSfxLumpNum(sfxinfo_t* sfxinfo)
{
	return 0;
}

void
I_StartSound
( void *data,
  int volume)
{
    int request_type = PLAY_SFX;
    
    register int a0 asm("a0") = request_type;
    register int a1 asm("a1") = (uintptr_t) data;
    register int a2 asm("a2") = volume;
    register int a7 asm("a7") = 0xD00D;

    asm volatile("scall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a7));
}

void
I_StopSound(int handle)
{
}

int
I_SoundIsPlaying(int handle)
{
    return 0;
}

void
I_UpdateSoundParams
( int handle,
  int vol,
  int sep,
  int pitch )
{
}


/* Music */
/* ----- */

void
I_InitMusic(void)
{
}

void
I_ShutdownMusic(void)
{
}

void
I_SetSfxVolume(int volume)
{
    /*
     * no need to trap into ecall here since 
     * each sfx is short and it will be played 
     * with latest sfxVolume via I_StartSound
     */
    snd_SfxVolume = volume;
}

void
I_SetMusicVolume(int volume)
{
    /*
     * need to trap into ecall to set music volume
     * because music will be played during a period
     * of time
     */
    snd_MusicVolume = volume;

    int request_type = SET_MUSIC_VOLUME;
    
    register int a0 asm("a0") = request_type;
    register int a1 asm("a1") = volume;
    register int a7 asm("a7") = 0xD00D;

    asm volatile("scall" : "+r"(a0) : "r"(a1), "r"(a7));
}

void
I_PauseSong(int handle)
{
    printf("pause\n");
}

void
I_ResumeSong(int handle)
{
}

int
I_RegisterSong(void *data)
{
    return 0;
}

void
I_PlaySong
( void *data,
  int looping,
  int volume )
{
    int request_type = PLAY_MUSIC;
    
    register int a0 asm("a0") = request_type;
    register int a1 asm("a1") = (uintptr_t) data;
    register int a2 asm("a2") = volume;
    register int a3 asm("a3") = looping;
    register int a7 asm("a7") = 0xD00D;

    asm volatile("scall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a7));
}

void
I_StopSong()
{
    int request_type = STOP_MUSIC;
    
    register int a0 asm("a0") = request_type;
    register int a7 asm("a7") = 0xD00D;

    asm volatile("scall" : "+r"(a0) : "r"(a7));
}

void
I_UnRegisterSong(int handle)
{
}
