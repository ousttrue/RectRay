#pragma once

namespace rectray {
struct Camera;
}

class Plane {
  struct PlaneImpl *m_impl;

public:
  Plane();
  ~Plane();
  void Render(const rectray::Camera &camera);
};
