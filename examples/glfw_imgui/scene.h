#pragma once

namespace rectray {
struct WindowMouseState;
struct Camera;
} // namespace rectray

class Scene {
  struct SceneImpl *m_impl;

public:
  Scene();
  ~Scene();
  void Render(float width, float height, rectray::Camera &camera,
              const rectray::WindowMouseState &mouse,
              struct ImDrawList *ImDrawList,
              rectray::Camera *otherCamera = nullptr);
};
