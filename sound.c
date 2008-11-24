
/*
 * Copyright 2001, 2002 David J. Hawkey Jr.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holder or the author not
 * be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission. The copyright holder
 * and the author make no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * THE COPYRIGHT HOLDER AND THE AUTHOR DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDER OR THE AUTHOR BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * sound.c
 *
 * D. J. Hawkey Jr. - 6/22/01 8/16/01 11/15/02
 */

#include "twm.h"

#ifdef SOUND_SUPPORT

#include <X11/Xmu/CharSet.h>
#include <string.h>
#include <stdlib.h>
#include "gram.h"
#include "parse.h"
#include "sound.h"
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef HAVE_RPLAY
#include <rplay.h>
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	256
#endif

#define AUDIO_DEV "/dev/dsp"
extern int parse_keyword();	/* in parse.c */
extern void twmrc_error_prefix();	/* in gram.y */

typedef struct sound_keyword
{
  char *name;
  int value;
} sound_keyword;

static sound_keyword sound_keywords[] =
{
  {"(vtwm start)",	S_START},
  {"(vtwm stop)",	S_STOP},
  {"(client map)",	S_CMAP},
  {"(client unmap)",	S_CUNMAP},
  {"(menu map)",	S_MMAP},
  {"(menu unmap)",	S_MUNMAP},
  {"(info unmap)",	S_IUNMAP},
  {"(autopan event)",	S_APAN},
  {"(bell event)",	S_BELL}
};

#define MAX_SOUNDKEYWORDS (sizeof(sound_keywords) / sizeof(sound_keyword))

#define SOUND_NONE 0
#define SOUND_ACTIVE 1
#define SOUND_RPLAY 2
#define SOUND_ESD 4
#define SOUND_OSS 8
typedef struct sound_entry
{
  int func;
#ifdef HAVE_RPLAY
  RPLAY *rp;
#endif
#ifdef HAVE_ESD
  int esd_sound_id;
#endif
  char *filename;
  int volume;
} sound_entry;

#ifdef HAVE_OSS
/* Stuff Related to reading .au and .wav files for use with OSS */
typedef struct
{
  uint32_t DataStart;
  uint32_t DataSize;
  uint32_t Format;
  uint32_t SampleRate;
  uint32_t Channels;
} AUHeader;

typedef struct
{
  uint32_t size;
  char Format[4];
} RIFFHeader;

typedef struct
{
  char ID[4];
  uint32_t size;
  uint16_t AudioFormat;
  uint16_t Channels;
  uint32_t SampleRate;
  uint32_t ByteRate;
  uint16_t BlockAlign;
  uint16_t BitsPerSample;
} WAVHeader;

typedef struct
{
  char ID[4];
  uint32_t size;
} WAVData;

#define SAMPLE_SIGNED 1

#endif

static sound_entry *sound_entries = NULL;
static int sound_count = 0;
static int sound_size = 0;

static int sound_fd = -1;
static int sound_vol = 63;	/* 1/4 attenuation */
static int sound_state = 0;
static char sound_host[MAXHOSTNAMELEN + 1] = "";

/* for qsort() */
static int
compare(const void *p, const void *q)
{
  sound_entry *pp = (sound_entry *) p, *qq = (sound_entry *) q;

  return (pp->func - qq->func);
}

static int
adjustVolume(int volume)
{
  float vol;

  if (volume > 100)
    volume = 100;

  /* volume for rplay is 1 to 255, not 1 to 100 */
  vol = (float) volume / 100.0;
  volume = vol * 255.0;

  return (volume);
}

void
SetSoundHost(char *host)
{
  strncpy(sound_host, host, MAXHOSTNAMELEN);
  sound_host[MAXHOSTNAMELEN] = '\0';
}

void
SetSoundVolume(int volume)
{
  if (volume < 0)
    volume = 0;
  sound_vol = adjustVolume(volume);
}

int
ToggleSounds()
{
  return ((sound_state ^= 1));
}

#ifdef HAVE_RPLAY

static int
RPlayOpenSound()
{
  if (sound_fd < 0)
  {
    if (sound_host[0] == '\0')
    {
      strncpy(sound_host, rplay_default_host(), MAXHOSTNAMELEN);
      sound_host[MAXHOSTNAMELEN] = '\0';
    }

    if ((sound_fd = rplay_open(sound_host)) >= 0)
      sound_state = SOUND_ACTIVE | SOUND_RPLAY;
  }

  return (sound_fd);
}

static void
RPlayCloseSound()
{
  int i;

  for (i = 0; i < sound_count; i++)
    rplay_destroy(sound_entries[i].rp);

  if (sound_entries != NULL)

    if (sound_fd >= 0)
      rplay_close(sound_fd);
}

