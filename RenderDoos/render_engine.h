#pragma once
#include <stdint.h>

#include "float.h"

#include "render_context.h"

namespace RenderDoos
  {

  struct renderer_type
    {
    enum backend
      {
      AUTO,
      OPENGL,
      METAL,
      NONE
      };
    };

  class render_engine
    {
    public:
      render_engine();
      ~render_engine();

      void init(void* device=nullptr, void* library = nullptr, renderer_type::backend vendor = renderer_type::AUTO);
      void destroy();
      
      renderer_type::backend get_renderer_type() const;
      
      void frame_begin(render_drawables drawables); // a frame can consist of multiple render passes
      void frame_end(bool wait_until_completed = false);
      void renderpass_begin(const renderpass_descriptor& descr);
      void renderpass_end();

      int32_t add_texture(int32_t w, int32_t h, int32_t format, const uint16_t* data, int32_t usage_flags = TEX_USAGE_READ | TEX_USAGE_RENDER_TARGET);
      bool update_texture(int32_t handle, const uint16_t* data);
      int32_t add_texture(int32_t w, int32_t h, int32_t format, const uint8_t* data, int32_t usage_flags = TEX_USAGE_READ | TEX_USAGE_RENDER_TARGET);
      bool update_texture(int32_t handle, const uint8_t* data);
      bool update_texture(int32_t handle, const float* data);
      void remove_texture(int32_t handle);
      void bind_texture_to_channel(int32_t handle, int32_t channel, int32_t flags = TEX_WRAP_REPEAT | TEX_FILTER_LINEAR);
      const texture* get_texture(int32_t handle) const;
      void get_data_from_texture(int32_t handle, void* data, int32_t size);

      int32_t add_geometry(int32_t vertex_declaration_type); // VERTEX_STANDARD or VERTEX_COMPACT or VERTEX_COLOR
      void remove_geometry(int32_t handle);

      int32_t add_buffer_object(const void* data, int32_t size, int32_t buffer_type = COMPUTE_BUFFER);
      void remove_buffer_object(int32_t handle);
      void update_buffer_object(int32_t handle, const void* data, int32_t size);
      void bind_buffer_object(int32_t handle, int32_t channel);
      void get_data_from_buffer_object(int32_t handle, void* data, int32_t size);
      const buffer_object* get_buffer_object(int32_t handle) const;
      void copy_buffer_object_data(int32_t source_handle, int32_t destination_handle, uint32_t read_offset, uint32_t write_offset, uint32_t size);

      void dispatch_compute(int32_t num_groups_x, int32_t num_groups_y, int32_t num_groups_z, int32_t local_size_x, int32_t local_size_y, int32_t local_size_z);

      int32_t add_render_buffer();
      void remove_render_buffer(int32_t handle);

      int32_t add_frame_buffer(int32_t w, int32_t h, bool make_depth_texture);
      void remove_frame_buffer(int32_t handle);      
      const frame_buffer* get_frame_buffer(int32_t handle) const;

      void geometry_begin(int32_t handle, int32_t number_of_vertices, int32_t number_of_indices, float** vertex_pointer, void** index_pointer, int32_t update = 3);
      void geometry_end(int32_t handle);
      void geometry_draw(int32_t handle);

      int32_t add_shader(const char* source, int32_t type, const char* name);
      void remove_shader(int32_t handle);

      int32_t add_program(int32_t vertex_shader_handle, int32_t fragment_shader_handle, int32_t compute_shader_handle=-1);
      void remove_program(int32_t handle);
      void bind_program(int32_t handle);      

      int32_t add_uniform(const char* name, uniform_type::type uniform_type, uint16_t num);
      void remove_uniform(int32_t handle);
      void set_uniform(int32_t handle, const void* values);
      void bind_uniform(int32_t program_handle, int32_t uniform_handle);

      int32_t add_query();
      void remove_query(int32_t handle);
      void query_timestamp(int32_t handle);
      uint64_t get_query_result(int32_t handle);


      bool is_initialized() const;

      void set_model_view_properties(const model_view_properties& props);

      const float4x4& get_view_project() const { return _last_view_project; }
      const float4x4& get_camera_space() const { return _last_camera; }
      const float4& get_light_dir() const { return _mv_props.light_dir; }
      const float4& get_light_pos() const { return _mv_props.light_pos; }
      const model_view_properties& get_model_view_properties() const { return _mv_props; }

    private:
      void _check_context();

    private:
      render_context* _context;
      float4x4 _last_projection, _last_camera, _last_view_project;
      model_view_properties _mv_props;
      renderer_type::backend _vendor;
    };


  } // namespace RenderDoos
