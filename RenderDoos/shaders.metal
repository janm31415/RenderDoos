#include <metal_stdlib>
using namespace metal;

struct VertexCompactIn {
  packed_float3 position;
  int color;
};

struct VertexCompactOut {
  float4 position [[position]];
  float4 color;
};

struct CompactMaterialUniforms {
  float4x4 view_projection_matrix;
};

vertex VertexCompactOut compact_material_vertex_shader(const device VertexCompactIn *vertices [[buffer(0)]], uint vertexId [[vertex_id]], constant CompactMaterialUniforms& input [[buffer(10)]]) {
  float4 pos(vertices[vertexId].position, 1);
  VertexCompactOut out;
  out.position = input.view_projection_matrix * pos;
  int color = vertices[vertexId].color;
  out.color = float4(float(color&uint(255))/255.f, float((color>>8)&uint(255))/255.f, float((color>>16)&uint(255))/255.f, float((color>>24)&uint(255))/255.f);
  return out;
}


fragment float4 compact_material_fragment_shader(const VertexCompactOut vertexIn [[stage_in]]) {
   return vertexIn.color;
}

struct VertexColoredIn {
  packed_float3 position;
  packed_float3 normal;
  int color;
};

struct VertexColoredMaterialUniforms {
  float4x4 view_projection_matrix;
  float4x4 camera_matrix;
  float3 light;
  float ambient;
};

struct VertexColoredOut {
  float4 position [[position]];
  float3 normal;
  float4 color;
};

vertex VertexColoredOut vertex_colored_material_vertex_shader(const device VertexColoredIn *vertices [[buffer(0)]], uint vertexId [[vertex_id]], constant VertexColoredMaterialUniforms& input [[buffer(10)]]) {
  float4 pos(vertices[vertexId].position, 1);
  VertexColoredOut out;
  out.position = input.view_projection_matrix * pos;
  out.normal = (input.camera_matrix * float4(vertices[vertexId].normal, 0)).xyz;
  int color = vertices[vertexId].color;
  out.color = float4(float(color&uint(255))/255.f, float((color>>8)&uint(255))/255.f, float((color>>16)&uint(255))/255.f, float((color>>24)&uint(255))/255.f);
  return out;
}


fragment float4 vertex_colored_material_fragment_shader(const VertexColoredOut vertexIn [[stage_in]], constant VertexColoredMaterialUniforms& input [[buffer(10)]]) {
  float l = clamp(dot(vertexIn.normal,input.light), 0.0, 1.0 - input.ambient) + input.ambient;
  return vertexIn.color*l;
}

struct VertexIn {
  packed_float3 position;
  packed_float3 normal;
  packed_float2 textureCoordinates;
};

struct SimpleMaterialUniforms {
  float4x4 view_projection_matrix;
  float4x4 camera_matrix;
  float4 color;
  float3 light;
  int texture_sample;
  float ambient;
  int tex0;
};

struct VertexOut {
  float4 position [[position]];
  float3 normal;
  float2 texcoord;
};

vertex VertexOut simple_material_vertex_shader(const device VertexIn *vertices [[buffer(0)]], uint vertexId [[vertex_id]], constant SimpleMaterialUniforms& input [[buffer(10)]]) {
  float4 pos(vertices[vertexId].position, 1);
  VertexOut out;
  out.position = input.view_projection_matrix * pos;
  out.normal = (input.camera_matrix * float4(vertices[vertexId].normal, 0)).xyz;
  out.texcoord = vertices[vertexId].textureCoordinates;
  return out;
}


fragment float4 simple_material_fragment_shader(const VertexOut vertexIn [[stage_in]], texture2d<float> texture [[texture(0)]], sampler sampler2d [[sampler(0)]], constant SimpleMaterialUniforms& input [[buffer(10)]]) {
  float l = clamp(dot(vertexIn.normal,input.light), 0.0, 1.0 - input.ambient) + input.ambient;
  return (texture.sample(sampler2d, vertexIn.texcoord)*input.texture_sample + input.color*(1-input.texture_sample))*l;
}

struct ShadertoyMaterialUniforms {
  float4x4 view_projection_matrix;
  float3 iResolution;
  float iTime;
  float iTimeDelta;
  int iFrame;
};

vertex VertexOut shadertoy_material_vertex_shader(const device VertexIn *vertices [[buffer(0)]], uint vertexId [[vertex_id]], constant ShadertoyMaterialUniforms& input [[buffer(10)]]) {
  float4 pos(vertices[vertexId].position, 1);
  VertexOut out;
  out.position = input.view_projection_matrix * pos;
  return out;
}
/*
void mainImage(thread float4& fragColor, float2 fragCoord, float iTime, float3 iResolution)
{
  float2 uv = fragCoord / iResolution.xy;
  float3 col = 0.5 + 0.5*cos(iTime + uv.xyx + float3(0, 2, 4));
  
  fragColor = float4(col[0], col[1], col[2], 1);
}

fragment float4 shadertoy_material_fragment_shader(const VertexOut vertexIn [[stage_in]], constant ShadertoyMaterialUniforms& input [[buffer(1)]]) {
  float4 fragColor;
  mainImage(fragColor, vertexIn.position.xy, input.iTime, input.iResolution);
  return float4(fragColor[0], fragColor[1], fragColor[2], 1);
}
*/
