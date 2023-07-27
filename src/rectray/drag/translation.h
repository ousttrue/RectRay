#pragma once
#include "../drawlist.h"
#include "../intersects.h"

namespace rectray {

struct Translation {
  enum class DragType {
    X,
    Y,
    Z,
    YZ,
    ZX,
    XY,
    SCREEN,
  };
  DragType Type;

  DirectX::XMFLOAT3 ModelPosition;
  Plain Plain;
  DirectX::XMFLOAT2 ModelPositionViewport;

  // drag plain
  DirectX::XMFLOAT3 PlainOrigin;
  DirectX::XMFLOAT2 PlainStartViewport;

  Translation(const Context &context, const DirectX::XMFLOAT4X4 &matrix,
              DragType type)
      : Type(type) {
    ModelPosition = MatrixPosition(matrix);
    auto normal = CalcNormal(context, matrix, type);
    Plain = Plain::Create(normal, ModelPosition);
    ModelPositionViewport = context.WorldToViewport(ModelPosition);

    if (auto ray = context.Ray) {
      if (auto t = Intersects(*ray, Plain)) {
        PlainOrigin = ray->Point(*t);
        PlainStartViewport = context.WorldToViewport(PlainOrigin);
      }
    } else {
      assert(false);
    }
  }

  DirectX::XMFLOAT3 CalcNormal(const Context &context,
                               const DirectX::XMFLOAT4X4 &model,
                               DragType type) {
    // find new possible way to move
    DirectX::XMFLOAT3 DragPlainNormals[] = {
        MatrixAxisX(model),     // x
        MatrixAxisY(model),     // y
        MatrixAxisZ(model),     // z
        MatrixAxisX(model),     // yz
        MatrixAxisY(model),     // zx
        MatrixAxisZ(model),     // xy
        -context.Ray->Direction // screen
    };

    auto cameraToModel = Normalized(rectray::MatrixPosition(model) -
                                    context.Camera.Transform.Translation);
    for (unsigned int i = 0; i < 3; i++) {
      auto orthoVector = Cross(DragPlainNormals[i], cameraToModel);
      DragPlainNormals[i] = Normalized(Cross(DragPlainNormals[i], orthoVector));
    }
    return DragPlainNormals[(int)type];
  }

  void operator()(const Context &context, DirectX::XMFLOAT4X4 *matrix,
                  DrawList &drawlist) {

    const uint32_t DRAG_COLOR = 0xFF0088FF;
    drawlist.AddCircle(ModelPositionViewport, 6.f, DRAG_COLOR);

    if (auto ray = context.Ray) {
      if (auto t = Intersects(*ray, Plain)) {
        auto newRayPos = ray->Point(*t);

        // compute delta
        auto newModelPos = ModelPosition + (newRayPos - PlainOrigin);
        auto delta = newModelPos - MatrixPosition(*matrix);

        // 1 axis constraint
        if (Type >= DragType::X && Type <= DragType::Z) {
          auto axisIndex = (int)Type - (int)DragType::X;
          auto axisValue = MatrixRow(*matrix, axisIndex);
          delta = axisValue * Dot(axisValue, delta);
        }

        DirectX::XMStoreFloat4x4(matrix, DirectX::XMLoadFloat4x4(matrix) *
                                             DirectX::XMMatrixTranslation(
                                                 delta.x, delta.y, delta.z));

        {
          // draw
          auto newPosViewport =
              context.WorldToViewport(MatrixPosition(*matrix));
          drawlist.AddLine(ModelPositionViewport, newPosViewport, DRAG_COLOR,
                           2.0f);
          drawlist.AddCircle(newPosViewport, 6.f, DRAG_COLOR);
        }
      }
    }
  }
};

} // namespace rectray
