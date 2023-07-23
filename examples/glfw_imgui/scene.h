#pragma once

namespace rectray {
struct MouseState;
}

class Scene {
  struct SceneImpl *m_impl;

public:
  Scene();
  ~Scene();
  void Render(int width, int height, const float clear_color[4],
              const rectray::MouseState &mouse);
};
