#pragma once
#include "../drawlist.h"
#include "../intersects.h"

namespace rectray {

class Translation : public IDragHandle {

  DrawList *m_drawlist = nullptr;
  Ray m_ray;

  // src: Model position
  DirectX::XMFLOAT4X4 m_src;
  DirectX::XMFLOAT3 m_srcWorld;
  DirectX::XMFLOAT2 m_srcViewport;

  // drag plain
  Plain m_plain;
  DirectX::XMFLOAT3 m_plainStart;
  DirectX::XMFLOAT2 m_plainViewport;

  Translation() {}
  Translation(const Translation &) = delete;
  Translation &operator=(const Translation &) = delete;

  void Initialize(DrawList &drawlist, const Context &context,
                  const DirectX::XMFLOAT4X4 &m,
                  const DirectX::XMFLOAT3 &normal) {
    m_drawlist = &drawlist;

    m_src = m;
    m_srcWorld = *((const DirectX::XMFLOAT3 *)&m.m[3]);
    m_plain = Plain::Create(normal, m_srcWorld);
    m_srcViewport = context.WorldToViewport(m_srcWorld);

    if (auto ray = context.Ray) {
      m_ray = *ray;
      if (auto t = Intersects(m_ray, m_plain)) {
        m_plainStart = m_ray.Point(*t);
        m_plainViewport = context.WorldToViewport(m_plainStart);
      }
    } else {
      assert(false);
    }
  }

public:
  // static Translation &GetInstance(DrawList &drawlist, const Context &context,
  //                                 const DirectX::XMFLOAT4X4 &m,
  //                                 const DirectX::XMFLOAT3 &normal) {
  //   Translation s_instance;
  //   s_instance.Initialize(drawlist, context, m, normal);
  //   return s_instance;
  // }

  void Drag(const Context &context, DirectX::XMFLOAT4X4 *m) override {
    uint32_t translationLineColor = 0xFF0088FF;
    // Vec2 sourcePosOnScreen = current.CameraMouse.WorldToPos(mMatrixOrigin);
    m_drawlist->AddCircle(m_srcViewport, 6.f, translationLineColor);

    if (auto t = Intersects(m_ray, m_plain)) {
      auto newPos = m_ray.Point(*t);
      {
        // draw
        auto pos = context.WorldToViewport(newPos);
        auto delta = m_srcViewport - m_plainViewport;
        m_drawlist->AddCircle(pos + delta, 6.f, translationLineColor);
      }

      auto delta = newPos - m_plainStart;

      // compute delta
      // const Vec4 newOrigin = newPos - m_relative;
      // Vec4 delta = newOrigin - current.Model.position();
      // auto delta = pos-m_srcOnPlain;
      // auto world = m_srcWorld + delta;
      DirectX::XMStoreFloat4x4(
          m, DirectX::XMLoadFloat4x4(&m_src) *
                 DirectX::XMMatrixTranslation(delta.x, delta.y, delta.z));
    }
  }

  static IDragHandle *LocalX(DrawList &drawlist, const Context &context,
                             const DirectX::XMFLOAT4X4 &m) {
    static Translation s_instance;

    if (!context.Ray) {
      return {};
    }
    auto &axis = *((const DirectX::XMFLOAT3 *)&m.m[0]);
    auto cross = Cross(axis, context.Ray->Direction);
    auto normal = Cross(cross, axis);

    s_instance.Initialize(drawlist, context, m, normal);
    // (drawlist, context, m, normal);

    return &s_instance;
  }
  // static std::shared_ptr<IDragHandle> LocalY(const Context &context,
  //                                            const DirectX::XMFLOAT4X4 &m)
  //                                            {
  //
  //   return std::make_shared<Translate>();
  // }
  // static std::shared_ptr<IDragHandle> LocalZ(const Context &context,
  //                                            const DirectX::XMFLOAT4X4 &m)
  //                                            {
  //
  //   return std::make_shared<Translate>();
  // }
};

} // namespace rectray
