#pragma once
#include "linearalgebra.h"

namespace rectray {

enum class ViewportFocus {
  None,
  // not drag. in area
  Hover,
  // drag captured
  Active,
};

struct ViewportState {
  ViewportFocus Focus = ViewportFocus::None;
  float ViewportX;
  float ViewportY;
  float ViewportWidth;
  float ViewportHeight;
  float MouseX;
  float MouseY;
  float MouseDeltaX = 0;
  float MouseDeltaY = 0;
  bool MouseLeftDown = false;
  bool MouseRightDown = false;
  bool MouseMiddleDown = false;
  float MouseWheel = 0;

  bool InViewport() const {
    if (MouseX < 0) {
      return false;
    }
    if (MouseX > ViewportWidth) {
      return false;
    }
    if (MouseY < 0) {
      return false;
    }
    if (MouseY > ViewportHeight) {
      return false;
    }
    return true;
  }

  DirectX::XMFLOAT2 ClipToViewport(const DirectX::XMFLOAT4 &c) const {
    auto x = (c.x / c.w) * 0.5f + 0.5f;
    auto y = (-c.y / c.w) * 0.5f + 0.5f;
    return {
        x * ViewportWidth,
        y * ViewportHeight,
    };
  }

  std::optional<DirectX::XMFLOAT2> Intersect(const DirectX::XMFLOAT2 &a,
                                             const DirectX::XMFLOAT2 &b,
                                             uint32_t pixel) {
    DirectX::XMFLOAT2 p{MouseX, MouseY};
    auto d = Dot(p - b, a - b);
    if (d < 0) {
      return {};
    }
    auto ab = Normalized(b - a);
    d = Dot(p - a, ab);
    if (d < 0) {
      return {};
    }
    auto x = a + ab * d;
    auto len = Length(p - x);
    if (len > pixel) {
      return {};
    }
    return x;
  }
};

} // namespace rectray
