/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "vector.hpp"

#include <cstddef>
#include <cstdint>

#define VECTOR_N_FIELDS 20

namespace fische
{

class CFische;

class CVectorField
{
public:
  CVectorField(CFische* parent, double& progress, bool& cancel);
  ~CVectorField();

  void Change();
  inline const uint16_t* Field() const { return m_field; };

private:
  inline void Randomize(fische::vector* vec);
  inline void Validate(fische::vector* vec, double x, double y);
  void FillField(uint_fast8_t fieldno);
  void FillThread(uint16_t* field,
                  uint_fast8_t fieldno,
                  uint_fast16_t start_y,
                  uint_fast16_t end_y);

  uint16_t* m_field{nullptr};

  CFische* const m_fische;
  const uint_fast16_t m_width;
  const uint_fast16_t m_height;
  const uint_fast16_t m_center_x;
  const uint_fast16_t m_center_y;
  const uint_fast16_t m_dimension;
  const uint_fast8_t m_threads;
  const size_t m_fieldsize;

  uint16_t* m_fields{nullptr};
  size_t m_n_fields;
  bool m_cancelled{false};

  static uint32_t m_rand_seed;
};

} // namespace fische
