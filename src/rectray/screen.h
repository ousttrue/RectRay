#pragma once
#include "camera.h"
#include "drawlist.h"
#include <DirectXMath.h>
#include <optional>

namespace rectray {

enum class Space {
  World,
  Local,
};

struct Result {
  void *Selected;
  std::optional<DirectX::XMFLOAT4X4> Updated;
};

class Screen {
  DrawList m_drawlist;
  std::optional<Ray> m_ray;

public:
  void Begin(const Camera &camera, const WindowMouseState &mouse) {
    m_drawlist.Clear();
    m_ray = camera.GetRay(mouse);
  }
  Result End() { return {}; }
  DrawList &DrawList() { return m_drawlist; }

  void Translate(void *id, Space space, DirectX::XMMATRIX m) {}

  void Cube(DirectX::XMMATRIX m) {
    auto hover = false;
    if (m_ray) {
      if (auto hit = Intersects(*m_ray, m)) {
        hover = true;
      }
    }
    // ABGR
    auto YELLOW = 0xFF00FFFF;
    auto WHITE = 0xFFFFFFFF;
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
    auto WHITE = 0xFFFFFFFF;
    m_drawlist.Gizmos.push_back({frustum, WHITE});
  }
};

} // namespace rectray
