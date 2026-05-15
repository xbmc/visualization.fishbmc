/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "fische.hpp"

#include "analyst.hpp"
#include "audiobuffer.hpp"
#include "blurengine.hpp"
#include "cpudetect.h"
#include "screenbuffer.hpp"
#include "vector.hpp"
#include "vectorfield.hpp"
#include "wavepainter.hpp"

#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>

namespace fische
{

CFische::CFische() : m_init_progress(0), m_init_cancel(false)
{
  used_cpus = _fische__cpu_detect_();
  if (used_cpus > 8)
    used_cpus = 8;

  frame_counter = 0;
  audio_format = FISCHE_AUDIOFORMAT_FLOAT;
  pixel_format = FISCHE_PIXELFORMAT_0xAABBGGRR;
  width = 512;
  height = 256;
  vector_store_load_usage = 0;
  read_vectors = nullptr;
  write_vectors = nullptr;
  on_beat = nullptr;
  nervous_mode = 0;
  blur_mode = FISCHE_BLUR_SLICK;
  line_style = FISCHE_LINESTYLE_ALPHA_SIMULATION;
  scale = 1;
  amplification = 0;
  error_text = "no error";
}

CFische::~CFische()
{
  // tell init threads to quit
  m_init_cancel = true;

  // wait for init threads to quit
  while (m_init_progress < 1)
    std::this_thread::sleep_for(std::chrono::microseconds(10));

  m_audiobuffer.reset();
  m_blurengine.reset();
  m_vectorfield.reset();
  m_wavepainter.reset();
  m_screenbuffer.reset();
  m_analyst.reset();
}

bool CFische::Start()
{
  // plausibility checks
  if ((used_cpus > 8) || (used_cpus < 1))
  {
    error_text = "CPU count out of range (1 <= used_cpus <= 8)";
    return false;
  }

  if (audio_format >= _FISCHE__AUDIOFORMAT_LAST_)
  {
    error_text = "audio format invalid";
    return false;
  }

  if (line_style >= _FISCHE__LINESTYLE_LAST_)
  {
    error_text = "line style invalid";
    return false;
  }

  if (frame_counter != 0)
  {
    error_text = "frame counter garbled";
    return false;
  }

  if ((amplification < -10) || (amplification > 10))
  {
    error_text = "amplification value out of range (-10 <= amplification <= 10)";
    return false;
  }

  if ((height < 16) || (height > 2048))
  {
    error_text = "height value out of range (16 <= height <= 2048)";
    return false;
  }

  if ((width < 16) || (width > 2048))
  {
    error_text = "width value out of range (16 <= width <= 2048)";
    return false;
  }

  if (width % 4 != 0)
  {
    error_text = "width value invalid (must be a multiple of four)";
    return false;
  }

  if (pixel_format >= _FISCHE__PIXELFORMAT_LAST_)
  {
    error_text = "pixel format invalid";
    return false;
  }

  if ((scale < 0.5) || (scale > 2))
  {
    error_text = "scale value out of range (0.5 <= scale <= 2.0)";
    return false;
  }

  if (blur_mode >= _FISCHE__BLUR_LAST_)
  {
    error_text = "blur option invalid";
    return false;
  }

  m_init_progress = -1;

  m_analyst = std::make_unique<fische::CAnalyst>(this);
  m_screenbuffer = std::make_unique<fische::CScreenBuffer>(this);
  m_wavepainter = std::make_unique<fische::CWavePainter>(this);
  m_blurengine = std::make_unique<fische::CBlurEngine>(this);
  m_audiobuffer = std::make_unique<fische::CAudioBuffer>(audio_format);

  // start vector creation and busy indicator threads
  std::thread(&CFische::ThreadCreateVectors, this).detach();
  std::thread(&CFische::ThreadIndicateBusy, this).detach();

  return true;
}

uint32_t* CFische::Render()
{
  // only if init completed
  if (m_init_progress >= 1)
  {
    // analyse sound data
    m_audiobuffer->Lock();
    m_audiobuffer->Get();
    int_fast8_t analysis =
        m_analyst->Analyse(m_audiobuffer->BackSamples(), m_audiobuffer->BackSampleCount());

    // act accordingly
    if (nervous_mode)
    {
      if (analysis >= 2)
        m_wavepainter->ChangeShape();
      if (analysis >= 1)
        m_vectorfield->Change();
    }
    else
    {
      if (analysis >= 1)
        m_wavepainter->ChangeShape();
      if (analysis >= 2)
        m_vectorfield->Change();
    }

    if (analysis >= 3)
    {
      m_wavepainter->Beat(m_analyst->GetFramesPerBeat());
    }
    if (analysis >= 4)
    {
      OnBeat(m_analyst->GetFramesPerBeat());
    }

    m_audio_valid = analysis >= 0;

    m_wavepainter->ChangeColor(m_analyst->GetFramesPerBeat(), m_analyst->GetRelativeEnergy());

    // wait for blurring to be finished
    // and swap buffers
    m_screenbuffer->Lock();
    m_blurengine->SwapBuffers();
    m_screenbuffer->Unlock();

    // draw waves
    if (m_audio_valid)
      m_wavepainter->Paint(m_audiobuffer->FrontSamples(), m_audiobuffer->FrontSampleCount());

    // start blurring for the next frame
    m_blurengine->Blur(m_vectorfield->Field());

    m_audiobuffer->Unlock();
  }

  frame_counter++;

  return m_screenbuffer->Pixels();
}

void CFische::AudioData(const void* data, size_t data_size)
{
  if (m_audiobuffer == nullptr)
    return;

  m_audiobuffer->Lock();
  m_audiobuffer->Insert(data, data_size);
  m_audiobuffer->Unlock();
}

void CFische::ThreadCreateVectors()
{
  m_vectorfield = std::make_unique<fische::CVectorField>(this, m_init_progress, m_init_cancel);
}

void CFische::ThreadIndicateBusy()
{
  fische::point center;
  center.x = m_screenbuffer->Width() / 2;
  center.y = m_screenbuffer->Height() / 2;
  double dim = (center.x > center.y) ? center.y / 2 : center.x / 2;

  double last = -1;

  while ((m_init_progress < 1) && (!m_init_cancel))
  {

    if ((m_init_progress < 0) || (m_init_progress == last))
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    last = m_init_progress;
    double angle = m_init_progress * -2 * 3.1415 + 3.0415;

    fische::vector c1;
    c1.x = sin(angle) * dim;
    c1.y = cos(angle) * dim;

    fische::vector c2;
    c2.x = sin(angle + 0.1) * dim;
    c2.y = cos(angle + 0.1) * dim;

    fische::vector e1 = fische::vector_single(&c1);
    fische::vector_mul(&e1, dim / 2);
    fische::vector e2 = fische::vector_single(&c2);
    fische::vector_mul(&e2, dim / 2);

    fische::vector c3 = c2;
    fische::vector_sub(&c3, &e2);
    fische::vector c4 = c1;
    fische::vector_sub(&c4, &e1);

    fische::vector_mul(&c1, scale);
    fische::vector_mul(&c2, scale);
    fische::vector_mul(&c3, scale);
    fische::vector_mul(&c4, scale);

    fische::vector_add(&c1, &center);
    fische::vector_add(&c2, &center);
    fische::vector_add(&c3, &center);
    fische::vector_add(&c4, &center);

    m_screenbuffer->Lock();
    m_screenbuffer->Line(c1.x, c1.y, c2.x, c2.y, 0xffffffff);
    m_screenbuffer->Line(c2.x, c2.y, c3.x, c3.y, 0xffffffff);
    m_screenbuffer->Line(c3.x, c3.y, c4.x, c4.y, 0xffffffff);
    m_screenbuffer->Line(c4.x, c4.y, c1.x, c1.y, 0xffffffff);
    m_screenbuffer->Unlock();
  }
}

bool CFische::SetWidthHeight(uint16_t width, uint16_t height)
{
  if ((width < 16) || (width > 2048) || (height < 16) || (height > 2048))
  {
    error_text = "width and height out of range (16 <= width, height <= 2048)";
    return false;
  }

  this->width = width;
  this->height = height;
  return true;
}

bool CFische::SetUsedCPUs(uint8_t used_cpus)
{
  if ((used_cpus < 1) || (used_cpus > 8))
  {
    error_text = "CPU count out of range (1 <= used_cpus <= 8)";
    return false;
  }
  this->used_cpus = used_cpus;
  return true;
}

bool CFische::SetNervousMode(bool nervous_mode)
{
  this->nervous_mode = nervous_mode ? 1 : 0;
  return true;
}

bool CFische::SetAudioFormat(FISCHE_AUDIOFORMAT audio_format)
{
  if (audio_format >= _FISCHE__AUDIOFORMAT_LAST_)
  {
    error_text = "audio format invalid";
    return false;
  }
  this->audio_format = audio_format;
  return true;
}

bool CFische::SetPixelFormat(FISCHE_PIXELFORMAT pixel_format)
{
  if (pixel_format >= _FISCHE__PIXELFORMAT_LAST_)
  {
    error_text = "pixel format invalid";
    return false;
  }
  this->pixel_format = pixel_format;
  return true;
}

bool CFische::SetBlurMode(FISCHE_BLUR blur_mode)
{
  if (blur_mode >= _FISCHE__BLUR_LAST_)
  {
    error_text = "blur option invalid";
    return false;
  }
  this->blur_mode = blur_mode;
  return true;
}

bool CFische::SetLineStyle(FISCHE_LINESTYLE line_style)
{
  if (line_style >= _FISCHE__LINESTYLE_LAST_)
  {
    error_text = "line style invalid";
    return false;
  }
  this->line_style = line_style;
  return true;
}

bool CFische::SetScale(double scale)
{
  if ((scale < 0.5) || (scale > 2))
  {
    error_text = "scale value out of range (0.5 <= scale <= 2.0)";
    return false;
  }
  this->scale = scale;
  return true;
}

bool CFische::SetAmplification(double amplification)
{
  if ((amplification < -10) || (amplification > 10))
  {
    error_text = "amplification value out of range (-10 <= amplification <= 10)";
    return false;
  }
  this->amplification = amplification;
  return true;
}

} // namespace fische
