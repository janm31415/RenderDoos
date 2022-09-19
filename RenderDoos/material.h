#pragma once

#include <stdint.h>
#include <string>

namespace RenderDoos
  {
  class render_engine;  

  class material
    {
    public:
      virtual ~material() {}

      virtual void compile(render_engine* engine) = 0;
      virtual void bind(render_engine* engine) = 0;
      virtual void destroy(render_engine* engine) = 0;
    };

  class compact_material : public material
    {
    public:
      compact_material();
      virtual ~compact_material();
      virtual void compile(render_engine* engine);
      virtual void bind(render_engine* engine);
      virtual void destroy(render_engine* engine);
      
    private:
      int32_t vs_handle, fs_handle;
      int32_t shader_program_handle;
      int32_t vp_handle;
    };

  class vertex_colored_material : public material
    {
    public:
      vertex_colored_material();
      virtual ~vertex_colored_material();

      void set_ambient(float a);

      virtual void compile(render_engine* engine);
      virtual void bind(render_engine* engine);
      virtual void destroy(render_engine* engine);

    private:
      int32_t vs_handle, fs_handle;
      int32_t shader_program_handle;
      float ambient;
      int32_t texture_flags;
      int32_t vp_handle, cam_handle, light_dir_handle, ambient_handle; // uniforms
    };

  class simple_material : public material
    {
    public:
      simple_material();
      virtual ~simple_material();

      void set_texture(int32_t handle, int32_t flags);
      void set_color(uint32_t clr);
      void set_ambient(float a);

      virtual void compile(render_engine* engine);
      virtual void bind(render_engine* engine);
      virtual void destroy(render_engine* engine);

    private:
      int32_t vs_handle, fs_handle;
      int32_t shader_program_handle;
      int32_t tex_handle, dummy_tex_handle;
      uint32_t color; // if no texture is set
      float ambient;
      int32_t texture_flags;
      int32_t vp_handle, cam_handle, light_dir_handle, tex_sample_handle, ambient_handle, color_handle, tex0_handle; // uniforms
    };

  class shadertoy_material : public material
    {
    public:
      struct properties
        {
        float time;
        float time_delta;
        int frame;
        };

      shadertoy_material();
      virtual ~shadertoy_material();

      void set_script(const std::string& script);
      void set_shadertoy_properties(const properties& props);

      virtual void compile(render_engine* engine);
      virtual void bind(render_engine* engine);
      virtual void destroy(render_engine* engine);
      
    private:
      int32_t vs_handle, fs_handle;
      int32_t shader_program_handle;
      std::string _script;
      properties _props;
      int32_t vp_handle, res_handle, time_handle, time_delta_handle, frame_handle;
    };

  }
