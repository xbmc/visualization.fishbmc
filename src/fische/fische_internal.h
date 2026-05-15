/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#ifndef FISCHE_INTERNAL_H
#define FISCHE_INTERNAL_H

#include "analyst.hpp"
#include "audiobuffer.hpp"
#include "blurengine.hpp"
#include "fische.h"
#include "screenbuffer.hpp"
#include "vector.hpp"
#include "vectorfield.hpp"
#include "wavepainter.hpp"

#ifdef WIN32
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define rand_r(_seed) (_seed == _seed ? rand() : rand())
#endif

#define FISCHE_PRIVATE(P) ((struct _fische__internal_*)P->fische->priv)

struct _fische__internal_
{
  struct fische__screenbuffer* screenbuffer;
  struct fische__wavepainter* wavepainter;
  struct fische__analyst* analyst;
  struct fische__blurengine* blurengine;
  struct fische__vectorfield* vectorfield;
  struct fische__audiobuffer* audiobuffer;
  double init_progress;
  uint_fast8_t init_cancel;
  uint_fast8_t audio_valid;
};

#endif
