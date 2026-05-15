/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "screenbuffer.hpp"

#include "fische.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace fische
{

CScreenBuffer::CScreenBuffer(const CFische* parent)
  : m_fische(parent), m_width(parent->GetWidth()), m_height(parent->GetHeight())

{
  m_pixels = new uint32_t[m_width * m_height]();

  switch (parent->GetPixelFormat())
  {
    case FISCHE_PIXELFORMAT_0xAABBGGRR:
      m_alpha_shift = 24;
      m_blue_shift = 16;
      m_green_shift = 8;
      m_red_shift = 0;
      break;
    case FISCHE_PIXELFORMAT_0xAARRGGBB:
      m_alpha_shift = 24;
      m_blue_shift = 0;
      m_green_shift = 8;
      m_red_shift = 16;
      break;
    case FISCHE_PIXELFORMAT_0xBBGGRRAA:
      m_alpha_shift = 0;
      m_blue_shift = 24;
      m_green_shift = 16;
      m_red_shift = 8;
      break;
    case FISCHE_PIXELFORMAT_0xRRGGBBAA:
      m_alpha_shift = 0;
      m_blue_shift = 8;
      m_green_shift = 16;
      m_red_shift = 24;
      break;
  }
}

CScreenBuffer::~CScreenBuffer()
{
  Lock();

  delete[] m_pixels;
}

void CScreenBuffer::Lock()
{
  bool expected = false;
  while (m_is_locked.compare_exchange_strong(expected, true) == false)
  {
    expected = false;
    std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
}

void CScreenBuffer::Unlock()
{
  m_is_locked = false;
}

void CScreenBuffer::Line(double x1, double y1, double x2, double y2, uint32_t color)
{
  double diff_x = (x1 > x2) ? (x1 - x2) : (x2 - x1);
  double diff_y = (y1 > y2) ? (y1 - y2) : (y2 - y1);
  double dir_x = (x2 < x1) ? -1 : 1;
  double dir_y = (y2 < y1) ? -1 : 1;

  if (!diff_x && !diff_y)
    return;

  uint32_t half_alpha_mask;

  if (m_fische->GetLineStyle() == FISCHE_LINESTYLE_ALPHA_SIMULATION)
    half_alpha_mask = (0x7f << m_red_shift) + (0x7f << m_green_shift) + (0x7f << m_blue_shift) +
                      (0x7f << m_alpha_shift);
  else
    half_alpha_mask = (0xff << m_red_shift) + (0xff << m_green_shift) + (0xff << m_blue_shift) +
                      (0x7f << m_alpha_shift);


  if (diff_x > diff_y)
  {
    for (int_fast16_t x = int_fast16_t(x1); x * dir_x <= x2 * dir_x; x += int_fast16_t(dir_x))
    {
      int_fast16_t y = int_fast16_t(y1 + diff_y / diff_x * dir_y * abs(x - x1) + 0.5);

      if ((x < 0) || (x >= m_width) || (y < 0) || (y >= m_height))
        continue;

      if (m_fische->GetLineStyle() != FISCHE_LINESTYLE_THIN)
      {
        y++;
        if (!(y < 0) && !(y >= m_height))
          *(m_pixels + y * m_width + x) = color & half_alpha_mask;
        y -= 2;
        if ((y < 0) || (y >= m_height))
          continue;
        *(m_pixels + y * m_width + x) = color & half_alpha_mask;
        y++;
      }

      *(m_pixels + y * m_width + x) = color;
    }
  }
  else
  {
    for (int_fast16_t y = int_fast16_t(y1); y * dir_y <= y2 * dir_y; y += int_fast16_t(dir_y))
    {
      int_fast16_t x = int_fast16_t(x1 + diff_x / diff_y * dir_x * abs(y - y1) + 0.5);

      if ((x < 0) || (x >= m_width) || (y < 0) || (y >= m_height))
        continue;

      if (m_fische->GetLineStyle() != FISCHE_LINESTYLE_THIN)
      {
        x++;
        if (!(x < 0) && !(x >= m_width))
          *(m_pixels + y * m_width + x) = color & half_alpha_mask;

        x -= 2;
        if ((x < 0) || (x >= m_width))
          continue;
        *(m_pixels + y * m_width + x) = color & half_alpha_mask;

        x++;
      }

      *(m_pixels + y * m_width + x) = color;
    }
  }
}

} // namespace fische
