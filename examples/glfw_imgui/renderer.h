#pragma once

namespace rectray {
struct WindowMouseState;
struct Camera;
class Screen;
} // namespace rectray

class Renderer {
  struct RendererImpl *m_impl;

public:
  Renderer();
  ~Renderer();
  void Render(rectray::Screen &screen, rectray::Camera &camera,
              const rectray::WindowMouseState &mouse,
              struct ImDrawList *ImDrawList, struct Scene *scene,
              rectray::Screen *other = nullptr);
};
