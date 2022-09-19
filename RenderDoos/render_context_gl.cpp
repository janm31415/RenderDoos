#include "render_context_gl.h"
#include <GL/glew.h>
#include <string>
#include <sstream>
#include <stdexcept>

#include "types.h"

#include <cassert>

namespace RenderDoos
  {

  namespace
    {
    static GLenum formats[] =
      {
      GL_RGBA8, // proxy for texture_format_none
      GL_RGBA8, // texture_format_rgba8
      GL_RGBA8, // texture_format_rgba32f
      GL_RGBA8, // texture_format_bgra8
      GL_RGBA8UI, // texture_format_rgba8ui
      GL_R32UI, //texture_format_r32ui
      GL_R32I, //texture_format_r32i
      GL_R32F, //texture_format_r32f
      GL_R8UI, //texture_format_r8ui
      GL_R8I, //texture_format_r8i
      GL_RGBA16, //texture_format_rgba16
      GL_DEPTH_COMPONENT24 // texture_format_depth      
      };

    GLenum glCheckError_(const char* file, int line)
      {
      GLenum errorCode;
      while ((errorCode = glGetError()) != GL_NO_ERROR)
        {
        std::string error;
        switch (errorCode)
          {
          case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
          case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
          case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
          case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
          case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
          case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
          case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
          }
        std::stringstream str;
        str << error << " | " << file << " (" << line << ")" << std::endl;
        throw std::runtime_error(str.str());
        }
      return errorCode;
      }
#define glCheckError() glCheckError_(__FILE__, __LINE__) 

    struct gl_buffer_declaration
      {
      int location;
      GLenum type;
      int offset;
      int tupleSize;
      int stride;
      };

    static gl_buffer_declaration gl_buffer_declaration_standard[] =
      {
          { 0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 8}, // x y z
          { 1, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 8}, // nx ny nz
          { 2, GL_FLOAT, sizeof(GLfloat) * 6, 2, sizeof(GLfloat) * 8}, // u v
          { 0, 0, 0, 0, 0} // end
      };

    static gl_buffer_declaration gl_buffer_declaration_compact[] =
      {
          { 0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 4}, // x y z
          { 1, GL_UNSIGNED_INT, sizeof(GLfloat) * 3, 1, sizeof(GLfloat) * 4}, // c0          
          { 0, 0, 0, 0, 0} // end
      };

    static gl_buffer_declaration gl_buffer_declaration_color[] =
      {
          { 0, GL_FLOAT, 0, 3, sizeof(GLfloat) * 7}, // x y z
          { 1, GL_FLOAT, sizeof(GLfloat) * 3, 3, sizeof(GLfloat) * 7}, // nx ny nz
          { 2, GL_UNSIGNED_INT, sizeof(GLfloat) * 6, 1, sizeof(GLfloat) * 7}, // c0          
          { 0, 0, 0, 0, 0} // end
      };

    struct gl_buffer_declaration_table_struct
      {
      int32_t size;                       // size in bytes
      gl_buffer_declaration* declaration; // declaration
      };

    static gl_buffer_declaration_table_struct gl_buffer_declaration_table[] =
      {
        {0, 0},
        {32, gl_buffer_declaration_standard},
        {16, gl_buffer_declaration_compact},
        {28, gl_buffer_declaration_color},
      };
    }

  render_context_gl::render_context_gl() : render_context()
    {
    }

  render_context_gl::~render_context_gl()
    {
    }

  void render_context_gl::frame_begin(render_drawables)
    {
    // lock semaphore here?
    _semaphore.lock();
    }

  void render_context_gl::frame_end(bool /*wait_until_completed*/)
    {    
    // release semaphore here?
    _semaphore.unlock();
    }

  void render_context_gl::renderpass_begin(const renderpass_descriptor& descr)
    {
    if (descr.compute_shader)
      {
      return;
      }
    if (descr.frame_buffer_handle >= 0)
      _bind_frame_buffer(descr.frame_buffer_handle, descr.frame_buffer_channel, descr.frame_buffer_flags);
    else
      _bind_screen();
    if (descr.w >= 0 && descr.h >= 0)
      glViewport(0, 0, descr.w, descr.h);
    else if (descr.frame_buffer_handle >= 0)
      {
      auto fb = get_frame_buffer(descr.frame_buffer_handle);
      glViewport(0, 0, fb->w, fb->h);
      }
    _clear(descr.clear_flags, descr.clear_color);
    }

  void render_context_gl::renderpass_end()
    {
    }

  void render_context_gl::_clear(int32_t flags, uint32_t color)
    {
    GLbitfield mask = 0;
    if (flags & CLEAR_COLOR)
      mask |= GL_COLOR_BUFFER_BIT;
    if (flags & CLEAR_DEPTH)
      mask |= GL_DEPTH_BUFFER_BIT;
    const uint32_t red = color & 255;
    const uint32_t green = (color >> 8) & 255;
    const uint32_t blue = (color >> 16) & 255;
    const uint32_t alpha = (color >> 24) & 255;
    glClearColor(red / 255.f, green / 255.f, blue / 255.f, alpha / 255.f);
    glClear(mask);
    }

  bool render_context_gl::update_texture(int32_t handle, const float* data)
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return false;
    if (data == nullptr)
      return false;
    texture* tex = &_textures[handle];
    if (tex->flags == 0)
      return false;

    if (tex->format == texture_format_r32f)
      {
      glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think    
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->w, tex->h, GL_RED, GL_FLOAT, data);
      glCheckError();
      return true;
      }
    return false;
    }

  bool render_context_gl::update_texture(int32_t handle, const uint8_t* data)
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return false;
    if (data == nullptr)
      return false;
    texture* tex = &_textures[handle];
    if (tex->flags == 0)
      return false;

    if (tex->format == texture_format_rgba8 || tex->format == texture_format_rgba8ui || tex->format == texture_format_bgra8)
      {      
      glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think    
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->w, tex->h, GL_RGBA, GL_UNSIGNED_BYTE, data);
      glCheckError();
      return true;
      }
    return false;
    }

  bool render_context_gl::update_texture(int32_t handle, const uint16_t* data)
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return false;
    if (data == nullptr)
      return false;
    texture* tex = &_textures[handle];
    if (tex->flags == 0)
      return false;    

    if (tex->format == texture_format_rgba8 || tex->format == texture_format_rgba8ui)
      {      
      uint8_t* bytes = new uint8_t[tex->w * tex->h * 4];
      uint32_t* d = (uint32_t*)bytes;
      const uint16_t* s = (const uint16_t*)data;
      for (int y = 0; y < tex->h; ++y)
        {
        for (int x = 0; x < tex->w; ++x, ++d, s += 4)
          {
          *d = (((s[0] >> 7) & 0xff)) |
            (((s[1] >> 7) & 0xff) << 8) |
            (((s[2] >> 7) & 0xff) << 16) |
            (((s[3] >> 7) & 0xff) << 24);
          }
        }
      glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think    
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->w, tex->h, GL_RGBA, GL_UNSIGNED_BYTE, bytes);
      delete[] bytes;
      glCheckError();      
      return true;
      }
    else if (tex->format == texture_format_rgba16)
      {
      uint16_t* bytes = new uint16_t[tex->w * tex->h * 4];     
      const uint16_t* s = (const uint16_t*)data;
      uint16_t* d = bytes;
      for (int y = 0; y < tex->h; ++y)
        {
        for (int x = 0; x < tex->w * 4; ++x, ++d, ++s)
          {
          *d = (*s & 0x7fff)*2;
          }
        }
      glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think    
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->w, tex->h, GL_RGBA, GL_UNSIGNED_SHORT, bytes);
      delete[] bytes;
      glCheckError();
      return true;
      }
    else if (tex->format == texture_format_r32ui || tex->format == texture_format_r32i)
      {
      uint32_t* bytes = new uint32_t[tex->w * tex->h];
      uint32_t* d = (uint32_t*)bytes;
      const uint64_t* s = (const uint64_t*)data;
      for (int y = 0; y < tex->h; ++y)
        {
        for (int x = 0; x < tex->w; ++x, ++d, ++s)
          {
          *d = (uint32_t)(*s);
          }
        }
      glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think    
      if (tex->format == texture_format_r32ui)
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->w, tex->h, GL_RED_INTEGER, GL_UNSIGNED_INT, bytes);
      else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->w, tex->h, GL_RED_INTEGER, GL_INT, bytes);
      delete[] bytes;
      glCheckError();
      return true;
      }
    else if (tex->format == texture_format_r32f)
      {
      float* bytes = new float[tex->w * tex->h];
      float* d = (float*)bytes;
      const uint64_t* s = (const uint64_t*)data;
      for (int y = 0; y < tex->h; ++y)
        {
        for (int x = 0; x < tex->w; ++x, ++d, ++s)
          {
          *d = *((float*)s);
          }
        }
      glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think         
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->w, tex->h, GL_RED, GL_FLOAT, bytes);
      delete[] bytes;
      glCheckError();
      return true;
      }
    else if (tex->format == texture_format_r8ui || tex->format == texture_format_r8i)
      {
      uint8_t* bytes = new uint8_t[tex->w * tex->h];
      uint8_t* d = (uint8_t*)bytes;
      const uint64_t* s = (const uint64_t*)data;
      for (int y = 0; y < tex->h; ++y)
        {
        for (int x = 0; x < tex->w; ++x, ++d, ++s)
          {
          *d = (uint8_t)(*s & 0xff);
          }
        }
      glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think    
      if (tex->format == texture_format_r8ui)
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->w, tex->h, GL_RED_INTEGER, GL_UNSIGNED_BYTE, bytes);
      else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex->w, tex->h, GL_RED_INTEGER, GL_BYTE, bytes);
      delete[] bytes;
      glCheckError();
      return true;
      }
    return false;
    }

  int32_t render_context_gl::_add_texture(int32_t w, int32_t h, int32_t format, const void* data, int32_t usage_flags, int32_t bytes_per_channel)
    {
    texture* tex = _textures;
    for (int32_t i = 0; i < MAX_TEXTURE; ++i)
      {
      if (tex->flags == 0)
        {
        tex->w = w;
        tex->h = h;
        tex->format = format;
        tex->flags = TEX_ALLOCATED;
        tex->usage_flags = usage_flags;
        glGenTextures(1, &tex->gl_texture_id);
        glCheckError();
        glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
        glCheckError();
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think 
        glTexStorage2D(GL_TEXTURE_2D, 1, formats[tex->format], w, h);
        glCheckError();
        switch (bytes_per_channel)
          {
          case 1: update_texture(i, (const uint8_t*)data); break;
          case 2: update_texture(i, (const uint16_t*)data); break;
          case 4: update_texture(i, (const float*)data); break;
          }
        return i;
        }
      ++tex;
      }
    return -1;
    }

  int32_t render_context_gl::add_texture(int32_t w, int32_t h, int32_t format, const uint16_t* data, int32_t usage_flags)
    {    
    return _add_texture(w, h, format, (const void*)data, usage_flags, 2);
    }

  int32_t render_context_gl::add_texture(int32_t w, int32_t h, int32_t format, const uint8_t* data, int32_t usage_flags)
    {
    return _add_texture(w, h, format, (const void*)data, usage_flags, 1);
    }

  void render_context_gl::remove_texture(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return;
    texture* tex = &_textures[handle];
    if (tex->flags == 0)
      return;
    glDeleteTextures(1, &tex->gl_texture_id);
    glCheckError();
    tex->flags = 0;
    }

  const texture* render_context_gl::get_texture(int32_t handle) const
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return nullptr;
    return &_textures[handle];
    }

  void render_context_gl::get_data_from_texture(int32_t handle, void* data, int32_t size)
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return;

    texture* tex = &_textures[handle];

    if (tex->format == texture_format_rgba8)
      {
      if (size < tex->w*tex->h*4)
        return;
      glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think
      glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)data);      
      glCheckError();
      }  
    else if (tex->format == texture_format_rgba16)
      {
      if (size < tex->w * tex->h * 8)
        return;
      glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think
      glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_SHORT, (void*)data);
      glCheckError();
      }
    else if (tex->format == texture_format_rgba8ui)
      {
      if (size < tex->w * tex->h * 4)
        return;
      glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think
      glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA_INTEGER, GL_UNSIGNED_BYTE, (void*)data);
      glCheckError();
      }
    else if (tex->format == texture_format_r32ui || tex->format == texture_format_r32i)
      {
      if (size < tex->w * tex->h * 4)
        return;
      glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think
      if (tex->format == texture_format_r32ui)
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, (void*)data);
      else
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_INT, (void*)data);
      glCheckError();
      }
    else if (tex->format == texture_format_r32f)
      {
      if (size < tex->w * tex->h * 4)
        return;
      glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think
      glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, (void*)data);
      glCheckError();
      }
    else if (tex->format == texture_format_r8ui || tex->format == texture_format_r8i)
      {
      if (size < tex->w * tex->h)
        return;
      glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // opengl by default aligns rows on 4 bytes I think
      if (tex->format == texture_format_r8ui)
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, (void*)data);
      else
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_BYTE, (void*)data);
      glCheckError();
      }
    }

  void render_context_gl::bind_texture_to_channel(int32_t handle, int32_t channel, int32_t flags)
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return;
    texture* tex = &_textures[handle];
    if (tex->flags == 0)
      return;
    glActiveTexture(GL_TEXTURE0 + channel);
    glCheckError();
    glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
    glCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, flags & TEX_WRAP_CLAMP_TO_EDGE ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, flags & TEX_WRAP_CLAMP_TO_EDGE ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    if (flags & TEX_FILTER_NEAREST)
      {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }
    else if (flags & TEX_FILTER_LINEAR_MIPMAP_LINEAR)
      {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
    else
      {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }

    GLint access = GL_READ_WRITE;
    if ((tex->usage_flags & TEX_USAGE_READ)==0)
      access = GL_WRITE_ONLY;
    if ((flags & TEX_USAGE_WRITE)==0)
      access = GL_READ_ONLY;
    glBindImageTexture(channel, tex->gl_texture_id, 0, GL_FALSE, 0, access, formats[tex->format]);

    glCheckError();
    }

  int32_t render_context_gl::add_geometry(int32_t vertex_declaration_type)
    {
    if (vertex_declaration_type < 1 || vertex_declaration_type > 3)
      return -1;
    geometry_handle* gh = _geometry_handles;
    for (int32_t i = 0; i < MAX_GEOMETRY; ++i)
      {
      if (gh->mode == 0)
        {
        memset(gh, 0, sizeof(*gh));
        gh->vertex_size = gl_buffer_declaration_table[vertex_declaration_type].size;
        gh->vertex_declaration_type = vertex_declaration_type;
        gh->mode = GEOMETRY_ALLOCATED;
        gh->locked = 0;
        gh->vertex.buffer = -1;
        gh->index.buffer = -1;
        glGenVertexArrays(1, &gh->gl_vertex_array_object_id);
        glCheckError();
        return i;
        }
      ++gh;
      }
    return -1;
    }

  int32_t render_context_gl::add_buffer_object(const void* data, int32_t size)
    {
    if (size <= 0)
      return -1;
    buffer_object* buf = _buffer_objects;
    for (int32_t i = 0; i < MAX_BUFFER_OBJECT; ++i)
      {
      if (buf->size == 0)
        {                        
        buf->size = size;
        buf->type = COMPUTE_BUFFER;
        glGenBuffers(1, &buf->gl_buffer_id);
        glCheckError();
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf->gl_buffer_id);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW);
        glCheckError();
        return i;
        }
      ++buf;
      }
    return -1;
    }

  void render_context_gl::remove_buffer_object(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_BUFFER_OBJECT)
      return;
    buffer_object* buf = &_buffer_objects[handle];
    if (buf->size > 0)
      {
      delete[] buf->raw;
      glDeleteBuffers(1, &buf->gl_buffer_id);
      glCheckError();
      }
    buf->size = 0;
    buf->raw = nullptr;
    buf->type = 0;
    }

  void render_context_gl::update_buffer_object(int32_t handle, const void* data, int32_t size)
    {
    if (handle < 0 || handle >= MAX_BUFFER_OBJECT)
      return;
    buffer_object* buf = &_buffer_objects[handle];
    if (buf->size > 0)
      {
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf->gl_buffer_id);
      if (size != buf->size)
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
      glCheckError();
      }
    }

  void render_context_gl::bind_buffer_object(int32_t handle, int32_t channel)
    {
    if (handle < 0 || handle >= MAX_BUFFER_OBJECT)
      return;
    buffer_object* buf = &_buffer_objects[handle];
    if (buf->size > 0)
      {
      switch (buf->type)
        {
        case GEOMETRY_VERTEX:
          glBindBufferBase(GL_ARRAY_BUFFER, channel, buf->gl_buffer_id);
          break;
        case GEOMETRY_INDEX:
          glBindBufferBase(GL_ELEMENT_ARRAY_BUFFER, channel, buf->gl_buffer_id);
          break;
        case COMPUTE_BUFFER:
          glBindBufferBase(GL_SHADER_STORAGE_BUFFER, channel, buf->gl_buffer_id);
          break;
        default:
          break;
        }
      
      glCheckError();
      }
    }

  const buffer_object* render_context_gl::get_buffer_object(int32_t handle) const
    {
    if (handle < 0 || handle >= MAX_BUFFER_OBJECT)
      return nullptr;
    return &_buffer_objects[handle];
    }

  void render_context_gl::copy_buffer_object_data(int32_t source_handle, int32_t destination_handle, uint32_t read_offset, uint32_t write_offset, uint32_t size)
    {
    if (source_handle < 0 || source_handle >= MAX_BUFFER_OBJECT)
      return;
    if (read_offset < 0 || read_offset >= MAX_BUFFER_OBJECT)
      return;
    buffer_object* src = &_buffer_objects[source_handle];
    buffer_object* dst = &_buffer_objects[destination_handle];

    glCopyNamedBufferSubData(src->gl_buffer_id, dst->gl_buffer_id, read_offset, write_offset, size);
    glCheckError();
    }

  void render_context_gl::get_data_from_buffer_object(int32_t handle, void* data, int32_t size)
    {
    if (handle < 0 || handle >= MAX_BUFFER_OBJECT)
      return;
    buffer_object* buf = &_buffer_objects[handle];
    if (buf->size > 0)
      {
      /*
      switch (buf->type)
        {
        case GEOMETRY_VERTEX:          
          glGetBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
          break;
        case GEOMETRY_INDEX:       
          glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, data);
          break;
        case COMPUTE_BUFFER:
          glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
          break;
        default:
          break;
        }
        */
      glGetNamedBufferSubData(buf->gl_buffer_id, 0, size, data);
      glCheckError();
      }
    }

  void render_context_gl::_remove_buffer_object(geometry_ref& ref)
    {
    if (ref.buffer < 0 || ref.buffer >= MAX_BUFFER_OBJECT)
      return;
    buffer_object* buf = &_buffer_objects[ref.buffer];
    if (buf->size > 0)
      {
      delete[] buf->raw;
      glDeleteBuffers(1, &buf->gl_buffer_id);
      glCheckError();
      }
    buf->size = 0;
    buf->raw = nullptr;
    buf->type = 0;
    ref.count = 0;
    }

  void render_context_gl::remove_geometry(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_GEOMETRY)
      return;
    geometry_handle* geo = &_geometry_handles[handle];
    if (geo->mode == 0)
      return;
    assert(geo->locked == 0);
    geo->mode = 0;
    glDeleteVertexArrays(1, &geo->gl_vertex_array_object_id);
    glCheckError();
    _remove_buffer_object(geo->vertex);
    _remove_buffer_object(geo->index);
    }

  void render_context_gl::_allocate_buffer_object(geometry_ref& ref, int32_t tuple_size, int32_t count, int32_t type, void** pointer)
    {
    assert(type == GEOMETRY_VERTEX || type == GEOMETRY_INDEX);
    if (ref.buffer < 0) // no actual buffer assigned yet
      {
      buffer_object* buf = _buffer_objects;
      for (int32_t i = 0; i < MAX_BUFFER_OBJECT; ++i)
        {
        if (buf->size == 0)
          {
          ref.buffer = i;
          buf->type = 0;
          break;
          }
        ++buf;
        }
      if (ref.buffer < 0) // all buffers are used
        throw std::runtime_error("Out of memory");
      }
    buffer_object* buf = &_buffer_objects[ref.buffer];
    int32_t size = tuple_size * count;
    if (buf->size < size || (buf->type != type))
      {
      if (buf->size == 0) // first initialization
        {
        glGenBuffers(1, &buf->gl_buffer_id);
        glCheckError();
        }
      if (buf->raw)
        {
        delete[] buf->raw;
        }
      buf->raw = new uint8_t[size];
      buf->size = size;
      buf->type = type;
      ref.count = count;
      glBindBuffer(type == GEOMETRY_VERTEX ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER, buf->gl_buffer_id);
      glBufferData(type == GEOMETRY_VERTEX ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
      glCheckError();
      }
    if (pointer)
      {
      *pointer = (void*)buf->raw;
      }
    }

  void render_context_gl::geometry_begin(int32_t handle, int32_t number_of_vertices, int32_t number_of_indices, float** vertex_pointer, void** index_pointer, int32_t update)
    {
    if (handle < 0 || handle >= MAX_GEOMETRY)
      return;
    geometry_handle* gh = &_geometry_handles[handle];
    if (gh->mode == 0)
      return;
    if (vertex_pointer)
      *vertex_pointer = 0;
    if (index_pointer)
      *index_pointer = 0;

    if ((update & GEOMETRY_VERTEX) && !(gh->locked & GEOMETRY_VERTEX)) // vertices
      {
      gh->locked |= GEOMETRY_VERTEX;
      _allocate_buffer_object(gh->vertex, gh->vertex_size, number_of_vertices, GEOMETRY_VERTEX, (void**)vertex_pointer);
      }
    if ((update & GEOMETRY_INDEX) && !(gh->locked & GEOMETRY_INDEX)) // indices
      {
      gh->locked |= GEOMETRY_INDEX;
      _allocate_buffer_object(gh->index, sizeof(uint32_t), number_of_indices, GEOMETRY_INDEX, (void**)index_pointer);
      }
    }

  void render_context_gl::_update_buffer_object(geometry_ref& ref)
    {
    if (ref.buffer < 0)
      return;
    buffer_object* buf = &_buffer_objects[ref.buffer];
    glBindBuffer(buf->type == GEOMETRY_VERTEX ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER, buf->gl_buffer_id);
    glBufferData(buf->type == GEOMETRY_VERTEX ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER, buf->size, nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(buf->type == GEOMETRY_VERTEX ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER, 0, buf->size, buf->raw);
    glCheckError();
    }

  void render_context_gl::geometry_end(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_GEOMETRY)
      return;
    geometry_handle* gh = &_geometry_handles[handle];
    if (gh->mode == 0)
      return;
    if (gh->locked & GEOMETRY_VERTEX)
      {
      _update_buffer_object(gh->vertex);
      gh->locked &= ~GEOMETRY_VERTEX;
      }
    if (gh->locked & GEOMETRY_INDEX)
      {
      _update_buffer_object(gh->index);
      gh->locked &= ~GEOMETRY_INDEX;
      }
    }

  void render_context_gl::geometry_draw(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_GEOMETRY)
      return;
    geometry_handle* gh = &_geometry_handles[handle];
    if (gh->mode == 0)
      return;
    glBindVertexArray(gh->gl_vertex_array_object_id);
    glCheckError();

    if (gh->vertex.buffer >= 0 && gh->vertex.buffer < MAX_BUFFER_OBJECT)
      {
      buffer_object* buf = &_buffer_objects[gh->vertex.buffer];
      glBindBuffer(GL_ARRAY_BUFFER, buf->gl_buffer_id);
      }
    if (gh->index.buffer >= 0 && gh->index.buffer < MAX_BUFFER_OBJECT)
      {
      buffer_object* buf = &_buffer_objects[gh->index.buffer];
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf->gl_buffer_id);
      }
    gl_buffer_declaration* decl = gl_buffer_declaration_table[gh->vertex_declaration_type].declaration;
    while (decl->stride)
      {
      glEnableVertexAttribArray(decl->location);
      switch (decl->type)
        {
        case GL_UNSIGNED_BYTE:
        case GL_BYTE:
        case GL_UNSIGNED_SHORT:
        case GL_SHORT:
        case GL_UNSIGNED_INT:
        case GL_INT:
          glVertexAttribIPointer(decl->location, decl->tupleSize, decl->type, decl->stride, reinterpret_cast<const void*>(intptr_t(decl->offset)));
          break;
        default:
          glVertexAttribPointer(decl->location, decl->tupleSize, decl->type, GL_FALSE, decl->stride, reinterpret_cast<const void*>(intptr_t(decl->offset)));
          break;
        }
      ++decl;
      }
    glCheckError();

    glEnable(GL_DEPTH_TEST);
    glDrawElements(GL_TRIANGLES, gh->index.count, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glCheckError();
    }

  int32_t render_context_gl::add_render_buffer()
    {
    render_buffer* rb = _render_buffers;
    for (int32_t i = 0; i < MAX_RENDERBUFFER; ++i)
      {
      if (rb->type == 0)
        {
        glGenRenderbuffersEXT(1, &rb->gl_render_buffer_id);
        glCheckError();
        rb->type = 1;
        return i;
        }
      ++rb;
      }
    return -1;
    }

  void render_context_gl::remove_render_buffer(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_RENDERBUFFER)
      return;
    render_buffer* rb = &_render_buffers[handle];
    if (rb->type == 0)
      return;
    glDeleteRenderbuffersEXT(1, &rb->gl_render_buffer_id);
    glCheckError();
    rb->type = 0;
    }

  int32_t render_context_gl::add_frame_buffer(int32_t w, int32_t h, bool make_depth_texture)
    {
    frame_buffer* fb = _frame_buffers;
    for (int32_t i = 0; i < MAX_FRAMEBUFFER; ++i)
      {
      if (fb->texture_handle < 0)
        {
        fb->w = w;
        fb->h = h;
        fb->texture_handle = add_texture(w, h, texture_format_rgba8, (const uint16_t*)nullptr, TEX_USAGE_RENDER_TARGET | TEX_USAGE_READ);
        if (make_depth_texture)
          fb->depth_texture_handle = add_texture(w, h, texture_format_depth, (const uint16_t*)nullptr, TEX_USAGE_RENDER_TARGET);
        else
          fb->render_buffer_handle = add_render_buffer();
        if (fb->texture_handle < 0)
          return -1;
        if (make_depth_texture && (fb->depth_texture_handle < 0))
          return -1;
        if (!make_depth_texture && (fb->render_buffer_handle < 0))
          return -1;

        glActiveTexture(GL_TEXTURE0 + 10);
        glCheckError();
        texture* tex = &_textures[fb->texture_handle];
        glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
        glCheckError();

        glGenFramebuffersEXT(1, &fb->gl_frame_buffer_id);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb->gl_frame_buffer_id);

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, _textures[fb->texture_handle].gl_texture_id, 0);

        if (make_depth_texture)
          {
          glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, _textures[fb->depth_texture_handle].gl_texture_id, 0);
          }
        else
          {
          glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, _render_buffers[fb->render_buffer_handle].gl_render_buffer_id);
          glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, w, h);
          glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, _render_buffers[fb->render_buffer_handle].gl_render_buffer_id);
          }
        GLenum status;
        status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        switch (status)
          {
          case GL_FRAMEBUFFER_COMPLETE_EXT:
          {
          break;
          }
          default:
          {
          throw std::runtime_error("frame buffer object is not complete");
          }
          }
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        glCheckError();
        return i;
        }
      ++fb;
      }
    return -1;
    }

  void render_context_gl::_bind_frame_buffer(int32_t handle, int32_t channel, int32_t flags)
    {
    if (handle < 0 || handle >= MAX_FRAMEBUFFER)
      return;
    frame_buffer* fb = &_frame_buffers[handle];
    if (fb->texture_handle < 0)
      return;
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, _render_buffers[fb->render_buffer_handle].gl_render_buffer_id);
    texture* tex = &_textures[fb->texture_handle];
    if (tex->flags == 0)
      return;
    glActiveTexture(GL_TEXTURE0 + channel);
    glCheckError();
    glBindTexture(GL_TEXTURE_2D, tex->gl_texture_id);
    glCheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, flags & TEX_WRAP_CLAMP_TO_EDGE ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, flags & TEX_WRAP_CLAMP_TO_EDGE ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    if (flags & TEX_FILTER_NEAREST)
      {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }
    else if (flags & TEX_FILTER_LINEAR_MIPMAP_LINEAR)
      {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
    else
      {
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb->gl_frame_buffer_id);
    glCheckError();
    }

  const frame_buffer* render_context_gl::get_frame_buffer(int32_t handle) const
    {
    if (handle < 0 || handle >= MAX_FRAMEBUFFER)
      return nullptr;
    return &_frame_buffers[handle];
    }

  void render_context_gl::_bind_screen()
    {
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glCheckError();
    }

  void render_context_gl::remove_frame_buffer(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_FRAMEBUFFER)
      return;
    frame_buffer* fb = &_frame_buffers[handle];
    if (fb->texture_handle < 0)
      return;
    remove_texture(fb->texture_handle);
    remove_render_buffer(fb->render_buffer_handle);
    glDeleteFramebuffersEXT(1, &fb->gl_frame_buffer_id);
    glCheckError();
    fb->texture_handle = -1;
    fb->render_buffer_handle = -1;
    }

  void render_context_gl::_compile_shader(int32_t handle, const char* source)
    {
    assert(handle >= 0 && handle < MAX_SHADER);
    shader* sh = &_shaders[handle];
    glShaderSource(sh->gl_shader_id, 1, &source, nullptr);
    glCompileShader(sh->gl_shader_id);
    int value;
    glGetShaderiv(sh->gl_shader_id, GL_COMPILE_STATUS, &value);
    sh->compiled = value;
    if (!sh->compiled)
      {
      glGetShaderiv(sh->gl_shader_id, GL_INFO_LOG_LENGTH, &value);
      if (value > 1)
        {
        int length;
        std::string log;
        log.resize(value);
        glGetShaderInfoLog(sh->gl_shader_id, value, &length, &log[0]);
        throw std::runtime_error(log);
        }
      }
    }

  int32_t render_context_gl::add_shader(const char* source, int32_t type, const char* name)
    {
    if (type < SHADER_VERTEX || type > SHADER_COMPUTE)
      return -1;
    shader* sh = _shaders;
    for (int32_t i = 0; i < MAX_SHADER; ++i)
      {
      if (sh->name && strcmp(sh->name, name)==0) // shader already exists
        {
        return i;
        }
      if (sh->type == 0)
        {
        sh->type = type;
        sh->name = name;
        switch (type)
          {
          case SHADER_VERTEX:
            sh->gl_shader_id = glCreateShader(GL_VERTEX_SHADER);
            break;
          case SHADER_FRAGMENT:
            sh->gl_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
            break;
          case SHADER_COMPUTE:
            sh->gl_shader_id = glCreateShader(GL_COMPUTE_SHADER);
            break;
          }
        glCheckError();
        _compile_shader(i, source);
        return i;
        }
      ++sh;
      }
    return -1;
    }

  void render_context_gl::remove_shader(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_SHADER)
      return;
    shader* sh = &_shaders[handle];
    if (sh->type == 0)
      return;
    glDeleteShader(sh->gl_shader_id);
    glCheckError();
    sh->type = 0;
    sh->compiled = 0;
    sh->name = nullptr;
    }

  int32_t render_context_gl::add_program(int32_t vertex_shader_handle, int32_t fragment_shader_handle, int32_t compute_shader_handle)
    {
    if ((vertex_shader_handle < 0 || fragment_shader_handle < 0) && (compute_shader_handle < -1))
      return -1;
    if (vertex_shader_handle >= MAX_SHADER || fragment_shader_handle >= MAX_SHADER || compute_shader_handle >= MAX_SHADER)
      return -1;
    shader_program* sh = _shader_programs;
    for (int32_t i = 0; i < MAX_SHADER_PROGRAM; ++i)
      {
      if (sh->vertex_shader_handle == vertex_shader_handle && sh->fragment_shader_handle == fragment_shader_handle && sh->compute_shader_handle == compute_shader_handle)
        {
        return i;
        }
      if (sh->vertex_shader_handle < 0 && sh->fragment_shader_handle < 0 && sh->compute_shader_handle < 0)
        {
        sh->vertex_shader_handle = vertex_shader_handle;
        sh->fragment_shader_handle = fragment_shader_handle;
        sh->compute_shader_handle = compute_shader_handle;
        sh->gl_program_id = glCreateProgram();
        glCheckError();
        if (compute_shader_handle >= 0)
          {
          shader* cs = &_shaders[compute_shader_handle];
          if (cs->compiled)
            {
            glAttachShader(sh->gl_program_id, cs->gl_shader_id);
            glLinkProgram(sh->gl_program_id);
            int value = 0;
            glGetProgramiv(sh->gl_program_id, GL_LINK_STATUS, &value);
            sh->linked = value;
            glCheckError();
            }
          }
        else
          {
          shader* vs = &_shaders[vertex_shader_handle];
          shader* fs = &_shaders[fragment_shader_handle];
          if (vs->compiled && fs->compiled)
            {
            glAttachShader(sh->gl_program_id, vs->gl_shader_id);
            glAttachShader(sh->gl_program_id, fs->gl_shader_id);
            glLinkProgram(sh->gl_program_id);
            int value = 0;
            glGetProgramiv(sh->gl_program_id, GL_LINK_STATUS, &value);
            sh->linked = value;
            glCheckError();
            }
          }
        return i;
        }
      ++sh;
      }
    return -1;
    }

  void render_context_gl::remove_program(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_SHADER_PROGRAM)
      return;
    shader_program* sh = &_shader_programs[handle];
    if (sh->linked == 0)
      return;
    if (sh->vertex_shader_handle >= 0)
      {
      shader* vs = &_shaders[sh->vertex_shader_handle];
      if (vs->compiled)
        glDetachShader(sh->gl_program_id, vs->gl_shader_id);
      }
    if (sh->fragment_shader_handle >= 0)
      {
      shader* fs = &_shaders[sh->fragment_shader_handle];    
      if (fs->compiled)
        glDetachShader(sh->gl_program_id, fs->gl_shader_id);
      }
    if (sh->compute_shader_handle >= 0)
      {
      shader* cs = &_shaders[sh->compute_shader_handle];
      if (cs->compiled)
        glDetachShader(sh->gl_program_id, cs->gl_shader_id);
      }
    sh->vertex_shader_handle = -1;
    sh->fragment_shader_handle = -1;
    sh->compute_shader_handle = -1;
    sh->linked = 0;
    glDeleteProgram(sh->gl_program_id);
    glCheckError();
    }

  void render_context_gl::bind_program(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_SHADER_PROGRAM)
      return;
    shader_program* sh = &_shader_programs[handle];
    if (sh->linked == 0)
      return;
    glUseProgram(sh->gl_program_id);
    glCheckError();
    }

  void render_context_gl::dispatch_compute(int32_t num_groups_x, int32_t num_groups_y, int32_t num_groups_z, int32_t /*local_size_x*/, int32_t /*local_size_y*/, int32_t /*local_size_z*/)
    {
    // in opengl, local_size_x, local_size_y, and local_size_z is set via the compute shader
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glCheckError();
    glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
    glCheckError();
    }

  void render_context_gl::bind_uniform(int32_t program_handle, int32_t uniform_handle)
    {
    if (program_handle < 0 || program_handle >= MAX_SHADER_PROGRAM)
      return;
    shader_program* sh = &_shader_programs[program_handle];
    if (sh->linked == 0)
      return;
    if (uniform_handle < 0 || uniform_handle >= MAX_UNIFORMS)
      return;
    uniform_value* uni = &_uniforms[uniform_handle];
    GLint location = glGetUniformLocation(sh->gl_program_id, uni->name);

#ifdef DEBUG_HARD
    if (location == -1)
      {            
      throw std::runtime_error("Uniform name does not correspond to an active uniform variable in program, or the name starts with the reserved prefix gl_, or the name is associated with an atomic counter or a named uniform block.");
      }
#endif
    
    if (location == -1)
      {
      return;
      }

    switch (uni->uniform_type)
      {
      case uniform_type::sampler:
      {
      int32_t* values = (int32_t*)uni->raw;
      if (uni->num == 1)
        glUniform1i(location, values[0]);
      else
        glUniform1iv(location, uni->num, values);
      break;
      }
      case uniform_type::vec2:
      {
      float* values = (float*)uni->raw;
      if (uni->num == 1)
        glUniform2f(location, values[0], values[1]);
      else
        glUniform2fv(location, uni->num, values);
      break;
      }
      case uniform_type::vec3:
      {
      float* values = (float*)uni->raw;
      if (uni->num == 1)
        glUniform3f(location, values[0], values[1], values[2]);
      else
        glUniform3fv(location, uni->num, values);
      break;
      }
      case uniform_type::vec4:
      {
      float* values = (float*)uni->raw;
      if (uni->num == 1)
        glUniform4f(location, values[0], values[1], values[2], values[3]);
      else
        glUniform4fv(location, uni->num, values);
      break;
      }
      case uniform_type::uvec2:
      {
      int32_t* values = (int32_t*)uni->raw;
      if (uni->num == 1)
        glUniform2i(location, values[0], values[1]);
      else
        glUniform2iv(location, uni->num, values);
      break;
      }
      case uniform_type::uvec3:
      {
      int32_t* values = (int32_t*)uni->raw;
      if (uni->num == 1)
        glUniform3i(location, values[0], values[1], values[2]);
      else
        glUniform3iv(location, uni->num, values);
      break;
      }
      case uniform_type::uvec4:
      {
      int32_t* values = (int32_t*)uni->raw;
      if (uni->num == 1)
        glUniform4i(location, values[0], values[1], values[2], values[3]);
      else
        glUniform4iv(location, uni->num, values);
      break;
      }
      case uniform_type::mat3:
      {
      assert(0); // todo
      break;
      }
      case uniform_type::mat4:
      {
      assert(0); // todo
      break;
      }
      case uniform_type::integer:
      {
      int32_t* values = (int32_t*)uni->raw;
      if (uni->num == 1)
        glUniform1i(location, values[0]);
      else
        glUniform1iv(location, uni->num, values);
      break;
      }
      case uniform_type::real:
      {
      float* values = (float*)uni->raw;
      if (uni->num == 1)
        glUniform1f(location, values[0]);
      else
        glUniform1fv(location, uni->num, values);
      break;
      }
      default:
        assert(0);
        break;
      }

    glCheckError();
    }

  int32_t render_context_gl::add_query()
    {
    query_handle* q = _queries;
    for (int32_t i = 0; i < MAX_QUERIES; ++i)
      {
      if (q->mode == 0)
        {        
        glGenQueries(1, &q->gl_query_id);
        q->mode = 1;       
        return i;
        }
      ++q;
      }
    return -1;
    }

  void render_context_gl::remove_query(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_QUERIES)
      return;
    query_handle* q = &_queries[handle];
    if (q->mode == 0)
      return;
    glDeleteQueries(1, &q->gl_query_id);
    glCheckError();
    q->mode = 0;
    }

  void render_context_gl::query_timestamp(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_QUERIES)
      return;
    query_handle* q = &_queries[handle];
    if (q->mode == 0)
      return;
    glQueryCounter(q->gl_query_id, GL_TIMESTAMP);
    glCheckError();
    }

  uint64_t render_context_gl::get_query_result(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_QUERIES)
      return 0xffffffffffffffff;
    query_handle* q = &_queries[handle];
    if (q->mode == 0)
      return 0xffffffffffffffff;
    // wait until the results are available
    GLint stopTimerAvailable = 0;
    while (!stopTimerAvailable) 
      {
      glGetQueryObjectiv(q->gl_query_id, GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
      }

    uint64_t tic;
    // get query results
    glGetQueryObjectui64v(q->gl_query_id, GL_QUERY_RESULT, &tic);
    return tic;
    }

  } // namespace RenderDoos
