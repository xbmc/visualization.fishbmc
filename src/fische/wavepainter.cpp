/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#define _USE_MATH_DEFINES

#include "wavepainter.hpp"

#include "fische.hpp"
#include "screenbuffer.hpp"
#include "vector.hpp"

#include <cmath>
#include <cstdlib>

namespace fische
{

CWavePainter::CWavePainter(const CFische* parent)
  : m_fische(parent),
    m_width(parent->GetWidth()),
    m_height(parent->GetHeight()),
    m_center_x(parent->GetWidth() / 2),
    m_center_y(parent->GetHeight() / 2),
    m_sbuf(m_fische->GetScreenbuffer())
{
  const uint32_t full_alpha = 0xff << m_sbuf->AlphaShift();
  m_color_1 = (rand() % 0xffffffff) | full_alpha;
  m_color_2 = (~m_color_1) | full_alpha;
}

void CWavePainter::Paint(const double* data, size_t size)
{
  if (!size)
    return;

  // rotation
  if (m_is_rotating)
  {
    m_angle += m_rotation_increment;
    if ((m_angle > 2 * M_PI) || (m_angle < -2 * M_PI))
    {
      m_angle = 0;
      m_is_rotating = false;
    }
  }

  // only init fische scale once
  static double f_scale = 0;
  if (f_scale == 0)
    f_scale = m_fische->GetScale();

  // necessary parameters
  double dim = (m_height < m_width) ? m_height : m_width;
  dim *= f_scale;
  double factor = pow(10, m_fische->GetAmplification() / 10);
  double scale = 6 / dim / factor;

  // alpha saturation fix
  m_sbuf->Line(0, 0, m_width - 1, 0, 0);
  m_sbuf->Line(m_width - 1, 0, m_width - 1, m_height - 1, 0);
  m_sbuf->Line(m_width - 1, m_height - 1, 0, m_height - 1, 0);
  m_sbuf->Line(0, m_height - 1, 0, 0, 0);

  switch (m_shape)
  {
    case 0:
    {
      fische::point center;
      center.x = m_center_x;
      center.y = m_center_y;

      // base will be the middle of a line,
      // normally horizontal (angle = 0), but could be rotating
      fische::point base1;
      base1.x = center.x + (dim / 6) * sin(m_angle);
      base1.y = center.y + (dim / 6) * cos(m_angle);

      fische::point base2;
      base2.x = m_width / 2 - (dim / 6) * sin(m_angle);
      base2.y = m_height / 2 - (dim / 6) * cos(m_angle);

      // create vectors perpendicular to the line center->base
      fische::vector _nvec1 = base1;
      fische::vector_sub(&_nvec1, &center);
      fische::vector nvec1 = fische::vector_normal(&_nvec1);

      fische::vector _nvec2 = base2;
      fische::vector_sub(&_nvec2, &center);
      fische::vector nvec2 = fische::vector_normal(&_nvec2);

      // find the points where the line would exit the screen
      fische::point start1 =
          fische::vector_intersect_border(&base1, &nvec1, m_width, m_height, fische::_VECTOR_LEFT_);
      fische::point end1 =
          fische::vector_intersect_border(&base1, &nvec1, m_width, m_height, fische::_VECTOR_RIGHT_);

      fische::point start2 =
          fische::vector_intersect_border(&base2, &nvec2, m_width, m_height, fische::_VECTOR_LEFT_);
      fische::point end2 =
          fische::vector_intersect_border(&base2, &nvec2, m_width, m_height, fische::_VECTOR_RIGHT_);

      // determine the direction and length (i.e. vector)
      // of the increment between two sound samples
      fische::vector v1 = end1;
      fische::vector_sub(&v1, &start1);
      fische::vector_div(&v1, static_cast<double>(size));

      fische::vector v2 = end2;
      fische::vector_sub(&v2, &start2);
      fische::vector_div(&v2, static_cast<double>(size));

      // determine the normal vectors
      // for calculating the sound sample offset (amplitude)
      fische::vector _n1 = fische::vector_normal(&v1);
      fische::vector n1 = fische::vector_single(&_n1);
      fische::vector _n2 = fische::vector_normal(&v2);
      fische::vector n2 = fische::vector_single(&_n2);

      // draw both lines
      fische::point base_p1 = start1;
      fische::point base_p2 = start2;

      for (size_t i = 0; i < size - 1; i++)
      {
        fische::point pt11 = base_p1;
        fische::vector offset11 = n1;
        fische::vector_mul(&offset11, (*(data + 2 * i)));
        fische::vector_div(&offset11, scale);
        fische::vector_add(&pt11, &offset11);

        fische::point pt21 = base_p2;
        fische::vector offset21 = n2;
        fische::vector_mul(&offset21, (*(data + 1 + 2 * i)));
        fische::vector_div(&offset21, scale);
        fische::vector_add(&pt21, &offset21);

        fische::vector_add(&base_p1, &v1);
        fische::vector_add(&base_p2, &v2);

        fische::point pt12 = base_p1;
        fische::vector offset12 = n1;
        fische::vector_mul(&offset12, (*(data + 2 * (i + 1))));
        fische::vector_div(&offset12, scale);
        fische::vector_add(&pt12, &offset12);

        fische::point pt22 = base_p2;
        fische::vector offset22 = n2;
        fische::vector_mul(&offset22, (*(data + 1 + 2 * (i + 1))));
        fische::vector_div(&offset22, scale);
        fische::vector_add(&pt22, &offset22);

        m_sbuf->Line(pt11.x, pt11.y, pt12.x, pt12.y, m_color_1);
        m_sbuf->Line(pt21.x, pt21.y, pt22.x, pt22.y, m_color_2);
      }
      return;
    }

    // circular shape
    case 1:
    {
      double f = cos(M_PI / 3 + 2 * m_angle) + 0.5;
      double e = 1;

      for (size_t i = 0; i < size - 1; i++)
      {
        double incr = static_cast<double>(i);

        // calculate angles for this and the next sound sample
        double phi1 = M_PI * (0.25 + incr / size) + m_angle;
        double phi2 = phi1 + M_PI / size;

        // calculate the corresponding radius
        double r1 = dim / 4 + *(data + 2 * i) / scale;
        double r2 = dim / 4 + *(data + 2 * (i + 1)) / scale;

        double x1 = floor((m_center_x + f * r1 * sin(phi1)) + 0.5);
        double x2 = floor((m_center_x + f * r2 * sin(phi2)) + 0.5);
        double y1 = floor((m_center_y + e * r1 * cos(phi1)) + 0.5);
        double y2 = floor((m_center_y + e * r2 * cos(phi2)) + 0.5);

        m_sbuf->Line(x1, y1, x2, y2, m_color_1);

        // the second line will be exactly on the
        // opposite side of the circle
        phi1 += M_PI;
        phi2 += M_PI;

        r1 = dim / 4 + *(data + 1 + 2 * i) / scale;
        r2 = dim / 4 + *(data + 1 + 2 * (i + 1)) / scale;

        x1 = floor((m_center_x + f * r1 * sin(phi1)) + 0.5);
        x2 = floor((m_center_x + f * r2 * sin(phi2)) + 0.5);
        y1 = floor((m_center_y + e * r1 * cos(phi1)) + 0.5);
        y2 = floor((m_center_y + e * r2 * cos(phi2)) + 0.5);

        m_sbuf->Line(x1, y1, x2, y2, m_color_2);
      }
      return;
    }
  }
}

void CWavePainter::Beat(double frames_per_beat)
{
  if (!m_is_rotating)
  {
    if (frames_per_beat != 0)
    {
      m_direction = 1 - 2 * (rand() % 2);
      m_rotation_increment = M_PI / frames_per_beat / 2 * m_direction;
      m_angle = 0;
      m_is_rotating = true;
    }
  }
}

void CWavePainter::ChangeColor(double frames_per_beat, double energy)
{
  uint32_t full_alpha = 0xff << m_sbuf->AlphaShift();

  if (!frames_per_beat && !energy)
  {
    m_color_1 = (rand() % 0xffffffff) | full_alpha;
    m_color_2 = (~m_color_1) | full_alpha;
  }

  if (!frames_per_beat)
    return;

  double hue = frames_per_beat / 2;
  while (hue >= 6)
    hue -= 6;

  double sv = (energy > 1) ? 1 : pow(energy, 4);
  double x = sv * (1 - fabs((int_fast32_t)hue % 2 - 1));

  double r, g, b;

  switch ((int_fast32_t)hue)
  {
    case 0:
      r = sv;
      g = x;
      b = 0;
      break;
    case 1:
      r = x;
      g = sv;
      b = 0;
      break;
    case 2:
      r = 0;
      g = sv;
      b = x;
      break;
    case 3:
      r = 0;
      g = x;
      b = sv;
      break;
    case 4:
      r = x;
      g = 0;
      b = sv;
      break;
    default:
    case 5:
      r = sv;
      g = 0;
      b = x;
  }

  uint32_t red = static_cast<uint32_t>(floor(r * 255 + 0.5));
  uint32_t green = static_cast<uint32_t>(floor(b * 255 + 0.5));
  uint32_t blue = static_cast<uint32_t>(floor(g * 255 + 0.5));

  m_color_1 = (blue << m_sbuf->BlueShift()) +
            (green << m_sbuf->GreenShift()) +
            (red << m_sbuf->RedShift()) +
            (0xff << m_sbuf->AlphaShift());

  m_color_2 = (~m_color_1) | full_alpha;
}

void CWavePainter::ChangeShape()
{
  if (m_is_rotating)
    return;
  int_fast8_t n = m_shape;
  while (n == m_shape)
    n = rand() % m_n_shapes;
  m_shape = n;
}

} // namespace fische