static int
RPlayPlaySoundAdhoc(char *filename)
{
  RPLAY *rp;
  int i;
  if (sound_fd < 0 || (!(sound_state & SOUND_ACTIVE)))
    return (1);	/* pretend success */

  if ((rp = rplay_create(RPLAY_PLAY)) == NULL)
  {
    twmrc_error_prefix();
    fprintf(stderr, "unable to create sound \"%s\"\n", filename);
    return (0);
  }

  if ((i = rplay_set(rp, RPLAY_INSERT, 0, RPLAY_SOUND, filename,
		     RPLAY_VOLUME, sound_vol, NULL)) >= 0)
    rplay(sound_fd, rp);

  rplay_destroy(rp);
  if (i < 0)
  {
    twmrc_error_prefix();
    fprintf(stderr, "unable to set sound \"%s\"\n", filename);
    return (0);
  }

  return (1);
}

#endif /* HAVE_RPLAY */


#ifdef HAVE_ESD

#include <esd.h>

static int
ESDOpenSound()
{
  sound_state = SOUND_ACTIVE | SOUND_ESD;
  return(sound_fd);
}

static void
ESDCloseSound()
{

  esd_audio_close();
  close(sound_fd);
}

static int
ESDSendFileData(char *SoundFilePath)
{
  char *Tempstr = NULL;
  int id;

  if (sound_fd == -1)
  {
    if (strlen(sound_host) == 0)
      sound_fd = esd_open_sound(NULL);
    else
      sound_fd = esd_open_sound(NULL);
  }

  Tempstr = calloc(strlen(SoundFilePath)+6, sizeof(char));
  strcpy(Tempstr, "vtwm:");
  strcat(Tempstr, SoundFilePath);

  id = esd_file_cache(sound_fd, "vtwm", SoundFilePath);
  free(Tempstr);
  return(id);
}

static int
ESDPlaySound(int id, int volume)
{
  if (volume != -1)
    esd_set_default_sample_pan(sound_fd, id, volume, volume);
  esd_sample_play(sound_fd, id);
  return(1);
}

static int
ESDPlaySoundAdhoc(char *filename)
{
  /*if (volume !=-1) esd_set_default_sample_pan(sound_fd,id,volume,volume);*/
  esd_play_file("vtwm", filename,1);
  return(1);
}

#endif

#ifdef HAVE_OSS

#include <sys/soundcard.h>
#include <sys/ioctl.h>

typedef struct
{
  unsigned int Format;
  unsigned int Channels;
  unsigned int SampleRate;
  unsigned int SampleSize;
  unsigned int DataSize;
} TAudioInfo;

static TAudioInfo *
ReadWAV(int fd)
{
  RIFFHeader Riff;
  WAVHeader Wav;
  WAVData Data;
  TAudioInfo *AudioInfo;

  AudioInfo = (TAudioInfo *) calloc(1,sizeof(TAudioInfo));
  read(fd, &Riff, sizeof(RIFFHeader));
  read(fd, &Wav, sizeof(WAVHeader));
  read(fd, &Data, sizeof(WAVData));
  AudioInfo->Channels = Wav.Channels;
  AudioInfo->SampleRate = Wav.SampleRate;
  AudioInfo->DataSize = 0xFFFFFFFF;
  if (Wav.BitsPerSample == 16)
  {
    AudioInfo->SampleSize = 2;
    AudioInfo->Format = AFMT_S16_LE;
  }
  else
  {
    AudioInfo->SampleSize = 1;
    AudioInfo->Format = AFMT_U8;
  }
  return(AudioInfo);
}

static TAudioInfo *
ReadAU(int fd)
{
  AUHeader Header;
  TAudioInfo *AudioInfo;

  AudioInfo = (TAudioInfo *)calloc(1, sizeof(TAudioInfo));
  read(fd, &Header, sizeof(AUHeader));
  /* AU uses big endian */
  Header.DataSize = htonl(Header.DataSize);
  Header.DataStart = htonl(Header.DataStart);
  Header.Channels = htonl(Header.Channels);
  Header.SampleRate = htonl(Header.SampleRate);
  Header.Format = htonl(Header.Format);
  AudioInfo->Channels = Header.Channels;
  AudioInfo->SampleRate = Header.SampleRate;
  AudioInfo->DataSize = Header.DataSize;
  switch(Header.Format)
  {
    case 1:
      AudioInfo->Format = AFMT_MU_LAW;
      break;
    case 2:
      AudioInfo->Format = AFMT_S8;
      break;
    case 3:
      AudioInfo->Format = AFMT_S16_BE;
      break;
  }
  lseek(fd, Header.DataStart, SEEK_SET);
  return(AudioInfo);
}

