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
  Triangle m_triangle;

public:
  RendererImpl() {}

  void Render(rectray::Gui &gui, rectray::Camera &camera,
              const rectray::ViewportState &viewport, ImDrawList *imDrawList,
              Scene *scene, const rectray::Gui *other) {
    // only ViewportX update
    camera.Projection.SetAspectRatio(viewport.ViewportWidth,
                                     viewport.ViewportHeight);
    camera.Update();

    gui.Begin(camera, viewport);

    for (auto &o : scene->Objects) {
      auto m = o->Matrix();
      gui.Cube(o.get(), m);
      if (o == scene->Selected) {
        DirectX::XMFLOAT4X4 matrix;
        DirectX::XMStoreFloat4x4(&matrix, m);
        if (gui.Translate(rectray::Space::Local, &matrix)) {
          o->SetMatrix(DirectX::XMLoadFloat4x4(&matrix));
        }
      }
    }

    auto result = gui.End();

    if (!result.Drag) {
      camera.MouseInputTurntable(viewport);
      if (viewport.MouseLeftDown) {
        for (auto &o : scene->Objects) {
          if (o.get() == result.Closest) {
            scene->Selected = o;
            break;
          }
        }
      }
    }

    if (other) {
      gui.Debug(*other);
    }

    auto drawlist = gui.DrawList();
    drawlist.ToMarker(camera, viewport);
    for (auto &c : drawlist.Markers) {
      std::visit(
          ImGuiVisitor{imDrawList, c, {viewport.ViewportX, viewport.ViewportY}},
          c.Shape);
    }

    m_triangle.Render(camera);
    m_plane.Render(camera);
  }
};

Renderer::Renderer() : m_impl(new RendererImpl) {
  assert(glGetError() == GL_NO_ERROR);
}

Renderer::~Renderer() { delete m_impl; }

void Renderer::Render(rectray::Gui &gui, rectray::Camera &camera,
                      const rectray::ViewportState &viewport,
                      struct ImDrawList *imDrawList, struct Scene *scene,
                      const rectray::Gui *other) {
  m_impl->Render(gui, camera, viewport, imDrawList, scene, other);
}
