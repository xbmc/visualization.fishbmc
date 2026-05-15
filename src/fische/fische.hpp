/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include "fische.h"

#include <memory>
#include <cstdint>
#include <string>

namespace fische
{

class CScreenBuffer;
class CWavePainter;
class CAnalyst;
class CBlurEngine;
class CVectorField;
class CAudioBuffer;

class CFische : public FISCHE
{
public:
  /* creates a new FISCHE object
   * and initialzes it with default values */
  CFische();
  ~CFische();

  /* starts FISCHE */
  bool Start();

  /* makes the next frame available */
  uint32_t* Render();

  /* inserts audio data */
  void AudioData(const void* data, size_t data_size);

  bool UseVectorStoreLoadUsage() const { return vector_store_load_usage; }
  void SetVectorStoreLoadUsage(bool store_and_load)
  {
    vector_store_load_usage = store_and_load ? 1 : 0;
  }

  /* if non-NULL,
   * fische calls this to read vector fields from an external source
   * takes a void** for data placement
   * returns the number of bytes read */
  virtual size_t ReadVectors(void** data)
  {
    if (read_vectors)
      return read_vectors(handler, data);
    return 0;
  }

  /* if non-NULL,
   * fische calls this to write vector field data to an external sink
   * takes a void* and the number of bytes to be written */
  virtual void WriteVectors(const void* data, size_t bytes)
  {
    if (write_vectors && vector_store_load_usage)
      write_vectors(handler, data, bytes);
  }

  /* if non-NULL,
   * fische calls this on major beats that are not handled internally
   * takes frames per beat */
  virtual void OnBeat(double frames_per_beat)
  {
    if (on_beat && vector_store_load_usage)
      on_beat(handler, frames_per_beat);
  }

  inline uint32_t GetFrameCounter() const { return frame_counter; }
  inline uint16_t GetWidth() const { return width; }
  inline uint16_t GetHeight() const { return height; }
  inline uint8_t GetUsedCPUs() const { return used_cpus; }
  inline double GetScale() const { return scale; }
  inline FISCHE_PIXELFORMAT GetPixelFormat() const { return pixel_format; }
  inline FISCHE_BLUR GetBlurMode() const { return blur_mode; }
  inline FISCHE_LINESTYLE GetLineStyle() const { return line_style; }
  inline double GetAmplification() const { return amplification; }
  CScreenBuffer* GetScreenbuffer() const { return m_screenbuffer.get(); }

  bool SetWidthHeight(uint16_t width, uint16_t height);
  bool SetUsedCPUs(uint8_t used_cpus);
  bool SetNervousMode(bool nervous_mode);
  bool SetAudioFormat(FISCHE_AUDIOFORMAT audio_format);
  bool SetPixelFormat(FISCHE_PIXELFORMAT pixel_format);
  bool SetBlurMode(FISCHE_BLUR blur_mode);
  bool SetLineStyle(FISCHE_LINESTYLE line_style);
  bool SetScale(double scale);
  bool SetAmplification(double amplification);

  std::string GetErrorText() const { return error_text; }

private:
  void ThreadCreateVectors();
  void ThreadIndicateBusy();

  std::unique_ptr<fische::CScreenBuffer> m_screenbuffer;
  std::unique_ptr<fische::CWavePainter> m_wavepainter;
  std::unique_ptr<fische::CAnalyst> m_analyst;
  std::unique_ptr<fische::CBlurEngine> m_blurengine;
  std::unique_ptr<fische::CVectorField> m_vectorfield;
  std::unique_ptr<fische::CAudioBuffer> m_audiobuffer;
  double m_init_progress;
  bool m_init_cancel;
  bool m_audio_valid{false};
};

} // namespace fische
