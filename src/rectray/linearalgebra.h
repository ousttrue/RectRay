#pragma once

#ifdef __EMSCRIPTEN__
#define _XM_NO_INTRINSICS_
#endif
#include <DirectXCollision.h>
#include <DirectXMath.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdio.h>
#include <vector>

namespace rectray {

inline DirectX::XMFLOAT2 operator+(const DirectX::XMFLOAT2 &l,
                                   const DirectX::XMFLOAT2 &r) {
  return DirectX::XMFLOAT2{l.x + r.x, l.y + r.y};
}
inline DirectX::XMFLOAT2 operator-(const DirectX::XMFLOAT2 &l,
                                   const DirectX::XMFLOAT2 &r) {
  return DirectX::XMFLOAT2{l.x - r.x, l.y - r.y};
}
inline DirectX::XMFLOAT2 operator*(const DirectX::XMFLOAT2 &l, float f) {
  return DirectX::XMFLOAT2{l.x * f, l.y * f};
}
inline DirectX::XMFLOAT2 operator/(const DirectX::XMFLOAT2 &l, float f) {
  return l * (1.0f / f);
}
inline float Dot(const DirectX::XMFLOAT2 &l, const DirectX::XMFLOAT2 &r) {
  return l.x * r.x + l.y * r.y;
}
inline float Length(const DirectX::XMFLOAT2 &v) { return sqrt(Dot(v, v)); }
inline DirectX::XMFLOAT2 Normalized(const DirectX::XMFLOAT2 &v) {
  auto f = 1.0f / Length(v);
  return {v.x * f, v.y * f};
}

inline DirectX::XMFLOAT3 operator+(const DirectX::XMFLOAT3 &l,
                                   const DirectX::XMFLOAT3 &r) {
  return DirectX::XMFLOAT3{l.x + r.x, l.y + r.y, l.z + r.z};
}
inline DirectX::XMFLOAT3 operator-(const DirectX::XMFLOAT3 &l,
                                   const DirectX::XMFLOAT3 &r) {
  return DirectX::XMFLOAT3{l.x - r.x, l.y - r.y, l.z - r.z};
}
inline DirectX::XMFLOAT3 operator*(const DirectX::XMFLOAT3 &l, float f) {
  return DirectX::XMFLOAT3{l.x * f, l.y * f, l.z * f};
}
inline float Dot(const DirectX::XMFLOAT3 &l, const DirectX::XMFLOAT3 &r) {
  return l.x * r.x + l.y * r.y + l.z * r.z;
}
inline float Length(const DirectX::XMFLOAT3 &v) { return sqrt(Dot(v, v)); }
inline DirectX::XMFLOAT3 Normalized(const DirectX::XMFLOAT3 &v) {
  auto f = 1.0f / Length(v);
  return {v.x * f, v.y * f, v.z * f};
}

struct EuclideanTransform {
  DirectX::XMFLOAT4 Rotation = {0, 0, 0, 1};
  DirectX::XMFLOAT3 Translation = {0, 0, 0};

  static EuclideanTransform Store(DirectX::XMVECTOR r, DirectX::XMVECTOR t) {
    EuclideanTransform transform;
    DirectX::XMStoreFloat4(&transform.Rotation, r);
    DirectX::XMStoreFloat3(&transform.Translation, t);
    return transform;
  }

  bool HasRotation() const {
    if (Rotation.x == 0 && Rotation.y == 0 && Rotation.z == 0 &&
        (Rotation.w == 1 || Rotation.w == -1)) {
      return false;
    }
    return true;
  }

  DirectX::XMMATRIX ScalingTranslationMatrix(float scaling) const {
    auto r =
        DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&Rotation));
    auto t = DirectX::XMMatrixTranslation(Translation.x * scaling,
                                          Translation.y * scaling,
                                          Translation.z * scaling);
    return r * t;
  }

  DirectX::XMMATRIX Matrix() const {
    auto r =
        DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&Rotation));
    auto t = DirectX::XMMatrixTranslation(Translation.x, Translation.y,
                                          Translation.z);
    return r * t;
  }

  EuclideanTransform &SetMatrix(DirectX::XMMATRIX m) {
    DirectX::XMVECTOR s;
    DirectX::XMVECTOR r;
    DirectX::XMVECTOR t;
    if (!DirectX::XMMatrixDecompose(&s, &r, &t, m)) {
      assert(false);
    }
    // DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)&InitialScale, s);
    DirectX::XMStoreFloat4(&Rotation, r);
    DirectX::XMStoreFloat3(&Translation, t);
    return *this;
  }

  DirectX::XMMATRIX InversedMatrix() const {
    auto r = DirectX::XMMatrixRotationQuaternion(
        DirectX::XMQuaternionInverse(DirectX::XMLoadFloat4(&Rotation)));
    auto t = DirectX::XMMatrixTranslation(-Translation.x, -Translation.y,
                                          -Translation.z);
    return t * r;
  }

  EuclideanTransform Invrsed() const {
    auto r = DirectX::XMQuaternionInverse(DirectX::XMLoadFloat4(&Rotation));
    auto t = DirectX::XMVector3Rotate(
        DirectX::XMVectorSet(-Translation.x, -Translation.y, -Translation.z, 1),
        r);
    return Store(r, t);
  }

  EuclideanTransform Rotate(DirectX::XMVECTOR r) {
    return EuclideanTransform::Store(
        DirectX::XMQuaternionMultiply(DirectX::XMLoadFloat4(&Rotation), r),
        DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&Translation),
                                    DirectX::XMMatrixRotationQuaternion(r)));
  }
};

struct Ray {
  DirectX::XMFLOAT3 Origin;
  DirectX::XMFLOAT3 Direction;

  bool IsValid() const {
    if (!std::isfinite(Direction.x)) {
      return false;
    }
    if (!std::isfinite(Direction.y)) {
      return false;
    }
    if (!std::isfinite(Direction.z)) {
      return false;
    }
    return true;
  }

  Ray Transform(const DirectX::XMMATRIX &m) const {
    Ray ray;
    DirectX::XMStoreFloat3(&ray.Origin, DirectX::XMVector3Transform(
                                            DirectX::XMLoadFloat3(&Origin), m));
    DirectX::XMStoreFloat3(
        &ray.Direction,
        DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(
            DirectX::XMLoadFloat3(&Direction), m)));
    return ray;
  }

  DirectX::XMFLOAT3 Point(float t) const {
    return {
        Origin.x + Direction.x * t,
        Origin.y + Direction.y * t,
        Origin.z + Direction.z * t,
    };
  }
};

} // namespace rectray
