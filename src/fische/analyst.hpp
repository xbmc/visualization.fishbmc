/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <array>
#include <cstdint>
#include <cstddef>

namespace fische
{

class CFische;

class CAnalyst
{
public:
  CAnalyst(const CFische* parent);
  ~CAnalyst() = default;

  int_fast8_t Analyse(const double* data, size_t size);
  inline double GetRelativeEnergy() const { return m_relative_energy; }
  inline double GetFramesPerBeat() const { return m_frames_per_beat; }

private:
  const CFische* m_fische;

  enum STATE
  {
    WAITING,
    MAYBEWAITING,
    FISCHE_BEAT
  };

  static const uint_fast8_t BEAT_GAP_HISTORY_SIZE{30};

  static int CompareInt(void const* value1, void const* value2);
  double GuessFramesPerBeat();
  double GetAudioLevel(const double* data, size_t data_size);

  double m_relative_energy{1.0};
  double m_frames_per_beat{0.0};

  STATE m_state{WAITING};
  double m_moving_avg_30{0.0};
  double m_moving_avg_03{0.0};
  double m_std_dev{0.0};
  double m_intensity_moving_avg{0.0};
  double m_intensity_std_dev{0.0};
  uint_fast32_t m_last_beat_frame{0};
  std::array<uint_fast16_t, BEAT_GAP_HISTORY_SIZE> m_beat_gap_history{'\0'};
  uint_fast8_t m_bghist_head{0};
};

} // namespace fische
