#pragma once

#ifdef RENDERDOOS_ARM
#include "sse2neon.h"
#else
#include <immintrin.h>
#include <emmintrin.h>
#endif

#include <stdint.h>

namespace RenderDoos
  {

#ifdef _WIN32
  __declspec(align(16))
#endif
    struct float4
    {
    union
      {
      __m128 m128;
      float f[4];
      uint32_t u[4];
      int32_t i[4];
      };

    template <typename T>
    float& operator [] (T i)
      {
      return f[i];
      }

    template <typename T>
    float operator [] (T i) const
      {
      return f[i];
      }

    float4();
    float4(const __m128 in);
    float4(float f);
    float4(float _x, float _y, float _z);
    float4(float _x, float _y, float _z, float _w);

    typedef float value_type;
    }
#ifndef _WIN32 // linux alignment in gcc
  __attribute__((aligned(16)))
#endif  
    ;
  float4 operator + (const float4& a);
  float4 operator - (const float4& a);
  float4 operator + (const float4& left, const float4& right);
  float4 operator - (const float4& left, const float4& right);
  float4 operator * (const float4& left, const float4& right);
  float4 operator * (const float4& left, float right);
  float4 operator * (float left, const float4& right);
  float4 operator / (const float4& left, const float4& right);
  float4 operator / (const float4& left, float right);
  float4 operator / (float left, const float4& right);
  float4 min(const float4& left, const float4& right);
  float4 max(const float4& left, const float4& right);
  float min_horizontal(const float4& x);
  float max_horizontal(const float4& x);
  float4 cross(const float4& left, const float4& right);
  float dot(const float4& left, const float4& right);
  float dot4(const float4& left, const float4& right);
  float4 abs(const float4& a);
  float4 sqrt(const float4& a);
  float4 rsqrt(const float4& a);
  float4 reciprocal(const float4& a);  
  float4 unpacklo(const float4& left, const float4& right);
  float4 unpackhi(const float4& left, const float4& right);
  void transpose(float4& r0, float4& r1, float4& r2, float4& r3, const float4& c0, const float4& c1, const float4& c2, const float4& c3);
  float4 normalize(const float4& v);

#ifdef _WIN32
  _declspec(align(16))
#endif
    struct float4x4
    {
    union
      {
      float4 col[4];
      float f[16];
      };

    template <typename T>
    float& operator [] (T i)
      {
      return f[i];
      }

    template <typename T>
    float operator [] (T i) const
      {
      return f[i];
      }

    float4x4();
    float4x4(const float4& col0, const float4& col1, const float4& col2, const float4& col3);
    float4x4(float* m);
    }
#ifndef _WIN32 // linux alignment in gcc
  __attribute__((aligned(16)))
#endif  
    ;
  float4x4 get_identity();
  float4x4 make_translation(float x, float y, float z);
  float4x4 transpose(const float4x4& m);
  float4x4 invert_orthonormal(const float4x4& m);
  // for column major matrix
  // we use __m128 to represent 2x2 matrix as A = | A0  A1 |
  //                                              | A2  A3 |
  // 2x2 column major matrix multiply A*B
  __m128 mat2mul(__m128 vec1, __m128 vec2);
  // 2x2 column major matrix adjugate multiply (A#)*B
  __m128 mat2adjmul(__m128 vec1, __m128 vec2);
  // 2x2 column major matrix multiply adjugate A*(B#)
  __m128 mat2muladj(__m128 vec1, __m128 vec2);
  float4x4 invert(const float4x4& m);
  float4 matrix_vector_multiply(const float4x4& m, const float4& v);
  float4x4 matrix_matrix_multiply(const float4x4& left, const float4x4& right);
  float4x4 operator + (const float4x4& left, const float4x4& right);
  float4x4 operator - (const float4x4& left, const float4x4& right);
  float4x4 operator / (const float4x4& left, float value);
  float4x4 operator * (const float4x4& left, float value);
  float4x4 operator * (float value, const float4x4& right);

  float4x4 make_identity();
  float4 transform(const float4x4& matrix, const float4& pt);
  float4 transform_vector(const float4x4& matrix, const float4& vec);
  float4 transform(const float4x4& matrix, const float4& pt, bool is_vector);
  float4 transform(const float4x4& matrix, const float4& pt);
  float4x4 make_transformation(const float4& i_origin, const float4& i_x_axis, const float4& i_y_axis, const float4& i_z_axis);
  float4x4 make_rotation(const float4& i_position, const float4& i_direction, float i_angle_radians);
  float4x4 make_scale3d(float scale_x, float scale_y, float scale_z);
  float4x4 make_translation(const float4& i_translation);
  float4 get_translation(const float4x4& matrix);
  void set_x_axis(float4x4& matrix, const float4& x);
  void set_y_axis(float4x4& matrix, const float4& y);
  void set_z_axis(float4x4& matrix, const float4& z);
  void set_translation(float4x4& matrix, const float4& t);
  float4 get_x_axis(const float4x4& matrix);
  float4 get_y_axis(const float4x4& matrix);
  float4 get_z_axis(const float4x4& matrix);
  float determinant(const float4x4& m);
  void solve_roll_pitch_yaw_transformation(float& rx, float& ry, float& rz, float& tx, float& ty, float& tz, const float4x4& m);
  float4x4 compute_from_roll_pitch_yaw_transformation(float rx, float ry, float rz, float tx, float ty, float tz);
  float4 roll_pitch_yaw_to_quaternion(float rx, float ry, float rz);
  float4 rotation_to_quaternion(const float4x4& rotation);
  float4x4 quaternion_to_rotation(const float4& quaternion);
  float4 quaternion_conjugate(const float4& quaternion);
  float4 quaternion_inverse(const float4& quaternion);
  float4 quaternion_normalize(const float4& quaternion);
  float4 quaternion_multiply(const float4& q1, const float4& q2);
  float4 quaternion_axis(const float4& q);
  float quaternion_angle(const float4& q);
  float4x4 look_at(const float4& eye, const float4& center, const float4& up);
  void get_eye_center_up(float4& eye, float4& center, float4& up, const float4x4& transform);
  float4x4 frustum(float left, float right, float bottom, float top, float near_plane, float far_plane);
  float4x4 orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane);
  }
