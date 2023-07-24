#pragma once
#include <optional>
#include <tuple>

class Platform {
  struct PlatformImpl *m_impl;

public:
  Platform();
  ~Platform();
  bool CreateWindow();
  bool BeginFrame();
  void UpdateGui(float clear_color[4]);
  void EndFrame();
};
