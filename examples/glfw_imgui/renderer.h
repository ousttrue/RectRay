#pragma once

namespace rectray {
struct WindowMouseState;
struct Camera;
} // namespace rectray

class Renderer {
  struct RendererImpl *m_impl;

public:
  Renderer();
  ~Renderer();
  void Render(rectray::Camera &camera, const rectray::WindowMouseState &mouse,
              struct ImDrawList *ImDrawList, struct Scene *scene,
              rectray::Camera *otherCamera = nullptr);
};
