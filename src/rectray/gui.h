#pragma once
#include "camera.h"
#include "context.h"
#include "drag/translation.h"
#include "drawlist.h"
#include <DirectXMath.h>
#include <list>
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
  void *Drag;
  // std::optional<DirectX::XMFLOAT4X4> Updated;
};

class Gui {
  DrawList m_drawlist;
  std::shared_ptr<IDragHandle> m_drag;

public:
  std::list<float> m_hits;
  Context m_context;

  void Begin(const Camera &camera, const ViewportState &viewport) {
    m_hits.clear();
    m_drawlist.Clear();
    m_context = Context(camera, viewport);
  }

  Result End() {
    Result result{};
    gizmo::Command *gizmo = nullptr;

    if (m_drag) {
      if (m_context.Viewport.MouseLeftDown) {
        // drag
        result.Drag = m_drag.get();
        result.Closest = {};
      } else {
        // drag end
        m_drag = {};
      }
    }
    if (!m_drag) {
      auto closest = std::numeric_limits<float>::infinity();
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
        if (m_context.Viewport.MouseLeftDown) {
          if (gizmo->BeginDrag) {
            m_drag = gizmo->BeginDrag(m_context);
          }
        }
      }
    }

    return result;
  }
  DrawList &DrawList() { return m_drawlist; }

  void Arrow(const DirectX::XMFLOAT3 &s, const DirectX::XMFLOAT3 &e,
             uint32_t color, const gizmo::DragFactory &drag) {
    gizmo::Arrow allow{
        s,
        e,
    };
    auto hit = m_context.Intersects(s, e, 4);
    m_drawlist.Gizmos.push_back({allow, color, nullptr, hit, drag});
    if (hit) {
      m_hits.push_back(*hit);
    }
  }

  void Cube(void *handle, DirectX::XMMATRIX m) {
    gizmo::Cube cube;
    DirectX::XMStoreFloat4x4(&cube.Matrix, m);

    std::optional<float> hit;
    if (auto ray = m_context.Ray) {
      hit = Intersects(*ray, m);
    }
    m_drawlist.Gizmos.push_back(
        {cube, WHITE, handle, hit}); // hover ? YELLOW : WHITE});
    if (hit) {
      m_hits.push_back(*hit);
    }
  }

  void Frustum(DirectX::XMMATRIX ViewProjection, float zNear, float zFar) {
    gizmo::Frustum frustum{
        .Near = zNear,
        .Far = zFar,
    };
    DirectX::XMStoreFloat4x4(&frustum.ViewProjection, ViewProjection);
    m_drawlist.Gizmos.push_back({frustum, WHITE});
  }

  void Ray(const Ray &ray, const Plain farPlain) {
    if (auto t = Intersects(ray, farPlain)) {
      primitive::Line line{
          ray.Origin,
          ray.Point(*t),
      };
      m_drawlist.Primitives.push_back({line, YELLOW});
    } else {
      assert(false);
    }
  }

  void Debug(const Gui &gui) {
    auto &otherCamera = gui.m_context.Camera;
    Frustum(otherCamera.ViewProjection(), otherCamera.Projection.NearZ,
            otherCamera.Projection.FarZ);
    if (auto ray = gui.m_context.Ray) {
      Ray(*ray, otherCamera.FarPlain());

      for (auto hit : gui.m_hits) {
        auto w = ray->Point(hit);
        auto p = m_context.WorldToViewport(w);
        m_drawlist.AddCircle(p, 3.f, 0xFFFF00FF);
        m_drawlist.AddCircleFilled(p, 2.f, 0xFF000000);
      }
    }
  }

  bool Translate(Space space, DirectX::XMFLOAT4X4 *matrix) {
    // auto s = o->Transform.Translation;
    auto s = *((const DirectX::XMFLOAT3 *)&matrix->m[3]);
    // Arrow(s, {s.x, s.y + 1, s.z}, 0xFF00FF00, Translate::LocalY);
    // Arrow(s, {s.x, s.y, s.z + 1}, 0xFFFF0000, Translate::LocalZ);

    if (m_drag && m_context.Viewport.MouseLeftDown) {
      // drag
      Arrow(s, {s.x + 1, s.y, s.z}, 0xFF00FFFF,
            [matrix, &drawlist = m_drawlist](const auto &c) {
              return Translation::LocalX(c, drawlist, *matrix);
            });
      m_drag->Drag(m_context, matrix);
      return true;
    }

    Arrow(s, {s.x + 1, s.y, s.z}, 0xFF0000FF,
          [matrix, &drawlist = m_drawlist](const auto &c) {
            return Translation::LocalX(c, drawlist, *matrix);
          });
    return false;
  }
};

} // namespace rectray
