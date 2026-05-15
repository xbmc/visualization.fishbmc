/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#ifndef FISCHE_H
#define FISCHE_H

#include <stdint.h>
#include <stdlib.h>

#ifdef WIN32
#define rand_r(_seed) (_seed == _seed ? rand() : rand())
#endif

/* audio sample formats */
typedef enum FISCHE_AUDIOFORMAT
{
  FISCHE_AUDIOFORMAT_U8,
  FISCHE_AUDIOFORMAT_S8,
  FISCHE_AUDIOFORMAT_U16,
  FISCHE_AUDIOFORMAT_S16,
  FISCHE_AUDIOFORMAT_U32,
  FISCHE_AUDIOFORMAT_S32,
  FISCHE_AUDIOFORMAT_FLOAT,
  FISCHE_AUDIOFORMAT_DOUBLE,
  _FISCHE__AUDIOFORMAT_LAST_
} FISCHE_AUDIOFORMAT;

/* pixel formats */
typedef enum FISCHE_PIXELFORMAT
{
  FISCHE_PIXELFORMAT_0xRRGGBBAA,
  FISCHE_PIXELFORMAT_0xAABBGGRR,
  FISCHE_PIXELFORMAT_0xAARRGGBB,
  FISCHE_PIXELFORMAT_0xBBGGRRAA,
  _FISCHE__PIXELFORMAT_LAST_
} FISCHE_PIXELFORMAT;

/* blur style */
typedef enum FISCHE_BLUR
{
  FISCHE_BLUR_SLICK,
  FISCHE_BLUR_FUZZY,
  _FISCHE__BLUR_LAST_
} FISCHE_BLUR;

/* line style */
typedef enum FISCHE_LINESTYLE
{
  FISCHE_LINESTYLE_THIN,
  FISCHE_LINESTYLE_THICK,
  FISCHE_LINESTYLE_ALPHA_SIMULATION,
  _FISCHE__LINESTYLE_LAST_
} FISCHE_LINESTYLE;

typedef struct FISCHE
{

  /* 16 <= width <= 2048
     * DEFAULT: 512
     * constant after fische_start() */
  uint16_t width;

  /* 16 <= height <= 2048
     * DEFAULT: 256
     * constant after fische_start() */
  uint16_t height;

  /* 1 <= used_cpus <= 8
     * DEFAULT: all available (autodetect)
     * constant after fische_start() */
  uint8_t used_cpus;

  /* true (!=0) or false (0)
     * DEFAULT: 0 */
  uint8_t nervous_mode;

  /* see below (audio format enum)
     * DEFAULT: FISCHE_AUDIOFORMAT_FLOAT
     * constant after fische_start() */
  FISCHE_AUDIOFORMAT audio_format;

  /* see below (pixel format enum)
     * DEFAULT: FISCHE_PIXELFORMAT_0xAABBGGRR
     * constant after fische_start() */
  FISCHE_PIXELFORMAT pixel_format;

  /* see below (blur mode enum)
     * DEFAULT: FISCHE_BLUR_SLICK
     * constant after fische_start() */
  FISCHE_BLUR blur_mode;

  /* see below (line style enum)
     * DEFAULT: FISCHE_LINESTYLE_ALPHA_SIMULATION */
  FISCHE_LINESTYLE line_style;

  /* 0.5 <= scale <= 2.0
     * DEFAULT: 1.0
     * constant after fische_start() */
  double scale;

  /* -10 <= amplification <= 10
     * DEFAULT: 0 */
  double amplification;

  /* true (!=0) or false (0)
     * DEFAULT: 0 */
  uint8_t vector_store_load_usage;

  /* if non-NULL,
     * fische calls this to read vector fields from an external source
     * takes a void** for data placement
     * returns the number of bytes read */
  size_t (*read_vectors)(void* handler, void**);

  /* if non-NULL,
     * fische calls this to write vector field data to an external sink
     * takes a void* and the number of bytes to be written */
  void (*write_vectors)(void* handler, const void*, size_t);

  /* if non-NULL,
     * fische calls this on major beats that are not handled internally
     * takes frames per beat */
  void (*on_beat)(void* handler, double);

  void* handler;

  /* read only */
  uint32_t frame_counter;

  /* read only */
  const char* error_text;
} FISCHE;

#endif
