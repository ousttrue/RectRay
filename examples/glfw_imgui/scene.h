#pragma once
#include <list>
#include <memory>
#include <rectray.h>
#include <vector>

struct Object {
  DirectX::XMFLOAT3 Shape{1, 1, 1};
  rectray::EuclideanTransform Transform;

  DirectX::XMMATRIX Matrix() const {
    return DirectX::XMMatrixMultiply(
        DirectX::XMMatrixScaling(Shape.x, Shape.y, Shape.z),
        Transform.Matrix());
  }

  bool SetMatrix(const DirectX::XMMATRIX m) {
    DirectX::XMVECTOR s;
    DirectX::XMVECTOR r;
    DirectX::XMVECTOR t;
    if (!DirectX::XMMatrixDecompose(&s, &r, &t, m)) {
      return false;
    }
    DirectX::XMStoreFloat3((DirectX::XMFLOAT3 *)&Shape, s);
    DirectX::XMStoreFloat4((DirectX::XMFLOAT4 *)&Transform.Rotation, r);
    DirectX::XMStoreFloat3((DirectX::XMFLOAT3 *)&Transform.Translation, t);
    return true;
  }
};

struct Scene {
  std::vector<std::shared_ptr<Object>> Objects;
  std::shared_ptr<Object> Selected;
};
