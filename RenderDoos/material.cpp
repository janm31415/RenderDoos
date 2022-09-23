#include "material.h"
#include <string>
#include "render_engine.h"
#include "types.h"

namespace RenderDoos
  {

  static std::string get_compact_material_vertex_shader()
    {
    return std::string(R"(#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in uint vColor;

out vec4 Color;

uniform mat4 ViewProject; // columns
uniform mat4 Camera; // columns

void main() 
  {
  Color = vec4(float(vColor&uint(255))/255.f, float((vColor>>8)&uint(255))/255.f, float((vColor>>16)&uint(255))/255.f, float((vColor>>24)&uint(255))/255.f);
  gl_Position = ViewProject*vec4(vPosition.xyz,1); 
  }
)");
    }

  static std::string get_compact_material_fragment_shader()
    {
    return std::string(R"(#version 330 core
out vec4 FragColor;
in vec4 Color;

void main()
  {  
  FragColor = Color;
  }
)");
    }

  compact_material::compact_material()
    {
    vs_handle = -1;
    fs_handle = -1;
    shader_program_handle = -1;
    vp_handle = -1;
    }

  compact_material::~compact_material()
    {
    }
    
  void compact_material::destroy(render_engine* engine)
    {
    engine->remove_shader(vs_handle);
    engine->remove_shader(fs_handle);
    engine->remove_program(shader_program_handle);
    engine->remove_uniform(vp_handle);
    }

  void compact_material::compile(render_engine* engine)
    {
    if (engine->get_renderer_type() == renderer_type::METAL)
      {
      vs_handle = engine->add_shader(nullptr, SHADER_VERTEX, "compact_material_vertex_shader");
      fs_handle = engine->add_shader(nullptr, SHADER_FRAGMENT, "compact_material_fragment_shader");
      }
    else if (engine->get_renderer_type() == renderer_type::OPENGL)
      {
      vs_handle = engine->add_shader(get_compact_material_vertex_shader().c_str(), SHADER_VERTEX, nullptr);
      fs_handle = engine->add_shader(get_compact_material_fragment_shader().c_str(), SHADER_FRAGMENT, nullptr);
      }
    shader_program_handle = engine->add_program(vs_handle, fs_handle);
    vp_handle = engine->add_uniform("ViewProject", uniform_type::mat4, 1);
    }

  void compact_material::bind(render_engine* engine)
    {
    engine->bind_program(shader_program_handle);    
    engine->set_uniform(vp_handle, (void*)(&engine->get_view_project()));
    engine->bind_uniform(shader_program_handle, vp_handle);
    }


  ///////////////////////////////////////////////////////////////////////

  static std::string get_vertex_colored_material_vertex_shader()
    {
    return std::string(R"(#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in uint vColor;
uniform mat4 ViewProject; // columns
uniform mat4 Camera; // columns

out vec3 Normal;
out vec4 Color;

void main() 
  {
  gl_Position = ViewProject*vec4(vPosition.xyz,1);
  Normal = (Camera*vec4(vNormal,0)).xyz;  
  Color = vec4(float(vColor&uint(255))/255.f, float((vColor>>8)&uint(255))/255.f, float((vColor>>16)&uint(255))/255.f, float((vColor>>24)&uint(255))/255.f);  
  }
)");
    }

  static std::string get_vertex_colored_material_fragment_shader()
    {
    return std::string(R"(#version 330 core
out vec4 FragColor;
  
in vec3 Normal;
in vec4 Color;

uniform vec3 LightDir;
uniform float Ambient;

void main()
  {
  float l = clamp(dot(Normal,LightDir), 0, 1.0 - Ambient) + Ambient;
  vec4 clr = Color*l;
  FragColor = Color;
  }
)");
    }

  vertex_colored_material::vertex_colored_material()
    {
    vs_handle = -1;
    fs_handle = -1;
    shader_program_handle = -1;
    ambient = 0.2f;
    vp_handle = -1;
    cam_handle = -1;
    light_dir_handle = -1;
    ambient_handle = -1;
    }

  vertex_colored_material::~vertex_colored_material()
    {
    }

  void vertex_colored_material::set_ambient(float a)
    {
    ambient = a;
    }

  void vertex_colored_material::destroy(render_engine* engine)
    {
    engine->remove_program(shader_program_handle);
    engine->remove_shader(vs_handle);
    engine->remove_shader(fs_handle);
    engine->remove_uniform(vp_handle);
    engine->remove_uniform(cam_handle);
    engine->remove_uniform(light_dir_handle);
    engine->remove_uniform(ambient_handle);
    }

  void vertex_colored_material::compile(render_engine* engine)
    {
    if (engine->get_renderer_type() == renderer_type::METAL)
      {
      vs_handle = engine->add_shader(nullptr, SHADER_VERTEX, "vertex_colored_material_vertex_shader");
      fs_handle = engine->add_shader(nullptr, SHADER_FRAGMENT, "vertex_colored_material_fragment_shader");
      }
    else if (engine->get_renderer_type() == renderer_type::OPENGL)
      {
      vs_handle = engine->add_shader(get_vertex_colored_material_vertex_shader().c_str(), SHADER_VERTEX, nullptr);
      fs_handle = engine->add_shader(get_vertex_colored_material_fragment_shader().c_str(), SHADER_FRAGMENT, nullptr);
      }   
    shader_program_handle = engine->add_program(vs_handle, fs_handle);
    vp_handle = engine->add_uniform("ViewProject", uniform_type::mat4, 1);
    cam_handle = engine->add_uniform("Camera", uniform_type::mat4, 1);
    light_dir_handle = engine->add_uniform("LightDir", uniform_type::vec3, 1);
    ambient_handle = engine->add_uniform("Ambient", uniform_type::real, 1);
    }

  void vertex_colored_material::bind(render_engine* engine)
    {
    engine->bind_program(shader_program_handle);
    const auto& ld = engine->get_light_dir();

    engine->set_uniform(vp_handle, (void*)&engine->get_view_project());
    engine->set_uniform(cam_handle, (void*)&engine->get_camera_space());
    engine->set_uniform(light_dir_handle, (void*)&ld);    
    engine->set_uniform(ambient_handle, (void*)&ambient);

    engine->bind_uniform(shader_program_handle, vp_handle);
    engine->bind_uniform(shader_program_handle, cam_handle);
    engine->bind_uniform(shader_program_handle, light_dir_handle);
    engine->bind_uniform(shader_program_handle, ambient_handle);
    }


  ///////////////////////////////////////////////////////////////////////

  static std::string get_simple_material_vertex_shader()
    {
    return std::string(R"(#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
uniform mat4 ViewProject; // columns
uniform mat4 Camera; // columns

out vec3 Normal;
out vec2 TexCoord;

void main() 
  {
  gl_Position = ViewProject*vec4(vPosition.xyz,1);
  Normal = (Camera*vec4(vNormal,0)).xyz;
  TexCoord = vTexCoord;
  }
)");
    }

  static std::string get_simple_material_fragment_shader()
    {
    return std::string(R"(#version 330 core
out vec4 FragColor;
  
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D Tex0;
uniform vec3 LightDir;
uniform vec4 Color;
uniform int TextureSample;
uniform float Ambient;

void main()
  {
  float l = clamp(dot(Normal,LightDir), 0, 1.0 - Ambient) + Ambient;
  vec4 clr = (texture(Tex0, TexCoord)*TextureSample + Color*(1-TextureSample))*l;
  FragColor = clr;
  }
)");
    }

  simple_material::simple_material()
    {
    vs_handle = -1;
    fs_handle = -1;
    shader_program_handle = -1;
    tex_handle = -1;
    color = 0xff0000ff;
    ambient = 0.2f;
    vp_handle = -1;
    cam_handle = -1;
    light_dir_handle = -1;
    tex_sample_handle = -1;
    ambient_handle = -1;
    color_handle = -1;
    tex0_handle = -1;
    dummy_tex_handle = -1;
    }

  simple_material::~simple_material()
    {
    }

  void simple_material::set_texture(int32_t handle, int32_t flags)
    {
    if (handle >= 0 && handle < MAX_TEXTURE)
      tex_handle = handle;
    else
      tex_handle = -1;
    texture_flags = flags;
    }

  void simple_material::set_color(uint32_t clr)
    {
    color = clr;
    }

  void simple_material::set_ambient(float a)
    {
    ambient = a;
    }
    
  void simple_material::destroy(render_engine* engine)
    {
    engine->remove_program(shader_program_handle);
    engine->remove_shader(vs_handle);
    engine->remove_shader(fs_handle);    
    engine->remove_texture(tex_handle);
    engine->remove_texture(dummy_tex_handle);
    engine->remove_uniform(vp_handle);
    engine->remove_uniform(cam_handle);
    engine->remove_uniform(light_dir_handle);
    engine->remove_uniform(tex_sample_handle);
    engine->remove_uniform(ambient_handle);
    engine->remove_uniform(color_handle);
    engine->remove_uniform(tex0_handle);
    }

  void simple_material::compile(render_engine* engine)
    {
    if (engine->get_renderer_type() == renderer_type::METAL)
      {
      vs_handle = engine->add_shader(nullptr, SHADER_VERTEX, "simple_material_vertex_shader");
      fs_handle = engine->add_shader(nullptr, SHADER_FRAGMENT, "simple_material_fragment_shader");
      }
    else if (engine->get_renderer_type() == renderer_type::OPENGL)
      {
      vs_handle = engine->add_shader(get_simple_material_vertex_shader().c_str(), SHADER_VERTEX, nullptr);
      fs_handle = engine->add_shader(get_simple_material_fragment_shader().c_str(), SHADER_FRAGMENT, nullptr);
      }
    dummy_tex_handle = engine->add_texture(1, 1, texture_format_rgba8, (const uint16_t*)nullptr);
    shader_program_handle = engine->add_program(vs_handle, fs_handle);
    vp_handle = engine->add_uniform("ViewProject", uniform_type::mat4, 1);
    cam_handle = engine->add_uniform("Camera", uniform_type::mat4, 1);
    light_dir_handle = engine->add_uniform("LightDir", uniform_type::vec3, 1);
    tex_sample_handle = engine->add_uniform("TextureSample", uniform_type::integer, 1);
    ambient_handle = engine->add_uniform("Ambient", uniform_type::real, 1);
    color_handle = engine->add_uniform("Color", uniform_type::vec4, 1);
    tex0_handle = engine->add_uniform("Tex0", uniform_type::sampler, 1);
    }

  void simple_material::bind(render_engine* engine)
    {
    engine->bind_program(shader_program_handle);
    const auto& ld = engine->get_light_dir();
    
    engine->set_uniform(vp_handle, (void*)&engine->get_view_project());
    engine->set_uniform(cam_handle, (void*)&engine->get_camera_space());
    engine->set_uniform(light_dir_handle, (void*)&ld);
    int32_t tex_sample = tex_handle >= 0 ? 1 : 0;
    engine->set_uniform(tex_sample_handle, (void*)&tex_sample);
    engine->set_uniform(ambient_handle, (void*)&ambient);
    float col[4] = {(color & 255) / 255.f, ((color >> 8) & 255) / 255.f, ((color >> 16) & 255) / 255.f, ((color >> 24) & 255) / 255.f};
    engine->set_uniform(color_handle, (void*)col);
    int32_t tex_0 = 0;
    engine->set_uniform(tex0_handle, (void*)&tex_0);
      
    engine->bind_uniform(shader_program_handle, vp_handle);
    engine->bind_uniform(shader_program_handle, cam_handle);
    engine->bind_uniform(shader_program_handle, color_handle);
    engine->bind_uniform(shader_program_handle, light_dir_handle);
    engine->bind_uniform(shader_program_handle, tex_sample_handle);
    engine->bind_uniform(shader_program_handle, ambient_handle);
    engine->bind_uniform(shader_program_handle, tex0_handle);
    if (tex_handle >= 0)
      {
      const texture* tex = engine->get_texture(tex_handle);
      if (tex->format == texture_format_bgra8 || tex->format == texture_format_rgba16 || tex->format == texture_format_rgba8 || tex->format == texture_format_rgba32f)
        engine->bind_texture_to_channel(tex_handle, 0, texture_flags);
      else
        engine->bind_texture_to_channel(dummy_tex_handle, 0, texture_flags);
      }
    else
      {
      engine->bind_texture_to_channel(dummy_tex_handle, 0, texture_flags);
      }
    }

  ///////////////////////////////////////////////////////////

  static std::string get_shadertoy_material_vertex_shader()
    {
    return std::string(R"(#version 330 core
layout (location = 0) in vec3 vPosition;
uniform mat4 ViewProject; // columns

void main() 
  {   
  gl_Position = ViewProject*vec4(vPosition.xyz,1); 
  }
)");
    }

  static std::string get_shadertoy_material_fragment_shader_header()
    {
    return std::string(R"(#version 330 core
uniform vec3 iResolution;
uniform float iTime;
uniform int iFrame;
uniform float iTimeDelta;

out vec4 FragColor;
)");
    }

  static std::string get_shadertoy_material_fragment_shader_footer()
    {
    return std::string(R"(
void main() 
  {
  mainImage(FragColor, gl_FragCoord.xy);
  }
)");
    }

  shadertoy_material::shadertoy_material()
    {
    vs_handle = -1;
    fs_handle = -1;
    shader_program_handle = -1;
    vp_handle = -1;
    res_handle = -1;
    time_handle = -1;
    time_delta_handle = -1;
    frame_handle = -1;
    _props.time = 0;
    _props.time_delta = 0;
    _props.frame = 0;
    _script = std::string(R"(void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    // Time varying pixel color
    vec3 col = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));

    // Output to screen
    fragColor = vec4(col,1.0);
})");
    }

  shadertoy_material::~shadertoy_material()
    {
    }

  void shadertoy_material::set_script(const std::string& script)
    {
    _script = script;
    }

  void shadertoy_material::set_shadertoy_properties(const properties& props)
    {
    _props = props;
    }
    
  void shadertoy_material::destroy(render_engine* engine)
    {
    engine->remove_shader(vs_handle);
    engine->remove_shader(fs_handle);
    engine->remove_program(shader_program_handle);
    engine->remove_uniform(vp_handle);
    engine->remove_uniform(res_handle);
    engine->remove_uniform(time_handle);
    engine->remove_uniform(time_delta_handle);
    engine->remove_uniform(frame_handle);
    }

  void shadertoy_material::compile(render_engine* engine)
    {
    std::string fragment_shader;
    fragment_shader.append(get_shadertoy_material_fragment_shader_header());
    fragment_shader.append(_script);
    fragment_shader.append(get_shadertoy_material_fragment_shader_footer());
    if (engine->get_renderer_type() == renderer_type::METAL)
      {
      std::string header = std::string(R"(
#include <metal_stdlib>
using namespace metal;

struct VertexOut {
  float4 position [[position]];
  float3 normal;
  float2 texcoord;
};

struct ShadertoyMaterialUniforms {
  float4x4 view_projection_matrix;
  float3 iResolution;
  float iTime;
  float iTimeDelta;
  int iFrame;
};)");

std::string footer = std::string(R"(
fragment float4 shadertoy_material_fragment_shader(const VertexOut vertexIn [[stage_in]], constant ShadertoyMaterialUniforms& input [[buffer(10)]]) {
  float4 fragColor;
  mainImage(fragColor, vertexIn.position.xy, input.iTime, input.iResolution);
  return float4(fragColor[0], fragColor[1], fragColor[2], 1);
})");
      std::string total_fragment_shader = header.append(_script).append(footer);
      vs_handle = engine->add_shader(nullptr, SHADER_VERTEX, "shadertoy_material_vertex_shader");
      fs_handle = engine->add_shader(total_fragment_shader.c_str(), SHADER_FRAGMENT, "shadertoy_material_fragment_shader");
      }
    else if (engine->get_renderer_type() == renderer_type::OPENGL)
      {
      vs_handle = engine->add_shader(get_shadertoy_material_vertex_shader().c_str(), SHADER_VERTEX, nullptr);
      fs_handle = engine->add_shader(fragment_shader.c_str(), SHADER_FRAGMENT, nullptr);
      }
    shader_program_handle = engine->add_program(vs_handle, fs_handle);
    vp_handle = engine->add_uniform("ViewProject", uniform_type::mat4, 1);
    res_handle = engine->add_uniform("iResolution", uniform_type::vec3, 1);
    time_handle = engine->add_uniform("iTime", uniform_type::real, 1);
    time_delta_handle = engine->add_uniform("iTimeDelta", uniform_type::real, 1);
    frame_handle = engine->add_uniform("iFrame", uniform_type::integer, 1);
    }

  void shadertoy_material::bind(render_engine* engine)
    {
    engine->bind_program(shader_program_handle);
    engine->set_uniform(vp_handle, (void*)(&engine->get_view_project()));
    const auto& mv = engine->get_model_view_properties();
    float res[3] = { (float)mv.viewport_width, (float)mv.viewport_height, 1.f };
    engine->set_uniform(res_handle, (void*)res);
    engine->set_uniform(time_handle, &_props.time);
    engine->set_uniform(time_delta_handle, &_props.time_delta);
    engine->set_uniform(frame_handle, &_props.frame);

    engine->bind_uniform(shader_program_handle, vp_handle);
    engine->bind_uniform(shader_program_handle, res_handle);
    engine->bind_uniform(shader_program_handle, time_handle);
    engine->bind_uniform(shader_program_handle, time_delta_handle);
    engine->bind_uniform(shader_program_handle, frame_handle);
    }
  } // namespace RenderDoos
