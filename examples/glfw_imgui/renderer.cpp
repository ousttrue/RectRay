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

  void Render(rectray::Interface &interface, rectray::Camera &camera,
              const rectray::ScreenState &mouse, ImDrawList *imDrawList,
              Scene *scene, const rectray::Interface *other) {
    // only ViewportX update
    camera.Projection.SetAspectRatio(mouse.ViewportWidth, mouse.ViewportHeight);
    camera.Update();

    interface.Begin(camera, mouse);

    for (auto &o : scene->Objects) {
      auto m = o->Matrix();
      interface.Cube(m);
      if (o == scene->Selected) {
        interface.Translate(o.get(), rectray::Space::Local, m);
      }
    }

    auto result = interface.End();
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

    if (other) {
      auto &otherCamera = other->m_context.Camera;
      interface.Frustum(otherCamera.ViewProjection(),
                        otherCamera.Projection.NearZ,
                        otherCamera.Projection.FarZ);
      if (auto ray = other->m_context.Ray) {
        interface.Ray(*ray, otherCamera.Projection.FarZ);
      }
    }

    auto drawlist = interface.DrawList();
    drawlist.ToMarker(camera, mouse);
    for (auto &c : drawlist.Markers) {
      std::visit(
          ImGuiVisitor{imDrawList, c, {mouse.ViewportX, mouse.ViewportY}},
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

void Renderer::Render(rectray::Interface &interface, rectray::Camera &camera,
                      const rectray::ScreenState &mouse,
                      struct ImDrawList *imDrawList, struct Scene *scene,
                      const rectray::Interface *other) {
  m_impl->Render(interface, camera, mouse, imDrawList, scene, other);
}
