/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "fische.h"

#include <atomic>
#include <cstdint>
#include <cstdlib>

namespace fische
{

class CFische;

class CAudioBuffer
{
public:
  CAudioBuffer(FISCHE_AUDIOFORMAT format);
  ~CAudioBuffer();

  void Insert(const void* data, size_t size);
  void Lock();
  void Unlock();
  void Get();

  inline const double* FrontSamples() const { return m_front_samples; }
  inline size_t FrontSampleCount() const { return m_front_sample_count; }

  inline const double* BackSamples() const { return m_back_samples; }
  inline size_t BackSampleCount() const { return m_back_sample_count; }

private:
  const FISCHE_AUDIOFORMAT m_format;

  double* m_front_samples{nullptr};
  size_t m_front_sample_count{0};

  double* m_back_samples{nullptr};
  size_t m_back_sample_count{0};

  double* m_buffer{nullptr};
  size_t m_buffer_size{0};
  std::atomic<bool> m_is_locked{false};
  uint_fast32_t m_puts{0};
  uint_fast32_t m_gets{0};
  size_t m_last_get{0};
};

} // namespace fische
