#pragma once
#include "camera.h"
#include "intersects.h"
#include <optional>

namespace rectray {

struct Context {
  Camera Camera;
  ViewportState Viewport;
  std::optional<Ray> Ray;

  void Begin(const struct Camera &camera, const ViewportState &viewport) {
    Camera = camera;
    Viewport = viewport;
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
    auto c = Viewport.Intersects(c0, c1, pixel);
    if (!c) {
      return {};
    }
    auto w = s + (e - s) * (Length(*c - c0) / Length(c1 - c0));
    return Length(Camera.Transform.Translation - w);
  }

  // std::optional<float> LessDistance(const DirectX::XMFLOAT3 &q0,
  //                                   const DirectX::XMFLOAT3 &q1,
  //                                   float distance) {
  //   if (!Ray) {
  //     return {};
  //   }
  //   auto &p0 = Ray->Origin;
  //   auto &pv = Ray->Direction;
  //   auto qv = Normalized(q1 - q0);
  //
  //   auto pv2 = Dot(pv, pv);
  //   auto qv2 = Dot(qv, qv);
  //   auto vpq = Dot(pv, qv);
  //
  //   auto dot0 = Dot((q0 - p0), pv);
  //   auto dot1 = Dot((p0 - q0), qv);
  //
  //   auto f = 1 / (pv2 * qv2 - vpq * vpq);
  //   auto s = f * qv2 * dot0 + vpq * dot1;
  //   auto t = f * vpq * dot0 + pv2 * dot1;
  //
  //   auto c0 = p0 + pv * s;
  //   auto c1 = q0 + qv * t;
  //   auto d = Length(c0 - c1);
  //   if (d > distance) {
  //     return {};
  //   }
  //   return s;
  // }
};

struct Drag {

  Plain DragPlain;
  DirectX::XMFLOAT4X4 Matrix;
  DirectX::XMFLOAT3 World;
  DirectX::XMFLOAT2 Viewport;

  Drag(const Context &context, const DirectX::XMFLOAT4X4 &m,
       const DirectX::XMFLOAT3 &plainNormal)
      : DragPlain(Plain::Create(plainNormal, MatrixPosition(m))), Matrix(m) {
    if (auto ray = context.Ray) {
      if (auto t = Intersects(*ray, DragPlain)) {
        World = ray->Point(*t);
        Viewport = context.WorldToViewport(World);
      }
    }
  }
};

} // namespace rectray
