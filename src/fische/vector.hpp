/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <cstdint>

namespace fische
{

struct _vector_
{
  double x;
  double y;
};

typedef struct _vector_ vector;
typedef struct _vector_ point;

enum
{
  _VECTOR_LEFT_,
  _VECTOR_RIGHT_
};

double vector_length(vector* self);
vector vector_normal(vector* self);
vector vector_single(vector* self);
double vector_angle(vector* self);
uint16_t vector_to_uint16(vector* self);
vector vector_from_uint16(uint16_t val);
void vector_add(vector* self, vector* other);
void vector_sub(vector* self, vector* other);
void vector_mul(vector* self, double val);
void vector_div(vector* self, double val);

vector vector_intersect_border(vector* self,
                               vector* normal_vec,
                               uint_fast16_t width,
                               uint_fast16_t height,
                               int_fast8_t direction);


} // namespace fische
