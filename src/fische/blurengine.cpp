/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "blurengine.hpp"

#include "fische.hpp"
#include "screenbuffer.hpp"

#include <chrono>

namespace fische
{

CBlurEngine::CBlurEngine(const CFische* parent)
  : m_fische(parent),
    m_width(parent->GetWidth()),
    m_height(parent->GetHeight()),
    m_threads(parent->GetUsedCPUs())
{
  m_sourcebuffer = m_fische->GetScreenbuffer()->Pixels();
  m_destinationbuffer = new uint32_t[m_width * m_height]();

  for (uint_fast8_t i = 0; i < m_threads; ++i)
  {
    m_worker[i].vectors = nullptr;
    m_worker[i].y_start = (i * m_height) / m_threads;
    m_worker[i].y_end = ((i + 1) * m_height) / m_threads;
    m_worker[i].kill = false;
    m_worker[i].work = false;
    m_worker[i].thread = new std::thread(&CBlurEngine::ThreadWorker, this, &m_worker[i]);
  }
}

CBlurEngine::~CBlurEngine()
{
  for (uint_fast8_t i = 0; i < m_threads; ++i)
  {
    m_worker[i].kill = true;
    m_worker[i].thread->join();
    delete m_worker[i].thread;
  }

  delete[] m_destinationbuffer;
}

void CBlurEngine::Blur(const uint16_t* vectors)
{
  for (uint_fast8_t i = 0; i < m_threads; ++i)
  {
    m_worker[i].vectors = vectors;
    m_worker[i].work = true;
  }
}

void CBlurEngine::SwapBuffers()
{
  // wait for all workers to finish
  bool work = true;

  while (work)
  {
    work = false;
    for (uint_fast8_t i = 0; i < m_threads; ++i)
    {
      work |= m_worker[i].work;
    }

    if (work)
      std::this_thread::sleep_for(std::chrono::microseconds(1));
  }

  uint32_t* t = m_destinationbuffer;
  m_destinationbuffer = m_sourcebuffer;
  m_sourcebuffer = t;
  m_fische->GetScreenbuffer()->SetPixels(t);
}

void CBlurEngine::ThreadWorker(_blurworker_* params)
{
  const uint32_t width_x2 = 2 * m_width;
  const uint32_t y_start = params->y_start;
  const uint32_t y_end = params->y_end;

  uint32_t source_component[4];

  const uint32_t two_lines = 2 * m_width;
  const uint32_t one_line = m_width;
  const uint32_t two_columns = 2;

  uint32_t x, y;
  int_fast8_t vector_x, vector_y;

  while (!params->kill)
  {
    if (!params->work)
    {
      std::this_thread::sleep_for(std::chrono::microseconds(1));
      continue;
    }

    uint32_t* source = m_sourcebuffer;
    uint32_t* source_pixel;

    uint32_t* destination_pixel = m_destinationbuffer + y_start * m_width;

    const uint8_t* vectors = reinterpret_cast<const uint8_t*>(params->vectors);
    const uint8_t* vector_pointer = vectors + y_start * width_x2;

    // vertical loop
    for (y = y_start; y < y_end; y++)
    {
      // horizontal loop
      for (x = 0; x < m_width; x++)
      {
        if (params->kill)
          return;

        // read the motion vector (actually its opposite)
        vector_x = *(vector_pointer + 0);
        vector_y = *(vector_pointer + 1);

        // point to the pixel at [present + motion vector]
        source_pixel = source + (y + vector_y) * m_width + x + vector_x;

        // read the pixels at [source + (2,1)]   [source + (-2,1)]   [source + (0,-2)]
        // shift them right by 2 and remove the bits that overflow each byte
        source_component[0] = (*(source_pixel + one_line - two_columns) >> 2) & 0x3f3f3f3f;
        source_component[1] = (*(source_pixel + one_line + two_columns) >> 2) & 0x3f3f3f3f;
        source_component[2] = (*(source_pixel - two_lines) >> 2) & 0x3f3f3f3f;
        source_component[3] = (*(source_pixel) >> 2) & 0x3f3f3f3f;

        // add those four components and write to the destination
        // increment destination pointer
        *(destination_pixel++) =
            source_component[0] + source_component[1] + source_component[2] + source_component[3];

        // increment vector source pointer
        vector_pointer += 2;
      }
    }

    // mark work as done
    params->work = false;
  }
}

} // namespace fische
