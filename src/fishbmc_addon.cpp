/*
 *  Copyright (C) 2005-2026 Team Kodi (https://kodi.tv)
 *  Copyright (C) 2012 Marcel Ebmer
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#define _USE_MATH_DEFINES

#include "fishbmc_addon.h"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <kodi/General.h>
#include <sstream>
#include <string>
#include <sys/stat.h>

CVisualizationFishBMC::CVisualizationFishBMC()
{
  m_aspect = float(Width()) / float(Height());
  m_texleft = (2 - m_aspect) / 4;
  m_texright = 1 - m_texleft;
  m_filemode = kodi::addon::GetSettingBoolean("filemode");

  int detail = kodi::addon::GetSettingInt("detail");
  m_size = 128;
  while (detail--)
  {
    m_size *= 2;
  }

  int divisor = kodi::addon::GetSettingInt("divisor");
  m_framedivisor = 8;
  while (divisor--)
  {
    m_framedivisor /= 2;
  }

  // coordinate system:
  //     screen top left: (-1, -1)
  //     screen bottom right: (1, 1)
  //     screen depth clipping: 3 to 15
  m_projMatrix = glm::frustum(-1.0f, 1.0f, 1.0f, -1.0f, 3.0f, 15.0f);

  CFische::SetAudioFormat(FISCHE_AUDIOFORMAT_FLOAT);
  CFische::SetPixelFormat(FISCHE_PIXELFORMAT_0xAABBGGRR);
  CFische::SetLineStyle(FISCHE_LINESTYLE_THICK);
  CFische::SetNervousMode(kodi::addon::GetSettingBoolean("nervous") ? 1 : 0);
  CFische::SetVectorStoreLoadUsage(m_filemode);
}

bool CVisualizationFishBMC::AudioStart(int channels, int samplesPerSec, int bitsPerSample)
{
  m_errorstate = false;

  CFische::SetAudioFormat(FISCHE_AUDIOFORMAT_FLOAT);
  CFische::SetWidthHeight(m_size * 2, m_size);

  if (!m_filemode)
  {
    DeleteVectors();
  }

  if (!CFische::Start())
  {
    kodi::Log(ADDON_LOG_ERROR, "fische failed to start: %s", CFische::GetErrorText().c_str());
    m_errorstate = true;
    return false;
  }

  uint32_t* pixels = CFische::Render();

  if (!m_shaderLoaded)
  {
    std::string fraqShader =
        kodi::addon::GetAddonPath("resources/shaders/" GL_TYPE_STRING "/frag.glsl");
    std::string vertShader =
        kodi::addon::GetAddonPath("resources/shaders/" GL_TYPE_STRING "/vert.glsl");
    if (!LoadShaderFiles(vertShader, fraqShader) || !CompileAndLink())
      return false;

    m_shaderLoaded = true;
  }

#ifdef HAS_GL
  glGenBuffers(2, m_vertexVBO);
  glGenBuffers(1, &m_indexVBO);
#endif

  // generate a texture for drawing into
  glGenTextures(1, &m_texture);
  glBindTexture(GL_TEXTURE_2D, m_texture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CFische::GetWidth(), CFische::GetHeight(), 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixels);

  m_isrotating = false;
  m_angle = 0;
  m_lastangle = 0;
  m_angleincrement = 0;
  m_startOK = true;
  return true;
}

void CVisualizationFishBMC::AudioStop()
{
  if (!m_startOK)
    return;

  glDeleteTextures(1, &m_texture);

#ifdef HAS_GL
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(2, m_vertexVBO);
  m_vertexVBO[0] = 0;
  m_vertexVBO[1] = 0;

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &m_indexVBO);
  m_indexVBO = 0;
#endif
}

void CVisualizationFishBMC::AudioData(const float* pAudioData, size_t iAudioDataLength)
{
  if (!m_startOK)
    return;

  CFische::AudioData(pAudioData, iAudioDataLength * 4);
}

void CVisualizationFishBMC::Render()
{
  static int frame = 0;

  if (!m_startOK)
    return;

  // check if this frame is to be skipped
  if (++frame % m_framedivisor == 0)
  {
    uint32_t* pixels = CFische::Render();
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CFische::GetWidth(), CFische::GetHeight(), GL_RGBA,
                    GL_UNSIGNED_BYTE, pixels);
    if (m_isrotating)
      m_angle += m_angleincrement;
  }

  // stop rotation if required
  if (m_isrotating)
  {
    if (m_angle - m_lastangle > 180)
    {
      m_lastangle = m_lastangle ? 0.0f : 180.0f;
      m_angle = m_lastangle;
      m_isrotating = false;
    }
  }

  // how many quads will there be?
  int n_Y = 8;
  int n_X = int(m_aspect * 8.0f + 0.5f);

  // one-time initialization of rotation axis array
  if (m_axis.empty())
  {
    m_axis.resize(n_X * n_Y);
    for (int i = 0; i < n_X * n_Y; ++i)
    {
      m_axis[i] = rand() % 2;
    }
  }

  start_render();

  // loop over and draw all quads
  int quad_count = 0;
  float quad_width = 4.0f / n_X;
  float quad_height = 4.0f / n_Y;
  float tex_width = (m_texright - m_texleft);

  for (float X = 0; X < n_X; X += 1)
  {
    for (float Y = 0; Y < n_Y; Y += 1)
    {
      float center_x = -2 + (X + 0.5f) * 4 / n_X;
      float center_y = -2 + (Y + 0.5f) * 4 / n_Y;
      float tex_left = m_texleft + tex_width * X / n_X;
      float tex_right = m_texleft + tex_width * (X + 1) / n_X;
      float tex_top = Y / n_Y;
      float tex_bottom = (Y + 1) / n_Y;
      float angle = (m_angle - m_lastangle) * 4 - (X + Y * n_X) / (n_X * n_Y) * 360;
      if (angle < 0)
        angle = 0;
      if (angle > 360)
        angle = 360;

      textured_quad(center_x, center_y, angle, m_axis[quad_count++], quad_width, quad_height,
                    tex_left, tex_right, tex_top, tex_bottom);
    }
  }

  finish_render();
}

ADDON_STATUS CVisualizationFishBMC::SetSetting(const std::string& settingName,
                                               const kodi::addon::CSettingValue& settingValue)
{
  if (settingName.empty() || settingValue.empty())
    return ADDON_STATUS_UNKNOWN;

  if (settingName == "nervous")
    CFische::SetNervousMode(settingValue.GetBoolean());
  else if (settingName == "filemode")
    m_filemode = settingValue.GetBoolean();
  else if (settingName == "detail")
  {
    int detail = settingValue.GetInt();
    m_size = 128;
    while (detail--)
    {
      m_size *= 2;
    }
  }
  else if (settingName == "divisor")
  {
    int divisor = settingValue.GetInt();
    m_framedivisor = 8;
    while (divisor--)
    {
      m_framedivisor /= 2;
    }
  }

  return ADDON_STATUS_OK;
}

void CVisualizationFishBMC::OnCompiledAndLinked()
{
  // Variables passed directly to the Vertex shader
  m_uProjMatrixLoc = glGetUniformLocation(ProgramHandle(), "u_projectionMatrix");
  m_uModelViewMatrixLoc = glGetUniformLocation(ProgramHandle(), "u_modelViewMatrix");

  m_aVertexLoc = glGetAttribLocation(ProgramHandle(), "a_pos");
  m_aCoordLoc = glGetAttribLocation(ProgramHandle(), "a_coord");
}

bool CVisualizationFishBMC::OnEnabled()
{
  glUniformMatrix4fv(m_uProjMatrixLoc, 1, GL_FALSE, glm::value_ptr(m_projMatrix));
  glUniformMatrix4fv(m_uModelViewMatrixLoc, 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
  return true;
}

void CVisualizationFishBMC::OnBeat(double frames_per_beat)
{
  if (!m_isrotating)
  {
    m_isrotating = true;
    if (frames_per_beat < 1)
      frames_per_beat = 12.0;
    m_angleincrement = float(180.0f / 4.0f / frames_per_beat);
  }
}

// OpenGL: paint a textured quad
void CVisualizationFishBMC::textured_quad(float center_x,
                                          float center_y,
                                          float angle,
                                          float axis,
                                          float width,
                                          float height,
                                          float tex_left,
                                          float tex_right,
                                          float tex_top,
                                          float tex_bottom)
{
  float scale = 1 - sinf(angle / 360.0f * float(M_PI)) / 3;

  glm::mat4 modelMatrixOld = m_modelMatrix;
  m_modelMatrix = glm::translate(m_modelMatrix, glm::vec3(center_x, center_y, 0));
  m_modelMatrix = glm::rotate(m_modelMatrix, angle, glm::vec3(axis, 1 - axis, 0.0f));
  m_modelMatrix = glm::scale(m_modelMatrix, glm::vec3(scale, scale, scale));

  m_coord[0] = sCoord(tex_left, tex_top);
  m_vertex[0] = sPosition(-width / 2, -height / 2, 0);

  m_coord[1] = sCoord(tex_right, tex_top);
  m_vertex[1] = sPosition(width / 2, -height / 2, 0);

  m_coord[2] = sCoord(tex_right, tex_bottom);
  m_vertex[2] = sPosition(width / 2, height / 2, 0);

  m_coord[3] = sCoord(tex_left, tex_bottom);
  m_vertex[3] = sPosition(-width / 2, height / 2, 0);

  EnableShader();
#ifdef HAS_GL
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO[0]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(sPosition) * 4, m_vertex, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO[1]);
  glBufferData(GL_ARRAY_BUFFER, sizeof(sCoord) * 4, m_coord, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 4, m_indexer, GL_STATIC_DRAW);
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0);
#else
  glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, m_indexer);
#endif
  DisableShader();

  m_modelMatrix = modelMatrixOld;
}

// OpenGL: setup to start rendering
void CVisualizationFishBMC::start_render()
{
#ifdef HAS_GL
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexVBO);

  glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO[0]);
  glVertexAttribPointer(m_aVertexLoc, 4, GL_FLOAT, GL_TRUE, sizeof(sPosition), nullptr);
  glEnableVertexAttribArray(m_aVertexLoc);

  glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO[1]);
  glVertexAttribPointer(m_aCoordLoc, 2, GL_FLOAT, GL_TRUE, sizeof(sCoord), nullptr);
  glEnableVertexAttribArray(m_aCoordLoc);
#else
  glVertexAttribPointer(m_aVertexLoc, 4, GL_FLOAT, GL_FALSE, 0, m_vertex);
  glEnableVertexAttribArray(m_aVertexLoc);

  glVertexAttribPointer(m_aCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, m_coord);
  glEnableVertexAttribArray(m_aCoordLoc);
#endif

  // enable blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // disable depth testing
  glDisable(GL_DEPTH_TEST);

#ifdef HAS_GL
  // paint both sides of polygons
  glPolygonMode(GL_FRONT, GL_FILL);
#endif

  // bind global texture
  glBindTexture(GL_TEXTURE_2D, m_texture);

  m_modelMatrix =
      glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -6.0f)); // move 6 units into the screen
  m_modelMatrix = glm::rotate(m_modelMatrix, m_angle, glm::vec3(0.0f, 1.0f, 0.0f)); // rotate
}

// OpenGL: done rendering
void CVisualizationFishBMC::finish_render()
{
  glDisableVertexAttribArray(m_aCoordLoc);
  glDisableVertexAttribArray(m_aVertexLoc);
}

void CVisualizationFishBMC::WriteVectors(const void* data, size_t bytes)
{
  if (!data)
    return;

  kodi::vfs::CreateDirectory(kodi::addon::GetUserPath("data"));

  const std::string filename =
      kodi::addon::GetUserPath("data/vector-" + std::to_string(CFische::GetHeight()) + "px");

  kodi::vfs::CFile vectorsfile;

  // open the file
  if (!vectorsfile.OpenFileForWrite(filename, true))
  {
    kodi::Log(ADDON_LOG_ERROR, "File write \"%s\": Failed to open file", filename.c_str());
    return;
  }

  // write it
  ssize_t size = vectorsfile.Write(data, bytes);
  if (size < 0 || bytes != size)
  {
    kodi::Log(ADDON_LOG_ERROR, "File write \"%s\": Size mismatch of writed file", filename.c_str());
    return;
  }
}

size_t CVisualizationFishBMC::ReadVectors(void** data)
{
  const std::string filename =
      kodi::addon::GetUserPath("data/vector-" + std::to_string(CFische::GetHeight()) + "px");
  if (!kodi::vfs::FileExists(filename))
    return 0;

  kodi::vfs::CFile vectorsfile;

  // open the file
  if (!vectorsfile.OpenFile(filename))
  {
    kodi::Log(ADDON_LOG_ERROR, "File read \"%s\": Failed to open file", filename);
    return 0;
  }

  ssize_t size = vectorsfile.GetLength();
  if (size <= 0)
  {
    kodi::Log(ADDON_LOG_ERROR, "File read \"%s\": Unable to get size of file", filename);
    return 0;
  }

  *data = malloc(size_t(size));
  ssize_t sizeRead = vectorsfile.Read(*data, size_t(size));
  if (sizeRead < 0 || sizeRead != size)
  {
    kodi::Log(ADDON_LOG_ERROR, "File read \"%s\": Size mismatch of readed file", filename);
    free(*data);
    return 0;
  }

  return sizeRead;
}

void CVisualizationFishBMC::DeleteVectors()
{
  const std::string dirname = kodi::addon::GetUserPath("data");
  if (!kodi::vfs::DirectoryExists(dirname))
    return;

  for (int i = 64; i <= 2048; i *= 2)
  {
    const std::string filename = dirname + "/vector-" + std::to_string(i) + "px";
    kodi::vfs::DeleteFile(filename);
  }
}

ADDONCREATOR(CVisualizationFishBMC) // Don't touch this!
