#pragma once

#include <stdint.h>

#include <string>

namespace RenderDoos
  {

  enum e_texture_format
    {
    texture_format_none = 0,
    texture_format_rgba8 = 1,
    texture_format_rgba32f = 2,
    texture_format_bgra8 = 3,
    texture_format_rgba8ui = 4,
    texture_format_r32ui = 5,
    texture_format_r32i = 6,
    texture_format_r32f = 7,
    texture_format_r8ui = 8,
    texture_format_r8i = 9,
    texture_format_rgba16 = 10,
    texture_format_depth = 11
    };

  } // namespace stacklab
