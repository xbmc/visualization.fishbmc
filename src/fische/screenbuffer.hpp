/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <atomic>
#include <cstdint>

namespace fische
{

class CFische;

class CScreenBuffer
{
public:
  CScreenBuffer(const CFische* parent);
  ~CScreenBuffer();

  void Lock();
  void Unlock();

  void Line(double x1, double y1, double x2, double y2, uint32_t color);

  inline uint32_t* Pixels() { return m_pixels; }
  inline void SetPixels(uint32_t* pixels) { m_pixels = pixels; }
  inline int_fast16_t Width() const { return m_width; }
  inline int_fast16_t Height() const { return m_height; }
  inline uint_fast8_t RedShift() const { return m_red_shift; }
  inline uint_fast8_t BlueShift() const { return m_blue_shift; }
  inline uint_fast8_t GreenShift() const { return m_green_shift; }
  inline uint_fast8_t AlphaShift() const { return m_alpha_shift; }

private:
  const CFische* m_fische;
  const int_fast16_t m_width;
  const int_fast16_t m_height;

  std::atomic<bool> m_is_locked{false};
  uint32_t* m_pixels;
  uint_fast8_t m_red_shift;
  uint_fast8_t m_blue_shift;
  uint_fast8_t m_green_shift;
  uint_fast8_t m_alpha_shift;
};

} // namespace fische
