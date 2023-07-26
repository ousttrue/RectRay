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
  void UpdateGui();
  void EndFrame();
};
