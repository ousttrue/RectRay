#pragma once

namespace rectray {
struct Camera;
struct ViewportState;
class Gui;
} // namespace rectray

class Renderer {
  struct RendererImpl *m_impl;

public:
  Renderer();
  ~Renderer();
  void Render(rectray::Gui &gui, rectray::Camera &camera,
              const rectray::ViewportState &viewport,
              struct ImDrawList *ImDrawList, struct Scene *scene,
              const rectray::Gui *other = nullptr);
};
