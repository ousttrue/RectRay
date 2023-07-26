#pragma once
#include "../drawlist.h"
#include "../intersects.h"

namespace rectray {

struct Translation {
  // src: Model position
  DirectX::XMFLOAT4X4 m_src;
  DirectX::XMFLOAT3 m_srcWorld;
  DirectX::XMFLOAT2 m_srcViewport;

  // drag plain
  Plain m_plain;
  DirectX::XMFLOAT3 m_plainStart;
  DirectX::XMFLOAT2 m_plainViewport;

  Translation(const Context &context, const DirectX::XMFLOAT4X4 &matrix)
      : m_src(matrix) {
    m_srcWorld = MatrixPosition(m_src);
    m_plain = Plain::Create({1, 0, 0}, m_srcWorld);
    m_srcViewport = context.WorldToViewport(m_srcWorld);

    if (auto ray = context.Ray) {
      if (auto t = Intersects(*ray, m_plain)) {
        m_plainStart = ray->Point(*t);
        m_plainViewport = context.WorldToViewport(m_plainStart);
      }
    } else {
      assert(false);
    }
  }

  void operator()(const Context &context, DirectX::XMFLOAT4X4 *matrix,
                  DrawList &drawlist) {

    const uint32_t translationLineColor = 0xFF0088FF;
    drawlist.AddCircle(m_srcViewport, 6.f, translationLineColor);

    if (auto ray = context.Ray) {
      if (auto t = Intersects(*ray, m_plain)) {
        auto newPos = ray->Point(*t);
        {
          // draw
          auto pos = context.WorldToViewport(newPos);
          auto delta = m_srcViewport - m_plainViewport;
          drawlist.AddCircle(pos + delta, 6.f, translationLineColor);
        }
        //
        //     auto delta = newPos - m_plainStart;
        //
        //     // compute delta
        //     // const Vec4 newOrigin = newPos - m_relative;
        //     // Vec4 delta = newOrigin - current.Model.position();
        //     // auto delta = pos-m_srcOnPlain;
        //     // auto world = m_srcWorld + delta;
        //     DirectX::XMStoreFloat4x4(
        //         m, DirectX::XMLoadFloat4x4(&m_src) *
        //                DirectX::XMMatrixTranslation(delta.x, delta.y,
        //                delta.z));
      }
    }
  }
};

} // namespace rectray
