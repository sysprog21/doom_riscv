/*
 * s_sound.c
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

#include "s_sound.h"
#include "i_sound.h"
#include "i_system.h"
#include "sounds.h"

#include "z_zone.h"
#include "w_wad.h"

/* Sound */
/* ----- */

static musicinfo_t *music_playing = NULL;

/* 
 * Maximum volume of music 
 * Internal default is max out of 0-15
 */
int snd_SfxVolume = 15;
int snd_MusicVolume = 15;

/* Must exist for setting by code in M_misc */
int numChannels;

/* sonud data for calling sound related SDL system calls */
sfxinfo_t *sfx;
musicinfo_t *music;

void
S_Init
( int sfxVolume,
  int musicVolume )
{
    S_SetSfxVolume(sfxVolume);
    S_SetMusicVolume(musicVolume);

    I_InitSound();
}

void S_Start(void)
{
  int music_id;

  if (gamemode == commercial)
    music_id = mus_runnin + gamemap - 1;
  else
  {
    int spmus[]=
    {
      // Song - Who? - Where?

      mus_e3m4, // American     e4m1
      mus_e3m2, // Romero       e4m2
      mus_e3m3, // Shawn        e4m3
      mus_e1m5, // American     e4m4
      mus_e2m7, // Tim  e4m5
      mus_e2m4, // Romero       e4m6
      mus_e2m6, // J.Anderson   e4m7 CHIRON.WAD
      mus_e2m5, // Shawn        e4m8
      mus_e1m9  // Tim          e4m9
    };

    if (gameepisode < 4)
      music_id = mus_e1m1 + (gameepisode-1)*9 + gamemap-1;
    else
      music_id = spmus[gamemap-1];
    }

    S_ChangeMusic(music_id, 1);
}

void
S_StartSound
( void* origin,
  int   sound_id )
{
    S_StartSoundAtVolume(origin, sound_id, snd_SfxVolume);
}

void
S_StartSoundAtVolume
( void* origin_p,
  int   sfx_id,
  int   volume )
{
  char buf[9];

  // check for bogus sound #
  if (sfx_id < 1 || sfx_id > NUMSFX)
    I_Error("Bad sfx #: %d", sfx_id);

  sfx = &S_sfx[sfx_id];

  if (sfx->link)
  {
    volume += sfx->volume;

    if (volume < 1)
      return;

    if (volume > snd_SfxVolume)
      volume = snd_SfxVolume;
  }

  /* increase the usefulness */
  if (sfx->usefulness++ < 0)
    sfx->usefulness = 1;

  sprintf(buf, "ds%s", sfx->name);
  sfx->lumpnum = W_GetNumForName(buf);
  sfx->data = W_CacheLumpNum(sfx->lumpnum, PU_SOUND);
  sfx->size = (lumpinfo + sfx->lumpnum)->size;

  I_StartSound(sfx, volume);
}

void
S_StopSound(void* origin)
{
}


void
S_PauseSound(void)
{
}

void
S_ResumeSound(void)
{
}


void
S_UpdateSounds(void* listener)
{
}


/* Music */
/* ----- */

void
S_StartMusic(int music_id)
{
    S_ChangeMusic(music_id, 0);
}

void
S_ChangeMusic
( int music_id,
  int looping )
{
    char buf[9];

    music = &S_music[music_id];

    if(music_playing == music)
        return;

    /* stop any music if any */
    S_StopMusic();

    sprintf(buf, "d_%s", music->name);
    music->lumpnum = W_GetNumForName(buf);
    music->data = W_CacheLumpNum(music->lumpnum, PU_MUSIC);
    music->size = (lumpinfo + music->lumpnum)->size;

    music_playing = music;

    I_PlaySong(music, looping, snd_MusicVolume);
}

void
S_StopMusic(void)
{
    if(!music_playing)
        return;

    I_StopSong();
    music_playing = NULL;
}


/* Volumes */
/* ------- */

/* Must exist for settings loading code ... */
void
S_SetMusicVolume(int volume)
{
    if(volume < 0 || volume > 127)
        I_Error("Attempt to set music volume at %d", volume);

    I_SetMusicVolume(volume);
}

void
S_SetSfxVolume(int volume)
{
    if(volume < 0 || volume > 127)
        I_Error("Attempt to set sfx volume at %d", volume);

    I_SetSfxVolume(volume);
}
