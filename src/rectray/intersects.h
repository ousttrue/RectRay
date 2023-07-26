#pragma once
#include "linearalgebra.h"

namespace rectray {

inline std::optional<float> Intersects(const Ray &ray, const Plain &plain) {
  return -(Dot(plain.Normal, ray.Origin) + plain.D) /
         (Dot(plain.Normal, ray.Direction));
}

// std::optional<float> IntersectPlaneDistance(const Plain &p) const {
//   if (!Ray) {
//     return {};
//   }
//   float numer = Dot(p.Normal, Ray->Origin) - p.D;
//   float denom = Dot(p.Normal, Ray->Direction);
//   if (fabsf(denom) <
//       FLT_EPSILON) // normal is orthogonal to vector, cant intersect
//   {
//     return {};
//   }
//   return -(numer / denom);
// }
//
// std::optional<DirectX::XMFLOAT3> Intersects(const Plain &p) const {
//
//   if (auto d = IntersectPlaneDistance(p)) {
//     // const float len = fabsf(*d); // near plan
//     return Ray->Point(*d);
//   } else {
//     return {};
//   }
// }

inline std::optional<float> Intersects(const Ray &ray, DirectX::XMMATRIX m) {
  // if (!Ray) {
  //   return {};
  // }

  //   7+-+6
  //   / /|
  // 3+-+2 +5
  // | |
  // 0+-+1
  DirectX::XMFLOAT3 p[8] = {
      {-0.5f, -0.5f, +0.5f}, //
      {+0.5f, -0.5f, +0.5f}, //
      {+0.5f, +0.5f, +0.5f}, //
      {-0.5f, +0.5f, +0.5f}, //
      {-0.5f, -0.5f, -0.5f}, //
      {+0.5f, -0.5f, -0.5f}, //
      {+0.5f, +0.5f, -0.5f}, //
      {-0.5f, +0.5f, -0.5f}, //
  };
  for (int i = 0; i < 8; ++i) {
    DirectX::XMStoreFloat3(
        &p[i], DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&p[i]), m));
  }

  static const std::array<int, 4> triangles[] = {
      {1, 5, 6, 2}, // x+
      {3, 2, 6, 7}, // y+
      {0, 1, 2, 3}, // z+
      {4, 7, 3, 0}, // x-
      {1, 0, 4, 5}, // y-
      {5, 6, 7, 4}, // z-
  };

  auto origin = DirectX::XMLoadFloat3(&ray.Origin);
  auto direction = DirectX::XMLoadFloat3(&ray.Direction);

  float closest = std::numeric_limits<float>::infinity();
  for (int t = 0; t < 6; ++t) {
    auto [i0, i1, i2, i3] = triangles[t];
    float dist;
    if (DirectX::TriangleTests::Intersects(origin, direction,
                                           DirectX::XMLoadFloat3(&p[i0]), //
                                           DirectX::XMLoadFloat3(&p[i1]), //
                                           DirectX::XMLoadFloat3(&p[i2]), //
                                           dist)) {
      if (dist < closest) {
        closest = dist;
      }
    } else if (DirectX::TriangleTests::Intersects(
                   origin, direction,             //
                   DirectX::XMLoadFloat3(&p[i2]), //
                   DirectX::XMLoadFloat3(&p[i3]), //
                   DirectX::XMLoadFloat3(&p[i0]), //
                   dist)) {
      if (dist < closest) {
        closest = dist;
      }
    }
  }
  if (std::isfinite(closest)) {
    return closest;
  } else {
    return std::nullopt;
  }
}

} // namespace rectray
