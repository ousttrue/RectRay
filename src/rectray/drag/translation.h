#pragma once
#include "../drawlist.h"
#include "../intersects.h"

namespace rectray {

struct Translation : IDragHandle {

  DrawList &m_drawlist;
  DirectX::XMFLOAT3 m_srcWorld;
  Plain m_plain;
  DirectX::XMFLOAT2 m_src;
  DirectX::XMFLOAT2 m_srcOnPlain;

  Translation(const Context &context, DrawList &drawlist,
              const DirectX::XMFLOAT4X4 &m, const DirectX::XMFLOAT3 &normal)
      : m_drawlist(drawlist) {
    m_srcWorld = *((const DirectX::XMFLOAT3 *)&m.m[3]);
    m_plain = Plain::Create(normal, m_srcWorld);
    m_src = context.WorldToViewport(m_srcWorld);
    if (auto ray = context.Ray) {
      if (auto t = Intersects(*ray, m_plain)) {
        m_srcOnPlain = context.WorldToViewport(ray->Point(*t));
      }
    }
  }

  void Drag(const Context &context, DirectX::XMFLOAT4X4 *m) override {
    {
      // auto delta = pos-m_srcOnPlain;
      // auto world = m_srcWorld + delta;

      //
      // draw
      //
      // line
      uint32_t translationLineColor = 0xFF0088FF;

      // Vec2 sourcePosOnScreen = current.CameraMouse.WorldToPos(mMatrixOrigin);
      m_drawlist.AddCircle(m_src, 6.f, translationLineColor);

      if (auto ray = context.Ray) {
        if (auto t = Intersects(*ray, m_plain)) {
          auto pos = context.WorldToViewport(ray->Point(*t));
          auto delta = m_src - m_srcOnPlain;
          m_drawlist.AddCircle(pos + delta, 6.f, translationLineColor);
        }
      }
    }
  }

  static std::shared_ptr<IDragHandle> LocalX(const Context &context,
                                             DrawList &drawlist,
                                             const DirectX::XMFLOAT4X4 &m) {

    auto normal = (const DirectX::XMFLOAT3 *)&m.m[0];
    return std::make_shared<Translation>(context, drawlist, m, *normal);
  }
  // static std::shared_ptr<IDragHandle> LocalY(const Context &context,
  //                                            const DirectX::XMFLOAT4X4 &m) {
  //
  //   return std::make_shared<Translate>();
  // }
  // static std::shared_ptr<IDragHandle> LocalZ(const Context &context,
  //                                            const DirectX::XMFLOAT4X4 &m) {
  //
  //   return std::make_shared<Translate>();
  // }
};

} // namespace rectray
