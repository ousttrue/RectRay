# build for emscripten: meson setup builddir_em --cross-file wasm.ini
imgui_dep = dependency('imgui')

deps = [rectray_dep, imgui_dep]
args = []
link_args = []

compiler = meson.get_compiler('cpp')
if compiler.get_id() == 'emscripten'
    args += ['-D__EMSCRIPTEN__=1']
    link_args += [
        '-sUSE_GLFW=3',
        '-sUSE_WEBGL2=1',
        # '-sMAX_WEBGL_VERSION=2',
        '-sFULL_ES3=1',
        # '-sLEGACY_GL_EMULATION=1',
    ]
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
        'renderer.cpp',
        'triangle.cpp',
        'plane.cpp',
    ],
    install: true,
    name_suffix: suffix,
    cpp_args: args,
    link_args: link_args,
    dependencies: deps,
)
