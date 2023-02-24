#include "render_engine.h"
#include <string>
#include <sstream>
#include <stdexcept>

#include "types.h"
#ifdef RENDERDOOS_OPENGL
#include "render_context_gl.h"
#endif
#ifdef RENDERDOOS_METAL
#include "render_context_metal.h"
#endif
#include <cassert>

namespace RenderDoos
  {

  render_engine::render_engine() : _context(nullptr)
    {
    }

  render_engine::~render_engine()
    {
    }

  void render_engine::_check_context()
    {
    if (_context == nullptr)
      throw std::runtime_error("No render context!");
    }

  void render_engine::init(void* device, void* library, renderer_type::backend vendor)
    {
    _vendor = renderer_type::NONE;
    switch (vendor)
      {
      case renderer_type::AUTO:
#if defined(RENDERDOOS_METAL)
        _context = new render_context_metal((MTL::Device*)device, (MTL::Library*)library);
        _vendor = renderer_type::METAL;
#elif defined(RENDERDOOS_OPENGL)
        (void)device;
        (void)library;
        _context = new render_context_gl();
        _vendor = renderer_type::OPENGL;
#endif
        break;
      case renderer_type::METAL:
#if defined(RENDERDOOS_METAL)
        _context = new render_context_metal((MTL::Device*)device, (MTL::Library*)library);
        _vendor = renderer_type::METAL;
#endif
        break;
      case renderer_type::OPENGL:
#if defined(RENDERDOOS_OPENGL)
        _context = new render_context_gl();
        _vendor = renderer_type::OPENGL;
#endif
        break;
      default:
        break;
      }

    _check_context();
    _context->init();
    }

  void render_engine::destroy()
    {
    _context->destroy();
    delete _context;
    _context = nullptr;
    }

  renderer_type::backend render_engine::get_renderer_type() const
    {
    return _vendor;
    }

  void render_engine::frame_begin(render_drawables drawables)
    {
    _context->frame_begin(drawables);
    }

  void render_engine::frame_end(bool wait_until_completed)
    {
    _context->frame_end(wait_until_completed);
    }

  void render_engine::renderpass_begin(const renderpass_descriptor& descr)
    {
    _context->renderpass_begin(descr);
    }

  void render_engine::renderpass_end()
    {
    _context->renderpass_end();
    }

  bool render_engine::update_texture(int32_t handle, const uint16_t* data)
    {
    return _context->update_texture(handle, data);
    }

  bool render_engine::update_texture(int32_t handle, const float* data)
    {
    return _context->update_texture(handle, data);
    }

  int32_t render_engine::add_texture(int32_t w, int32_t h, int32_t format, const uint16_t* data, int32_t usage_flags)
    {
    return _context->add_texture(w, h, format, data, usage_flags);
    }

  bool render_engine::update_texture(int32_t handle, const uint8_t* data)
    {
    return _context->update_texture(handle, data);
    }

  int32_t render_engine::add_texture(int32_t w, int32_t h, int32_t format, const uint8_t* data, int32_t usage_flags)
    {
    return _context->add_texture(w, h, format, data, usage_flags);
    }

  int32_t render_engine::add_cubemap_texture(int32_t w, int32_t h, int32_t format,
    const uint8_t* front,
    const uint8_t* back,
    const uint8_t* left,
    const uint8_t* right,
    const uint8_t* top,
    const uint8_t* bottom,
    int32_t usage_flags)
    {
    return _context->add_cubemap_texture(w, h, format, front, back, left, right, top, bottom, usage_flags);
    }
    
  void render_engine::remove_texture(int32_t handle)
    {
    _context->remove_texture(handle);
    }

  const texture* render_engine::get_texture(int32_t handle) const
    {
    return _context->get_texture(handle);
    }

  void render_engine::get_data_from_texture(int32_t handle, void* data, int32_t size)
    {
    _context->get_data_from_texture(handle, data, size);
    }

  void render_engine::bind_texture_to_channel(int32_t handle, int32_t channel, int32_t flags)
    {
    _context->bind_texture_to_channel(handle, channel, flags);
    }

  int32_t render_engine::add_geometry(int32_t vertex_declaration_type)
    {
    return _context->add_geometry(vertex_declaration_type);
    }

  void render_engine::remove_geometry(int32_t handle)
    {
    _context->remove_geometry(handle);
    }

  int32_t render_engine::add_buffer_object(const void* data, int32_t size, int32_t buffer_type)
    {
    return _context->add_buffer_object(data, size, buffer_type);
    }

  void render_engine::remove_buffer_object(int32_t handle)
    {
    _context->remove_buffer_object(handle);
    }

  void render_engine::copy_buffer_object_data(int32_t source_handle, int32_t destination_handle, uint32_t read_offset, uint32_t write_offset, uint32_t size)
    {
    _context->copy_buffer_object_data(source_handle, destination_handle, read_offset, write_offset, size);
    }

  const buffer_object* render_engine::get_buffer_object(int32_t handle) const
    {
    return _context->get_buffer_object(handle);
    }

  void render_engine::update_buffer_object(int32_t handle, const void* data, int32_t size)
    {
    _context->update_buffer_object(handle, data, size);
    }

  void render_engine::bind_buffer_object(int32_t handle, int32_t channel)
    {
    _context->bind_buffer_object(handle, channel);
    }

  void render_engine::get_data_from_buffer_object(int32_t handle, void* data, int32_t size)
    {
    _context->get_data_from_buffer_object(handle, data, size);
    }

  void render_engine::geometry_begin(int32_t handle, int32_t number_of_vertices, int32_t number_of_indices, float** vertex_pointer, void** index_pointer, int32_t update)
    {
    _context->geometry_begin(handle, number_of_vertices, number_of_indices, vertex_pointer, index_pointer, update);
    }

  void render_engine::geometry_end(int32_t handle)
    {
    _context->geometry_end(handle);
    }

  void render_engine::geometry_draw(int32_t handle)
    {
    _context->geometry_draw(handle);
    }

  int32_t render_engine::add_render_buffer()
    {
    return _context->add_render_buffer();
    }

  void render_engine::remove_render_buffer(int32_t handle)
    {
    _context->remove_render_buffer(handle);
    }

  int32_t render_engine::add_frame_buffer(int32_t w, int32_t h, bool make_depth_texture)
    {
    return _context->add_frame_buffer(w, h, make_depth_texture);
    }

  const frame_buffer* render_engine::get_frame_buffer(int32_t handle) const
    {
    return _context->get_frame_buffer(handle);
    }

  void render_engine::remove_frame_buffer(int32_t handle)
    {
    _context->remove_frame_buffer(handle);
    }

  int32_t render_engine::add_shader(const char* source, int32_t type, const char* name)
    {
    return _context->add_shader(source, type, name);
    }

  void render_engine::remove_shader(int32_t handle)
    {
    _context->remove_shader(handle);
    }

  int32_t render_engine::add_program(int32_t vertex_shader_handle, int32_t fragment_shader_handle, int32_t compute_shader_handle)
    {
    return _context->add_program(vertex_shader_handle, fragment_shader_handle, compute_shader_handle);
    }

  void render_engine::remove_program(int32_t handle)
    {
    _context->remove_program(handle);
    }

  void render_engine::bind_program(int32_t handle)
    {
    _context->bind_program(handle);
    }

  void render_engine::dispatch_compute(int32_t num_groups_x, int32_t num_groups_y, int32_t num_groups_z, int32_t local_size_x, int32_t local_size_y, int32_t local_size_z)
    {
    _context->dispatch_compute(num_groups_x, num_groups_y, num_groups_z, local_size_x, local_size_y, local_size_z);
    }

  int32_t render_engine::add_uniform(const char* name, uniform_type::type uniform_type, uint16_t num)
    {
    return _context->add_uniform(name, uniform_type, num);
    }

  void render_engine::remove_uniform(int32_t handle)
    {
    _context->remove_uniform(handle);
    }

  void render_engine::set_uniform(int32_t handle, const void* values)
    {
    _context->set_uniform(handle, values);
    }

  void render_engine::bind_uniform(int32_t program_handle, int32_t uniform_handle)
    {
    _context->bind_uniform(program_handle, uniform_handle);
    }

  bool render_engine::is_initialized() const
    {
    if (!_context)
      return false;
    return _context->is_initialized();
    }

  void render_engine::set_model_view_properties(const model_view_properties& props)
    {
    _mv_props = props;
    props.make_projection_matrix(_last_projection);
    _last_camera = invert_orthonormal(props.camera_space);
    _last_view_project = matrix_matrix_multiply(_last_projection, _last_camera);
    }

  int32_t render_engine::add_query()
    {
    return _context->add_query();
    }

  void render_engine::remove_query(int32_t handle)
    {
    _context->remove_query(handle);
    }

  void render_engine::query_timestamp(int32_t handle)
    {
    _context->query_timestamp(handle);
    }

  uint64_t render_engine::get_query_result(int32_t handle)
    {
    return _context->get_query_result(handle);
    }

  void render_engine::set_blending_enabled(bool enable)
    {
    _context->set_blending_enabled(enable);
    }

  } // namespace RenderDoos
