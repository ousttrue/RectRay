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

struct Projection {
  // Viewport Viewport;
  float FovY = DirectX::XMConvertToRadians(30.0f);
  float NearZ = 0.01f;
  float FarZ = 1000.0f;
  float AspectRatio = 1.0f;
  void SetAspectRatio(float width, float height) {
    AspectRatio = width / height;
  }

  void Update(DirectX::XMFLOAT4X4 *projection) {
    // auto aspectRatio = Viewport.AspectRatio();
#if 1
    DirectX::XMStoreFloat4x4(projection, DirectX::XMMatrixPerspectiveFovRH(
                                             FovY, AspectRatio, NearZ, FarZ));
#else
    float cot = 1 / tan(FovY);
    float a = aspectRatio;
    float f = FarZ;
    float n = NearZ;

    *projection = {
        cot,
        0,
        0,
        0,
        //
        0,
        cot * a,
        0,
        0,
        //
        0,
        0,
        -(f + n) / (f - n),
        -1,
        //
        0,
        0,
        -2 * f * n / (f - n),
        0,
    };
#endif
  }

  // void SetViewport(const struct Viewport &viewport) { Viewport = viewport; }
  //
  // void SetRect(float x, float y, float w, float h) {
  //   SetViewport({x, y, w, h});
  // }

  // void SetSize(float w, float h) { SetRect(0, 0, w, h); }
};

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

enum class ScreenFocus {
  None,
  // not drag. in area
  Hover,
  // drag captured
  Active,
};

