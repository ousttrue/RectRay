#pragma once
#include "camera.h"
#include "context.h"
#include <functional>
#include <list>
#include <memory>
#include <variant>

namespace rectray {

struct IDragHandle {
  virtual ~IDragHandle() {}
  virtual void Drag(const Context &context, DirectX::XMFLOAT4X4 *m) = 0;
};

namespace gizmo {

using DragFactory = std::function<IDragHandle *(const Context &conext)>;

struct Rect {
  DirectX::XMFLOAT3 P0;
  DirectX::XMFLOAT3 P1;
  DirectX::XMFLOAT3 P2;
  DirectX::XMFLOAT3 P3;
};

struct Cube {
  DirectX::XMFLOAT4X4 Matrix;
};

struct Frustum {
  DirectX::XMFLOAT4X4 ViewProjection;
  float Near;
  float Far;
};

struct Arrow {
  DirectX::XMFLOAT3 P0;
  DirectX::XMFLOAT3 P1;
  static std::tuple<DirectX::XMFLOAT2, DirectX::XMFLOAT2>
  GetSide(const DirectX::XMFLOAT2 &s, const DirectX::XMFLOAT2 &e) {
    auto dir = Normalized(e - s) * 10.0f;
    auto p = e - dir;
    auto x = DirectX::XMFLOAT2{-dir.y, dir.x};
    auto l = p + x;
    auto r = p - x;
    return {l, r};
  }
};

struct Command {
  std::variant<Rect, Cube, Frustum, Arrow> Shape;
  uint32_t Color = 0xFFFFFFFF;
  void *Handle = nullptr;
  std::optional<float> RayHit;
  DragFactory BeginDrag;
};

} // namespace gizmo

namespace primitive {

struct Line {
  DirectX::XMFLOAT3 P0;
  DirectX::XMFLOAT3 P1;
};

struct Triangle {
  DirectX::XMFLOAT3 P0;
  DirectX::XMFLOAT3 P1;
  DirectX::XMFLOAT3 P2;
};

struct Command {
  std::variant<Line, Triangle> Shape;
  uint32_t Color;
  std::optional<float> RayHit;
};

} // namespace primitive

namespace marker {
// 2D

struct Line {
  DirectX::XMFLOAT2 P0;
  DirectX::XMFLOAT2 P1;
};
struct Triangle {
  DirectX::XMFLOAT2 P0;
  DirectX::XMFLOAT2 P1;
  DirectX::XMFLOAT2 P2;
};
struct Circle {
  DirectX::XMFLOAT2 Center;
  float Radius;
  int Segments;
};
struct Text {
  DirectX::XMFLOAT2 Pos;
  std::string Label;
};
struct Polyline {
  std::vector<DirectX::XMFLOAT2> Points;
  int Flags = 0;
};

struct Command {
  std::variant<Line, Triangle, Circle, Polyline, Text> Shape;
  uint32_t Color;
  std::optional<float> Thickness;
};

} // namespace marker

struct DrawList {
  std::list<gizmo::Command> Gizmos;
  std::vector<primitive::Command> Primitives;
  std::vector<marker::Command> Markers;

  void Clear() {
    Gizmos.clear();
    Primitives.clear();
    Markers.clear();
  }

  void AddLine(const DirectX::XMFLOAT2 &p0, const DirectX::XMFLOAT2 &p1,
               uint32_t col, float thickness = 1.0f) {
    Markers.push_back({marker::Line{p0, p1}, col, thickness});
  }

  void AddTriangleFilled(const DirectX::XMFLOAT2 &p0,
                         const DirectX::XMFLOAT2 &p1,
                         const DirectX::XMFLOAT2 &p2, uint32_t col) {
    Markers.push_back({marker::Triangle{p0, p1, p2}, col});
  }

  void AddCircle(const DirectX::XMFLOAT2 &center, float radius, uint32_t col,
                 int num_segments = 0, float thickness = 1.0f) {
    Markers.push_back(
        {marker::Circle{center, radius, num_segments}, col, thickness});
  }

  void AddCircleFilled(const DirectX::XMFLOAT2 &center, float radius,
                       uint32_t col, int num_segments = 0) {
    Markers.push_back({marker::Circle{center, radius, num_segments}, col});
  }

