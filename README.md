# RectRay

Gizmo Library to Project Ray to Rectangle

## dependencies

- DirectXMath

## TODO

- Camera
  - view matrix
  - projection matrix
    - fovY
    - near, far
    - viewport(aspect ratio)
  - world position
- Mouse
  - cursor x, y
  - button left, right, middle
  - wheel
- Ray
  - ray triangle test
- Gizmo
  - Translate
  - Rotate
  - Scale

## build

### Desktop

```
> meson setup builddir --prefix $(pwd)/prefix
> meson install -C builddir
```

### Emscripten

```
 setup emscripten
> source ~/emsdk/emsdk_env.sh 

> meson setup builddir_em
> meson compile -C builddir_em

# created:
# rectray_glfw_imgui.wasm
# rectray_glfw_imgui.js
# rectray_glfw_imgui.html

> cd builddir_em/examples/glfw_imgui
> python3 -m http.server 8000
# open webbrowser: `rectray_glfw_imgui.html`
```

