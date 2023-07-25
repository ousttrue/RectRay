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
  void *Selected;
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
  Result End() { return {}; }
  DrawList &DrawList() { return m_drawlist; }

  void Translate(void *id, Space space, DirectX::XMMATRIX m) {}

  void Cube(DirectX::XMMATRIX m) {
    auto hover = false;
    if (m_context.Ray) {
      if (auto hit = Intersects(*m_context.Ray, m)) {
        hover = true;
      }
    }
    // ABGR
    gizmo::Cube cube;
    DirectX::XMStoreFloat4x4(&cube.Matrix, m);
    m_drawlist.Gizmos.push_back({cube, hover ? YELLOW : WHITE});
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
