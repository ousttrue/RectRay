#pragma once

#include "viewport.h"

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

  void MouseInputTurntable(const ViewportState &viewport) {
    Projection.SetAspectRatio(viewport.ViewportWidth, viewport.ViewportHeight);
    if (viewport.Focus == ViewportFocus::Active) {
      if (viewport.MouseRightDown) {
        YawPitch(static_cast<int>(viewport.MouseDeltaX),
                 static_cast<int>(viewport.MouseDeltaY));
      }
      if (viewport.MouseMiddleDown) {
        Shift(static_cast<int>(viewport.MouseDeltaX),
              static_cast<int>(viewport.MouseDeltaY), viewport.ViewportHeight);
      }
    }
    if (viewport.Focus != ViewportFocus::None) {
      Dolly(static_cast<int>(viewport.MouseWheel));
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
  std::optional<Ray> GetRay(const ViewportState &viewport) const {
    Ray ret{
        Transform.Translation,
    };

    auto t = tan(Projection.FovY / 2);
    auto h = viewport.ViewportHeight / 2;
    auto y = t * (h - viewport.MouseY) / h;
    auto w = viewport.ViewportWidth / 2;
    auto x = t * Projection.AspectRatio * (viewport.MouseX - w) / w;

    auto q = DirectX::XMLoadFloat4(&Transform.Rotation);
    DirectX::XMStoreFloat3(&ret.Direction,
                           DirectX::XMVector3Normalize(DirectX::XMVector3Rotate(
                               DirectX::XMVectorSet(x, y, -1, 0), q)));

    // std::cout << x << "," << y << std::endl;

    if (!ret.IsValid()) {
      return std::nullopt;
    }
    return ret;
  }
};

} // namespace rectray
