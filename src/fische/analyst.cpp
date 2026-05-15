/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "analyst.hpp"

#include "fische.hpp"

#include <cmath>
#include <cstring>

namespace fische
{

CAnalyst::CAnalyst(const CFische* parent) : m_fische(parent)
{
}

int CAnalyst::CompareInt(void const* value1, void const* value2)
{
  return (*(int_fast16_t*)value1 - *(int_fast16_t*)value2);
}

double CAnalyst::GuessFramesPerBeat()
{
  uint_fast16_t gap_history_sorted[BEAT_GAP_HISTORY_SIZE];

  memcpy(gap_history_sorted, m_beat_gap_history.data(), sizeof(m_beat_gap_history));
  qsort(gap_history_sorted, BEAT_GAP_HISTORY_SIZE, sizeof(uint_fast16_t), CompareInt);

  uint_fast16_t guess = gap_history_sorted[14];

  double result = 0.0;
  int count = 0;

  uint16_t value;
  for (size_t i = 0; i < 30; ++i)
  {
    value = gap_history_sorted[i] - guess;
    if (abs(value) <= 2)
    {
      result += gap_history_sorted[i];
      ++count;
    }
  }

  return result / count;
}

double CAnalyst::GetAudioLevel(const double* data, size_t data_size)
{
  double E = 0.0;

  for (size_t i = 0; i < data_size; ++i)
  {
    E += fabs(*(data + i));
  }

  if (E <= 0)
    E = 1e-9;
  E /= data_size;

  return log10(E) * 10;
}

int_fast8_t CAnalyst::Analyse(const double* data, size_t size)
{
  if (!size)
    return -1;

  const double dezibel{GetAudioLevel(data, size * 2)};

  if (m_moving_avg_30 == 0)
    m_moving_avg_30 = dezibel;
  else
    m_moving_avg_30 = m_moving_avg_30 * 0.9667 + dezibel * 0.0333;

  m_std_dev = m_std_dev * 0.9667 + fabs(dezibel - m_moving_avg_30) * 0.0333;

  const uint_fast32_t frameno = m_fische->GetFrameCounter();
  if ((frameno - m_last_beat_frame) > 90)
  {
    m_frames_per_beat = 0;
    m_beat_gap_history.fill('\0');
    m_bghist_head = 0;
  }

  m_relative_energy = m_moving_avg_03 / m_moving_avg_30;

  double relative_intensity = 0.0;
  double new_frames_per_beat;

  switch (m_state)
  {
    case WAITING:
      // don't bother if intensity too low
      if (dezibel < m_moving_avg_30 + m_std_dev)
        break;

      // initialisation fallbacks
      if (m_std_dev == 0)
        relative_intensity = 1.0; // avoid div by 0
      else
        relative_intensity = (dezibel - m_moving_avg_30) / m_std_dev;

      if (m_intensity_moving_avg == 0)
        m_intensity_moving_avg = relative_intensity; // initial assignment
      else
        m_intensity_moving_avg = m_intensity_moving_avg * 0.95 + relative_intensity * 0.05;

      // update intensity standard deviation
      m_intensity_std_dev =
          m_intensity_std_dev * 0.95 + fabs(m_intensity_moving_avg - relative_intensity) * 0.05;

      // we DO have a beat
      m_state = FISCHE_BEAT;

      // update beat gap history
      m_beat_gap_history[m_bghist_head++] = frameno - m_last_beat_frame;
      if (m_bghist_head == BEAT_GAP_HISTORY_SIZE)
        m_bghist_head = 0;

      // remember this as the last beat
      m_last_beat_frame = frameno;

      // reset the short-term moving average
      m_moving_avg_03 = dezibel;

      // try a guess at the tempo
      new_frames_per_beat = GuessFramesPerBeat();
      if ((m_frames_per_beat) && (m_frames_per_beat / new_frames_per_beat < 1.2) &&
          (new_frames_per_beat / m_frames_per_beat < 1.2))
        m_frames_per_beat = (m_frames_per_beat * 2.0 + new_frames_per_beat) / 3.0;
      else
        m_frames_per_beat = new_frames_per_beat;

      // return based on relative beat intensity
      if (relative_intensity > m_intensity_moving_avg + 3 * m_intensity_std_dev)
        return 4;
      if (relative_intensity > m_intensity_moving_avg + 2 * m_intensity_std_dev)
        return 3;
      if (relative_intensity > m_intensity_moving_avg + 1 * m_intensity_std_dev)
        return 2;

      return 1;

    case FISCHE_BEAT:
    case MAYBEWAITING:
      // update short term moving average
      m_moving_avg_03 = m_moving_avg_03 * 0.6667 + dezibel * 0.3333;

      // needs to be low enough twice to exit BEAT state
      if (m_moving_avg_03 < m_moving_avg_30 + m_std_dev)
      {
        m_state = m_state == MAYBEWAITING ? WAITING : MAYBEWAITING;
        return 0;
      }
  }

  // report level too low
  if (dezibel < -45.0)
    return -1;
  return 0;
}

} // namespace fische