static int
OSSOpenSound()
{
  if (access(AUDIO_DEV, W_OK) == 0)
  {
    sound_state = SOUND_ACTIVE | SOUND_OSS;
    /*
       this is a bit worrying, as sound_fd is now set to
       be channel 1, (stdout), but we cannot hold the sound
       device open under oss, as we will block other things
       from accessing it
     */
    return(1);
  }
  return(-1);
}

static void
OSSCloseSound()
{
}

static int
OSSPlaySound(char *filename, int volume)
{
  int fd, mixfd, soundfilefd, i;
  unsigned char *data = NULL;
  int Flags, val, oldvol, result;
  char FourCharacter[5];
  TAudioInfo *AudioInfo = NULL;
  int buflen = BUFSIZ;

  i = fork();
  if (i == 0)
  {
    fd = open(AUDIO_DEV,O_WRONLY);
    if (fd == -1)
      _exit(0);
    soundfilefd = open(filename, O_RDONLY);
    if (soundfilefd == -1)
      return(0);
    read(soundfilefd, FourCharacter, 4);
    FourCharacter[4] = '\0';
    if (strcmp(FourCharacter, ".snd") == 0)
      AudioInfo = ReadAU(soundfilefd);
    else if (strcmp(FourCharacter, "RIFF") == 0)
      AudioInfo = ReadWAV(soundfilefd);
    if (AudioInfo == NULL)
    {
      close(soundfilefd);
      _exit(0);
    }
    /* Tell the sound card that the sound about to be played is stereo. 0=mono 1=stereo */
    val = AudioInfo->Channels-1;
    if (val < 0)
      val = 0;
    if (ioctl(fd, SNDCTL_DSP_STEREO, &val) == -1)
    {
      perror("ioctl stereo");
      _exit(0);
    }
    /* Inform the sound card of the audio format */
    if (ioctl(fd, SNDCTL_DSP_SETFMT, &AudioInfo->Format) == -1)
    {
      perror("ioctl format");
      _exit(0);
    }

    /* Set the DSP playback rate, sampling rate of the raw PCM audio */
    if (ioctl(fd, SNDCTL_DSP_SPEED, &AudioInfo->SampleRate) == -1)
    {
      perror("ioctl sample rate");
      _exit(0);
    }
    mixfd = open(AUDIO_DEV, O_RDWR);
    if (mixfd > -1)
    {
      ioctl(mixfd, SOUND_MIXER_READ_PCM, &oldvol);
      result = sound_vol;
      val = (result) | (result <<8);
      ioctl(mixfd, SOUND_MIXER_WRITE_PCM, &val);
    }
    data = (char *)malloc(buflen);
    result = read(soundfilefd, data, buflen);
    val = 0;
    while ((result > 0) && (val < AudioInfo->DataSize))
    {
      write(fd, data, result);
      val += result;
      result = read(soundfilefd, data, buflen);
    }

    close(fd);
    if (mixfd > -1)
    {
      ioctl(mixfd, SOUND_MIXER_WRITE_PCM, &oldvol);
      close(mixfd);
    }

    close(soundfilefd);
    _exit(0);
  }

}

static int
OSSPlaySoundAdhoc(char *filename)
{
  return(OSSPlaySound(filename, sound_vol));
}

#endif /* HAVE_OSS */

int
OpenSound()
{
  int fd = -1;
#ifdef HAVE_RPLAY
  fd=RPlayOpenSound();
#endif
#ifdef HAVE_ESD
  if (fd == -1)
    fd = ESDOpenSound();
#endif
#ifdef HAVE_OSS
  if (fd == -1)
    fd = OSSOpenSound();
#endif
  if (fd > -1)
  {
    qsort((void *)sound_entries, (size_t)sound_count,
	  (size_t)sizeof(sound_entry), compare);
  }
  return(fd);
}

void
CloseSound()
{
#ifdef HAVE_RPLAY
  if (sound_state & SOUND_RPLAY)
    RPlayCloseSound();
#endif
#ifdef HAVE_ESD
  if (sound_state & SOUND_ESD)
    ESDCloseSound();
#endif
#ifdef HAVE_OSS
  if (sound_state & SOUND_OSS)
    OSSCloseSound();
#endif
  free((void *) sound_entries);
  sound_entries = NULL;
  sound_count = 0;
  sound_size = 0;
  sound_fd = -1;
  sound_vol = 63;
  sound_state = 0;
  sound_host[0] = '\0';
}

