#pragma once
#include <DirectXMath.h>
#include <functional>

namespace rectray {

using DragFunc =
    std::function<void(const struct Context &context,
                       DirectX::XMFLOAT4X4 *matrix, struct DrawList &drawlist)>;

} // namespace rectray