  void AddText(const DirectX::XMFLOAT2 &pos, uint32_t col,
               const char *text_begin, const char *text_end = NULL) {
    Markers.push_back(
        {marker::Text{pos, text_end ? std::string{text_begin, text_end}
                                    : std::string{text_begin}},
         col});
  }

  void AddPolyline(const DirectX::XMFLOAT2 *points, int num_points,
                   uint32_t col, int flags, float thickness) {
    marker::Polyline line;
    line.Points.assign(points, points + num_points);
    line.Flags = flags;
    Markers.push_back({line, col, thickness});
  }

  void AddConvexPolyFilled(const DirectX::XMFLOAT2 *points, int num_points,
                           uint32_t col) {
    marker::Polyline line;
    line.Points.assign(points, points + num_points);
    Markers.push_back({line, col});
  }

  void ToMarker(const Camera &camera, const ViewportState &screen) {

    auto vp = camera.ViewProjection();
    DirectX::XMFLOAT4X4 m;
    DirectX::XMStoreFloat4x4(&m, vp);

    struct GizmoVisitor {
      DrawList *Self;
      const Camera &Camera;
      const ViewportState &Viewport;
      DirectX::XMFLOAT4X4 Matrix;
      uint32_t Color;

      void operator()(const gizmo::Rect &r) {
        auto m = DirectX::XMLoadFloat4x4(&Matrix);
        DirectX::XMFLOAT4 p0;
        DirectX::XMStoreFloat4(
            &p0, DirectX::XMVector4Transform(
                     DirectX::XMVectorSet(r.P0.x, r.P0.y, r.P0.z, 1), m));
        DirectX::XMFLOAT4 p1;
        DirectX::XMStoreFloat4(
            &p1, DirectX::XMVector4Transform(
                     DirectX::XMVectorSet(r.P1.x, r.P1.y, r.P1.z, 1), m));
        DirectX::XMFLOAT4 p2;
        DirectX::XMStoreFloat4(
            &p2, DirectX::XMVector4Transform(
                     DirectX::XMVectorSet(r.P2.x, r.P2.y, r.P2.z, 1), m));
        DirectX::XMFLOAT4 p3;
        DirectX::XMStoreFloat4(
            &p3, DirectX::XMVector4Transform(
                     DirectX::XMVectorSet(r.P3.x, r.P3.y, r.P3.z, 1), m));

        auto ToScreenW = [w = Viewport.ViewportWidth](const auto &c)
        //
        { return (c * 0.5f + 0.5f) * w; };
        auto ToScreenH = [h = Viewport.ViewportHeight](const auto &c)
        //
        { return (-c * 0.5f + 0.5f) * h; };

        DirectX::XMFLOAT2 points[5] = {
            {ToScreenW(p0.x / p0.w), ToScreenH(p0.y / p0.w)},
            {ToScreenW(p1.x / p1.w), ToScreenH(p1.y / p1.w)},
            {ToScreenW(p2.x / p2.w), ToScreenH(p2.y / p2.w)},
            {ToScreenW(p3.x / p3.w), ToScreenH(p3.y / p3.w)},
        };
        points[4] = points[0];
        Self->AddPolyline(points, 5, Color, 0, 1);
      }

      void operator()(const gizmo::Cube &cube) {

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
        auto m = DirectX::XMLoadFloat4x4(&cube.Matrix);
        for (int i = 0; i < 8; ++i) {
          DirectX::XMStoreFloat3(&p[i], DirectX::XMVector3Transform(
                                            DirectX::XMLoadFloat3(&p[i]), m));
        }

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
        for (int i = 0; i < 6; ++i) {
          auto [i0, i1, i2, i3] = faces[i];

          gizmo::Rect rect{p[i0], p[i1], p[i2], p[i3]};
          (*this)(rect);
        }
      }

      void operator()(const gizmo::Frustum &frustum) {

        auto inv = DirectX::XMMatrixInverse(
            nullptr, DirectX::XMLoadFloat4x4(&frustum.ViewProjection));
        DirectX::XMFLOAT4 p[8];
        DirectX::XMStoreFloat4(&p[0],
                               DirectX::XMVector3Transform(
                                   DirectX::XMVectorSet(-1, -1, +1, 1), inv));
        DirectX::XMStoreFloat4(&p[1],
                               DirectX::XMVector3Transform(
                                   DirectX::XMVectorSet(+1, -1, +1, 1), inv));
        DirectX::XMStoreFloat4(&p[2],
                               DirectX::XMVector3Transform(
                                   DirectX::XMVectorSet(+1, +1, +1, 1), inv));
        DirectX::XMStoreFloat4(&p[3],
                               DirectX::XMVector3Transform(
                                   DirectX::XMVectorSet(-1, +1, +1, 1), inv));
        DirectX::XMStoreFloat4(&p[4],
                               DirectX::XMVector3Transform(
                                   DirectX::XMVectorSet(-1, -1, 0, 1), inv));
        DirectX::XMStoreFloat4(&p[5],
                               DirectX::XMVector3Transform(
                                   DirectX::XMVectorSet(+1, -1, 0, 1), inv));
        DirectX::XMStoreFloat4(&p[6],
                               DirectX::XMVector3Transform(
                                   DirectX::XMVectorSet(+1, +1, 0, 1), inv));
        DirectX::XMStoreFloat4(&p[7],
                               DirectX::XMVector3Transform(
                                   DirectX::XMVectorSet(-1, +1, 0, 1), inv));

        struct Face {
          int I0;
          int I1;
          int I2;
          int I3;
        };
        Face faces[] = {
            {1, 5, 6, 2}, {2, 6, 7, 3}, {0, 1, 2, 3}, //+x+y+z
            {4, 0, 3, 7}, {5, 1, 0, 4}, {5, 4, 7, 6}, //-x-y-z
        };
        for (int i = 0; i < std::size(faces); ++i) {
          auto [i0, i1, i2, i3] = faces[i];

          DirectX::XMFLOAT3 p0 = {p[i0].x / p[i0].w, p[i0].y / p[i0].w,
                                  p[i0].z / p[i0].w};
          DirectX::XMFLOAT3 p1 = {p[i1].x / p[i1].w, p[i1].y / p[i1].w,
                                  p[i1].z / p[i1].w};
          DirectX::XMFLOAT3 p2 = {p[i2].x / p[i2].w, p[i2].y / p[i2].w,
                                  p[i2].z / p[i2].w};
          DirectX::XMFLOAT3 p3 = {p[i3].x / p[i3].w, p[i3].y / p[i3].w,
                                  p[i3].z / p[i3].w};

          gizmo::Rect rect{p0, p1, p2, p3};
          (*this)(rect);
        }
      }

      void operator()(const gizmo::Arrow &l) {
        auto m = DirectX::XMLoadFloat4x4(&Matrix);
        DirectX::XMFLOAT4 p0, p1;
        DirectX::XMStoreFloat4(
            &p0, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&l.P0), m));
        DirectX::XMStoreFloat4(
            &p1, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&l.P1), m));

