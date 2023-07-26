#pragma once
#include "camera.h"
#include <optional>

namespace rectray {

struct Context {
  Camera Camera;
  ViewportState Viewport;
  std::optional<Ray> Ray;

  Context() {}

  Context(const struct Camera &camera, const ViewportState &screen)
      : Camera(camera), Viewport(screen) {
    if (Viewport.Focus != ViewportFocus::None) {
      Ray = Camera.GetRay(Viewport);
    }
  }

  float PixelToLength(uint32_t pixel, const DirectX::XMFLOAT3 &world) {
    // TODO:
    return 0.1f;
  }

  DirectX::XMFLOAT2 WorldToViewport(const DirectX::XMFLOAT3 &v) const {
    DirectX::XMFLOAT4 p;
    DirectX::XMStoreFloat4(
        &p, DirectX::XMVector4Transform(DirectX::XMVectorSet(v.x, v.y, v.z, 1),
                                        Camera.ViewProjection()));
    return Viewport.ClipToViewport(p);
  }

  DirectX::XMFLOAT2 WorldToViewport(const DirectX::XMFLOAT4X4 &m) const {
    return WorldToViewport(*((const DirectX::XMFLOAT3 *)&m.m[3]));
  }

  std::optional<float> Intersects(const DirectX::XMFLOAT3 &s,
                                  const DirectX::XMFLOAT3 &e, uint32_t pixel) {
    if (!Ray) {
      return {};
    }
    // return LessDistance(s, e, PixelToLength(pixel, s));
    auto c0 = WorldToViewport(s);
    auto c1 = WorldToViewport(e);
    auto c = Viewport.Intersect(c0, c1, pixel);
    if (!c) {
      return {};
    }
    auto w = s + (e - s) * (Length(*c - c0) / Length(c1 - c0));
    return Length(Camera.Transform.Translation - w);
  }

  std::optional<float> Intersects(DirectX::XMMATRIX m) {
    if (!Ray) {
      return {};
    }

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

    auto origin = DirectX::XMLoadFloat3(&Ray->Origin);
    auto direction = DirectX::XMLoadFloat3(&Ray->Direction);

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

  std::optional<float> LessDistance(const DirectX::XMFLOAT3 &q0,
                                    const DirectX::XMFLOAT3 &q1,
                                    float distance) {
    if (!Ray) {
      return {};
    }
    auto &p0 = Ray->Origin;
    auto &pv = Ray->Direction;
    auto qv = Normalized(q1 - q0);

    auto pv2 = Dot(pv, pv);
    auto qv2 = Dot(qv, qv);
    auto vpq = Dot(pv, qv);

    auto dot0 = Dot((q0 - p0), pv);
    auto dot1 = Dot((p0 - q0), qv);

    auto f = 1 / (pv2 * qv2 - vpq * vpq);
    auto s = f * qv2 * dot0 + vpq * dot1;
    auto t = f * vpq * dot0 + pv2 * dot1;

    auto c0 = p0 + pv * s;
    auto c1 = q0 + qv * t;
    auto d = Length(c0 - c1);
    if (d > distance) {
      return {};
    }
    return s;
  }

  std::optional<float> IntersectPlaneDistance(const Plain &p) const {
    if (!Ray) {
      return {};
    }
    float numer = Dot(p.Normal, Ray->Origin) - p.D;
    float denom = Dot(p.Normal, Ray->Direction);
    if (fabsf(denom) <
        FLT_EPSILON) // normal is orthogonal to vector, cant intersect
    {
      return {};
    }
    return -(numer / denom);
  }

  std::optional<DirectX::XMFLOAT3> Intersects(const Plain &p) const {

    if (auto d = IntersectPlaneDistance(p)) {
      // const float len = fabsf(*d); // near plan
      return Ray->Point(*d);
    } else {
      return {};
    }
  }
};

} // namespace rectray
