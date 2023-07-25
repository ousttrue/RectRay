#pragma once

namespace rectray {
struct ScreenState;
struct Camera;
class Interface;
} // namespace rectray

class Renderer {
  struct RendererImpl *m_impl;

public:
  Renderer();
  ~Renderer();
  void Render(rectray::Interface &interface, rectray::Camera &camera,
              const rectray::ScreenState &mouse,
              struct ImDrawList *ImDrawList, struct Scene *scene,
              const rectray::Interface *other = nullptr);
};
