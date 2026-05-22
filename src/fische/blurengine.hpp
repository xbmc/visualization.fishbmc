/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <thread>

namespace fische
{

class CFische;

class CBlurEngine
{
public:
  CBlurEngine(const CFische* parent);
  ~CBlurEngine();

  void Blur(const uint16_t* vectors);
  void SwapBuffers();

private:
  const CFische* m_fische;
  const uint_fast16_t m_width;
  const uint_fast16_t m_height;
  const uint_fast8_t m_threads;

  uint32_t* m_sourcebuffer;
  uint32_t* m_destinationbuffer;

  struct _blurworker_
  {
    std::thread* thread;
    uint_fast16_t y_start;
    uint_fast16_t y_end;
    const uint16_t* vectors;
    std::atomic<bool> work;
    std::atomic<bool> kill;
  };

  void ThreadWorker(_blurworker_* params);

  std::array<_blurworker_, 8> m_worker;
};

} // namespace fische
