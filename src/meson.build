sdl_dep = dependency('sdl2', include_type: 'system')
vk_dep = dependency('vulkan', include_type: 'system')

legs_src = files(
  'renderer/buffer.cpp',
  'renderer/descriptor_set.cpp',
  'renderer/device.cpp',
  'renderer/instance.cpp',
  'renderer/renderer.cpp',
  'renderer/vma_usage.cpp',

  'ui/ui.cpp',

  'window/window.cpp',

  'world/world.cpp',

  'engine.cpp',
  'entry.cpp',
  'job_system_thread_pool.cpp',
  'job_system_with_barrier.cpp',
  'physics.cpp',
)

legs_phc = [
  'public/legs/jolt_pch.hpp',
]

legs_shaders = files(
  'public/legs/shaders/fullscreen_frag.frag',
  'public/legs/shaders/fullscreen_vert.vert',
  'public/legs/shaders/lit_pnc_frag.frag',
  'public/legs/shaders/lit_pnc_vert.vert',
  'public/legs/shaders/sky_frag.frag',
  'public/legs/shaders/sky_vert.vert',
  'public/legs/shaders/unlit_pc_frag.frag',
  'public/legs/shaders/unlit_pc_vert.vert',
)

legs_public_includes = include_directories(['public/'])

legs_lib = library(
  'legs',
  legs_src + imgui_src,
  glsl_generator.process(legs_shaders),
  legs_version,
  link_with: [],
  dependencies: [jolt_dep, sdl_dep, vk_dep],
  include_directories: [legs_includes, legs_public_includes],
  cpp_pch: legs_phc,
)

legs_dep = declare_dependency(
  link_with: [legs_lib],
  dependencies: [jolt_dep, sdl_dep, vk_dep],
  include_directories: [legs_includes, legs_public_includes],
)