struct ScreenState {
  ScreenFocus Focus = ScreenFocus::None;
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
    if (MouseX < ViewportX) {
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

  DirectX::XMFLOAT2 ClipToScreen(const DirectX::XMFLOAT4 &c) const {
    auto x = (c.x / c.w) * 0.5f + 0.5f;
    auto y = (-c.y / c.w) * 0.5f + 0.5f;
    return {
        x * ViewportWidth,
        y * ViewportHeight,
    };
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

struct Camera {
  Projection Projection;
  EuclideanTransform Transform;

  DirectX::XMFLOAT4X4 ViewMatrix;
  DirectX::XMFLOAT4X4 ProjectionMatrix;

  float GazeDistance = 5;
  float TmpYaw;
  float TmpPitch;

  void YawPitch(int dx, int dy) {
    auto inv = Transform.Invrsed();
    auto _m = DirectX::XMMatrixRotationQuaternion(
        DirectX::XMLoadFloat4(&Transform.Rotation));
    DirectX::XMFLOAT4X4 m;
    DirectX::XMStoreFloat4x4(&m, _m);

    auto x = m._31;
    auto y = m._32;
    auto z = m._33;

    auto yaw =
        atan2(x, z) - DirectX::XMConvertToRadians(static_cast<float>(dx));
    TmpYaw = yaw;
    auto qYaw = DirectX::XMQuaternionRotationAxis(
        DirectX::XMVectorSet(0, 1, 0, 0), yaw);

    auto half_pi = static_cast<float>(std::numbers::pi / 2) - 0.01f;
    auto pitch =
        std::clamp(atan2(y, sqrt(x * x + z * z)) +
                       DirectX::XMConvertToRadians(static_cast<float>(dy)),
                   -half_pi, half_pi);
    TmpPitch = pitch;
    auto qPitch = DirectX::XMQuaternionRotationAxis(
        DirectX::XMVectorSet(-1, 0, 0, 0), pitch);

    auto q = DirectX::XMQuaternionInverse(
        DirectX::XMQuaternionMultiply(qPitch, qYaw));
    auto et =
        EuclideanTransform::Store(q, DirectX::XMLoadFloat3(&inv.Translation));
    Transform = et.Invrsed();
  }

  void Shift(int dx, int dy, float viewportHeight) {
    auto factor =
        std::tan(Projection.FovY * 0.5f) * 2.0f * GazeDistance / viewportHeight;

    auto _m = DirectX::XMMatrixRotationQuaternion(
        DirectX::XMLoadFloat4(&Transform.Rotation));
    DirectX::XMFLOAT4X4 m;
    DirectX::XMStoreFloat4x4(&m, _m);

    auto left_x = m._11;
    auto left_y = m._12;
    auto left_z = m._13;
    auto up_x = m._21;
    auto up_y = m._22;
    auto up_z = m._23;
    Transform.Translation.x += (-left_x * dx + up_x * dy) * factor;
    Transform.Translation.y += (-left_y * dx + up_y * dy) * factor;
    Transform.Translation.z += (-left_z * dx + up_z * dy) * factor;
  }

  void Dolly(int d) {
    if (d == 0) {
      return;
    }

    auto _m = DirectX::XMMatrixRotationQuaternion(
        DirectX::XMLoadFloat4(&Transform.Rotation));
    DirectX::XMFLOAT4X4 m;
    DirectX::XMStoreFloat4x4(&m, _m);
    auto x = m._31;
    auto y = m._32;
    auto z = m._33;
    DirectX::XMFLOAT3 Gaze{
        Transform.Translation.x - x * GazeDistance,
        Transform.Translation.y - y * GazeDistance,
        Transform.Translation.z - z * GazeDistance,
    };
    if (d > 0) {
      GazeDistance *= 0.9f;
    } else {
      GazeDistance *= 1.1f;
    }
    Transform.Translation.x = Gaze.x + x * GazeDistance;
    Transform.Translation.y = Gaze.y + y * GazeDistance;
    Transform.Translation.z = Gaze.z + z * GazeDistance;
  }

  void MouseInputTurntable(const ScreenState &mouse) {
    Projection.SetAspectRatio(mouse.ViewportWidth, mouse.ViewportHeight);
    if (mouse.Focus == ScreenFocus::Active) {
      if (mouse.MouseRightDown) {
        YawPitch(static_cast<int>(mouse.MouseDeltaX),
                 static_cast<int>(mouse.MouseDeltaY));
      }
      if (mouse.MouseMiddleDown) {
        Shift(static_cast<int>(mouse.MouseDeltaX),
              static_cast<int>(mouse.MouseDeltaY), mouse.ViewportHeight);
      }
    }
    if (mouse.Focus != ScreenFocus::None) {
      Dolly(static_cast<int>(mouse.MouseWheel));
    }
    Update();
  }

  void Update() {
    Projection.Update(&ProjectionMatrix);
    DirectX::XMStoreFloat4x4(&ViewMatrix, Transform.InversedMatrix());
  }

  DirectX::XMMATRIX ViewProjection() const {
    return DirectX::XMLoadFloat4x4(&ViewMatrix) *
           DirectX::XMLoadFloat4x4(&ProjectionMatrix);
  }

  void Fit(const DirectX::XMFLOAT3 &min, const DirectX::XMFLOAT3 &max) {
    // Yaw = {};
    // Pitch = {};
    Transform.Rotation = {0, 0, 0, 1};
    auto height = max.y - min.y;
    if (fabs(height) < 1e-4) {
      return;
    }
    auto distance = height * 0.5f / std::atan(Projection.FovY * 0.5f);
    Transform.Translation.x = (max.x + min.x) * 0.5f;
    ;
    Transform.Translation.y = (max.y + min.y) * 0.5f;
    Transform.Translation.z = distance * 1.2f;
    GazeDistance = Transform.Translation.z;
    auto r = DirectX::XMVectorGetX(
        DirectX::XMVector3Length(DirectX::XMVectorSubtract(
            DirectX::XMLoadFloat3(&min), DirectX::XMLoadFloat3(&max))));
    Projection.NearZ = r * 0.01f;
    Projection.FarZ = r * 100.0f;
  }

  // 0-> X
  // |
  // v
  //
  // Y
  std::optional<Ray> GetRay(const ScreenState &mouse) const {
    Ray ret{
        Transform.Translation,
    };

    auto t = tan(Projection.FovY / 2);
    auto h = mouse.ViewportHeight / 2;
    auto y = t * (h - (mouse.MouseY - mouse.ViewportY)) / h;
    auto w = mouse.ViewportWidth / 2;
    auto x =
        t * Projection.AspectRatio * ((mouse.MouseX - mouse.ViewportX) - w) / w;

    auto q = DirectX::XMLoadFloat4(&Transform.Rotation);
    DirectX::XMStoreFloat3(
        &ret.Direction,
        DirectX::XMVector3Rotate(
            DirectX::XMVector3Normalize(DirectX::XMVectorSet(x, y, -1, 0)), q));

    // std::cout << x << "," << y << std::endl;

    if (!ret.IsValid()) {
      return std::nullopt;
    }
    return ret;
  }
};

inline std::optional<float> Intersects(const Ray &ray, DirectX::XMMATRIX m) {
  //   7+-+6
  //   / /|
  // 3+-+2 +5
  // | |
  // 0+-+1
  DirectX::XMFLOAT3 p[8] = {
      {-0.5f, -0.5f, +0.5f}, //
      {+0.5f, -0.5f, +0.5f}, //
      {+0.5f, +0.5f, +0.5f}, //
      {-0.5f, +0.5f, +0.5f}, //
      {-0.5f, -0.5f, -0.5f}, //
      {+0.5f, -0.5f, -0.5f}, //
      {+0.5f, +0.5f, -0.5f}, //
      {-0.5f, +0.5f, -0.5f}, //
  };
  for (int i = 0; i < 8; ++i) {
    DirectX::XMStoreFloat3(
        &p[i], DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&p[i]), m));
  }

  static const std::array<int, 4> triangles[] = {
      {1, 5, 6, 2}, // x+
      {3, 2, 6, 7}, // y+
      {0, 1, 2, 3}, // z+
      {4, 7, 3, 0}, // x-
      {1, 0, 4, 5}, // y-
      {5, 6, 7, 4}, // z-
  };

  auto origin = DirectX::XMLoadFloat3(&ray.Origin);
  auto direction = DirectX::XMLoadFloat3(&ray.Direction);

  float closest = std::numeric_limits<float>::infinity();
  for (int t = 0; t < 6; ++t) {
    auto [i0, i1, i2, i3] = triangles[t];
    float dist;
    if (DirectX::TriangleTests::Intersects(origin, direction,
                                           DirectX::XMLoadFloat3(&p[i0]), //
                                           DirectX::XMLoadFloat3(&p[i1]), //
                                           DirectX::XMLoadFloat3(&p[i2]), //
                                           dist)) {
      closest = std::min(dist, closest);
    } else if (DirectX::TriangleTests::Intersects(
                   origin, direction,             //
                   DirectX::XMLoadFloat3(&p[i2]), //
                   DirectX::XMLoadFloat3(&p[i3]), //
                   DirectX::XMLoadFloat3(&p[i0]), //
                   dist)) {
      closest = std::min(dist, closest);
    }
  }
  if (std::isfinite(closest)) {
    return closest;
  } else {
    return std::nullopt;
  }
}

} // namespace rectray
