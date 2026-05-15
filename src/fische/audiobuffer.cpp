/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "audiobuffer.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace fische
{

CAudioBuffer::CAudioBuffer(FISCHE_AUDIOFORMAT format) : m_format(format)
{
}

CAudioBuffer::~CAudioBuffer()
{
  Lock();

  free(m_buffer);
}

void CAudioBuffer::Insert(const void* data, size_t size)
{
  if (m_buffer_size > 44100)
    return;

  uint_fast8_t width = 1;

  switch (m_format)
  {
    case FISCHE_AUDIOFORMAT_DOUBLE:
      width = 8;
      break;
    case FISCHE_AUDIOFORMAT_FLOAT:
    case FISCHE_AUDIOFORMAT_S32:
    case FISCHE_AUDIOFORMAT_U32:
      width = 4;
      break;
    case FISCHE_AUDIOFORMAT_S16:
    case FISCHE_AUDIOFORMAT_U16:
      width = 2;
  }

  const size_t old_bufsize = m_buffer_size;
  m_buffer_size += size / width;
  m_buffer = static_cast<double*>(realloc(m_buffer, m_buffer_size * sizeof(double)));

  for (size_t i = 0; i < size / width; ++i)
  {
    double* dest = (m_buffer + old_bufsize + i);

    switch (m_format)
    {
      case FISCHE_AUDIOFORMAT_FLOAT:
        *dest = *(static_cast<const float*>(data) + i);
        break;
      case FISCHE_AUDIOFORMAT_DOUBLE:
        *dest = *(static_cast<const double*>(data) + i);
        break;
      case FISCHE_AUDIOFORMAT_S32:
        *dest = *(static_cast<const int32_t*>(data) + i);
        *dest /= INT32_MAX;
        break;
      case FISCHE_AUDIOFORMAT_U32:
        *dest = *(static_cast<const uint32_t*>(data) + i);
        *dest -= INT32_MAX;
        *dest /= INT32_MAX;
        break;
      case FISCHE_AUDIOFORMAT_S16:
        *dest = *(static_cast<const int16_t*>(data) + i);
        *dest /= INT16_MAX;
        break;
      case FISCHE_AUDIOFORMAT_U16:
        *dest = *(static_cast<const uint16_t*>(data) + i);
        *dest -= INT16_MAX;
        *dest /= INT16_MAX;
        break;
      case FISCHE_AUDIOFORMAT_S8:
        *dest = *(static_cast<const int8_t*>(data) + i);
        *dest /= INT8_MAX;
        break;
      case FISCHE_AUDIOFORMAT_U8:
        *dest = *(static_cast<const uint8_t*>(data) + i);
        *dest /= INT8_MAX;
        *dest /= INT8_MAX;
        break;
    }
  }

  ++m_puts;
}

void CAudioBuffer::Get()
{
  if (m_buffer_size == 0)
    return;

  double* new_start = m_buffer + m_last_get * 2;
  m_buffer_size -= m_last_get * 2;

  // pop used data off front
  memmove(m_buffer, new_start, m_buffer_size * sizeof(double));
  m_buffer = static_cast<double*>(realloc(m_buffer, m_buffer_size * sizeof(double)));

  if (!m_puts)
    return;

  // fallback for first get
  if (m_gets == 0)
  {
    m_gets = 3;
    m_puts = 1;
  }

  // get/put ratio
  double d_ratio = static_cast<double>(m_gets) / m_puts;
  uint_fast8_t ratio = static_cast<uint_fast8_t>(ceil(d_ratio));

  // how many samples to return
  size_t n_samples = m_buffer_size / 2 / ratio;

  // set return data size and remember
  m_front_sample_count = n_samples;
  m_back_sample_count = n_samples;
  m_last_get = n_samples;

  // set export buffer
  m_front_samples = m_buffer;
  m_back_samples = m_buffer + m_buffer_size - n_samples * 2;

  // increment get counter
  ++m_gets;
}

void CAudioBuffer::Lock()
{
  bool expected = false;
  while (m_is_locked.compare_exchange_strong(expected, true) == false)
  {
    expected = false;
    std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
}

void CAudioBuffer::Unlock()
{
  m_is_locked = false;
}

} // namespace fische
