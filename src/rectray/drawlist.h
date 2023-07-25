#pragma once
#include "camera.h"
#include <DirectXMath.h>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace rectray {

namespace gizmo {
// 3D
struct Line {
  DirectX::XMFLOAT3 P0;
  DirectX::XMFLOAT3 P1;
};

struct Triangle {
  DirectX::XMFLOAT3 P0;
  DirectX::XMFLOAT3 P1;
  DirectX::XMFLOAT3 P2;
};

struct Rect {
  DirectX::XMFLOAT3 P0;
  DirectX::XMFLOAT3 P1;
  DirectX::XMFLOAT3 P2;
  DirectX::XMFLOAT3 P3;
};

struct Command {
  std::variant<Line, Triangle, Rect> Shape;
  uint32_t Color;
};

} // namespace gizmo

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
  std::vector<gizmo::Command> Gizmos;
  std::vector<marker::Command> Markers;

  void Clear() {
    Gizmos.clear();
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

  void GizmoToMarker(const Camera &camera) {

    auto vp = camera.ViewProjection();
    DirectX::XMFLOAT4X4 m;
    DirectX::XMStoreFloat4x4(&m, vp);

    struct Visitor {
      const Camera &Camera;
      DrawList *Self;
      DirectX::XMFLOAT4X4 Matrix;
      uint32_t Color;

      void operator()(const gizmo::Line &shape) {}
      void operator()(const gizmo::Triangle &shape) {}
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

        auto ToScreenW = [w = Camera.Projection.Viewport.Width](const auto &c)
        //
        { return (c * 0.5f + 0.5f) * w; };
        auto ToScreenH = [h = Camera.Projection.Viewport.Height](const auto &c)
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
    };

    for (auto &g : Gizmos) {
      std::visit(Visitor{camera, this, m, g.Color}, g.Shape);
    }
    Gizmos.clear();
  }
};

} // namespace rectray
