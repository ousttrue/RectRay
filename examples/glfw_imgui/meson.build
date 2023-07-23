# build for emscripten: meson setup builddir_em --cross-file wasm.ini
imgui_dep = dependency('imgui')

deps = [imgui_dep]
args = []
link_args = []

compiler = meson.get_compiler('cpp')
if compiler.get_id() == 'emscripten'
    link_args += ['-sUSE_GLFW=3', '-sUSE_WEBGL2=1']
    suffix = 'html'
else
    glfw_dep = dependency('glfw3', default_options: ['install=true'])
    glew_dep = dependency('glew')
    deps += [glfw_dep, glew_dep]
    suffix = []
endif

executable(
    'rectray_glfw_imgui',
    [
        'main.cpp',
        'platform.cpp',
        'scene.cpp',
        'triangle.cpp',
    ],
    install: true,
    name_suffix: suffix,
    cpp_args: args,
    link_args: link_args,
    dependencies: deps,
)
