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
    m_ray = camera.GetRay(mouse.MouseX - mouse.ViewportX,
                          mouse.MouseY - mouse.ViewportY);
  }
  Result End() { return {}; }
  DrawList &DrawList() { return m_drawlist; }

  void Translate(void *id, Space space, const DirectX::XMMATRIX m) {}

  void Cube(const DirectX::XMMATRIX m) {
    const float s = 0.5f;
    //  7+-+6
    //  / /|
    // 3+-+2+5
    // | |/
    // 0+-+1
    DirectX::XMFLOAT3 p[]{
        {-s, -s, +s}, {+s, -s, +s}, {+s, +s, +s}, {-s, +s, +s},
        {-s, -s, -s}, {+s, -s, -s}, {+s, +s, -s}, {-s, +s, -s},
    };

    struct Face {
      int I0;
      int I1;
      int I2;
      int I3;
    };
    Face faces[6] = {
        {1, 5, 6, 2}, {2, 6, 7, 3}, {0, 1, 2, 3}, //+x+y+z
        {4, 0, 3, 7}, {5, 1, 0, 4}, {5, 4, 7, 6}, //-x-y-z
    };

    for (int i = 0; i < 8; ++i) {
      DirectX::XMStoreFloat3(
          &p[i], DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&p[i]), m));
    }

    auto hover = false;
    if (m_ray) {
      if (auto hit = Intersects(*m_ray, m)) {
        hover = true;
      }
    }

    // ABGR
    auto YELLOW = 0xFF00FFFF;
    auto WHITE = 0xFFFFFFFF;

    for (int i = 0; i < 6; ++i) {
      auto [i0, i1, i2, i3] = faces[i];

      gizmo::Rect rect{p[i0], p[i1], p[i2], p[i3]};
      m_drawlist.Gizmos.push_back({rect, hover ? YELLOW : WHITE});
    }
  }
};

} // namespace rectray
