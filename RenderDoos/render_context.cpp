#include "render_context.h"

#include <cassert>

namespace RenderDoos
  {

  namespace
    {
    struct uniform_declaration
      {
      uniform_type::type uniform_type;
      uint32_t size_of_entry;
      uint32_t number_of_entries;
      };

    static uniform_declaration uniform_type_to_declaration[] =
      {
          { uniform_type::sampler, sizeof(int32_t), 1},
          { uniform_type::vec2, sizeof(float), 2},
          { uniform_type::vec3, sizeof(float), 3},
          { uniform_type::vec4, sizeof(float), 4},
          { uniform_type::uvec2, sizeof(int32_t), 2},
          { uniform_type::uvec3, sizeof(int32_t), 3},
          { uniform_type::uvec4, sizeof(int32_t), 4},
          { uniform_type::mat3, sizeof(float), 9},
          { uniform_type::mat4, sizeof(float), 16},
          { uniform_type::integer, sizeof(int32_t), 1},
          { uniform_type::real, sizeof(float), 1}
      };
    }

  render_context::render_context() : _initialized(false)
    {
    }

  void render_context::init()
    {
    for (int i = 0; i < MAX_TEXTURE; ++i)
      {
      _textures[i].flags = 0;
      _textures[i].metal_texture = 0;
      }
    for (int i = 0; i < MAX_GEOMETRY; ++i)
      {
      _geometry_handles[i].mode = 0;
      }
    for (int i = 0; i < MAX_BUFFER_OBJECT; ++i)
      {
      _buffer_objects[i].type = 0;
      _buffer_objects[i].size = 0;
      _buffer_objects[i].raw = nullptr;
      _buffer_objects[i].metal_buffer = 0;
      }
    for (int i = 0; i < MAX_SHADER; ++i)
      {
      _shaders[i].type = 0;
      _shaders[i].compiled = 0;
      _shaders[i].metal_shader = 0;
      _shaders[i].name = nullptr;
      }
    for (int i = 0; i < MAX_SHADER_PROGRAM; ++i)
      {
      _shader_programs[i].linked = 0;
      _shader_programs[i].vertex_shader_handle = -1;
      _shader_programs[i].fragment_shader_handle = -1;
      _shader_programs[i].compute_shader_handle = -1;
      }
    for (int i = 0; i < MAX_RENDERBUFFER; ++i)
      {
      _render_buffers[i].type = 0;
      }
    for (int i = 0; i < MAX_FRAMEBUFFER; ++i)
      {
      _frame_buffers[i].render_buffer_handle = -1;
      _frame_buffers[i].texture_handle = -1;
      _frame_buffers[i].depth_texture_handle = -1;
      }
    for (int i = 0; i < MAX_UNIFORMS; ++i)
      {
      _uniforms[i].name = nullptr;
      _uniforms[i].num = 0;
      _uniforms[i].raw = nullptr;
      _uniforms[i].size = 0;
      }
    for (int i = 0; i < MAX_QUERIES; ++i)
      {
      _queries[i].mode = 0;
      }
    _initialized = true;
    }

  void render_context::destroy()
    {
    for (int i = 0; i < MAX_TEXTURE; ++i)
      {
      remove_texture(i);
      }
    for (int i = 0; i < MAX_GEOMETRY; ++i)
      {
      remove_geometry(i);
      }
    for (int i = 0; i < MAX_FRAMEBUFFER; ++i)
      {
      remove_frame_buffer(i);
      }
    for (int i = 0; i < MAX_RENDERBUFFER; ++i)
      {
      remove_render_buffer(i);
      }
    for (int i = 0; i < MAX_SHADER_PROGRAM; ++i)
      {
      remove_program(i);
      }
    for (int i = 0; i < MAX_SHADER; ++i)
      {
      remove_shader(i);
      }
    for (int i = 0; i < MAX_UNIFORMS; ++i)
      {
      remove_uniform(i);
      }
    for (int i = 0; i < MAX_BUFFER_OBJECT; ++i)
      {
      remove_buffer_object(i);
      }
    for (int i = 0; i < MAX_QUERIES; ++i)
      {
      remove_query(i);
      }
    _initialized = false;
    }

  void render_context::remove_uniform(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_UNIFORMS)
      return;
    uniform_value* uni = &_uniforms[handle];
    if (uni->num == 0)
      return;
    delete[] uni->raw;
    uni->raw = nullptr;
    uni->size = 0;
    uni->num = 0;
    uni->name = nullptr;
    }

  void render_context::set_uniform(int32_t handle, const void* values)
    {
    if (handle < 0 || handle >= MAX_UNIFORMS)
      return;
    uniform_value* uni = &_uniforms[handle];
    memcpy(uni->raw, values, uni->size);
    }

  int32_t render_context::add_uniform(const char* name, uniform_type::type uniform_type, uint16_t num)
    {
    if (num <= 0)
      return -1;
    uint32_t hash = 2166136261;
    const char* s = name;
    while (*s)
      hash = (hash ^ (uint32_t)(*s++)) * 16777619;
    uint32_t bucket = hash % MAX_UNIFORMS;
    uniform_value* uni = &(_uniforms[bucket]);
    for (int32_t i = 0; i < MAX_UNIFORMS; ++i)
      {
      if (uni->num == 0 || strcmp(uni->name, name) == 0)
        {
        if (uni->num != 0)
          remove_uniform(bucket);
        uni->num = num;
        uni->name = name;
        uni->uniform_type = uniform_type;
        const auto& decl = uniform_type_to_declaration[uniform_type];
        assert(decl.uniform_type == uniform_type);
        uni->size = decl.number_of_entries * decl.size_of_entry * num;
        uni->raw = new uint8_t[uni->size];
        return bucket;
        }
      bucket = (bucket + 1) % MAX_UNIFORMS;
      uni = &(_uniforms[bucket]);
      }
    return -1;
    }
  }
