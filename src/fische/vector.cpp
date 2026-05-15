/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "vector.hpp"

#include <cmath>

namespace fische
{

double vector_length(vector* self)
{
  return sqrt(pow(self->x, 2) + pow(self->y, 2));
}

vector vector_normal(vector* self)
{
  vector r;
  r.x = self->y;
  r.y = -self->x;
  return r;
}

vector vector_single(vector* self)
{
  double l = vector_length(self);
  vector r;
  r.x = self->x / l;
  r.y = self->y / l;
  return r;
}

double vector_angle(vector* self)
{
  vector su = vector_single(self);
  double a = acos(su.x);
  if (self->y > 0)
    return a;
  else
    return -a;
}

// conversion to 2x int8
uint16_t vector_to_uint16(vector* self)
{
  if (self->x < -127)
    self->x = -127;
  if (self->x > 127)
    self->x = 127;
  if (self->y < -127)
    self->y = -127;
  if (self->y > 127)
    self->y = 127;

  int8_t ix = static_cast<int8_t>((self->x < 0) ? self->x - 0.5 : self->x + 0.5);
  int8_t iy = static_cast<int8_t>((self->y < 0) ? self->y - 0.5 : self->y + 0.5);

  uint16_t retval = (uint8_t)ix;
  retval |= ((uint8_t)iy) << 8;
  return retval;
}

vector vector_from_uint16(uint16_t val)
{
  int8_t ix = val & 0xff;
  int8_t iy = val >> 8;
  vector r;
  r.x = ix;
  r.y = iy;
  return r;
}

void vector_add(vector* self, vector* other)
{
  self->x += other->x;
  self->y += other->y;
}

void vector_sub(vector* self, vector* other)
{
  self->x -= other->x;
  self->y -= other->y;
}

void vector_mul(vector* self, double val)
{
  self->x *= val;
  self->y *= val;
}

void vector_div(vector* self, double val)
{
  self->x /= val;
  self->y /= val;
}

vector vector_intersect_border(vector* self,
                               vector* normal_vec,
                               uint_fast16_t width,
                               uint_fast16_t height,
                               int_fast8_t direction)
{
  width--;
  height--;

  vector nvec = *normal_vec;
  if (direction == _VECTOR_RIGHT_)
  {
    vector_mul(&nvec, -1);
  }

  double t1, t2, t3, t4;

  if (nvec.x == 0)
  {
    t1 = 1e6;
    t2 = 1e6;
  }
  else
  {
    t1 = -self->x / nvec.x;
    t2 = (width - self->x) / nvec.x;
  }

  if (nvec.y == 0)
  {
    t3 = 1e6;
    t4 = 1e6;
  }
  else
  {
    t3 = -self->y / nvec.y;
    t4 = (height - self->y) / nvec.y;
  }

  t1 = (t1 < 0) ? 1e6 : t1;
  t2 = (t2 < 0) ? 1e6 : t2;
  t3 = (t3 < 0) ? 1e6 : t3;
  t4 = (t4 < 0) ? 1e6 : t4;

  double a = (t1 < t2) ? t1 : t2;
  double b = (t3 < t4) ? t3 : t4;

  double min_t = (a < b) ? a : b;

  int_fast16_t ret_x = static_cast<int8_t>(self->x + nvec.x * min_t);
  while (ret_x < 0)
    ret_x++;
  while ((unsigned)ret_x > width)
    ret_x--;

  int_fast16_t ret_y = static_cast<int8_t>(self->y + nvec.y * min_t);
  while (ret_y < 0)
    ret_y++;
  while ((unsigned)ret_y > height)
    ret_y--;

  vector r;
  r.x = ret_x;
  r.y = ret_y;
  return r;
}

} // namespace fische
