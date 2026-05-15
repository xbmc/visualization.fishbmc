/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace fische
{

class CFische;
class CScreenBuffer;

class CWavePainter
{
public:
  CWavePainter(const CFische* parent);
  ~CWavePainter() = default;

  void Paint(const double* data, size_t size);
  void Beat(double bpm);
  void ChangeColor(double bpm, double energy);
  void ChangeShape();

private:
  const CFische* m_fische;
  const uint_fast16_t m_width;
  const uint_fast16_t m_height;
  const uint_fast16_t m_center_x;
  const uint_fast16_t m_center_y;
  CScreenBuffer* m_sbuf;

  uint32_t m_color_1;
  uint32_t m_color_2;
  int_fast8_t m_direction{1};
  uint_fast8_t m_shape{0};
  uint_fast8_t m_n_shapes{2};
  double m_angle{0.0};
  bool m_is_rotating{false};
  double m_rotation_increment{0.0};
};

} // namespace fische
