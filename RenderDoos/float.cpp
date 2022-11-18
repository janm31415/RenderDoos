#include "float.h"

#include <cmath>

#ifndef RENDERDOOS_SIMD
#ifdef _WIN32
#include <utility> // for std::min and std::max.
#else
#include <vector> // for std::min and std::max.
#endif
#endif

namespace RenderDoos
  {
#ifdef RENDERDOOS_SIMD
  static const __m128 three_128 = _mm_set1_ps(3.0f);
  static const __m128 half_128 = _mm_set1_ps(0.5f);
#endif

  float4::float4() {}
#ifdef RENDERDOOS_SIMD
  float4::float4(const __m128 in) : m128(in) {}
  float4::float4(float f) : m128(_mm_set1_ps(f)) {}
  float4::float4(float _x, float _y, float _z) : m128(_mm_set_ps(1.f, _z, _y, _x)) {}
  float4::float4(float _x, float _y, float _z, float _w) : m128(_mm_set_ps(_w, _z, _y, _x)) {}
#else
  float4::float4(float fl)
    {
    f[0] = f[1] = f[2] = f[3] = fl;
    }
  float4::float4(float _x, float _y, float _z)
    {
    f[0] = _x;
    f[1] = _y;
    f[2] = _z;
    f[3] = 1.f;
    }
  float4::float4(float _x, float _y, float _z, float _w)
    {
    f[0] = _x;
    f[1] = _y;
    f[2] = _z;
    f[3] = _w;
    }
#endif

  float4x4::float4x4() {}
  float4x4::float4x4(const float4& col0, const float4& col1, const float4& col2, const float4& col3) : col{ col0, col1, col2, col3 } {}
  float4x4::float4x4(float* m)
    {
    for (int i = 0; i < 16; ++i)
      f[i] = m[i];
    }
#ifdef RENDERDOOS_SIMD
  float4 operator + (const float4& a)
    {
    return a;
    }

  float4 operator - (const float4& a)
    {
    return _mm_xor_ps(a.m128, _mm_castsi128_ps(_mm_set1_epi32(0x80000000)));
    }

  float4 operator + (const float4& left, const float4& right)
    {
    return _mm_add_ps(left.m128, right.m128);
    }

  float4 operator - (const float4& left, const float4& right)
    {
    return _mm_sub_ps(left.m128, right.m128);
    }

  float4 operator * (const float4& left, const float4& right)
    {
    return _mm_mul_ps(left.m128, right.m128);
    }

  float4 operator * (const float4& left, float right)
    {
    return left * float4(right);
    }

  float4 operator * (float left, const float4& right)
    {
    return float4(left) * right;
    }

  float4 operator / (const float4& left, const float4& right)
    {
    return _mm_div_ps(left.m128, right.m128);
    }

  float4 operator / (const float4& left, float right)
    {
    return left / float4(right);
    }

  float4 operator / (float left, const float4& right)
    {
    return float4(left) / right;
    }

  float4 min(const float4& left, const float4& right)
    {
    return _mm_min_ps(left.m128, right.m128);
    }

  float4 max(const float4& left, const float4& right)
    {
    return _mm_max_ps(left.m128, right.m128);
    }

  float min_horizontal(const float4& x)
    {
    __m128 max1 = _mm_shuffle_ps(x.m128, x.m128, _MM_SHUFFLE(0, 0, 3, 2));
    __m128 max2 = _mm_min_ps(x.m128, max1);
    __m128 max3 = _mm_shuffle_ps(max2, max2, _MM_SHUFFLE(0, 0, 0, 1));
    __m128 max4 = _mm_min_ps(max2, max3);
    float result = _mm_cvtss_f32(max4);
    return result;
    }

  float max_horizontal(const float4& x)
    {
    __m128 max1 = _mm_shuffle_ps(x.m128, x.m128, _MM_SHUFFLE(0, 0, 3, 2));
    __m128 max2 = _mm_max_ps(x.m128, max1);
    __m128 max3 = _mm_shuffle_ps(max2, max2, _MM_SHUFFLE(0, 0, 0, 1));
    __m128 max4 = _mm_max_ps(max2, max3);
    float result = _mm_cvtss_f32(max4);
    return result;
    }

  float4 cross(const float4& left, const float4& right)
    {
    float4 rs(_mm_shuffle_ps(right.m128, right.m128, _MM_SHUFFLE(3, 0, 2, 1)));
    float4 ls(_mm_shuffle_ps(left.m128, left.m128, _MM_SHUFFLE(3, 0, 2, 1)));
    float4 res = left * rs - ls * right;
    return float4(_mm_shuffle_ps(res.m128, res.m128, _MM_SHUFFLE(3, 0, 2, 1)));
    }

  float dot(const float4& left, const float4& right)
    {
    return _mm_cvtss_f32(_mm_dp_ps(left.m128, right.m128, 0x7F));
    }

  float dot4(const float4& left, const float4& right)
    {
    return _mm_cvtss_f32(_mm_dp_ps(left.m128, right.m128, 255));
    }

  float4 abs(const float4& a)
    {
    const __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
    return _mm_and_ps(a.m128, mask);
    }

  float4 sqrt(const float4& a)
    {
    return _mm_sqrt_ps(a.m128);
    }

  float4 rsqrt(const float4& a)
    {
    __m128 mask = _mm_cmpeq_ps(_mm_set1_ps(0.f), a.m128);
    __m128 res = _mm_rsqrt_ps(a.m128);
    __m128 muls = _mm_mul_ps(_mm_mul_ps(a.m128, res), res);
    auto res_newton_raphson = _mm_mul_ps(_mm_mul_ps(half_128, res), _mm_sub_ps(three_128, muls));
    return _mm_or_ps(_mm_and_ps(mask, res), _mm_andnot_ps(mask, res_newton_raphson));
    }

  float4 reciprocal(const float4& a)
    {
    __m128 mask = _mm_cmpeq_ps(_mm_set1_ps(0.f), a.m128);
    auto res = _mm_rcp_ps(a.m128);
    __m128 muls = _mm_mul_ps(a.m128, _mm_mul_ps(res, res));
    auto res_newton_raphson = _mm_sub_ps(_mm_add_ps(res, res), muls);
    return _mm_or_ps(_mm_and_ps(mask, res), _mm_andnot_ps(mask, res_newton_raphson));
    }

  float4 unpacklo(const float4& left, const float4& right)
    {
    return _mm_unpacklo_ps(left.m128, right.m128);
    }

  float4 unpackhi(const float4& left, const float4& right)
    {
    return _mm_unpackhi_ps(left.m128, right.m128);
    }

  void transpose(float4& r0, float4& r1, float4& r2, float4& r3, const float4& c0, const float4& c1, const float4& c2, const float4& c3)
    {
    float4 l02(unpacklo(c0.m128, c2.m128));
    float4 h02(unpackhi(c0.m128, c2.m128));
    float4 l13(unpacklo(c1.m128, c3.m128));
    float4 h13(unpackhi(c1.m128, c3.m128));
    r0 = unpacklo(l02, l13);
    r1 = unpackhi(l02, l13);
    r2 = unpacklo(h02, h13);
    r3 = unpackhi(h02, h13);
    }

  float4x4 get_identity()
    {
    float4x4 m(_mm_set_ps(0.f, 0.f, 0.f, 1.f), _mm_set_ps(0.f, 0.f, 1.f, 0.f), _mm_set_ps(0.f, 1.f, 0.f, 0.f), _mm_set_ps(1.f, 0.f, 0.f, 0.f));
    return m;
    }

  float4x4 make_translation(float x, float y, float z)
    {
    float4x4 m(_mm_set_ps(0.f, 0.f, 0.f, 1.f), _mm_set_ps(0.f, 0.f, 1.f, 0.f), _mm_set_ps(0.f, 1.f, 0.f, 0.f), _mm_set_ps(1.f, z, y, x));
    return m;
    }



  // for column major matrix
  // we use __m128 to represent 2x2 matrix as A = | A0  A1 |
  //                                              | A2  A3 |
  // 2x2 column major matrix multiply A*B
  __m128 mat2mul(__m128 vec1, __m128 vec2)
    {
    const auto vec3 = _mm_mul_ps(vec1, _mm_shuffle_ps(vec2, vec2, _MM_SHUFFLE(3, 3, 0, 0)));
    const auto vec4 = _mm_mul_ps(_mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(1, 0, 3, 2)), _mm_shuffle_ps(vec2, vec2, _MM_SHUFFLE(2, 2, 1, 1)));
    return _mm_add_ps(vec3, vec4);
    }

  // 2x2 column major matrix adjugate multiply (A#)*B
  __m128 mat2adjmul(__m128 vec1, __m128 vec2)
    {
    const auto vec3 = _mm_mul_ps(_mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(0, 3, 0, 3)), vec2);
    const auto vec4 = _mm_mul_ps(_mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(1, 2, 1, 2)), _mm_shuffle_ps(vec2, vec2, _MM_SHUFFLE(2, 3, 0, 1)));
    return _mm_sub_ps(vec3, vec4);
    }

  // 2x2 column major matrix multiply adjugate A*(B#)
  __m128 mat2muladj(__m128 vec1, __m128 vec2)
    {
    const auto vec3 = _mm_mul_ps(vec1, _mm_shuffle_ps(vec2, vec2, _MM_SHUFFLE(0, 0, 3, 3)));
    const auto vec4 = _mm_mul_ps(_mm_shuffle_ps(vec1, vec1, _MM_SHUFFLE(1, 0, 3, 2)), _mm_shuffle_ps(vec2, vec2, _MM_SHUFFLE(2, 2, 1, 1)));
    return _mm_sub_ps(vec3, vec4);
    }

  float4x4 invert(const float4x4& m)
    {
    float4x4 out;
    // sub matrices
    __m128 A = _mm_shuffle_ps(m.col[0].m128, m.col[1].m128, _MM_SHUFFLE(1, 0, 1, 0));
    __m128 C = _mm_shuffle_ps(m.col[0].m128, m.col[1].m128, _MM_SHUFFLE(3, 2, 3, 2));
    __m128 B = _mm_shuffle_ps(m.col[2].m128, m.col[3].m128, _MM_SHUFFLE(1, 0, 1, 0));
    __m128 D = _mm_shuffle_ps(m.col[2].m128, m.col[3].m128, _MM_SHUFFLE(3, 2, 3, 2));

    // determinant as (|A| |B| |C| |D|)
    __m128 detSub = _mm_sub_ps(_mm_mul_ps(
      _mm_shuffle_ps(m.col[0].m128, m.col[2].m128, _MM_SHUFFLE(2, 0, 2, 0)),
      _mm_shuffle_ps(m.col[1].m128, m.col[3].m128, _MM_SHUFFLE(3, 1, 3, 1))),
      _mm_mul_ps(_mm_shuffle_ps(m.col[0].m128, m.col[2].m128, _MM_SHUFFLE(3, 1, 3, 1)),
        _mm_shuffle_ps(m.col[1].m128, m.col[3].m128, _MM_SHUFFLE(2, 0, 2, 0))));
    __m128 detA = _mm_shuffle_ps(detSub, detSub, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 detC = _mm_shuffle_ps(detSub, detSub, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 detB = _mm_shuffle_ps(detSub, detSub, _MM_SHUFFLE(2, 2, 2, 2));
    __m128 detD = _mm_shuffle_ps(detSub, detSub, _MM_SHUFFLE(3, 3, 3, 3));

    // let iM = 1/|M| * | X  Y |
    //                  | Z  W |

    // D#C
    __m128 D_C = mat2adjmul(D, C);
    // A#B
    __m128 A_B = mat2adjmul(A, B);
    // X# = |D|A - B(D#C)
    __m128 X_ = _mm_sub_ps(_mm_mul_ps(detD, A), mat2mul(B, D_C));
    // W# = |A|D - C(A#B)
    __m128 W_ = _mm_sub_ps(_mm_mul_ps(detA, D), mat2mul(C, A_B));

    // |M| = |A|*|D| + ... (continue later)
    __m128 detM = _mm_mul_ps(detA, detD);

    // Y# = |B|C - D(A#B)#
    __m128 Y_ = _mm_sub_ps(_mm_mul_ps(detB, C), mat2muladj(D, A_B));
    // Z# = |C|B - A(D#C)#
    __m128 Z_ = _mm_sub_ps(_mm_mul_ps(detC, B), mat2muladj(A, D_C));

    // |M| = |A|*|D| + |B|*|C| ... (continue later)
    detM = _mm_add_ps(detM, _mm_mul_ps(detB, detC));

    // tr((A#B)(D#C))
    __m128 tr = _mm_mul_ps(A_B, _mm_shuffle_ps(D_C, D_C, _MM_SHUFFLE(3, 1, 2, 0)));
    tr = _mm_hadd_ps(tr, tr);
    tr = _mm_hadd_ps(tr, tr);
    // |M| = |A|*|D| + |B|*|C| - tr((A#B)(D#C)
    detM = _mm_sub_ps(detM, tr);

    const __m128 adjSignMask = _mm_setr_ps(1.f, -1.f, -1.f, 1.f);
    // (1/|M|, -1/|M|, -1/|M|, 1/|M|)
    __m128 rDetM = _mm_div_ps(adjSignMask, detM);

    X_ = _mm_mul_ps(X_, rDetM);
    Y_ = _mm_mul_ps(Y_, rDetM);
    Z_ = _mm_mul_ps(Z_, rDetM);
    W_ = _mm_mul_ps(W_, rDetM);

    // apply adjugate and store, here we combine adjugate shuffle and store shuffle
    out.col[0] = float4(_mm_shuffle_ps(X_, Z_, _MM_SHUFFLE(1, 3, 1, 3)));
    out.col[1] = float4(_mm_shuffle_ps(X_, Z_, _MM_SHUFFLE(0, 2, 0, 2)));
    out.col[2] = float4(_mm_shuffle_ps(Y_, W_, _MM_SHUFFLE(1, 3, 1, 3)));
    out.col[3] = float4(_mm_shuffle_ps(Y_, W_, _MM_SHUFFLE(0, 2, 0, 2)));

    return out;
    }

  float4x4 matrix_matrix_multiply(const float4x4& left, const float4x4& right)
    {
    float4x4 out;
    float4 r[4];
    transpose(r[0], r[1], r[2], r[3], left.col[0], left.col[1], left.col[2], left.col[3]);
    out[0] = _mm_cvtss_f32(_mm_dp_ps(r[0].m128, right.col[0].m128, 255));
    out[1] = _mm_cvtss_f32(_mm_dp_ps(r[1].m128, right.col[0].m128, 255));
    out[2] = _mm_cvtss_f32(_mm_dp_ps(r[2].m128, right.col[0].m128, 255));
    out[3] = _mm_cvtss_f32(_mm_dp_ps(r[3].m128, right.col[0].m128, 255));
    out[4] = _mm_cvtss_f32(_mm_dp_ps(r[0].m128, right.col[1].m128, 255));
    out[5] = _mm_cvtss_f32(_mm_dp_ps(r[1].m128, right.col[1].m128, 255));
    out[6] = _mm_cvtss_f32(_mm_dp_ps(r[2].m128, right.col[1].m128, 255));
    out[7] = _mm_cvtss_f32(_mm_dp_ps(r[3].m128, right.col[1].m128, 255));
    out[8] = _mm_cvtss_f32(_mm_dp_ps(r[0].m128, right.col[2].m128, 255));
    out[9] = _mm_cvtss_f32(_mm_dp_ps(r[1].m128, right.col[2].m128, 255));
    out[10] = _mm_cvtss_f32(_mm_dp_ps(r[2].m128, right.col[2].m128, 255));
    out[11] = _mm_cvtss_f32(_mm_dp_ps(r[3].m128, right.col[2].m128, 255));
    out[12] = _mm_cvtss_f32(_mm_dp_ps(r[0].m128, right.col[3].m128, 255));
    out[13] = _mm_cvtss_f32(_mm_dp_ps(r[1].m128, right.col[3].m128, 255));
    out[14] = _mm_cvtss_f32(_mm_dp_ps(r[2].m128, right.col[3].m128, 255));
    out[15] = _mm_cvtss_f32(_mm_dp_ps(r[3].m128, right.col[3].m128, 255));
    return out;
    }

  float4x4 frustum(float left, float right, float bottom, float top, float near_plane, float far_plane)
    {
    float4x4 out(_mm_set_ps(0.f, 0.f, 0.f, 2.f * near_plane / (right - left)), _mm_set_ps(0.f, 0.f, -2.f * near_plane / (top - bottom), 0.f),
      _mm_set_ps(-1.f, -(far_plane + near_plane) / (far_plane - near_plane), -(top + bottom) / (top - bottom), (right + left) / (right - left)),
      _mm_set_ps(0.f, -(2.f * far_plane * near_plane) / (far_plane - near_plane), 0.f, 0.f));
    return out;
    }

  float4x4 orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane)
    {
    float4x4 out(_mm_set_ps(-(right + left) / (right - left), 0.f, 0.f, 2.f / (right - left)),
      _mm_set_ps(-(top + bottom) / (top - bottom), 0.f, 2.f / (top - bottom), 0.f),
      _mm_set_ps(-(far_plane + near_plane) / (far_plane - near_plane), -2.f / (far_plane - near_plane), 0.f, 0.f),
      _mm_set_ps(1.f, 0.f, 0.f, 0.f));
    return out;
    }


#else
  float4 operator + (const float4& a)
    {
    return a;
    }

  float4 operator - (const float4& a)
    {
    return float4(-a.f[0], -a.f[1], -a.f[2], -a.f[3]);
    }

  float4 operator + (const float4& left, const float4& right)
    {
    return float4(left.f[0] + right.f[0], left.f[1] + right.f[1], left.f[2] + right.f[2], left.f[3] + right.f[3]);
    }

  float4 operator - (const float4& left, const float4& right)
    {
    return float4(left.f[0] - right.f[0], left.f[1] - right.f[1], left.f[2] - right.f[2], left.f[3] - right.f[3]);
    }

  float4 operator * (const float4& left, const float4& right)
    {
    return float4(left.f[0] * right.f[0], left.f[1] * right.f[1], left.f[2] * right.f[2], left.f[3] * right.f[3]);
    }

  float4 operator * (const float4& left, float right)
    {
    return left * float4(right);
    }

  float4 operator * (float left, const float4& right)
    {
    return float4(left) * right;
    }

  float4 operator / (const float4& left, const float4& right)
    {
    return float4(left.f[0] / right.f[0], left.f[1] / right.f[1], left.f[2] / right.f[2], left.f[3] / right.f[3]);
    }

  float4 operator / (const float4& left, float right)
    {
    return left / float4(right);
    }

  float4 operator / (float left, const float4& right)
    {
    return float4(left) / right;
    }

  float4 min(const float4& left, const float4& right)
    {
    return float4(std::min(left.f[0], right.f[0]), std::min(left.f[1], right.f[1]), std::min(left.f[2], right.f[2]), std::min(left.f[3], right.f[3]));
    }

  float4 max(const float4& left, const float4& right)
    {
    return float4(std::max(left.f[0], right.f[0]), std::max(left.f[1], right.f[1]), std::max(left.f[2], right.f[2]), std::max(left.f[3], right.f[3]));
    }

  float min_horizontal(const float4& x)
    {
    return std::min(std::min(std::min(x.f[0], x.f[1]), x.f[2]), x.f[3]);
    }

  float max_horizontal(const float4& x)
    {
    return std::max(std::max(std::max(x.f[0], x.f[1]), x.f[2]), x.f[3]);
    }

  float4 cross(const float4& left, const float4& right)
    {
    return float4(left.f[1] * right.f[2] - left.f[2] * right.f[1], left.f[2] * right.f[0] - left.f[0] * right.f[2], left.f[0] * right.f[1] - left.f[1] * right.f[0], 0.f);
    }

  float dot(const float4& left, const float4& right)
    {
    return left.f[0] * right.f[0] + left.f[1] * right.f[1] + left.f[2] * right.f[2];
    }

  float dot4(const float4& left, const float4& right)
    {
    return left.f[0] * right.f[0] + left.f[1] * right.f[1] + left.f[2] * right.f[2] + left.f[3] * right.f[3];
    }

  float4 abs(const float4& a)
    {
    return float4(fabs(a.f[0]), fabs(a.f[1]), fabs(a.f[2]), fabs(a.f[3]));
    }

  float4 sqrt(const float4& a)
    {
    return float4(std::sqrt(a.f[0]), std::sqrt(a.f[1]), std::sqrt(a.f[2]), std::sqrt(a.f[3]));
    }

  float4 rsqrt(const float4& a)
    {
    return float4(1.f / std::sqrt(a.f[0]), 1.f / std::sqrt(a.f[1]), 1.f / std::sqrt(a.f[2]), 1.f / std::sqrt(a.f[3]));
    }

  float4 reciprocal(const float4& a)
    {
    return float4(1.f / a.f[0], 1.f / a.f[1], 1.f / a.f[2], 1.f / a.f[3]);
    }

  float4 unpacklo(const float4& left, const float4& right)
    {
    return float4(left.f[0], right.f[0], left.f[1], right.f[1]);
    }

  float4 unpackhi(const float4& left, const float4& right)
    {
    return float4(left.f[2], right.f[2], left.f[3], right.f[3]);
    }

  void transpose(float4& r0, float4& r1, float4& r2, float4& r3, const float4& c0, const float4& c1, const float4& c2, const float4& c3)
    {
    float4 l02(unpacklo(c0, c2));
    float4 h02(unpackhi(c0, c2));
    float4 l13(unpacklo(c1, c3));
    float4 h13(unpackhi(c1, c3));
    r0 = unpacklo(l02, l13);
    r1 = unpackhi(l02, l13);
    r2 = unpacklo(h02, h13);
    r3 = unpackhi(h02, h13);
    }

  float4x4 get_identity()
    {
    float4x4 m(float4(1.f, 0.f, 0.f, 0.f), float4(0.f, 1.f, 0.f, 0.f), float4(0.f, 0.f, 1.f, 0.f), float4(0.f, 0.f, 0.f, 1.f));
    return m;
    }

  float4x4 make_translation(float x, float y, float z)
    {
    float4x4 m(float4(1.f, 0.f, 0.f, 0.f), float4(0.f, 1.f, 0.f, 0.f), float4(0.f, 0.f, 1.f, 0.f), float4(x, y, z, 1.f));
    return m;
    }

  float4x4 invert(const float4x4& m)
    {
    float4x4 out;
    float det;
    int i;

    out[0] = m[5] * m[10] * m[15] -
      m[5] * m[11] * m[14] -
      m[9] * m[6] * m[15] +
      m[9] * m[7] * m[14] +
      m[13] * m[6] * m[11] -
      m[13] * m[7] * m[10];

    out[4] = -m[4] * m[10] * m[15] +
      m[4] * m[11] * m[14] +
      m[8] * m[6] * m[15] -
      m[8] * m[7] * m[14] -
      m[12] * m[6] * m[11] +
      m[12] * m[7] * m[10];

    out[8] = m[4] * m[9] * m[15] -
      m[4] * m[11] * m[13] -
      m[8] * m[5] * m[15] +
      m[8] * m[7] * m[13] +
      m[12] * m[5] * m[11] -
      m[12] * m[7] * m[9];

    out[12] = -m[4] * m[9] * m[14] +
      m[4] * m[10] * m[13] +
      m[8] * m[5] * m[14] -
      m[8] * m[6] * m[13] -
      m[12] * m[5] * m[10] +
      m[12] * m[6] * m[9];

    out[1] = -m[1] * m[10] * m[15] +
      m[1] * m[11] * m[14] +
      m[9] * m[2] * m[15] -
      m[9] * m[3] * m[14] -
      m[13] * m[2] * m[11] +
      m[13] * m[3] * m[10];

    out[5] = m[0] * m[10] * m[15] -
      m[0] * m[11] * m[14] -
      m[8] * m[2] * m[15] +
      m[8] * m[3] * m[14] +
      m[12] * m[2] * m[11] -
      m[12] * m[3] * m[10];

    out[9] = -m[0] * m[9] * m[15] +
      m[0] * m[11] * m[13] +
      m[8] * m[1] * m[15] -
      m[8] * m[3] * m[13] -
      m[12] * m[1] * m[11] +
      m[12] * m[3] * m[9];

    out[13] = m[0] * m[9] * m[14] -
      m[0] * m[10] * m[13] -
      m[8] * m[1] * m[14] +
      m[8] * m[2] * m[13] +
      m[12] * m[1] * m[10] -
      m[12] * m[2] * m[9];

    out[2] = m[1] * m[6] * m[15] -
      m[1] * m[7] * m[14] -
      m[5] * m[2] * m[15] +
      m[5] * m[3] * m[14] +
      m[13] * m[2] * m[7] -
      m[13] * m[3] * m[6];

    out[6] = -m[0] * m[6] * m[15] +
      m[0] * m[7] * m[14] +
      m[4] * m[2] * m[15] -
      m[4] * m[3] * m[14] -
      m[12] * m[2] * m[7] +
      m[12] * m[3] * m[6];

    out[10] = m[0] * m[5] * m[15] -
      m[0] * m[7] * m[13] -
      m[4] * m[1] * m[15] +
      m[4] * m[3] * m[13] +
      m[12] * m[1] * m[7] -
      m[12] * m[3] * m[5];

    out[14] = -m[0] * m[5] * m[14] +
      m[0] * m[6] * m[13] +
      m[4] * m[1] * m[14] -
      m[4] * m[2] * m[13] -
      m[12] * m[1] * m[6] +
      m[12] * m[2] * m[5];

    out[3] = -m[1] * m[6] * m[11] +
      m[1] * m[7] * m[10] +
      m[5] * m[2] * m[11] -
      m[5] * m[3] * m[10] -
      m[9] * m[2] * m[7] +
      m[9] * m[3] * m[6];

    out[7] = m[0] * m[6] * m[11] -
      m[0] * m[7] * m[10] -
      m[4] * m[2] * m[11] +
      m[4] * m[3] * m[10] +
      m[8] * m[2] * m[7] -
      m[8] * m[3] * m[6];

    out[11] = -m[0] * m[5] * m[11] +
      m[0] * m[7] * m[9] +
      m[4] * m[1] * m[11] -
      m[4] * m[3] * m[9] -
      m[8] * m[1] * m[7] +
      m[8] * m[3] * m[5];

    out[15] = m[0] * m[5] * m[10] -
      m[0] * m[6] * m[9] -
      m[4] * m[1] * m[10] +
      m[4] * m[2] * m[9] +
      m[8] * m[1] * m[6] -
      m[8] * m[2] * m[5];

    det = m[0] * out[0] + m[1] * out[4] + m[2] * out[8] + m[3] * out[12];

    for (i = 0; i < 16; ++i)
      out[i] /= det;
    return out;
    }

  float4x4 matrix_matrix_multiply(const float4x4& left, const float4x4& right)
    {
    float4x4 out;
    float4 r[4];
    transpose(r[0], r[1], r[2], r[3], left.col[0], left.col[1], left.col[2], left.col[3]);

    out[0] = dot4(r[0], right.col[0]);
    out[1] = dot4(r[1], right.col[0]);
    out[2] = dot4(r[2], right.col[0]);
    out[3] = dot4(r[3], right.col[0]);
    out[4] = dot4(r[0], right.col[1]);
    out[5] = dot4(r[1], right.col[1]);
    out[6] = dot4(r[2], right.col[1]);
    out[7] = dot4(r[3], right.col[1]);
    out[8] = dot4(r[0], right.col[2]);
    out[9] = dot4(r[1], right.col[2]);
    out[10] = dot4(r[2], right.col[2]);
    out[11] = dot4(r[3], right.col[2]);
    out[12] = dot4(r[0], right.col[3]);
    out[13] = dot4(r[1], right.col[3]);
    out[14] = dot4(r[2], right.col[3]);
    out[15] = dot4(r[3], right.col[3]);
    return out;
    }

  float4x4 frustum(float left, float right, float bottom, float top, float near_plane, float far_plane)
    {
    //float4x4 out(_mm_set_ps(0.f, 0.f, 0.f, 2.f * near_plane / (right - left)), _mm_set_ps(0.f, 0.f, -2.f * near_plane / (top - bottom), 0.f),
    //  _mm_set_ps(-1.f, -(far_plane + near_plane) / (far_plane - near_plane), -(top + bottom) / (top - bottom), (right + left) / (right - left)),
    //  _mm_set_ps(0.f, -(2.f * far_plane * near_plane) / (far_plane - near_plane), 0.f, 0.f));
    float4x4 out(float4(2.f * near_plane / (right - left), 0.f, 0.f, 0.f), 
      float4(0.f, -2.f * near_plane / (top - bottom), 0.f, 0.f),
      float4((right + left) / (right - left), -(top + bottom) / (top - bottom), -(far_plane + near_plane) / (far_plane - near_plane), -1.f),
      float4(0.f, 0.f, -(2.f * far_plane * near_plane) / (far_plane - near_plane), 0.f));
    return out;
    }

  float4x4 orthographic(float left, float right, float bottom, float top, float near_plane, float far_plane)
    {
    //float4x4 out(_mm_set_ps(-(right + left) / (right - left), 0.f, 0.f, 2.f / (right - left)),
    //  _mm_set_ps(-(top + bottom) / (top - bottom), 0.f, 2.f / (top - bottom), 0.f),
    //  _mm_set_ps(-(far_plane + near_plane) / (far_plane - near_plane), -2.f / (far_plane - near_plane), 0.f, 0.f),
    //  _mm_set_ps(1.f, 0.f, 0.f, 0.f));
    float4x4 out(float4(2.f / (right - left), 0.f, 0.f, -(right + left) / (right - left)),
      float4(0.f, 2.f / (top - bottom), 0.f, -(top + bottom) / (top - bottom)),
      float4(0.f, 0.f, -2.f / (far_plane - near_plane), -(far_plane + near_plane) / (far_plane - near_plane)),
      float4(0.f, 0.f, 0.f, 1.f));
    return out;
    }

#endif

  float4 normalize(const float4& v)
    {
    double d = dot(v, v);
    if (d < 1e-20f)
      {
      return float4(1, 0, 0, v[3]);
      }
    else
      {
      d = 1.0 / std::sqrt(d);
      return float4((float)(v[0] * d), (float)(v[1] * d), (float)(v[2] * d), v[3]);
      }
    }

  void solve_roll_pitch_yaw_transformation(float& rx, float& ry, float& rz, float& tx, float& ty, float& tz, const float4x4& m)
    {
    rz = std::atan2(m.col[0][1], m.col[0][0]);
    const auto sg = std::sin(rz);
    const auto cg = std::cos(rz);
    ry = std::atan2(-m.col[0][2], m.col[0][0] * cg + m.col[0][1] * sg);
    rx = std::atan2(m.col[2][0] * sg - m.col[2][1] * cg, m.col[1][1] * cg - m.col[1][0] * sg);
    tx = m.col[3][0];
    ty = m.col[3][1];
    tz = m.col[3][2];
    }

  float4x4 compute_from_roll_pitch_yaw_transformation(
    float rx, float ry, float rz,
    float tx, float ty, float tz)
    {
    float4x4 m = get_identity();
    float ca = std::cos(rx);
    float sa = std::sin(rx);
    float cb = std::cos(ry);
    float sb = std::sin(ry);
    float cg = std::cos(rz);
    float sg = std::sin(rz);
    m.col[0][0] = cb * cg;
    m.col[1][0] = cg * sa * sb - ca * sg;
    m.col[2][0] = sa * sg + ca * cg * sb;
    m.col[0][1] = cb * sg;
    m.col[1][1] = sa * sb * sg + ca * cg;
    m.col[2][1] = ca * sb * sg - cg * sa;
    m.col[0][2] = -sb;
    m.col[1][2] = cb * sa;
    m.col[2][2] = ca * cb;
    m.col[3][0] = tx;
    m.col[3][1] = ty;
    m.col[3][2] = tz;
    return m;
    }

  float4x4 quaternion_to_rotation(const float4& quaternion)
    {
    float4x4 rot;
    rot[0] = 1.f - 2.f * (quaternion[1] * quaternion[1] + quaternion[2] * quaternion[2]);
    rot[4] = 2.f * (quaternion[0] * quaternion[1] - quaternion[2] * quaternion[3]);
    rot[8] = 2.f * (quaternion[2] * quaternion[0] + quaternion[1] * quaternion[3]);
    rot[12] = 0.f;

    rot[1] = 2.f * (quaternion[0] * quaternion[1] + quaternion[2] * quaternion[3]);
    rot[5] = 1.f - 2.f * (quaternion[2] * quaternion[2] + quaternion[0] * quaternion[0]);
    rot[9] = 2.f * (quaternion[1] * quaternion[2] - quaternion[0] * quaternion[3]);
    rot[13] = 0.f;

    rot[2] = 2.f * (quaternion[2] * quaternion[0] - quaternion[1] * quaternion[3]);
    rot[6] = 2.f * (quaternion[1] * quaternion[2] + quaternion[0] * quaternion[3]);
    rot[10] = 1.f - 2.f * (quaternion[1] * quaternion[1] + quaternion[0] * quaternion[0]);
    rot[14] = 0.f;

    rot[3] = 0.f;
    rot[7] = 0.f;
    rot[11] = 0.f;
    rot[15] = 1.f;
    return rot;
    }

  float4 quaternion_multiply(const float4& q1, const float4& q2)
    {
    return float4(q1[3] * q2[0] + q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1],
      q1[3] * q2[1] - q1[0] * q2[2] + q1[1] * q2[3] + q1[2] * q2[0],
      q1[3] * q2[2] + q1[0] * q2[1] - q1[1] * q2[0] + q1[2] * q2[3],
      q1[3] * q2[3] - q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2]);
    }

  float4 quaternion_axis(const float4& q)
    {
    return normalize(float4(q[0], q[1], q[2], 0));
    }

  float quaternion_angle(const float4& q)
    {
    return 2.f * std::acos(q[3]);
    }

  float4 quaternion_conjugate(const float4& quaternion)
    {
    return float4(-quaternion[0], -quaternion[1], -quaternion[2], quaternion[3]);
    }

  float4 quaternion_inverse(const float4& quaternion)
    {
    //float denom = quaternion[0] * quaternion[0] + quaternion[1] * quaternion[1] + quaternion[2] * quaternion[2] + quaternion[3] * quaternion[3];
    float denom = dot4(quaternion, quaternion);
    return quaternion_conjugate(quaternion) / denom;
    }

  float4 quaternion_normalize(const float4& quaternion)
    {
    //float denom = std::sqrt(quaternion[0] * quaternion[0] + quaternion[1] * quaternion[1] + quaternion[2] * quaternion[2] + quaternion[3] * quaternion[3]);
    float denom = std::sqrt(dot4(quaternion, quaternion));
    return quaternion / denom;
    }

  float4x4 look_at(const float4& eye, const float4& center, const float4& up)
    {
    float4x4 m;
    float4 z = normalize(eye - center);
    float4 x = normalize(cross(up, z));
    float4 y = cross(z, x);
    float4 X(x[0], y[0], z[0], 0.f);
    float4 Y(x[1], y[1], z[1], 0.f);
    float4 Z(x[2], y[2], z[2], 0.f);
    float4 W(-dot(x, eye), -dot(y, eye), -dot(z, eye), 1.f);
    m.col[0] = X;
    m.col[1] = Y;
    m.col[2] = Z;
    m.col[3] = W;
    return m;
    }

  void get_eye_center_up(float4& eye, float4& center, float4& up, const float4x4& transform)
    {
    float4x4 tr_inv = invert_orthonormal(transform);
    eye[0] = tr_inv[12];
    eye[1] = tr_inv[13];
    eye[2] = tr_inv[14];

    up = float4(tr_inv[4], tr_inv[5], tr_inv[6]);
    center = eye - float4(tr_inv[8], tr_inv[9], tr_inv[10]);
    }

  float4 roll_pitch_yaw_to_quaternion(float rx, float ry, float rz)
    {
    float cy = std::cos(rz * 0.5f);
    float sy = std::sin(rz * 0.5f);
    float cp = std::cos(ry * 0.5f);
    float sp = std::sin(ry * 0.5f);
    float cr = std::cos(rx * 0.5f);
    float sr = std::sin(rx * 0.5f);

    float4 q;
    q[0] = sr * cp * cy - cr * sp * sy;
    q[1] = cr * sp * cy + sr * cp * sy;
    q[2] = cr * cp * sy - sr * sp * cy;
    q[3] = cr * cp * cy + sr * sp * sy;
    return q;
    }

  float4 rotation_to_quaternion(const float4x4& m)
    {
    float rz = std::atan2(m.col[0][1], m.col[0][0]);
    const auto sg = std::sin(rz);
    const auto cg = std::cos(rz);
    float ry = std::atan2(-m.col[0][2], m.col[0][0] * cg + m.col[0][1] * sg);
    float rx = std::atan2(m.col[2][0] * sg - m.col[2][1] * cg, m.col[1][1] * cg - m.col[1][0] * sg);
    return roll_pitch_yaw_to_quaternion(rx, ry, rz);
    }

  float4x4 transpose(const float4x4& m)
    {
    float4x4 out;
    transpose(out.col[0], out.col[1], out.col[2], out.col[3], m.col[0], m.col[1], m.col[2], m.col[3]);
    return out;
    }

  float4x4 invert_orthonormal(const float4x4& m)
    {
    float4x4 out;
    transpose(out.col[0], out.col[1], out.col[2], out.col[3], m.col[0], m.col[1], m.col[2], float4(0.f, 0.f, 0.f, 1.f));
    out.col[3] = -(out.col[0] * m[12] + out.col[1] * m[13] + out.col[2] * m[14]);
    out.f[15] = 1.f;
    return out;
    }

  float4 matrix_vector_multiply(const float4x4& m, const float4& v)
    {
    float4 out = m.col[0] * v[0] + m.col[1] * v[1] + m.col[2] * v[2] + m.col[3] * v[3];
    return out;
    }

  float4x4 operator + (const float4x4& left, const float4x4& right)
    {
    return float4x4(left.col[0] + right.col[0], left.col[1] + right.col[1], left.col[2] + right.col[2], left.col[3] + right.col[3]);
    }

  float4x4 operator - (const float4x4& left, const float4x4& right)
    {
    return float4x4(left.col[0] - right.col[0], left.col[1] - right.col[1], left.col[2] - right.col[2], left.col[3] - right.col[3]);
    }

  float4x4 operator / (const float4x4& left, float value)
    {
    return float4x4(left.col[0] / value, left.col[1] / value, left.col[2] / value, left.col[3] / value);
    }

  float4x4 operator * (const float4x4& left, float value)
    {
    return float4x4(left.col[0] * value, left.col[1] * value, left.col[2] * value, left.col[3] * value);
    }

  float4x4 operator * (float value, const float4x4& right)
    {
    return float4x4(right.col[0] * value, right.col[1] * value, right.col[2] * value, right.col[3] * value);
    }

  float4x4 make_identity()
    {
    return get_identity();
    }

  float4 transform_vector(const float4x4& matrix, const float4& vec)
    {
    auto res = matrix_vector_multiply(matrix, float4(vec[0], vec[1], vec[2], 0.f));
    return float4(res[0], res[1], res[2]);
    }

  float4 transform(const float4x4& matrix, const float4& pt, bool is_vector)
    {
    if (is_vector)
      return transform_vector(matrix, pt);
    else
      return transform(matrix, pt);
    }

  float4 transform(const float4x4& matrix, const float4& pt)
    {
    auto res = matrix_vector_multiply(matrix, pt);
    if (res[3] != 1.f && res[3])
      {
      res[0] /= res[3];
      res[1] /= res[3];
      res[2] /= res[3];
      res[3] = 1.f;
      }
    return res;
    }

  float4x4 make_transformation(const float4& i_origin, const float4& i_x_axis, const float4& i_y_axis, const float4& i_z_axis)
    {
    float4x4 matrix;
    matrix[0] = i_x_axis[0];
    matrix[1] = i_x_axis[1];
    matrix[2] = i_x_axis[2];
    matrix[3] = 0;
    matrix[4] = i_y_axis[0];
    matrix[5] = i_y_axis[1];
    matrix[6] = i_y_axis[2];
    matrix[7] = 0;
    matrix[8] = i_z_axis[0];
    matrix[9] = i_z_axis[1];
    matrix[10] = i_z_axis[2];
    matrix[11] = 0;
    matrix[12] = i_origin[0];
    matrix[13] = i_origin[1];
    matrix[14] = i_origin[2];
    matrix[15] = 1;
    return matrix;
    }

  float4x4 make_rotation(const float4& i_position, const float4& i_direction, float i_angle_radians)
    {
    auto matrix = make_identity();
    auto direction = normalize(i_direction);

    auto cos_alpha = std::cos(i_angle_radians);
    auto sin_alpha = std::sin(i_angle_radians);

    matrix[0] = (direction[0] * direction[0]) * (1 - cos_alpha) + cos_alpha;
    matrix[4] = (direction[0] * direction[1]) * (1 - cos_alpha) - direction[2] * sin_alpha;
    matrix[8] = (direction[0] * direction[2]) * (1 - cos_alpha) + direction[1] * sin_alpha;

    matrix[1] = (direction[0] * direction[1]) * (1 - cos_alpha) + direction[2] * sin_alpha;
    matrix[5] = (direction[1] * direction[1]) * (1 - cos_alpha) + cos_alpha;
    matrix[9] = (direction[1] * direction[2]) * (1 - cos_alpha) - direction[0] * sin_alpha;

    matrix[2] = (direction[0] * direction[2]) * (1 - cos_alpha) - direction[1] * sin_alpha;
    matrix[6] = (direction[1] * direction[2]) * (1 - cos_alpha) + direction[0] * sin_alpha;
    matrix[10] = (direction[2] * direction[2]) * (1 - cos_alpha) + cos_alpha;

    auto rotated_position = transform_vector(matrix, i_position);

    matrix[12] = i_position[0] - rotated_position[0];
    matrix[13] = i_position[1] - rotated_position[1];
    matrix[14] = i_position[2] - rotated_position[2];

    return matrix;
    }

  float4x4 make_scale3d(float scale_x, float scale_y, float scale_z)
    {
    return float4x4(float4(scale_x, 0.f, 0.f, 0.f), float4(0.f, scale_y, 0.f, 0.f), float4(0.f, 0.f, scale_z, 0.f), float4(0.f, 0.f, 0.f, 1.f));
    }

  float4x4 make_translation(const float4& i_translation)
    {
    return make_translation(i_translation[0], i_translation[1], i_translation[2]);
    }

  float4 get_translation(const float4x4& matrix)
    {
    return float4(matrix[12], matrix[13], matrix[14]);
    }

  void set_x_axis(float4x4& matrix, const float4& x)
    {
    matrix[0] = x[0];
    matrix[1] = x[1];
    matrix[2] = x[2];
    }

  void set_y_axis(float4x4& matrix, const float4& y)
    {
    matrix[4] = y[0];
    matrix[5] = y[1];
    matrix[6] = y[2];
    }

  void set_z_axis(float4x4& matrix, const float4& z)
    {
    matrix[8] = z[0];
    matrix[9] = z[1];
    matrix[10] = z[2];
    }

  void set_translation(float4x4& matrix, const float4& t)
    {
    matrix[12] = t[0];
    matrix[13] = t[1];
    matrix[14] = t[2];
    }

  float4 get_x_axis(const float4x4& matrix)
    {
    return float4(matrix[0], matrix[1], matrix[2]);
    }

  float4 get_y_axis(const float4x4& matrix)
    {
    return float4(matrix[4], matrix[5], matrix[6]);
    }

  float4 get_z_axis(const float4x4& matrix)
    {
    return float4(matrix[8], matrix[9], matrix[10]);
    }

  float determinant(const float4x4& m)
    {
    auto inv0 = m[5] * m[10] * m[15] -
      m[5] * m[11] * m[14] -
      m[9] * m[6] * m[15] +
      m[9] * m[7] * m[14] +
      m[13] * m[6] * m[11] -
      m[13] * m[7] * m[10];

    auto inv4 = -m[4] * m[10] * m[15] +
      m[4] * m[11] * m[14] +
      m[8] * m[6] * m[15] -
      m[8] * m[7] * m[14] -
      m[12] * m[6] * m[11] +
      m[12] * m[7] * m[10];

    auto inv8 = m[4] * m[9] * m[15] -
      m[4] * m[11] * m[13] -
      m[8] * m[5] * m[15] +
      m[8] * m[7] * m[13] +
      m[12] * m[5] * m[11] -
      m[12] * m[7] * m[9];

    auto inv12 = -m[4] * m[9] * m[14] +
      m[4] * m[10] * m[13] +
      m[8] * m[5] * m[14] -
      m[8] * m[6] * m[13] -
      m[12] * m[5] * m[10] +
      m[12] * m[6] * m[9];

    return m[0] * inv0 + m[1] * inv4 + m[2] * inv8 + m[3] * inv12;
    }
  }
