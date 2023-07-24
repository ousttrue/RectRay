#include "scene.h"
#include "gl_api.h"
#include "plane.h"
#include "triangle.h"
#include <assert.h>
#include <list>
#include <rectray.h>
#include <variant>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

struct Object {
  DirectX::XMFLOAT3 Shape{1, 1, 1};
  rectray::EuclideanTransform Transform;

  DirectX::XMMATRIX Matrix() const {
    return DirectX::XMMatrixMultiply(
        DirectX::XMMatrixScaling(Shape.x, Shape.y, Shape.z),
        Transform.Matrix());
  }
};

struct ImGuiVisitor {
  ImDrawList *m_drawlist;
  const rectray::marker::Command &m_command;
  ImVec2 m_offset;

  inline const ImVec2 &IM(const DirectX::XMFLOAT2 &p) {
    return m_offset + *((const ImVec2 *)&p);
  };

  void operator()(rectray::marker::Line &shape) {
    if (m_command.Thickness) {
      m_drawlist->AddLine(IM(shape.P0), IM(shape.P1), m_command.Color,
                          *m_command.Thickness);
    } else {
    }
  }
  void operator()(rectray::marker::Triangle &shape) {
    if (m_command.Thickness) {
    } else {
      m_drawlist->AddTriangleFilled(IM(shape.P0), IM(shape.P1), IM(shape.P2),
                                    m_command.Color);
    }
  }
  void operator()(rectray::marker::Circle &shape) {
    if (m_command.Thickness) {
      m_drawlist->AddCircle(IM(shape.Center), shape.Radius, m_command.Color,
                            shape.Segments, *m_command.Thickness);
    } else {
      m_drawlist->AddCircleFilled(IM(shape.Center), shape.Radius,
                                  m_command.Color, shape.Segments);
    }
  }
  void operator()(rectray::marker::Polyline &shape) {
    std::span<ImVec2> span{(ImVec2 *)shape.Points.data(), shape.Points.size()};
    for (auto &p : span) {
      p += m_offset;
    }
    if (m_command.Thickness) {
      m_drawlist->AddPolyline(span.data(), span.size(), m_command.Color, 0,
                              *m_command.Thickness);
    } else {
      m_drawlist->AddConvexPolyFilled(span.data(), span.size(),
                                      m_command.Color);
    }
  }
  void operator()(rectray::marker::Text &shape) {
    m_drawlist->AddText(IM(shape.Pos), m_command.Color, shape.Label.data(),
                        shape.Label.data() + shape.Label.size());
  }
};
struct SceneImpl {
  Plane m_plane;
  Triangle m_triangle;

  rectray::Screen m_screen;
  std::list<Object> m_objects;

public:
  SceneImpl() { m_objects.push_back({}); }

  void Render(float width, float height, rectray::Camera &camera,
              const rectray::WindowMouseState &mouse, ImDrawList *imDrawList,
              rectray::Camera *otherCamera) {
    camera.Projection.SetSize(width, height);
    camera.MouseInputTurntable(mouse);
    camera.Update();

    m_screen.Begin(camera, mouse);

    for (auto &o : m_objects) {
      DirectX::XMFLOAT4X4 m;
      DirectX::XMStoreFloat4x4(&m, o.Matrix());
      m_screen.Cube(m);
    }

    auto drawlist = m_screen.End();
    drawlist.GizmoToMarker(camera);
    for (auto &c : drawlist.Markers) {
      std::visit(
          ImGuiVisitor{imDrawList, c, {mouse.ViewportX, mouse.ViewportY}},
          c.Shape);
    }

    m_triangle.Render(camera);
    m_plane.Render(camera);
  }
};

Scene::Scene() : m_impl(new SceneImpl) { assert(glGetError() == GL_NO_ERROR); }

Scene::~Scene() { delete m_impl; }

void Scene::Render(float width, float height, rectray::Camera &camera,
                   const rectray::WindowMouseState &mouse,
                   ImDrawList *imDrawList, rectray::Camera *otherCamera) {

  m_impl->Render(width, height, camera, mouse, imDrawList, otherCamera);
}
