/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#define _USE_MATH_DEFINES

#include "vectorfield.hpp"

#include "fische.hpp"

#include <cmath>
#include <thread>

#define N_FIELDS 20
#define MAX_THREADS 8

namespace fische
{

uint32_t CVectorField::m_rand_seed;

CVectorField::CVectorField(CFische* parent, double& progress, bool& cancel)
  : m_fische(parent),
    m_width(parent->GetWidth()),
    m_height(parent->GetHeight()),
    m_center_x(m_width / 2),
    m_center_y(m_height / 2),
    m_dimension(m_width < m_height ? uint_fast16_t(m_width * m_fische->GetScale())
                                   : uint_fast16_t(m_height * m_fische->GetScale())),
    m_threads(parent->GetUsedCPUs() >= MAX_THREADS ? MAX_THREADS : parent->GetUsedCPUs()),
    m_fieldsize(m_width * m_height * sizeof(uint16_t))
{
  const bool useStoredVectors = m_fische->UseVectorStoreLoadUsage();

  m_rand_seed = static_cast<uint32_t>(time(nullptr));
  progress = 0.0;

  // if we have stored fields, load them
  if (useStoredVectors)
  {
    size_t bytes = m_fische->ReadVectors((void**)(&m_fields));
    if (bytes)
    {
      progress = 1.0;
      m_n_fields = bytes / m_fieldsize;
      m_field = m_fields;
      return;
    }
  }

  // if not, recalculate everything
  // NOTE: Leave by `malloc` as the memory can used from "C" code where makes `free(...)`!
  m_fields = static_cast<uint16_t*>(malloc(N_FIELDS * m_fieldsize));
  m_n_fields = N_FIELDS;

  for (uint_fast8_t i = 0; i < N_FIELDS; ++i)
  {
    if (cancel)
    {
      m_cancelled = true;
      break;
    }

    FillField(i);
    progress = (i + 1);
    progress /= N_FIELDS;
  }

  // If we use stored vectors and was not present then store it now
  if (useStoredVectors)
    m_fische->WriteVectors(m_fields, m_n_fields * m_fieldsize);

  progress = 1.0;

  m_field = m_fields;
}

CVectorField::~CVectorField()
{
  free(m_fields);
}

void CVectorField::Change()
{
  uint16_t* n = m_field;
  while (n == m_field)
  {
    m_field = m_fields + uint16_t(rand()) % m_n_fields * m_width * m_height;
  }
}

inline void CVectorField::Randomize(fische::vector* vec)
{
  vec->x += rand_r(&m_rand_seed) % 3;
  vec->x -= 1;
  vec->y += rand_r(&m_rand_seed) % 3;
  vec->y -= 1;
}


inline void CVectorField::Validate(fische::vector* vec, double x, double y)
{
  while (x + vec->x < 2)
    vec->x += 1;
  while (x + vec->x > m_width - 3)
    vec->x -= 1;
  while (y + vec->y < 2)
    vec->y += 1;
  while (y + vec->y > m_height - 2)
    vec->y -= 1;
}

void CVectorField::FillField(uint_fast8_t fieldno)
{
  uint16_t* field = m_fields + fieldno * m_fieldsize / 2;

  // threads maximum is 8
  std::thread vec_threads[MAX_THREADS];

  for (uint_fast8_t i = 0; i < m_threads; ++i)
  {
    const uint_fast16_t start_y = (i * m_height) / m_threads;
    const uint_fast16_t end_y = ((i + 1) * m_height) / m_threads;

    vec_threads[i] = std::thread(&CVectorField::FillThread, this, field, fieldno, start_y, end_y);
  }

  for (uint_fast8_t i = 0; i < m_threads; ++i)
  {
    vec_threads[i].join();
  }
}

void CVectorField::FillThread(uint16_t* field,
                              uint_fast8_t fieldno,
                              uint_fast16_t start_y,
                              uint_fast16_t end_y)
{
  for (uint_fast16_t y = start_y; y < end_y; y++)
  {
    for (uint_fast16_t x = 0; x < m_width; x++)
    {
      uint16_t* vector = field + x + y * m_width;

      // distance and direction relative to center
      fische::vector rvec;
      rvec.x = x;
      rvec.x -= m_center_x;
      rvec.y = y;
      rvec.y -= m_center_y;

      fische::vector e = fische::vector_single(&rvec);
      fische::vector n = fische::vector_normal(&e);

      double r = fische::vector_length(&rvec) / m_dimension;

      // distance and direction relative to left co-center
      fische::vector rvec_left;
      rvec_left.x = (double)x - m_center_x + m_width / 3 * m_fische->GetScale();
      rvec_left.y = (double)y - m_center_y;

      fische::vector e_left = fische::vector_single(&rvec_left);
      fische::vector n_left = fische::vector_normal(&e_left);

      double r_left = fische::vector_length(&rvec_left) / m_dimension;

      // distance and direction relative to right co-center
      fische::vector rvec_right;
      rvec_right.x = (double)x - m_center_x - m_width / 3 * m_fische->GetScale();
      rvec_right.y = (double)y - m_center_y;

      fische::vector e_right = fische::vector_single(&rvec_right);
      fische::vector n_right = fische::vector_normal(&e_right);

      double r_right = fische::vector_length(&rvec_right) / m_dimension;

      double speed = m_dimension / 45;

      // correction factors ensure consistent average speeds with all field types
      // double const corr[] = {0.77, 0.92, 1.72, 2.06, 1.45, 1.45, 1.73, 1.18, 3.24, 2.76, 0.82, 1.21, 1.73, 3.55, 0.47, 0.66, 0.96, 0.97, 1.00, 1.00};
      double const corr[] = {0.83, 0.83, 1.56, 1.56, 1.08, 3.54, 1.56, 1.00, 4.47, 2.77,
                             0.74, 1.01, 1.56, 3.12, 0.67, 0.67, 0.83, 2.43, 1.21, 0.77};

      fische::vector v;
      switch (fieldno)
      {
        case 0:
          // linear vectors showing away from a horizontal mirror axis
          v.x = 0;
          v.y = (y < m_center_y) ? speed * corr[fieldno] : -speed * corr[fieldno];
          break;

        case 1:
          // linear vectors showing away from a vertical mirror axis
          v.x = (x < m_center_x) ? speed * corr[fieldno] : -speed * corr[fieldno];
          v.y = 0;
          break;

        case 2:
          // radial vectors showing away from the center
          v = e;
          fische::vector_mul(&v, -r * speed * corr[fieldno]);
          break;

        case 3:
          // tangential vectors (right)
          v = n;
          fische::vector_mul(&v, r * speed * corr[fieldno]);
          break;

        case 4:
        {
          // tangential-radial vectors (left)
          fische::vector _v1 = n;
          fische::vector_mul(&_v1, -r * speed * corr[fieldno]);
          v = e;
          fische::vector_mul(&v, -r * speed * corr[fieldno]);
          fische::vector_add(&v, &_v1);
          break;
        }

        case 5:
        {
          // tree rings
          double dv = cos(M_PI * 24 * r);
          v = e;
          fische::vector_mul(&v, speed * 0.33 * corr[fieldno] * dv);
          break;
        }

        case 6:
        {
          // hyperbolic vectors
          v.x = e.y;
          v.y = e.x;
          fische::vector_mul(&v, -r * speed * corr[fieldno]);
          break;
        }

        case 7:
          // purely random
          v.x = rand_r(&m_rand_seed) % (int_fast32_t)(2 * speed * corr[fieldno] + 1) -
                (speed * corr[fieldno]);
          v.y = rand_r(&m_rand_seed) % (int_fast32_t)(2 * speed * corr[fieldno] + 1) -
                (speed * corr[fieldno]);
          break;

        case 8:
        {
          // sphere
          double dv = cos(M_PI * r);
          v = e;
          fische::vector_mul(&v, -r * dv * speed * corr[fieldno]);
          break;
        }

        case 9:
        {
          // sine distortion
          double dv = sin(M_PI * 8 * r);
          v = e;
          fische::vector_mul(&v, -r * dv * speed * corr[fieldno]);
          break;
        }

        case 10:
        {
          // black hole
          fische::vector _v1 = n;
          fische::vector_mul(&_v1, speed * corr[fieldno]);
          v = e;
          fische::vector_mul(&v, r * speed * corr[fieldno]);
          fische::vector_add(&v, &_v1);
          if (r * m_dimension < 10)
          {
            v.x = 0;
            v.y = 0;
          }
          break;
        }

        case 11:
        {
          // circular waves
          double dim = pow(11 * M_PI, 2);
          double _r = r * dim;
          v = e;
          fische::vector_mul(&v,
                             -speed * corr[fieldno] *
                                 sqrt(1.04 - pow(cos(sqrt(_r)), 2) + 0.25 * pow(sin(sqrt(_r)), 2)));
          break;
        }

        case 12:
          // spinning CD
          v = n;
          if (fabs(r - 0.25) < 0.15)
            fische::vector_mul(&v, r * speed * corr[fieldno]);
          else
            fische::vector_mul(&v, -r * speed * corr[fieldno]);
          break;

        case 13:
        {
          // three spinning disks
          double rt = 0.3;
          if (r < rt * 1.2)
          {
            v = n;
            fische::vector_mul(&v, -r * speed * corr[fieldno]);
          }
          else if (r_left < rt)
          {
            v = n_left;
            fische::vector_mul(&v, r_left * speed * corr[fieldno]);
          }
          else if (r_right < rt)
          {
            v = n_right;
            fische::vector_mul(&v, r_right * speed * corr[fieldno]);
          }
          else
          {
            v.x = 0;
            v.y = 0;
          }
          break;
        }

        case 14:
        {
          // 3-centered fields - radial
          fische::vector _v1 = e_left;
          fische::vector_mul(&_v1, (2 - r_left) * speed * corr[fieldno]);
          fische::vector _v2 = e_right;
          fische::vector_mul(&_v2, (2 - r_right) * speed * corr[fieldno]);
          v = e;
          fische::vector_mul(&v, (2 - r) * -speed * corr[fieldno]);
          fische::vector_add(&v, &_v1);
          fische::vector_add(&v, &_v2);
          break;
        }

        case 15:
        {
          // 3-centered fields - tangential
          fische::vector _v1 = n_left;
          fische::vector_mul(&_v1, (2 - r_left) * -speed * corr[fieldno]);
          fische::vector _v2 = n_right;
          fische::vector_mul(&_v2, (2 - r_right) * -speed * corr[fieldno]);
          v = n;
          fische::vector_mul(&v, (2 - r) * speed * corr[fieldno]);
          fische::vector_add(&v, &_v1);
          fische::vector_add(&v, &_v2);
          break;
        }

        case 16:
        {
          // lenses effect
          double _r = r * 8 * M_PI;
          fische::vector _v1 = e;
          fische::vector_mul(&_v1, sin(_r) * -speed * corr[fieldno]);
          v = n;
          fische::vector_mul(&v, sin(8 * fische::vector_angle(&e)) * -speed * corr[fieldno]);
          fische::vector_add(&v, &_v1);
          break;
        }

        case 17:
        {
          // lenses effect
          double _r = r * 24 * M_PI;
          fische::vector _v1 = e;
          fische::vector_mul(&_v1, sin(_r) * -speed * 0.33 * corr[fieldno]);
          v = n;
          fische::vector_mul(&v,
                             sin(24 * fische::vector_angle(&e)) * -speed * 0.33 * corr[fieldno]);
          fische::vector_add(&v, &_v1);
          break;
        }

        case 18:
        {
          // fan 1
          v = e;
          double angle = fische::vector_angle(&e);
          fische::vector_mul(&v, -speed * corr[fieldno] * sin(8 * angle));
          break;
        }

        case 19:
        {
          // fan 1
          v = e;
          double angle = fische::vector_angle(&e);
          fische::vector_mul(&v, -speed * corr[fieldno] * (1.1 + sin(8 * angle)));
          break;
        }

        default:
          // index too high. return nothing.
          return;
      }

      if (m_fische->GetBlurMode() == FISCHE_BLUR_FUZZY)
        Randomize(&v);

      Validate(&v, x, y);

      *vector = fische::vector_to_uint16(&v);
    }
  }
}

} // namespace fische
