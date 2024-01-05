#include "render_context_metal.h"

//#define NS_PRIVATE_IMPLEMENTATION
//#define CA_PRIVATE_IMPLEMENTATION
//#define MTL_PRIVATE_IMPLEMENTATION

#include "metal/Metal.hpp"

#include "types.h"

#include <cassert>
#include <stdexcept>
#include <string>

namespace RenderDoos
  {

  namespace
    {
    struct uniform_alignment
      {
      uniform_type::type uniform_type;
      uint32_t size;
      uint32_t align;
      };

    static uniform_alignment uniform_type_to_alignment[] =
      {
        { uniform_type::sampler, 4, 4},
        { uniform_type::vec2, 8, 8},
        { uniform_type::vec3, 16, 16},
        { uniform_type::vec4, 16, 16},
        { uniform_type::uvec2, 8, 8},
        { uniform_type::uvec3, 16, 16},
        { uniform_type::uvec4, 16, 16},
        { uniform_type::mat3, 48, 16},
        { uniform_type::mat4, 64, 16},
        { uniform_type::integer, 4, 4},
        { uniform_type::real, 4, 4}
      };
    }

  render_context_metal::render_context_metal(MTL::Device* device, MTL::Library* library) : render_context(), mp_device(device), mp_default_library(nullptr),
    mp_command_queue(nullptr), mp_drawable(nullptr), mp_command_buffer(nullptr), mp_render_command_encoder(nullptr), mp_screen(nullptr),
    mp_depth_stencil_state(nullptr), mp_compute_command_encoder(nullptr), _enable_blending(false), _blending_source(blending_type::one), _blending_destination(blending_type::one),
    _blending_func(blending_equation_type::add)
    {
    //mp_auto_release_pool = NS::AutoreleasePool::alloc()->init();
    memset(m_pipeline_state_cache, 0, sizeof(RenderPipelineStateCache) * MAX_PIPELINESTATE_CACHE);
    memset(m_compute_pipeline_state_cache, 0, sizeof(ComputePipelineStateCache) * MAX_PIPELINESTATE_CACHE);
    assert(mp_device != nullptr);
    if (library == nullptr)
      mp_default_library = mp_device->newDefaultLibrary();
    else
      mp_default_library = library;
    mp_command_queue = mp_device->newCommandQueue();
    _semaphore = dispatch_semaphore_create(1);
    MTL::DepthStencilDescriptor* depth_descr = MTL::DepthStencilDescriptor::alloc()->init();
    depth_descr->setDepthCompareFunction(MTL::CompareFunctionLess);
    depth_descr->setDepthWriteEnabled(true);
    mp_depth_stencil_state = mp_device->newDepthStencilState(depth_descr);
    depth_descr->release();
    }

  render_context_metal::~render_context_metal()
    {
    mp_command_queue->release();
    mp_default_library->release();
    mp_depth_stencil_state->release();
    for (int i = 0; i < MAX_PIPELINESTATE_CACHE; ++i)
      {
      if (m_pipeline_state_cache[i].p_pipeline)
        m_pipeline_state_cache[i].p_pipeline->release();
      if (m_compute_pipeline_state_cache[i].p_pipeline)
        m_compute_pipeline_state_cache[i].p_pipeline->release();
      }
    //mp_auto_release_pool->release();
    }

  void render_context_metal::frame_begin(render_drawables drawables)
    {
    mp_auto_release_pool = NS::AutoreleasePool::alloc()->init();
    mp_drawable = (MTL::Drawable*)drawables.metal_drawable;
    mp_screen = (MTL::Texture*)drawables.metal_screen_texture;
    dispatch_semaphore_wait(_semaphore, DISPATCH_TIME_FOREVER);

    mp_command_buffer = mp_command_queue->commandBuffer();
    mp_command_buffer->addCompletedHandler([&](MTL::CommandBuffer* buf) {dispatch_semaphore_signal(_semaphore); });
    }

  void render_context_metal::frame_end(bool wait_until_completed)
    {
    if (mp_drawable)
      mp_command_buffer->presentDrawable(mp_drawable);
    mp_command_buffer->commit();
    if (wait_until_completed)
      mp_command_buffer->waitUntilCompleted();
    mp_drawable = nullptr;
    mp_command_buffer = nullptr;
    mp_auto_release_pool->release();
    mp_auto_release_pool = nullptr;
    }

  void render_context_metal::renderpass_begin(const renderpass_descriptor& descr)
    {
    m_current_renderpass_descriptor = descr;
    if (descr.compute_shader)
      {
      mp_compute_command_encoder = mp_command_buffer->computeCommandEncoder();
      }
    else
      {
      MTL::RenderPassDescriptor* p_descriptor = MTL::RenderPassDescriptor::alloc()->init();
      double alpha = ((descr.clear_color >> 24) & 0xff) / 255.0;
      double blue = ((descr.clear_color >> 16) & 0xff) / 255.0;
      double green = ((descr.clear_color >> 8) & 0xff) / 255.0;
      double red = ((descr.clear_color) & 0xff) / 255.0;
      bool has_depth = false;
      if (descr.frame_buffer_handle >= 0)
        {
        const frame_buffer* p_framebuffer = get_frame_buffer(descr.frame_buffer_handle);
        MTL::Texture* tex = (MTL::Texture*)(get_texture(p_framebuffer->texture_handle)->metal_texture);
        p_descriptor->colorAttachments()->object(0)->setTexture(tex);
        if (p_framebuffer->depth_texture_handle >= 0)
          {
          MTL::Texture* depth = (MTL::Texture*)(get_texture(p_framebuffer->depth_texture_handle)->metal_texture);
          p_descriptor->depthAttachment()->setTexture(depth);
          has_depth = true;
          }
        }
      else
        {
        p_descriptor->colorAttachments()->object(0)->setTexture(mp_screen);
        if (descr.depth_texture_handle >= 0)
          {
          MTL::Texture* depth = (MTL::Texture*)(get_texture(descr.depth_texture_handle)->metal_texture);
          p_descriptor->depthAttachment()->setTexture(depth);
          has_depth = true;
          }
        }
      p_descriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor(red, green, blue, alpha));
      p_descriptor->colorAttachments()->object(0)->setLoadAction((descr.clear_flags & CLEAR_COLOR) ? MTL::LoadActionClear : MTL::LoadActionLoad);
      p_descriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);
      p_descriptor->depthAttachment()->setLoadAction((descr.clear_flags & CLEAR_DEPTH) ? MTL::LoadActionClear : MTL::LoadActionLoad);
      p_descriptor->depthAttachment()->setStoreAction(MTL::StoreActionStore);
      p_descriptor->depthAttachment()->setClearDepth(descr.clear_depth);


      mp_render_command_encoder = mp_command_buffer->renderCommandEncoder(p_descriptor);

      if (has_depth)
        mp_render_command_encoder->setDepthStencilState(mp_depth_stencil_state);

      p_descriptor->release();
      }
    _raw_uniforms.clear();
    }

  void render_context_metal::renderpass_end()
    {
    if (mp_render_command_encoder)
      {
      mp_render_command_encoder->endEncoding();
      mp_render_command_encoder = nullptr;
      }
    if (mp_compute_command_encoder)
      {
      mp_compute_command_encoder->endEncoding();
      mp_compute_command_encoder = nullptr;
      }
    }


  int32_t render_context_metal::_add_texture(int32_t w, int32_t h, int32_t format, const void* data, int32_t usage_flags, int32_t bytes_per_channel)
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
        tex->texture_target = TEX_TARGET_2D;
        tex->usage_flags = usage_flags;

        MTL::TextureDescriptor* descr = MTL::TextureDescriptor::alloc()->init();
        descr->setTextureType(MTL::TextureType2D);
        descr->setWidth(w);
        descr->setHeight(h);
        descr->setSampleCount(1);
        switch (format)
          {
          case texture_format_rgba8:
            descr->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
            break;
          case texture_format_rgba32f:
            descr->setPixelFormat(MTL::PixelFormatRGBA32Float);
            break;
          case texture_format_bgra8:
            descr->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
            break;
          case texture_format_rgba8ui:
            descr->setPixelFormat(MTL::PixelFormatRGBA8Uint);
            break;
          case texture_format_depth:
            descr->setPixelFormat(MTL::PixelFormatDepth32Float);
            break;
          case texture_format_r32ui:
            descr->setPixelFormat(MTL::PixelFormatR32Uint);
            break;
          case texture_format_r32i:
            descr->setPixelFormat(MTL::PixelFormatR32Sint);
            break;
          case texture_format_r32f:
            descr->setPixelFormat(MTL::PixelFormatR32Float);
            break;
          case texture_format_r8ui:
            descr->setPixelFormat(MTL::PixelFormatR8Uint);
            break;
          case texture_format_r8i:
            descr->setPixelFormat(MTL::PixelFormatR8Sint);
            break;
          case texture_format_rgba16:
            descr->setPixelFormat(MTL::PixelFormatRGBA16Unorm);
            break;
          default:
            descr->setPixelFormat(MTL::PixelFormatInvalid);
            break;
          }
        descr->setStorageMode(MTL::StorageModeShared);
        MTL::TextureUsage usage = 0;
        if (usage_flags & TEX_USAGE_READ)
          usage |= MTL::TextureUsageShaderRead;
        if (usage_flags & TEX_USAGE_WRITE)
          usage |= MTL::TextureUsageShaderWrite;
        if (usage_flags & TEX_USAGE_RENDER_TARGET)
          usage |= MTL::TextureUsageRenderTarget;
        descr->setUsage(usage);
        MTL::Texture* p_color_texture = mp_device->newTexture(descr);
        tex->metal_texture = (void*)p_color_texture;
        descr->release();
        if (bytes_per_channel == 1)
          update_texture(i, (const uint8_t*)data);
        else
          update_texture(i, (const uint16_t*)data);
        return i;
        }
      ++tex;
      }
    return -1;
    }

  int32_t render_context_metal::add_texture(int32_t w, int32_t h, int32_t format, const uint16_t* data, int32_t usage_flags)
    {
    return _add_texture(w, h, format, (const void*)data, usage_flags, 2);
    }

  int32_t render_context_metal::add_texture(int32_t w, int32_t h, int32_t format, const uint8_t* data, int32_t usage_flags)
    {
    return _add_texture(w, h, format, (const void*)data, usage_flags, 1);
    }

  int32_t render_context_metal::add_cubemap_texture(int32_t w, int32_t h, int32_t format,
    const uint8_t* front,
    const uint8_t* back,
    const uint8_t* left,
    const uint8_t* right,
    const uint8_t* top,
    const uint8_t* bottom,
    int32_t usage_flags)
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
        tex->texture_target = TEX_TARGET_CUBEMAP;
        tex->usage_flags = usage_flags;

        MTL::TextureDescriptor* descr = MTL::TextureDescriptor::alloc()->init();
        descr->setTextureType(MTL::TextureTypeCube);
        descr->setWidth(w);
        descr->setHeight(h);
        descr->setSampleCount(1);
        switch (format)
          {
          case texture_format_rgba8:
            descr->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
            break;
          case texture_format_rgba32f:
            descr->setPixelFormat(MTL::PixelFormatRGBA32Float);
            break;
          case texture_format_bgra8:
            descr->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
            break;
          case texture_format_rgba8ui:
            descr->setPixelFormat(MTL::PixelFormatRGBA8Uint);
            break;
          case texture_format_depth:
            descr->setPixelFormat(MTL::PixelFormatDepth32Float);
            break;
          case texture_format_r32ui:
            descr->setPixelFormat(MTL::PixelFormatR32Uint);
            break;
          case texture_format_r32i:
            descr->setPixelFormat(MTL::PixelFormatR32Sint);
            break;
          case texture_format_r32f:
            descr->setPixelFormat(MTL::PixelFormatR32Float);
            break;
          case texture_format_r8ui:
            descr->setPixelFormat(MTL::PixelFormatR8Uint);
            break;
          case texture_format_r8i:
            descr->setPixelFormat(MTL::PixelFormatR8Sint);
            break;
          case texture_format_rgba16:
            descr->setPixelFormat(MTL::PixelFormatRGBA16Unorm);
            break;
          default:
            descr->setPixelFormat(MTL::PixelFormatInvalid);
            break;
          }
        descr->setStorageMode(MTL::StorageModeShared);
        MTL::TextureUsage usage = 0;
        if (usage_flags & TEX_USAGE_READ)
          usage |= MTL::TextureUsageShaderRead;
        if (usage_flags & TEX_USAGE_WRITE)
          usage |= MTL::TextureUsageShaderWrite;
        if (usage_flags & TEX_USAGE_RENDER_TARGET)
          usage |= MTL::TextureUsageRenderTarget;
        descr->setUsage(usage);
        MTL::Texture* p_color_texture = mp_device->newTexture(descr);
        tex->metal_texture = (void*)p_color_texture;
        descr->release();
        if (tex->format == texture_format_rgba8 || tex->format == texture_format_bgra8) {

          p_color_texture->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, 0, right, tex->w * 4 * sizeof(uint8_t), tex->w * 4 * tex->h * sizeof(uint8_t));
          p_color_texture->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, 1, left, tex->w * 4 * sizeof(uint8_t), tex->w * 4 * tex->h * sizeof(uint8_t));
          p_color_texture->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, 2, top, tex->w * 4 * sizeof(uint8_t), tex->w * 4 * tex->h * sizeof(uint8_t));
          p_color_texture->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, 3, bottom, tex->w * 4 * sizeof(uint8_t), tex->w * 4 * tex->h * sizeof(uint8_t));
          p_color_texture->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, 4, front, tex->w * 4 * sizeof(uint8_t), tex->w * 4 * tex->h * sizeof(uint8_t));
          p_color_texture->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, 5, back, tex->w * 4 * sizeof(uint8_t), tex->w * 4 * tex->h * sizeof(uint8_t));
          }
        return i;
        }
      ++tex;
      }
    return -1;
    }

  bool render_context_metal::update_texture(int32_t handle, const uint8_t* data)
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return false;
    if (data == nullptr)
      return false;
    texture* tex = &_textures[handle];
    if (tex->flags == 0)
      return false;

    if (tex->format == texture_format_rgba8 || tex->format == texture_format_bgra8) {

      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, data, tex->w * 4 * sizeof(uint8_t));

      return true;
      }
    else if (tex->format == texture_format_r8ui || tex->format == texture_format_r8i)
      {
      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, data, tex->w * sizeof(uint8_t));
      return true;
      }
    return false;
    }

  bool render_context_metal::update_texture(int32_t handle, const float* data)
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return false;
    if (data == nullptr)
      return false;
    texture* tex = &_textures[handle];
    if (tex->flags == 0)
      return false;

    if (tex->format == texture_format_r32f) {

      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, data, tex->w * sizeof(float));

      return true;
      }

    return false;
    }

  bool render_context_metal::update_texture(int32_t handle, const uint16_t* data)
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return false;
    if (data == nullptr)
      return false;
    texture* tex = &_textures[handle];
    if (tex->flags == 0)
      return false;

    if (tex->format == texture_format_rgba8) {
      uint8_t* bytes = new uint8_t[tex->w * tex->h * 4];
      uint8_t* d = (uint8_t*)bytes;
      const uint16_t* s = (const uint16_t*)data;
      for (int y = 0; y < tex->h; ++y)
        {
        for (int x = 0; x < tex->w; ++x)
          {
          *d++ = ((*s++ >> 7) & 0xff);
          *d++ = ((*s++ >> 7) & 0xff);
          *d++ = ((*s++ >> 7) & 0xff);
          *d++ = ((*s++ >> 7) & 0xff);
          }
        }

      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, bytes, tex->w * 4 * sizeof(uint8_t));

      delete[] bytes;
      return true;
      }
    else if (tex->format == texture_format_rgba32f)
      {
      float* bytes = new float[tex->w * tex->h * 4];
      float* d = (float*)bytes;
      const uint16_t* s = (const uint16_t*)data;
      for (int y = 0; y < tex->h; ++y)
        {
        for (int x = 0; x < tex->w; ++x)
          {
          *d++ = ((*s++ >> 7) & 0xff) / 255.f;
          *d++ = ((*s++ >> 7) & 0xff) / 255.f;
          *d++ = ((*s++ >> 7) & 0xff) / 255.f;
          *d++ = ((*s++ >> 7) & 0xff) / 255.f;
          }
        }

      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, bytes, tex->w * 4 * sizeof(float));

      delete[] bytes;
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
          *d = (*s & 0x7fff) * 2;
          }
        }
      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, bytes, tex->w * 4 * sizeof(uint16_t));
      delete[] bytes;
      return true;
      }
    else if (tex->format == texture_format_rgba8ui)
      {
      uint8_t* bytes = new uint8_t[tex->w * tex->h * 4];
      uint8_t* d = (uint8_t*)bytes;
      const uint16_t* s = (const uint16_t*)data;
      for (int y = 0; y < tex->h; ++y)
        {
        for (int x = 0; x < tex->w; ++x)
          {
          *d++ = ((*s++ >> 7) & 0xff);
          *d++ = ((*s++ >> 7) & 0xff);
          *d++ = ((*s++ >> 7) & 0xff);
          *d++ = ((*s++ >> 7) & 0xff);
          }
        }

      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, bytes, tex->w * 4 * sizeof(uint8_t));

      delete[] bytes;
      return true;
      }
    else if (tex->format == texture_format_r32ui)
      {
      uint32_t* bytes = new uint32_t[tex->w * tex->h];
      uint32_t* d = (uint32_t*)bytes;
      const uint64_t* s = (const uint64_t*)data;
      for (int y = 0; y < tex->h; ++y)
        {
        for (int x = 0; x < tex->w; ++x)
          {
          *d++ = (uint32_t)(*s++);
          }
        }

      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, bytes, tex->w * sizeof(uint32_t));

      delete[] bytes;
      return true;
      }
    else if (tex->format == texture_format_r32i)
      {
      int32_t* bytes = new int32_t[tex->w * tex->h];
      int32_t* d = (int32_t*)bytes;
      const uint64_t* s = (const uint64_t*)data;
      for (int y = 0; y < tex->h; ++y)
        {
        for (int x = 0; x < tex->w; ++x)
          {
          *d++ = (int32_t)(*s++);
          }
        }

      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, bytes, tex->w * sizeof(int32_t));

      delete[] bytes;
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

      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, bytes, tex->w * sizeof(float));

      delete[] bytes;
      return true;
      }
    else if (tex->format == texture_format_r8ui)
      {
      uint8_t* bytes = new uint8_t[tex->w * tex->h];
      uint8_t* d = (uint8_t*)bytes;
      const uint64_t* s = (const uint64_t*)data;
      for (int y = 0; y < tex->h; ++y)
        {
        for (int x = 0; x < tex->w; ++x)
          {
          *d++ = (uint8_t)(*s++ & 0xff);
          }
        }

      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, bytes, tex->w * sizeof(uint8_t));

      delete[] bytes;
      return true;
      }
    else if (tex->format == texture_format_r8i)
      {
      int8_t* bytes = new int8_t[tex->w * tex->h];
      int8_t* d = (int8_t*)bytes;
      const uint64_t* s = (const uint64_t*)data;
      for (int y = 0; y < tex->h; ++y)
        {
        for (int x = 0; x < tex->w; ++x)
          {
          *d++ = (int8_t)(*s++ & 0xff);
          }
        }

      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->replaceRegion(MTL::Region(0, 0, tex->w, tex->h), 0, bytes, tex->w * sizeof(uint32_t));

      delete[] bytes;
      return true;
      }
    return false;
    }

  void render_context_metal::remove_texture(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return;
    texture* tex = &_textures[handle];
    if (tex->flags == 0)
      return;
    MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
    p_tex->release();
    tex->metal_texture = 0;
    tex->flags = 0;
    }

  void render_context_metal::bind_texture_to_channel(int32_t handle, int32_t channel, int32_t flags)
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return;
    texture* tex = &_textures[handle];
    if (tex->flags == 0)
      return;
    MTL::Texture* p_texture = (MTL::Texture*)tex->metal_texture;

    MTL::SamplerDescriptor* sdescr = MTL::SamplerDescriptor::alloc()->init();
    if (flags & TEX_FILTER_NEAREST)
      {
      sdescr->setMinFilter(MTL::SamplerMinMagFilterNearest);
      sdescr->setMagFilter(MTL::SamplerMinMagFilterNearest);
      }
    else
      {
      sdescr->setMinFilter(MTL::SamplerMinMagFilterLinear);
      sdescr->setMagFilter(MTL::SamplerMinMagFilterLinear);
      }
    if (flags & TEX_WRAP_CLAMP_TO_EDGE)
      {
      sdescr->setSAddressMode(MTL::SamplerAddressModeClampToEdge);
      sdescr->setTAddressMode(MTL::SamplerAddressModeClampToEdge);
      }
    else
      {
      sdescr->setSAddressMode(MTL::SamplerAddressModeRepeat);
      sdescr->setTAddressMode(MTL::SamplerAddressModeRepeat);
      }
    MTL::SamplerState* p_sampler_state = mp_device->newSamplerState(sdescr);
    sdescr->release();

    if (mp_render_command_encoder)
      {
      mp_render_command_encoder->setFragmentSamplerState(p_sampler_state, channel);
      mp_render_command_encoder->setFragmentTexture(p_texture, channel);
      }
    if (mp_compute_command_encoder)
      {
      mp_compute_command_encoder->setTexture(p_texture, channel);
      }
    p_sampler_state->release();
    }

  const texture* render_context_metal::get_texture(int32_t handle) const
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return nullptr;
    return &_textures[handle];
    }

  void render_context_metal::get_data_from_texture(int32_t handle, void* data, int32_t size)
    {
    if (handle < 0 || handle >= MAX_TEXTURE)
      return false;
    if (data == nullptr)
      return false;
    texture* tex = &_textures[handle];
    if (tex->flags == 0)
      return false;

    if (tex->format == texture_format_rgba8)
      {
      if (size < tex->w * tex->h * 4)
        return;
      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->getBytes(data, tex->w * 4 * sizeof(uint8_t), MTL::Region(0, 0, tex->w, tex->h), 0);
      }
    else if (tex->format == texture_format_bgra8)
      {
      if (size < tex->w * tex->h * 4)
        return;
      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->getBytes(data, tex->w * 4 * sizeof(uint8_t), MTL::Region(0, 0, tex->w, tex->h), 0);
      }
    else if (tex->format == texture_format_rgba32f)
      {
      if (size < tex->w * tex->h * 4)
        return;
      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      float* bytes = new float[tex->w * tex->h * 4];
      p_tex->getBytes(bytes, tex->w * 4 * sizeof(float), MTL::Region(0, 0, tex->w, tex->h), 0);
      uint8_t* d = (uint8_t*)data;
      float* b = bytes;
      for (int y = 0; y < tex->h; ++y)
        {
        for (int x = 0; x < tex->w; ++x)
          {
          *d++ = (uint8_t)(*b++ * 255.f);
          *d++ = (uint8_t)(*b++ * 255.f);
          *d++ = (uint8_t)(*b++ * 255.f);
          *d++ = (uint8_t)(*b++ * 255.f);
          }
        }

      delete[] bytes;
      }
    else if (tex->format == texture_format_rgba16)
      {
      if (size < tex->w * tex->h * 8)
        return;
      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->getBytes(data, tex->w * 4 * sizeof(uint16_t), MTL::Region(0, 0, tex->w, tex->h), 0);
      }
    else if (tex->format == texture_format_rgba8ui)
      {
      if (size < tex->w * tex->h * 4)
        return;
      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->getBytes(data, tex->w * 4 * sizeof(uint8_t), MTL::Region(0, 0, tex->w, tex->h), 0);
      }
    else if (tex->format == texture_format_r32f)
      {
      if (size < tex->w * tex->h * 4)
        return;
      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->getBytes(data, tex->w * sizeof(float), MTL::Region(0, 0, tex->w, tex->h), 0);
      }
    else if (tex->format == texture_format_r32ui || tex->format == texture_format_r32i)
      {
      if (size < tex->w * tex->h * 4)
        return;
      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->getBytes(data, tex->w * sizeof(uint32_t), MTL::Region(0, 0, tex->w, tex->h), 0);
      }
    else if (tex->format == texture_format_r8ui || tex->format == texture_format_r8i)
      {
      if (size < tex->w * tex->h)
        return;
      MTL::Texture* p_tex = (MTL::Texture*)tex->metal_texture;
      p_tex->getBytes(data, tex->w * sizeof(uint8_t), MTL::Region(0, 0, tex->w, tex->h), 0);
      }
    }

  int32_t render_context_metal::add_geometry(int32_t vertex_declaration_type)
    {
    if (vertex_declaration_type < VERTEX_STANDARD || vertex_declaration_type > VERTEX_2_2_3)
      return -1;
    geometry_handle* gh = _geometry_handles;
    for (int32_t i = 0; i < MAX_GEOMETRY; ++i)
      {
      if (gh->mode == 0)
        {
        memset(gh, 0, sizeof(*gh));
        //gh->vertex_size = gl_buffer_declaration_table[vertex_declaration_type].size;
        switch (vertex_declaration_type)
          {
          case VERTEX_STANDARD:
            gh->vertex_size = 32;
            break;
          case VERTEX_COMPACT:
            gh->vertex_size = 16;
            break;
          case VERTEX_COLOR:
            gh->vertex_size = 28;
            break;
          case VERTEX_2_2_3:
            gh->vertex_size = 28;
            break;
          default:
            gh->vertex_size = 32;
            break;
          }
        gh->vertex_declaration_type = vertex_declaration_type;
        gh->mode = GEOMETRY_ALLOCATED;
        gh->locked = 0;
        gh->vertex.buffer = -1;
        gh->index.buffer = -1;
        return i;
        }
      ++gh;
      }
    return -1;
    }

  void render_context_metal::_remove_geometry_buffer(geometry_ref& ref)
    {
    if (ref.buffer < 0 || ref.buffer >= MAX_BUFFER_OBJECT)
      return;
    buffer_object* buf = &_buffer_objects[ref.buffer];
    if (buf->size > 0)
      {
      delete[] buf->raw;
      MTL::Buffer* p_buf = (MTL::Buffer*)buf->metal_buffer;
      p_buf->release();
      }
    buf->size = 0;
    buf->raw = nullptr;
    buf->type = 0;
    buf->metal_buffer = 0;
    ref.count = 0;
    }

  void render_context_metal::remove_geometry(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_GEOMETRY)
      return;
    geometry_handle* geo = &_geometry_handles[handle];
    if (geo->mode == 0)
      return;
    assert(geo->locked == 0);
    geo->mode = 0;
    //glDeleteVertexArrays(1, &geo->gl_vertex_array_object_id);
    //glCheckError();
    _remove_geometry_buffer(geo->vertex);
    _remove_geometry_buffer(geo->index);
    }

  int32_t render_context_metal::add_render_buffer()
    {
    return -1;
    }

  void render_context_metal::remove_render_buffer(int32_t handle)
    {

    }

  int32_t render_context_metal::add_frame_buffer(int32_t w, int32_t h, bool make_depth_texture)
    {
    frame_buffer* fb = _frame_buffers;
    for (int32_t i = 0; i < MAX_FRAMEBUFFER; ++i)
      {
      if (fb->texture_handle < 0)
        {
        fb->w = w;
        fb->h = h;
        fb->texture_handle = add_texture(w, h, texture_format_bgra8, (const uint16_t*)nullptr, TEX_USAGE_RENDER_TARGET | TEX_USAGE_READ);
        if (make_depth_texture)
          fb->depth_texture_handle = add_texture(w, h, texture_format_depth, (const uint16_t*)nullptr, TEX_USAGE_RENDER_TARGET);
        else
          fb->render_buffer_handle = add_render_buffer();
        if (fb->texture_handle < 0)
          return -1;
        if (make_depth_texture && fb->depth_texture_handle < 0)
          return -1;
        return i;
        }
      ++fb;
      }
    return -1;
    }

  void render_context_metal::remove_frame_buffer(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_FRAMEBUFFER)
      return;
    frame_buffer* fb = &_frame_buffers[handle];
    if (fb->texture_handle < 0)
      return;
    remove_texture(fb->texture_handle);
    remove_render_buffer(fb->render_buffer_handle);

    fb->texture_handle = -1;
    fb->render_buffer_handle = -1;
    }

  const frame_buffer* render_context_metal::get_frame_buffer(int32_t handle) const
    {
    if (handle < 0 || handle >= MAX_FRAMEBUFFER)
      return nullptr;
    return &_frame_buffers[handle];
    }

  int32_t render_context_metal::add_buffer_object(const void* data, int32_t size, int32_t buffer_type)
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
        MTL::ResourceOptions options = MTL::ResourceStorageModeShared;
        MTL::Buffer* p_buffer;
        if (data == nullptr)
          p_buffer = mp_device->newBuffer(size, options);
        else
          p_buffer = mp_device->newBuffer(data, size, options);
        buf->metal_buffer = (MTL::Buffer*)p_buffer;
        return i;
        }
      ++buf;
      }
    return -1;
    }

  void render_context_metal::remove_buffer_object(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_BUFFER_OBJECT)
      return;
    buffer_object* buf = &_buffer_objects[handle];
    if (buf->size > 0)
      {
      delete[] buf->raw;
      MTL::Buffer* p_buf = (MTL::Buffer*)buf->metal_buffer;
      p_buf->release();
      }
    buf->size = 0;
    buf->raw = nullptr;
    buf->type = 0;
    }

  void render_context_metal::update_buffer_object(int32_t handle, const void* data, int32_t size)
    {
    if (handle < 0 || handle >= MAX_BUFFER_OBJECT)
      return;
    buffer_object* buf = &_buffer_objects[handle];
    if (buf->size > 0)
      {
      MTL::Buffer* p_buf = (MTL::Buffer*)buf->metal_buffer;
      assert(0); // todo
      }
    }

  void render_context_metal::bind_buffer_object(int32_t handle, int32_t channel, int32_t target)
    {
    if (handle < 0 || handle >= MAX_BUFFER_OBJECT)
      return;
    buffer_object* buf = &_buffer_objects[handle];
    if (buf->size > 0)
      {
      MTL::Buffer* p_buf = (MTL::Buffer*)buf->metal_buffer;
      if (mp_render_command_encoder)
        {
        if (target == BIND_TO_DEFAULT)
          {
          switch (buf->type)
            {
            case GEOMETRY_VERTEX:
              mp_render_command_encoder->setVertexBuffer(p_buf, 0, channel);
              break;
            case GEOMETRY_INDEX:
              mp_render_command_encoder->setFragmentBuffer(p_buf, 0, channel);
              break;
            case COMPUTE_BUFFER:
              mp_render_command_encoder->setFragmentBuffer(p_buf, 0, channel);
              break;
            default:
              break;
            }
          }
        else
          {
          switch (target)
            {
            case BIND_TO_VERTEX_SHADER:
              mp_render_command_encoder->setVertexBuffer(p_buf, 0, channel);
              break;
            case BIND_TO_FRAGMENT_SHADER:
              mp_render_command_encoder->setFragmentBuffer(p_buf, 0, channel);
              break;              
            }
          }
        }
      if (mp_compute_command_encoder)
        {
        switch (buf->type)
          {
          case COMPUTE_BUFFER:
            mp_compute_command_encoder->setBuffer(p_buf, 0, channel);
            break;
          default:
            break;
          }
        }
      }
    }

  void render_context_metal::get_data_from_buffer_object(int32_t handle, void* data, int32_t size)
    {
    if (handle < 0 || handle >= MAX_BUFFER_OBJECT)
      return;
    buffer_object* buf = &_buffer_objects[handle];
    if (buf->size > 0)
      {
      MTL::Buffer* p_buf = (MTL::Buffer*)buf->metal_buffer;
      memcpy(data, p_buf->contents(), size);
      }
    }

  const buffer_object* render_context_metal::get_buffer_object(int32_t handle) const
    {
    if (handle < 0 || handle >= MAX_BUFFER_OBJECT)
      return nullptr;
    return &_buffer_objects[handle];
    }

  void render_context_metal::copy_buffer_object_data(int32_t source_handle, int32_t destination_handle, uint32_t read_offset, uint32_t write_offset, uint32_t size)
    {
    if (source_handle < 0 || source_handle >= MAX_BUFFER_OBJECT)
      return;
    if (read_offset < 0 || read_offset >= MAX_BUFFER_OBJECT)
      return;
    buffer_object* src = &_buffer_objects[source_handle];
    buffer_object* dst = &_buffer_objects[destination_handle];
    MTL::Buffer* p_buf_src = (MTL::Buffer*)src->metal_buffer;
    MTL::Buffer* p_buf_dst = (MTL::Buffer*)dst->metal_buffer;
    MTL::CommandBuffer* p_command_buffer = mp_command_queue->commandBuffer();
    //p_command_buffer->addCompletedHandler([&](MTL::CommandBuffer* buf){dispatch_semaphore_signal(_semaphore);});
    MTL::BlitCommandEncoder* p_blit = p_command_buffer->blitCommandEncoder();
    p_blit->copyFromBuffer(p_buf_src, read_offset, p_buf_dst, write_offset, size);
    p_blit->endEncoding();
    p_command_buffer->commit();
    }


  void render_context_metal::_allocate_geometry_buffer(geometry_ref& ref, int32_t tuple_size, int32_t count, int32_t type, void** pointer)
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
      if (buf->size != 0) // release old buffer
        {
        MTL::Buffer* p_buf = (MTL::Buffer*)buf->metal_buffer;
        buf->metal_buffer = 0;
        p_buf->release();
        }
      if (buf->raw)
        {
        delete[] buf->raw;
        }
      buf->raw = new uint8_t[size];
      buf->size = size;
      buf->type = type;
      ref.count = count;
      }
    if (pointer)
      {
      *pointer = (void*)buf->raw;
      }
    }

  void render_context_metal::geometry_begin(int32_t handle, int32_t number_of_vertices, int32_t number_of_indices, float** vertex_pointer, void** index_pointer, int32_t update)
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
      _allocate_geometry_buffer(gh->vertex, gh->vertex_size, number_of_vertices, GEOMETRY_VERTEX, (void**)vertex_pointer);
      }
    if ((update & GEOMETRY_INDEX) && !(gh->locked & GEOMETRY_INDEX)) // indices
      {
      gh->locked |= GEOMETRY_INDEX;
      _allocate_geometry_buffer(gh->index, sizeof(uint32_t), number_of_indices, GEOMETRY_INDEX, (void**)index_pointer);
      }
    }

  void render_context_metal::_update_geometry_buffer(geometry_ref& ref)
    {
    if (ref.buffer < 0)
      return;
    buffer_object* buf = &_buffer_objects[ref.buffer];
    MTL::ResourceOptions options = 0;
    MTL::Buffer* p_buffer = mp_device->newBuffer((const void*)buf->raw, buf->size, options);
    buf->metal_buffer = p_buffer;
    }

  void render_context_metal::geometry_end(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_GEOMETRY)
      return;
    geometry_handle* gh = &_geometry_handles[handle];
    if (gh->mode == 0)
      return;
    if (gh->locked & GEOMETRY_VERTEX)
      {
      _update_geometry_buffer(gh->vertex);
      gh->locked &= ~GEOMETRY_VERTEX;
      }
    if (gh->locked & GEOMETRY_INDEX)
      {
      _update_geometry_buffer(gh->index);
      gh->locked &= ~GEOMETRY_INDEX;
      }
    }

  void render_context_metal::geometry_draw(int32_t handle, int32_t instance_count)
    {
    if (!mp_render_command_encoder)
      return;
    if (handle < 0 || handle >= MAX_GEOMETRY)
      return;
    geometry_handle* gh = &_geometry_handles[handle];
    if (gh->mode == 0)
      return;

    while (_raw_uniforms.size() % 16)
      _raw_uniforms.push_back(0);

    mp_render_command_encoder->setVertexBytes(_raw_uniforms.data(), _raw_uniforms.size(), 10);
    mp_render_command_encoder->setFragmentBytes(_raw_uniforms.data(), _raw_uniforms.size(), 10);

    if (gh->vertex.buffer >= 0 && gh->vertex.buffer < MAX_BUFFER_OBJECT)
      {
      buffer_object* buf = &_buffer_objects[gh->vertex.buffer];
      MTL::Buffer* p_buffer = (MTL::Buffer*)buf->metal_buffer;
      mp_render_command_encoder->setVertexBuffer(p_buffer, 0, 0);
      }
    if (gh->index.buffer >= 0 && gh->index.buffer < MAX_BUFFER_OBJECT)
      {
      buffer_object* buf = &_buffer_objects[gh->index.buffer];
      MTL::Buffer* p_buffer = (MTL::Buffer*)buf->metal_buffer;
      mp_render_command_encoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, gh->index.count, MTL::IndexTypeUInt32, p_buffer, 0, instance_count, 0, 0);
      }
    }

  int32_t render_context_metal::add_shader(const char* source, int32_t type, const char* name)
    {
    if (type < SHADER_VERTEX || type > SHADER_COMPUTE)
      return -1;
    shader* sh = _shaders;
    for (int32_t i = 0; i < MAX_SHADER; ++i)
      {
      if (sh->type == 0)
        {
        sh->type = type;
        MTL::Function* shader_function = nullptr;
        if (source == nullptr)
          {
          NS::String* shader_name = NS::String::string(name, NS::UTF8StringEncoding);
          shader_function = mp_default_library->newFunction(shader_name);
          }
        else
          {
          NS::Error* error;
          NS::String* source_code = NS::String::string(source, NS::UTF8StringEncoding);
          MTL::CompileOptions* options = MTL::CompileOptions::alloc()->init();
          MTL::Library* lib = mp_device->newLibrary(source_code, options, &error);
          NS::String* shader_name = NS::String::string(name, NS::UTF8StringEncoding);
          shader_function = lib->newFunction(shader_name);
          options->release();
          lib->release();
          if (error != NULL)
            {
            std::string log(error->localizedDescription()->utf8String());
            throw std::runtime_error(log);
            }
          }
        sh->metal_shader = (void*)shader_function;
        sh->compiled = 1;
        return i;
        }
      ++sh;
      }
    return -1;
    }

  void render_context_metal::remove_shader(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_SHADER)
      return;
    shader* sh = &_shaders[handle];
    if (sh->type == 0)
      return;
    MTL::Function* shader_function = (MTL::Function*)sh->metal_shader;
    shader_function->release();
    sh->type = 0;
    sh->compiled = 0;
        
    for (int32_t i = 0; i < MAX_PIPELINESTATE_CACHE; ++i)
      {
      if (m_pipeline_state_cache[i].p_pipeline)
        {
        if (m_pipeline_state_cache[i].fragment_shader_handle == handle || m_pipeline_state_cache[i].vertex_shader_handle == handle)
          {
          m_pipeline_state_cache[i].p_pipeline->release();
          m_pipeline_state_cache[i].color_pixel_format = 0;
          m_pipeline_state_cache[i].depth_pixel_format = 0;
          m_pipeline_state_cache[i].fragment_shader_handle = 0;
          m_pipeline_state_cache[i].vertex_shader_handle = 0;
          m_pipeline_state_cache[i].p_pipeline = nullptr;
          }
        }
      if (m_compute_pipeline_state_cache[i].p_pipeline)
        {
        if (m_compute_pipeline_state_cache[i].compute_shader_handle == handle)
          {
          m_compute_pipeline_state_cache[i].p_pipeline->release();
          m_compute_pipeline_state_cache[i].compute_shader_handle = 0;
          m_compute_pipeline_state_cache[i].p_pipeline = nullptr;
          }
        }
      }
    }

  namespace
    {

    MTL::PixelFormat _convert(int32_t v)
      {
      switch (v)
        {
        case texture_format_none:
          return MTL::PixelFormatInvalid;
        case texture_format_rgba8:
          return MTL::PixelFormatBGRA8Unorm;
        case texture_format_rgba32f:
          return MTL::PixelFormatBGRA8Unorm;
        case texture_format_bgra8:
          return MTL::PixelFormatBGRA8Unorm;
        case texture_format_rgba8ui:
          return MTL::PixelFormatRGBA8Uint;
        case texture_format_r32i:
          return MTL::PixelFormatR32Sint;
        case texture_format_r32ui:
          return MTL::PixelFormatR32Uint;
        case texture_format_r32f:
          return MTL::PixelFormatR32Float;
        case texture_format_r8i:
          return MTL::PixelFormatR8Sint;
        case texture_format_r8ui:
          return MTL::PixelFormatR8Uint;
        case texture_format_rgba16:
          return MTL::PixelFormatRGBA16Unorm;
        case texture_format_depth:
          return MTL::PixelFormatDepth32Float;
        default: break;
        }
      return MTL::PixelFormatInvalid;
      }

    }
    
  namespace
    {
    MTL::BlendFactor convert(blending_type source)
      {
      MTL::BlendFactor factor = MTL::BlendFactorOne;
      switch (source)
        {
          case blending_type::zero:
            factor = MTL::BlendFactorZero;
            break;
          case blending_type::one:
          factor = MTL::BlendFactorOne;
            break;
          case blending_type::src_color:
            factor = MTL::BlendFactorSourceColor;
            break;
          case blending_type::one_minus_src_color:
            factor = MTL::BlendFactorOneMinusSourceColor;
            break;
          case blending_type::dst_color:
            factor = MTL::BlendFactorDestinationColor;
            break;
          case blending_type::one_minus_dst_color:
            factor = MTL::BlendFactorOneMinusDestinationColor;
            break;
          case blending_type::src_alpha:
            factor = MTL::BlendFactorSourceAlpha;
            break;
          case blending_type::one_minus_src_alpha:
            factor = MTL::BlendFactorOneMinusSourceAlpha;
            break;
          case blending_type::dst_alpha:
            factor = MTL::BlendFactorDestinationAlpha;
            break;
          case blending_type::one_minus_dst_alpha:
            factor = MTL::BlendFactorOneMinusDestinationAlpha;
            break;
        }
        return factor;
      }
      
    MTL::BlendOperation convert(blending_equation_type func)
      {
      MTL::BlendOperation f = MTL::BlendOperationAdd;
      switch (func)
        {
        case blending_equation_type::add:
          f = MTL::BlendOperationAdd;
          break;
        case blending_equation_type::subtract:
          f = MTL::BlendOperationSubtract;
          break;
        case blending_equation_type::reverse_subtract:
          f = MTL::BlendOperationReverseSubtract;
          break;
        case blending_equation_type::minimum:
          f = MTL::BlendOperationMin;
          break;
        case blending_equation_type::maximum:
          f = MTL::BlendOperationMax;
          break;
        }
      return f;
      }
    }

  MTL::RenderPipelineState* render_context_metal::_get_render_pipeline_state(int32_t vertex_shader_handle, int32_t fragment_shader_handle, int32_t color_pixel_format, int32_t depth_pixel_format)
    {
    uint32_t hash = 2166136261;
    hash = (hash ^ (uint32_t)(vertex_shader_handle)) * 16777619;
    hash = (hash ^ (uint32_t)(fragment_shader_handle)) * 16777619;
    hash = (hash ^ (uint32_t)(color_pixel_format)) * 16777619;
    hash = (hash ^ (uint32_t)(depth_pixel_format)) * 16777619;
    uint32_t bucket = hash % MAX_PIPELINESTATE_CACHE;
    RenderPipelineStateCache* pipeline = &(m_pipeline_state_cache[bucket]);
    for (int32_t i = 0; i < MAX_PIPELINESTATE_CACHE; ++i)
      {
      if (pipeline->p_pipeline == nullptr)
        {
        shader* vs = vertex_shader_handle >= 0 ? &_shaders[vertex_shader_handle] : nullptr;
        shader* fs = fragment_shader_handle >= 0 ? &_shaders[fragment_shader_handle] : nullptr;

        MTL::RenderPipelineDescriptor* descr = MTL::RenderPipelineDescriptor::alloc()->init();
        MTL::Function* vertex_function = (MTL::Function*)vs->metal_shader;
        MTL::Function* fragment_function = (MTL::Function*)fs->metal_shader;
        descr->setVertexFunction(vertex_function);
        descr->setFragmentFunction(fragment_function);
        descr->colorAttachments()->object(0)->setPixelFormat(_convert(color_pixel_format));
        descr->colorAttachments()->object(0)->setBlendingEnabled(_enable_blending);

        descr->colorAttachments()->object(0)->setAlphaBlendOperation(convert(_blending_func));
        descr->colorAttachments()->object(0)->setRgbBlendOperation(convert(_blending_func));

        descr->colorAttachments()->object(0)->setSourceRGBBlendFactor(convert(_blending_source));
        descr->colorAttachments()->object(0)->setSourceAlphaBlendFactor(convert(_blending_source));

        descr->colorAttachments()->object(0)->setDestinationRGBBlendFactor(convert(_blending_destination));
        descr->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(convert(_blending_destination));

        descr->setDepthAttachmentPixelFormat(_convert(depth_pixel_format));

        NS::Error* err;
        pipeline->p_pipeline = mp_device->newRenderPipelineState(descr, &err);
        pipeline->vertex_shader_handle = vertex_shader_handle;
        pipeline->fragment_shader_handle = fragment_shader_handle;
        pipeline->color_pixel_format = color_pixel_format;
        pipeline->depth_pixel_format = depth_pixel_format;
        descr->release();
        return pipeline->p_pipeline;
        }
      else if (pipeline->vertex_shader_handle == vertex_shader_handle &&
        pipeline->fragment_shader_handle == fragment_shader_handle &&
        pipeline->color_pixel_format == color_pixel_format &&
        pipeline->depth_pixel_format == depth_pixel_format)
        return pipeline->p_pipeline;
      bucket = (bucket + 1) % MAX_PIPELINESTATE_CACHE;
      pipeline = &(m_pipeline_state_cache[bucket]);
      }
    return nullptr;
    }

  MTL::ComputePipelineState* render_context_metal::_get_compute_pipeline_state(int32_t compute_shader_handle)
    {
    uint32_t hash = 2166136261;
    hash = (hash ^ (uint32_t)(compute_shader_handle)) * 16777619;
    uint32_t bucket = hash % MAX_PIPELINESTATE_CACHE;
    ComputePipelineStateCache* pipeline = &(m_compute_pipeline_state_cache[bucket]);
    for (int32_t i = 0; i < MAX_PIPELINESTATE_CACHE; ++i)
      {
      if (pipeline->p_pipeline == nullptr)
        {
        shader* cs = compute_shader_handle >= 0 ? &_shaders[compute_shader_handle] : nullptr;
        MTL::Function* compute_function = (MTL::Function*)cs->metal_shader;
        NS::Error* err;
        pipeline->p_pipeline = mp_device->newComputePipelineState(compute_function, &err);
        pipeline->compute_shader_handle = compute_shader_handle;
        return pipeline->p_pipeline;
        }
      else if (pipeline->compute_shader_handle == compute_shader_handle)
        return pipeline->p_pipeline;
      bucket = (bucket + 1) % MAX_PIPELINESTATE_CACHE;
      pipeline = &(m_compute_pipeline_state_cache[bucket]);
      }
    return nullptr;
    }

  void render_context_metal::dispatch_compute(int32_t num_groups_x, int32_t num_groups_y, int32_t num_groups_z, int32_t local_size_x, int32_t local_size_y, int32_t local_size_z)
    {
    if (mp_compute_command_encoder)
      {
      while (_raw_uniforms.size() % 16)
        _raw_uniforms.push_back(0);
      mp_compute_command_encoder->setBytes(_raw_uniforms.data(), _raw_uniforms.size(), 10); // channel 10 is reserved for uniforms!!
      MTL::Size num_threads_groups(num_groups_x, num_groups_y, num_groups_z);
      MTL::Size threads_per_thread_group(local_size_x, local_size_y, local_size_z);
      mp_compute_command_encoder->dispatchThreadgroups(num_threads_groups, threads_per_thread_group);
      }
    }

  int32_t render_context_metal::add_program(int32_t vertex_shader_handle, int32_t fragment_shader_handle, int32_t compute_shader_handle)
    {
    if ((vertex_shader_handle < 0 || fragment_shader_handle < 0) && (compute_shader_handle < -1))
      return -1;
    if (vertex_shader_handle >= MAX_SHADER || fragment_shader_handle >= MAX_SHADER || compute_shader_handle >= MAX_SHADER)
      return -1;
    shader_program* sh = _shader_programs;
    for (int32_t i = 0; i < MAX_SHADER_PROGRAM; ++i)
      {
      if (sh->vertex_shader_handle < 0 && sh->fragment_shader_handle < 0 && sh->compute_shader_handle < 0)
        {
        sh->vertex_shader_handle = vertex_shader_handle;
        sh->fragment_shader_handle = fragment_shader_handle;
        sh->compute_shader_handle = compute_shader_handle;
        if (compute_shader_handle >= 0)
          {
          shader* cs = &_shaders[compute_shader_handle];
          if (cs->compiled)
            {
            sh->linked = 1;
            }
          }
        else
          {
          shader* vs = &_shaders[vertex_shader_handle];
          shader* fs = &_shaders[fragment_shader_handle];
          if (vs->compiled && fs->compiled)
            {
            sh->linked = 1;
            }
          }
        return i;
        }
      ++sh;
      }
    return -1;
    }

  void render_context_metal::remove_program(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_SHADER_PROGRAM)
      return;
    shader_program* sh = &_shader_programs[handle];
    if (sh->linked == 0)
      return;
    sh->vertex_shader_handle = -1;
    sh->fragment_shader_handle = -1;
    }

  void render_context_metal::bind_program(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_SHADER_PROGRAM)
      return;
    shader_program* sh = &_shader_programs[handle];
    if (sh->linked == 0)
      return;
    int32_t vs = sh->vertex_shader_handle;
    int32_t fs = sh->fragment_shader_handle;
    int32_t cs = sh->compute_shader_handle;
    int32_t color_pixel_format = texture_format_bgra8;
    int32_t depth_pixel_format = texture_format_none;
    if (m_current_renderpass_descriptor.frame_buffer_handle >= 0)
      {
      const frame_buffer* p_framebuffer = get_frame_buffer(m_current_renderpass_descriptor.frame_buffer_handle);
      if (p_framebuffer->depth_texture_handle >= 0)
        {
        depth_pixel_format = texture_format_depth;
        }
      }
    else if (m_current_renderpass_descriptor.depth_texture_handle >= 0)
      {
      depth_pixel_format = texture_format_depth;
      }
    if (cs >= 0)
      {
      MTL::ComputePipelineState* pipeline = _get_compute_pipeline_state(cs);
      mp_compute_command_encoder->setComputePipelineState(pipeline);
      }
    else
      {
      MTL::RenderPipelineState* pipeline = _get_render_pipeline_state(vs, fs, color_pixel_format, depth_pixel_format);
      mp_render_command_encoder->setRenderPipelineState(pipeline);
      }
    }

  void render_context_metal::bind_uniform(int32_t program_handle, int32_t uniform_handle)
    {
    if (program_handle < 0 || program_handle >= MAX_SHADER_PROGRAM)
      return;
    shader_program* sh = &_shader_programs[program_handle];
    if (sh->linked == 0)
      return;
    if (uniform_handle < 0 || uniform_handle >= MAX_UNIFORMS)
      return;
    uniform_value* uni = &_uniforms[uniform_handle];
    uint32_t alignment = uniform_type_to_alignment[uni->uniform_type].align;
    uint32_t size = uniform_type_to_alignment[uni->uniform_type].size;
    assert(uni->uniform_type == uniform_type_to_alignment[uni->uniform_type].uniform_type);
    while (_raw_uniforms.size() % alignment)
      _raw_uniforms.push_back(0);
    for (int32_t i = 0; i < uni->size; ++i)
      _raw_uniforms.push_back(uni->raw[i]);
    for (int32_t i = uni->size; i < size * uni->num; ++i)
      _raw_uniforms.push_back(0);
    }

  int32_t render_context_metal::add_query()
    {
    query_handle* q = _queries;
    for (int32_t i = 0; i < MAX_QUERIES; ++i)
      {
      if (q->mode == 0)
        {
        q->mode = 1;
        return i;
        }
      ++q;
      }
    return -1;
    }

  void render_context_metal::remove_query(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_QUERIES)
      return;
    query_handle* q = &_queries[handle];
    q->mode = 0;
    }

  void render_context_metal::query_timestamp(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_QUERIES)
      return;
    query_handle* q = &_queries[handle];
    if (q->mode == 0)
      return;
    MTL::Timestamp cputime, gputime;
    mp_device->sampleTimestamps(&cputime, &gputime);
    q->metal_timestamp = (uint64_t)gputime;
    }

  uint64_t render_context_metal::get_query_result(int32_t handle)
    {
    if (handle < 0 || handle >= MAX_QUERIES)
      return 0xffffffffffffffff;
    query_handle* q = &_queries[handle];
    if (q->mode == 0)
      return 0xffffffffffffffff;
    return q->metal_timestamp;
    }

  void render_context_metal::set_blending_enabled(bool enable)
    {
    _enable_blending = enable;
    }

  void render_context_metal::set_blending_function(blending_type source, blending_type destination)
    {
    _blending_source = source;
    _blending_destination = destination;
    }

  void render_context_metal::set_blending_equation(blending_equation_type func)
    {
    _blending_func = func;
    }
    
  void* render_context_metal::get_command_buffer()
    {
    return mp_command_buffer;
    }

  } // namespace RenderDoos
