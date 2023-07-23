#pragma once

namespace rectray {
struct Camera;
}

class Triangle {
  struct TriangleImpl *m_impl;

public:
  Triangle();
  ~Triangle();
  void Render(const rectray::Camera &camera);
};
