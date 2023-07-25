#include "renderer.h"
#include "gl_api.h"
#include "plane.h"
#include "scene.h"
#include "triangle.h"
#include <assert.h>
#include <list>
#include <rectray.h>
#include <variant>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

struct ImGuiVisitor {
  ImDrawList *m_drawlist;
  const rectray::marker::Command &m_command;
  ImVec2 m_offset;

  inline const ImVec2 IM(const DirectX::XMFLOAT2 &p) {
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
struct RendererImpl {
  Plane m_plane;
  // Triangle m_triangle;

  rectray::Screen m_screen;

public:
  RendererImpl() {}

  void Render(rectray::Camera &camera, const rectray::WindowMouseState &mouse,
              ImDrawList *imDrawList, Scene *scene,
              rectray::Camera *otherCamera) {
    // only ViewportX update
    camera.Projection.SetRect(mouse.ViewportX, mouse.ViewportY,
                              mouse.ViewportWidth, mouse.ViewportHeight);
    camera.Update();

    m_screen.Begin(camera, mouse);

    for (auto &o : scene->Objects) {
      auto m = o->Matrix();
      m_screen.Cube(m);
      if (o == scene->Selected) {
        m_screen.Translate(o.get(), rectray::Space::Local, m);
      }
    }

    auto result = m_screen.End();
    if (result.Selected) {
      for (auto &o : scene->Objects) {
        if (o.get() == result.Selected) {
          scene->Selected = o;
          if (result.Updated) {
            scene->Selected->SetMatrix(
                DirectX::XMLoadFloat4x4(&*result.Updated));
          }
          break;
        }
      }
    } else {
      // scene->Selected = nullptr;
    }
    if (!result.Updated) {
      // if no manipulation. camera update
      camera.MouseInputTurntable(mouse);
    }

    auto drawlist = m_screen.DrawList();
    drawlist.GizmoToMarker(camera);
    for (auto &c : drawlist.Markers) {
      std::visit(
          ImGuiVisitor{imDrawList, c, {mouse.ViewportX, mouse.ViewportY}},
          c.Shape);
    }

    m_plane.Render(camera);
  }
};

Renderer::Renderer() : m_impl(new RendererImpl) {
  assert(glGetError() == GL_NO_ERROR);
}

Renderer::~Renderer() { delete m_impl; }

void Renderer::Render(rectray::Camera &camera,
                      const rectray::WindowMouseState &mouse,
                      struct ImDrawList *imDrawList, struct Scene *scene,
                      rectray::Camera *otherCamera) {
  m_impl->Render(camera, mouse, imDrawList, scene, otherCamera);
}