        const float THICKNESS = 4;
        auto c0 = Viewport.ClipToViewport(p0);
        auto c1 = Viewport.ClipToViewport(p1);
        Self->AddLine(c0, c1, Color, THICKNESS);

        auto [hl, hr] = gizmo::Arrow::GetSide(c0, c1);
        Self->AddTriangleFilled(c1, hl, hr, Color);
      }
    };

    for (auto &g : Gizmos) {
      std::visit(GizmoVisitor{this, camera, screen, m, g.Color}, g.Shape);
    }
    Gizmos.clear();

    struct PrimitiveVisitor {
      DrawList *Self;
      const Camera &Camera;
      const ViewportState &Viewport;
      DirectX::XMFLOAT4X4 Matrix;
      uint32_t Color;

      void operator()(const primitive::Line &l) {
        auto m = DirectX::XMLoadFloat4x4(&Matrix);
        DirectX::XMFLOAT4 p0, p1;
        DirectX::XMStoreFloat4(
            &p0, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&l.P0), m));
        DirectX::XMStoreFloat4(
            &p1, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&l.P1), m));

        Self->AddLine(Viewport.ClipToViewport(p0), Viewport.ClipToViewport(p1),
                      Color);
      }

      void operator()(const primitive::Triangle &t) {
        auto m = DirectX::XMLoadFloat4x4(&Matrix);
      }
    };

    for (auto &p : Primitives) {
      std::visit(PrimitiveVisitor{this, camera, screen, m, p.Color}, p.Shape);
    }
    Primitives.clear();
  }
};

} // namespace rectray
