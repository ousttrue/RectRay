#pragma once

namespace rectray {
struct MouseState;
}

class Scene {
  struct SceneImpl *m_impl;

public:
  Scene();
  ~Scene();
  void Render(float width, float height, const float clear_color[4],
              const rectray::MouseState &mouse);
};
