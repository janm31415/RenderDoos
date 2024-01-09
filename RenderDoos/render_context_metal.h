
#pragma once

#include "render_context.h"

#include "metal/Metal.hpp"

#include <stdint.h>
#include <vector>

namespace RenderDoos
  {
  
  #define MAX_PIPELINESTATE_CACHE 1024

  class render_context_metal : public render_context
    {
    public:
      render_context_metal(MTL::Device* device, MTL::Library* library);
      ~render_context_metal();
      
      virtual void frame_begin(render_drawables drawables);
      virtual void frame_end(bool wait_until_completed);
      
      virtual void renderpass_begin(const renderpass_descriptor& descr);
      virtual void renderpass_end();

      virtual int32_t add_texture(int32_t w, int32_t h, int32_t format, const uint16_t* data, int32_t flags);
      virtual bool update_texture(int32_t handle, const uint16_t* data);
      virtual int32_t add_texture(int32_t w, int32_t h, int32_t format, const uint8_t* data, int32_t flags);
      virtual int32_t add_cubemap_texture(int32_t w, int32_t h, int32_t format,
        const uint8_t* front,
        const uint8_t* back,
        const uint8_t* left,
        const uint8_t* right,
        const uint8_t* top,
        const uint8_t* bottom,
        int32_t usage_flags);
      virtual bool update_texture(int32_t handle, const uint8_t* data);
      virtual bool update_texture(int32_t handle, const float* data);
      virtual void remove_texture(int32_t handle);
      virtual void bind_texture_to_channel(int32_t handle, int32_t channel, int32_t flags = TEX_WRAP_REPEAT | TEX_FILTER_LINEAR);
      virtual const texture* get_texture(int32_t handle) const;
      virtual void get_data_from_texture(int32_t handle, void* data, int32_t size);

      virtual int32_t add_geometry(int32_t vertex_declaration_type); // VERTEX_STANDARD or VERTEX_COMPACT
      virtual void remove_geometry(int32_t handle);

      virtual int32_t add_render_buffer();
      virtual void remove_render_buffer(int32_t handle);
      
      virtual int32_t add_buffer_object(const void* data, int32_t size, int32_t buffer_type);
      virtual void remove_buffer_object(int32_t handle);
      virtual void update_buffer_object(int32_t handle, const void* data, int32_t size);
      virtual void bind_buffer_object(int32_t handle, int32_t channel, int32_t target);
      virtual void get_data_from_buffer_object(int32_t handle, void* data, int32_t size);
      virtual const buffer_object* get_buffer_object(int32_t handle) const;
      virtual void copy_buffer_object_data(int32_t source_handle, int32_t destination_handle, uint32_t read_offset, uint32_t write_offset, uint32_t size);
      
      virtual int32_t add_frame_buffer(int32_t w, int32_t h, bool make_depth_texture);
      virtual void remove_frame_buffer(int32_t handle);
      virtual const frame_buffer* get_frame_buffer(int32_t handle) const;

      virtual void geometry_begin(int32_t handle, int32_t number_of_vertices, int32_t number_of_indices, float** vertex_pointer, void** index_pointer, int32_t update = 3);
      virtual void geometry_end(int32_t handle);
      virtual void geometry_draw(int32_t handle, int32_t instance_count, bool enable_depth_testing);

      virtual int32_t add_shader(const char* source, int32_t type, const char* name);
      virtual void remove_shader(int32_t handle);

      virtual int32_t add_program(int32_t vertex_shader_handle, int32_t fragment_shader_handle, int32_t compute_shader_handle);
      virtual void remove_program(int32_t handle);
      virtual void bind_program(int32_t handle);
      
      virtual void dispatch_compute(int32_t num_groups_x, int32_t num_groups_y, int32_t num_groups_z, int32_t local_size_x, int32_t local_size_y, int32_t local_size_z);
      
      virtual void bind_uniform(int32_t program_handle, int32_t uniform_handle);
      
      virtual bool is_initialized() const { return _initialized; }
      
      virtual int32_t add_query();
      virtual void remove_query(int32_t handle);
      virtual void query_timestamp(int32_t handle);
      virtual uint64_t get_query_result(int32_t handle);
      
      virtual void set_blending_enabled(bool enable);
      virtual void set_blending_function(blending_type source, blending_type destination);
      virtual void set_blending_equation(blending_equation_type func);

      virtual void* get_command_buffer();
      
    private:
      void _allocate_geometry_buffer(geometry_ref& ref, int32_t tuple_size, int32_t count, int32_t type, void** pointer);
      void _remove_geometry_buffer(geometry_ref& ref);
      void _update_geometry_buffer(geometry_ref& ref);
      
      MTL::RenderPipelineState* _get_render_pipeline_state(int32_t vertex_shader_handle, int32_t fragment_shader_handle, int32_t color_pixel_format, int32_t depth_pixel_format);
      MTL::ComputePipelineState* _get_compute_pipeline_state(int32_t compute_shader_handle);
      
      int32_t _add_texture(int32_t w, int32_t h, int32_t format, const void* data, int32_t flags, int32_t bytes_per_channel);
      
    private:
      MTL::Device* mp_device;
      MTL::Library* mp_default_library;
      MTL::CommandQueue* mp_command_queue;
      MTL::Drawable* mp_drawable;
      MTL::Texture* mp_screen;
      MTL::CommandBuffer* mp_command_buffer;
      MTL::RenderCommandEncoder* mp_render_command_encoder;
      MTL::ComputeCommandEncoder* mp_compute_command_encoder;
      NS::AutoreleasePool* mp_auto_release_pool;
      dispatch_semaphore_t _semaphore;
      std::vector<uint8_t> _raw_uniforms;
      MTL::DepthStencilState* mp_depth_stencil_state;
      renderpass_descriptor m_current_renderpass_descriptor;
      bool _enable_blending;
      blending_type _blending_source, _blending_destination;
      blending_equation_type _blending_func;

      struct RenderPipelineStateCache
      {
        MTL::RenderPipelineState* p_pipeline;
        int32_t vertex_shader_handle;
        int32_t fragment_shader_handle;
        int32_t color_pixel_format;
        int32_t depth_pixel_format;
      };
      
      RenderPipelineStateCache m_pipeline_state_cache[MAX_PIPELINESTATE_CACHE];
      
      struct ComputePipelineStateCache
      {
        MTL::ComputePipelineState* p_pipeline;
        int32_t compute_shader_handle;
      };
      
      ComputePipelineStateCache m_compute_pipeline_state_cache[MAX_PIPELINESTATE_CACHE];
    };

  }