int
SetSound(char *function, char *filename, int volume)
{
  sound_entry *sptr;
  int i, func, subfunc;

  func = parse_keyword(function, &subfunc);
  if (func != FKEYWORD && func != FSKEYWORD)
  {
    XmuCopyISOLatin1Lowered(function, function);
    for (i = 0; i < MAX_SOUNDKEYWORDS; i++)
      if (strcmp(function, sound_keywords[i].name) == 0)
      {
	func = FKEYWORD;
	subfunc = sound_keywords[i].value;
	break;
      }
  }
  if (func == FKEYWORD || func == FSKEYWORD)
  {
    if (sound_count >= sound_size)
    {
      sound_size += 10;
      sptr = (sound_entry *) realloc((sound_entry *) sound_entries,
				     sound_size * sizeof(sound_entry));
      if (sptr == NULL)
      {
	twmrc_error_prefix();
	fprintf(stderr,
		"unable to allocate %d bytes for sound_entries\n",
		sound_size * sizeof(sound_entry));
	Done(0);
      }
      else
	sound_entries = sptr;
    }
    sptr = &sound_entries[sound_count];
    sptr->func = subfunc;
    sptr->filename = strdup(filename);
    if (volume < 0)
      sptr->volume = sound_vol;
    else
      sptr->volume = adjustVolume(volume);
#ifdef HAVE_RPLAY
    if ((sptr->rp = rplay_create(RPLAY_PLAY)) == NULL)
    {
      twmrc_error_prefix();
      fprintf(stderr, "unable to add to sound list\n");
      Done(0);
    }
    if (rplay_set(sptr->rp, RPLAY_INSERT, 0, RPLAY_SOUND, sptr->filename,
		  RPLAY_VOLUME, sptr->volume, NULL) < 0)
    {
      twmrc_error_prefix();
      fprintf(stderr, "unable to set \"%s\" in sound list\n", filename);
      Done(0);
    }
#endif
#ifdef HAVE_ESD
    sptr->esd_sound_id = ESDSendFileData(filename);
    if (sptr->esd_sound_id < 0)
    {
      twmrc_error_prefix();
      fprintf(stderr, "unable to set \"%s\" in sound list\n", filename);
      Done(0);
    }
#endif
    sound_count++;
    return (1);
  }

  twmrc_error_prefix();
  fprintf(stderr, "unknown function \"%s\" for sound_entry\n", function);
  return (0);
}

int
PlaySound(int function)
{
  register int i, low, mid, high;
  int result;

  if (!(sound_state & SOUND_ACTIVE))
    return (1);	/* pretend success */
  low = 0;
  high = sound_count - 1;
  while (low <= high)
  {
    mid = (low + high) / 2;
    i = sound_entries[mid].func - function;
    if (i < 0)
      low = mid + 1;
    else if (i > 0)
      high = mid - 1;
    else
    {
#ifdef HAVE_RPLAY
      if (sound_state & SOUND_RPLAY)
      {
	rplay(sound_fd, sound_entries[mid].rp);
	return(1);
      }
#endif
#ifdef HAVE_ESD
      if (sound_state & SOUND_ESD)
	return(ESDPlaySound(sound_entries[mid].esd_sound_id,
			    sound_entries[mid].volume));
#endif
#ifdef HAVE_OSS
      if (sound_state & SOUND_OSS)
	return(OSSPlaySound(sound_entries[mid].filename,
			    sound_entries[mid].volume));
#endif
      /* we found a file to play, but no player to send it to pretend success */
      return(1);
    }
  }
  return(0);
}



int
PlaySoundAdhoc(char *filename)
{
#ifdef HAVE_RPLAY
  if ((sound_state & SOUND_RPLAY) && RPlayPlaySoundAdhoc(filename))
    return(1);
#endif
#ifdef HAVE_ESD
  if ((sound_state & SOUND_ESD) && ESDPlaySoundAdhoc(filename))
    return(1);
#endif
#ifdef HAVE_OSS
  if ((sound_state & SOUND_OSS) && OSSPlaySoundAdhoc(filename))
    return(1);
#endif
  return(0);
}

#else	/* SOUND_SUPPORT */

/* stub function for gram.y */
int
SetSound(char *function, char *filename, int volume)
{
  return (1);
}

#endif	/* SOUND_SUPPORT */

/*
   Local Variables:
mode:c
c-file-style:"GNU"
c-file-offsets:((substatement-open 0)(brace-list-open 0)(c-hanging-comment-ender-p . nil)(c-hanging-comment-beginner-p . nil)(comment-start . "// ")(comment-end . "")(comment-column . 48))
End:
 */
/* vim: sw=2
 */
