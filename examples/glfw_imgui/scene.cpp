#include "scene.h"
#include "gl_api.h"
#include "plane.h"
#include "triangle.h"
#include <assert.h>
#include <imgui.h>
#include <list>
#include <rectray.h>
#include <variant>

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

  static inline const ImVec2 &IM(const DirectX::XMFLOAT2 &p) {
    return *((const ImVec2 *)&p);
  };

  void operator()(const rectray::marker::Line &shape) {
    if (m_command.Thickness) {
      m_drawlist->AddLine(IM(shape.P0), IM(shape.P1), m_command.Color,
                          *m_command.Thickness);
    } else {
    }
  }
  void operator()(const rectray::marker::Triangle &shape) {
    if (m_command.Thickness) {
    } else {
      m_drawlist->AddTriangleFilled(IM(shape.P0), IM(shape.P1), IM(shape.P2),
                                    m_command.Color);
    }
  }
  void operator()(const rectray::marker::Circle &shape) {
    if (m_command.Thickness) {
      m_drawlist->AddCircle(IM(shape.Center), shape.Radius, m_command.Color,
                            shape.Segments, *m_command.Thickness);
    } else {
      m_drawlist->AddCircleFilled(IM(shape.Center), shape.Radius,
                                  m_command.Color, shape.Segments);
    }
  }
  void operator()(const rectray::marker::Polyline &shape) {
    if (m_command.Thickness) {
      m_drawlist->AddPolyline((const ImVec2 *)shape.Points.data(),
                              shape.Points.size(), m_command.Color, 0,
                              *m_command.Thickness);
    } else {
      m_drawlist->AddConvexPolyFilled((const ImVec2 *)shape.Points.data(),
                                      shape.Points.size(), m_command.Color);
    }
  }
  void operator()(const rectray::marker::Text &shape) {
    m_drawlist->AddText(IM(shape.Pos), m_command.Color, shape.Label.data(),
                        shape.Label.data() + shape.Label.size());
  }
};
struct SceneImpl {
  rectray::Camera m_camera;
  Plane m_plane;
  Triangle m_triangle;

  rectray::Screen m_screen;
  std::list<Object> m_objects;

public:
  SceneImpl() {
    m_camera.Transform.Translation = {0, 1, 10};
    m_camera.Projection.NearZ = 0.01f;
    m_camera.Projection.FarZ = 1000.0f;

    m_objects.push_back({});
  }

  void Render(float width, float height, const float clear_color[4],
              const rectray::MouseState &mouse) {
    glEnable(GL_DEPTH_TEST);
    // glDepthFunc(GL_ALWAYS);
    glDepthFunc(GL_LEQUAL);
    glViewport(0, 0, (int)width, (int)height);
    glClearColor(clear_color[0] * clear_color[3],
                 clear_color[1] * clear_color[3],
                 clear_color[2] * clear_color[3], clear_color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_camera.Projection.SetSize(width, height);
    m_camera.MouseInputTurntable(mouse);
    m_camera.Update();

    m_screen.Begin(m_camera, mouse);

    for (auto &o : m_objects) {
      DirectX::XMFLOAT4X4 m;
      DirectX::XMStoreFloat4x4(&m, o.Matrix());
      m_screen.Cube(m);
    }

    auto drawlist = m_screen.End();
    drawlist.GizmoToMarker(m_camera);
    for (auto &c : drawlist.Markers) {
      std::visit(ImGuiVisitor{ImGui::GetBackgroundDrawList(), c}, c.Shape);
    }

    m_triangle.Render(m_camera);
    m_plane.Render(m_camera);
  }
};

Scene::Scene() : m_impl(new SceneImpl) { assert(glGetError() == GL_NO_ERROR); }

Scene::~Scene() { delete m_impl; }

void Scene::Render(float width, float height, const float clear_color[4],
                   const rectray::MouseState &mouse) {

  m_impl->Render(width, height, clear_color, mouse);
}
