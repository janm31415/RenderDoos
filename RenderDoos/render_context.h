#pragma once

#include <stdint.h>
#include <string.h>

#include "float.h"

namespace RenderDoos
  {

#define MAX_TEXTURE 1024
#define MAX_GEOMETRY 1024
#define MAX_BUFFER_OBJECT 2048
#define MAX_SHADER 1024
#define MAX_SHADER_PROGRAM 1024
#define MAX_FRAMEBUFFER 1024
#define MAX_RENDERBUFFER 1024
#define MAX_UNIFORMS 1024
#define MAX_QUERIES 16
#define MAX_TEXSTAGE  16

#define TEX_ALLOCATED 1

#define TEX_WRAP_REPEAT 1
#define TEX_WRAP_CLAMP_TO_EDGE 2
#define TEX_FILTER_NEAREST 4
#define TEX_FILTER_LINEAR 8
#define TEX_FILTER_LINEAR_MIPMAP_LINEAR 16
#define TEX_USAGE_READ 1
#define TEX_USAGE_WRITE 2
#define TEX_USAGE_RENDER_TARGET 4

#define GEOMETRY_ALLOCATED 1

#define GEOMETRY_VERTEX 1
#define GEOMETRY_INDEX 2
#define COMPUTE_BUFFER 3

#define VERTEX_STANDARD  1   // pos,normal,tex0
#define VERTEX_COMPACT   2   // pos,color

#define SHADER_VERTEX 1
#define SHADER_FRAGMENT 2
#define SHADER_COMPUTE 3

#define CLEAR_COLOR 1
#define CLEAR_DEPTH 2

  struct vertex_standard // 32 bytes
    {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
    };

  struct vertex_compact // 16 bytes
    {
    float x, y, z;
    uint32_t c0;
    };

  struct texture
    {
    int32_t w, h;
    int32_t flags;
    int32_t usage_flags;
    int32_t format;
    uint32_t gl_texture_id;
    void* metal_texture;
    };

  struct buffer_object
    {
    int32_t type;       // 0 = unused, 1 = index, 2 = vertex
    int32_t size;       // size of the buffer in bytes    
    uint32_t gl_buffer_id;
    uint8_t* raw;       // cpu memory
    void* metal_buffer;
    };

  struct geometry_ref
    {
    int32_t buffer;    // buffer handle    
    int32_t count;     // number of elements
    };

  struct geometry_handle
    {
    int32_t mode;
    int32_t vertex_size; // size of one vertex in bytes
    int32_t vertex_declaration_type;
    int32_t locked;      // lock is on when user fills data
    uint32_t gl_vertex_array_object_id; // vertex array object
    geometry_ref vertex; // vertex buffer
    geometry_ref index;  // index buffer
    };

  struct query_handle
    {
    int32_t mode; // handle available or not
    uint32_t gl_query_id;
    uint64_t metal_timestamp;
    };

  struct shader
    {
    int32_t type;           // 0 = unused, 1 = vertex, 2 = fragment
    uint32_t gl_shader_id;
    int32_t compiled;
    void* metal_shader;
    const char* name;
    };

  struct shader_program
    {
    int32_t vertex_shader_handle;
    int32_t fragment_shader_handle;
    int32_t compute_shader_handle;
    uint32_t gl_program_id;
    int32_t linked;
    };

  struct render_buffer
    {
    int32_t type; // 0 is unused, 1 is used
    uint32_t gl_render_buffer_id;
    };

  struct frame_buffer
    {
    int32_t texture_handle;
    int32_t render_buffer_handle;
    int32_t depth_texture_handle;
    int32_t w, h;
    uint32_t gl_frame_buffer_id;
    };

  struct uniform_type
    {
    enum type
      {
      sampler=0, 
      vec2=1,
      vec3=2,
      vec4=3,
      uvec2=4,
      uvec3=5,
      uvec4=6,
      mat3=7,
      mat4=8, 
      integer=9,
      real=10
      };
    };

  struct uniform_value
    {
    const char* name;
    uint8_t* raw;       // cpu memory
    int32_t size;       // size of the buffer in bytes
    uint16_t num;
    uniform_type::type uniform_type;
    };

  struct renderpass_descriptor
    {
    int32_t frame_buffer_handle = -1; // -1 for screen
    int32_t frame_buffer_channel; // channel for framebuffer
    int32_t frame_buffer_flags = TEX_WRAP_REPEAT | TEX_FILTER_LINEAR;
    int32_t w = -1;
    int32_t h = -1; // viewport size in case of screen
    int32_t clear_flags = CLEAR_COLOR | CLEAR_DEPTH;
    uint32_t clear_color = 0xff000000;
    float clear_depth = 1.f;
    int32_t depth_texture_handle = -1; // -1 for no depth
    bool compute_shader = false; // set to true for compute shaders
    };
    
  struct render_drawables
    {
    render_drawables() : metal_drawable(nullptr), metal_screen_texture(nullptr) {}
    void* metal_drawable;
    void* metal_screen_texture;
    };

  struct model_view_properties
    {
    float4x4 model_space;          // model in world
    float4x4 camera_space;         // camera in world
    float zoom_x, zoom_y;          // zoom-factors (usually 1.f)
    float center_x, center_y;      // center-offset (usually 0.f)
    float near_clip;               // near clipping plane
    float far_clip;                // far clipping plane
    float4 light_pos;
    float4 light_dir;
    uint32_t light_color;
    int32_t orthogonal;            // no perspective
    uint32_t viewport_width, viewport_height;

    void init(uint32_t vp_width, uint32_t vp_height)
      {
      memset(this, 0, sizeof(*this));

      model_space = get_identity();
      camera_space = get_identity();
      near_clip = 0.125f;
      far_clip = 4096.f;
      zoom_x = 1.f;
      zoom_y = 1.f;
      center_x = 0.f;
      center_y = 0.f;
      viewport_width = vp_width;
      viewport_height = vp_height;
      }
    void make_projection_matrix(float4x4& m) const
      {
      float top = zoom_y * near_clip;
      float right = zoom_x * near_clip;
      float bottom = -top;
      float left = -right;
      if (orthogonal)
        m = orthographic(left, right, top, bottom, near_clip, far_clip);
      else
        m = frustum(left, right, top, bottom, near_clip, far_clip);
      }
    };

  class render_context
    {
    public:
      render_context();
      virtual ~render_context() {}

      void init();
      void destroy();    
      
      virtual void frame_begin(render_drawables drawables) = 0;
      virtual void frame_end(bool wait_until_completed) = 0;
      virtual void renderpass_begin(const renderpass_descriptor& descr) = 0;
      virtual void renderpass_end() = 0;

      virtual int32_t add_texture(int32_t w, int32_t h, int32_t format, const uint16_t* data, int32_t usage_flags) = 0;
      virtual bool update_texture(int32_t handle, const uint16_t* data) = 0;
      virtual int32_t add_texture(int32_t w, int32_t h, int32_t format, const uint8_t* data, int32_t usage_flags) = 0;
      virtual bool update_texture(int32_t handle, const uint8_t* data) = 0;
      virtual void remove_texture(int32_t handle) = 0;
      virtual void bind_texture_to_channel(int32_t handle, int32_t channel, int32_t flags) = 0;
      virtual const texture* get_texture(int32_t handle) const = 0;
      virtual void get_data_from_texture(int32_t handle, void* data, int32_t size) = 0;
      
      virtual int32_t add_geometry(int32_t vertex_declaration_type) = 0; // VERTEX_STANDARD or VERTEX_COMPACT
      virtual void remove_geometry(int32_t handle) = 0;

      virtual int32_t add_render_buffer() = 0;
      virtual void remove_render_buffer(int32_t handle) = 0;

      virtual int32_t add_buffer_object(const void* data, int32_t size) = 0;
      virtual void remove_buffer_object(int32_t handle) = 0;
      virtual void update_buffer_object(int32_t handle, const void* data, int32_t size) = 0;
      virtual void bind_buffer_object(int32_t handle, int32_t channel) = 0;
      virtual void get_data_from_buffer_object(int32_t handle, void* data, int32_t size) = 0;
      virtual const buffer_object* get_buffer_object(int32_t handle) const = 0;
      virtual void copy_buffer_object_data(int32_t source_handle, int32_t destination_handle, uint32_t read_offset, uint32_t write_offset, uint32_t size) = 0;

      virtual void dispatch_compute(int32_t num_groups_x, int32_t num_groups_y, int32_t num_groups_z, int32_t local_size_x, int32_t local_size_y, int32_t local_size_z) = 0;

      virtual int32_t add_frame_buffer(int32_t w, int32_t h, bool make_depth_texture) = 0;
      virtual void remove_frame_buffer(int32_t handle) = 0;      
      virtual const frame_buffer* get_frame_buffer(int32_t handle) const = 0;

      virtual void geometry_begin(int32_t handle, int32_t number_of_vertices, int32_t number_of_indices, float** vertex_pointer, void** index_pointer, int32_t update) = 0;
      virtual void geometry_end(int32_t handle) = 0;
      virtual void geometry_draw(int32_t handle) = 0;

      virtual int32_t add_shader(const char* source, int32_t type, const char* name) = 0;
      virtual void remove_shader(int32_t handle) = 0;

      virtual int32_t add_program(int32_t vertex_shader_handle, int32_t fragment_shader_handle, int32_t compute_shader_handle) = 0;
      virtual void remove_program(int32_t handle) = 0;
      virtual void bind_program(int32_t handle) = 0;
     
      int32_t add_uniform(const char* name, uniform_type::type uniform_type, uint16_t num);
      void remove_uniform(int32_t handle);
      void set_uniform(int32_t handle, const void* values);
      virtual void bind_uniform(int32_t program_handle, int32_t uniform_handle) = 0;

      virtual bool is_initialized() const = 0;

      virtual int32_t add_query() = 0;
      virtual void remove_query(int32_t handle) = 0;
      virtual void query_timestamp(int32_t handle) = 0;
      virtual uint64_t get_query_result(int32_t handle) = 0;

    protected:
      texture _textures[MAX_TEXTURE];
      geometry_handle _geometry_handles[MAX_GEOMETRY];
      buffer_object _buffer_objects[MAX_BUFFER_OBJECT];
      shader _shaders[MAX_SHADER];
      shader_program _shader_programs[MAX_SHADER_PROGRAM];
      render_buffer _render_buffers[MAX_RENDERBUFFER];
      frame_buffer _frame_buffers[MAX_FRAMEBUFFER];
      uniform_value _uniforms[MAX_UNIFORMS];
      query_handle _queries[MAX_QUERIES];
      bool _initialized;
    };

  }
