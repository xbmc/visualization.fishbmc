/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "fische.hpp"

using namespace fische;

/* creates a new FISCHE object
 * and initialzes it with default values
 */
FISCHE* fische_new()
{
  CFische* fische = new CFische;
  FISCHE* cfische = fische;
  cfische->handler = fische;

  return cfische;
}

/* destructs the FISCHE object */
void fische_free(FISCHE* handle)
{
  if (!handle)
    return;
  CFische* fische = static_cast<CFische*>(handle->handler);
  if (!fische)
    return;

  delete fische;
}

/* starts FISCHE */
int fische_start(FISCHE* handle)
{
  if (!handle)
    return -1;
  CFische* fische = static_cast<CFische*>(handle->handler);
  if (!fische)
    return -1;

  return fische->Start();
}

/* makes the next frame available */
uint32_t* fische_render(FISCHE* handle)
{
  if (!handle)
    return 0;
  CFische* fische = static_cast<CFische*>(handle->handler);
  if (!fische)
    return 0;

  return fische->Render();
}

/* inserts audio data */
void fische_audiodata(FISCHE* handle, const void* data, size_t data_size)
{
  if (!handle)
    return;
  CFische* fische = static_cast<CFische*>(handle->handler);
  if (!fische)
    return;

  fische->AudioData(data, data_size);
}
