#pragma once
#include "camera.h"
#include "drawlist.h"
#include <DirectXMath.h>
#include <optional>

namespace rectray {

inline const auto YELLOW = 0xFF00FFFF;
inline const auto WHITE = 0xFFFFFFFF;

enum class Space {
  World,
  Local,
};

struct Result {
  void *Closest;
  void *Target;
  std::optional<DirectX::XMFLOAT4X4> Updated;
};

struct Context {
  Camera Camera;
  ScreenState Mouse;
  std::optional<Ray> Ray;
};

class Interface {
  DrawList m_drawlist;

public:
  Context m_context;
  void Begin(const Camera &camera, const ScreenState &mouse) {
    m_drawlist.Clear();

    m_context = Context{camera, mouse};
    if (mouse.Focus != ScreenFocus::None) {
      m_context.Ray = camera.GetRay(mouse);
    }
  }
  Result End() {
    auto closest = std::numeric_limits<float>::infinity();
    Result result{};
    gizmo::Command *gizmo = nullptr;
    for (auto &g : m_drawlist.Gizmos) {
      if (g.RayHit && *g.RayHit < closest) {
        result.Closest = g.Handle;
        gizmo = &g;
        closest = *g.RayHit;
      }
    }

    if (gizmo) {
      // hover
      gizmo->Color = YELLOW;
    }

    return result;
  }
  DrawList &DrawList() { return m_drawlist; }

  void Arrow(const DirectX::XMFLOAT3 &s, const DirectX::XMFLOAT3 &e,
             uint32_t color) {
    gizmo::Arrow allow{
        s,
        e,
    };
    m_drawlist.Gizmos.push_back({allow, color});
  }

  void Cube(void *handle, DirectX::XMMATRIX m) {
    std::optional<float> hit;
    if (m_context.Ray) {
      hit = Intersects(*m_context.Ray, m);
    }

    // ABGR
    gizmo::Cube cube;
    DirectX::XMStoreFloat4x4(&cube.Matrix, m);
    m_drawlist.Gizmos.push_back(
        {cube, WHITE, handle, hit}); // hover ? YELLOW : WHITE});
  }

  void Frustum(DirectX::XMMATRIX ViewProjection, float zNear, float zFar) {
    gizmo::Frustum frustum{
        .Near = zNear,
        .Far = zFar,
    };
    DirectX::XMStoreFloat4x4(&frustum.ViewProjection, ViewProjection);
    m_drawlist.Gizmos.push_back({frustum, WHITE});
  }

  void Ray(const Ray &ray, float zFar) {
    primitive::Line line{
        ray.Origin,
        ray.Point(zFar),
    };
    m_drawlist.Primitives.push_back({line, YELLOW});
  }
};

} // namespace rectray