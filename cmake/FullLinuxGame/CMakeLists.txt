cmake_minimum_required(VERSION 3.20)
project(DuckDodgersLinux LANGUAGES C CXX)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "This recipe builds the Linux game; Windows remains a later target")
endif()
set(DUCK_GAME_SOURCE "" CACHE PATH "Private full game project, including runtime and generated build sources")
set(N64RECOMP_ROOT "" CACHE PATH "Pinned recompiler source")
set(N64MODERN_ROOT "" CACHE PATH "Pinned modern runtime source")
set(DUCK_RT64_ROOT "" CACHE PATH "Pinned RT64 source with submodules")
set(DUCK_RT64_BINARY_DIR "${CMAKE_BINARY_DIR}/rt64" CACHE PATH "Renderer build directory")
foreach(input
  "${DUCK_GAME_SOURCE}/runtime/boot_probe.cpp"
  "${DUCK_GAME_SOURCE}/build/runtime/audio_rsp.cpp"
  "${DUCK_GAME_SOURCE}/build/runtime/bindings.cpp"
  "${DUCK_GAME_SOURCE}/build/modules/funcs_0.c"
  "${DUCK_GAME_SOURCE}/build/runtime/funcs_0.c"
  "${N64RECOMP_ROOT}/include/recomp.h"
  "${N64MODERN_ROOT}/CMakeLists.txt"
  "${DUCK_RT64_ROOT}/CMakeLists.txt")
  if(NOT EXISTS "${input}")
    message(FATAL_ERROR "Full native Linux game input missing: ${input}")
  endif()
endforeach()

# Use the renderer's CMake targets. Their link interfaces carry SDL2, DBus,
# Plume, zstd and re-spirv; no hardcoded archive or distro library paths.
set(RT64_STATIC ON CACHE BOOL "" FORCE)
set(RT64_SDL_WINDOW_VULKAN ON CACHE BOOL "" FORCE)
set(NFD_PORTAL ON CACHE BOOL "" FORCE)
find_package(SDL2 REQUIRED)
find_package(X11 REQUIRED)
add_subdirectory("${DUCK_RT64_ROOT}" "${DUCK_RT64_BINARY_DIR}" EXCLUDE_FROM_ALL)
add_subdirectory("${N64MODERN_ROOT}" "${CMAKE_BINARY_DIR}/modern-runtime" EXCLUDE_FROM_ALL)

set(generated "${DUCK_GAME_SOURCE}/build")
file(GLOB module_c CONFIGURE_DEPENDS "${generated}/modules/funcs_*.c")
file(GLOB resident_c CONFIGURE_DEPENDS "${generated}/runtime/funcs_*.c")
add_library(duck_modules STATIC ${module_c})
target_include_directories(duck_modules PRIVATE "${N64RECOMP_ROOT}/include" "${generated}/modules")
target_compile_options(duck_modules PRIVATE -O1 -Werror=implicit-function-declaration
  -include "${generated}/modules/resident_refs.h")
add_library(duck_runtime_resident STATIC ${resident_c})
target_include_directories(duck_runtime_resident PRIVATE "${N64RECOMP_ROOT}/include"
  "${generated}/runtime" "${DUCK_GAME_SOURCE}/runtime")
target_compile_options(duck_runtime_resident PRIVATE -O1 -Werror=implicit-function-declaration)

add_executable(duckdodgers "${generated}/runtime/audio_rsp.cpp"
  "${generated}/runtime/bindings.cpp"
  "${DUCK_GAME_SOURCE}/runtime/boot_probe.cpp"
  "${DUCK_GAME_SOURCE}/runtime/pc_backend.cpp"
  "${DUCK_GAME_SOURCE}/runtime/dialog_lifecycle.cpp"
  "${DUCK_GAME_SOURCE}/runtime/sections.cpp"
  "${DUCK_GAME_SOURCE}/runtime/loader_bridge.cpp"
  "${DUCK_GAME_SOURCE}/runtime/pi.cpp"
  "${DUCK_GAME_SOURCE}/runtime/memory.cpp")
target_compile_definitions(duckdodgers PRIVATE DUCK_WITH_RT64 RT64_SDL_WINDOW_VULKAN
  PLUME_SDL_VULKAN_ENABLED HLSL_CPU FFX_GCC DUCK_DEFER_OVERLAY_UNLOAD)
target_include_directories(duckdodgers PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../port/linux")
file(READ "${DUCK_GAME_SOURCE}/runtime/loader_bridge.cpp" loader_source)
if(NOT loader_source MATCHES "retired_code.before_load")
  message(FATAL_ERROR "Apply patches/linux-overlay-lifetime.patch to the private loader before building this release")
endif()
option(DUCK_ENABLE_GRAPHICS_SETTINGS "Build the native graphics settings window" ON)
if(DUCK_ENABLE_GRAPHICS_SETTINGS)
  file(READ "${DUCK_GAME_SOURCE}/runtime/pc_backend.cpp" backend_source)
  if(NOT backend_source MATCHES "duck_graphics::initialize")
    message(FATAL_ERROR "Apply patches/linux-graphics-settings.patch to the private project, or set DUCK_ENABLE_GRAPHICS_SETTINGS=OFF to build the older reference host")
  endif()
  target_compile_definitions(duckdodgers PRIVATE DUCK_GRAPHICS_SETTINGS)
  target_sources(duckdodgers PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../port/linux/graphics_settings.cpp"
    "${DUCK_RT64_ROOT}/src/contrib/imgui/backends/imgui_impl_sdlrenderer2.cpp")
  target_include_directories(duckdodgers PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../port/linux"
    "${DUCK_RT64_ROOT}/src/contrib/imgui")
endif()
target_include_directories(duckdodgers PRIVATE "${N64RECOMP_ROOT}/include"
  "${generated}/runtime" "${DUCK_GAME_SOURCE}/runtime"
  "${N64MODERN_ROOT}/librecomp/include" "${N64MODERN_ROOT}/ultramodern/include"
  "${N64MODERN_ROOT}/thirdparty" "${DUCK_RT64_ROOT}/src"
  "${DUCK_RT64_ROOT}/src/contrib" "${DUCK_RT64_ROOT}/src/contrib/plume"
  "${DUCK_RT64_ROOT}/src/contrib/hlslpp/include"
  "${DUCK_RT64_ROOT}/src/contrib/hlslpp"
  "${DUCK_RT64_ROOT}/src/contrib/hlslpp/include/hlsl++"
  "${DUCK_RT64_ROOT}/src/contrib/nativefiledialog-extended/src/include")
target_link_libraries(duckdodgers PRIVATE duck_modules duck_runtime_resident librecomp rt64 SDL2::SDL2 X11::X11 pthread dl m)
target_link_options(duckdodgers PRIVATE -rdynamic -Wl,--wrap=NFD_Init -Wl,--wrap=NFD_Quit)
set_source_files_properties("${generated}/runtime/audio_rsp.cpp" PROPERTIES COMPILE_OPTIONS "-msse4.1;-O1")
set_target_properties(duckdodgers PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE INSTALL_RPATH "$ORIGIN/lib")

enable_testing()
add_executable(duck_memory_alias "${DUCK_GAME_SOURCE}/tests/memory_alias.cpp" "${DUCK_GAME_SOURCE}/runtime/memory.cpp")
target_include_directories(duck_memory_alias PRIVATE "${N64MODERN_ROOT}/librecomp/include"
  "${N64MODERN_ROOT}/ultramodern/include" "${N64RECOMP_ROOT}/include")
add_test(NAME rdram_cached_uncached_alias COMMAND duck_memory_alias)
add_executable(duck_overlay_lifetime
  "${CMAKE_CURRENT_LIST_DIR}/../../tests/linux_overlay_lifetime.cpp"
  "${N64MODERN_ROOT}/librecomp/src/overlays.cpp"
  "${N64MODERN_ROOT}/ultramodern/src/error_handling.cpp")
target_include_directories(duck_overlay_lifetime PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/../../port/linux" "${N64RECOMP_ROOT}/include"
  "${N64MODERN_ROOT}/librecomp/include" "${N64MODERN_ROOT}/librecomp/include/librecomp"
  "${N64MODERN_ROOT}/ultramodern/include" "${N64MODERN_ROOT}/thirdparty"
  "${N64MODERN_ROOT}/thirdparty/concurrentqueue")
target_compile_options(duck_overlay_lifetime PRIVATE -ffunction-sections -fdata-sections)
target_link_options(duck_overlay_lifetime PRIVATE -Wl,--gc-sections)
target_link_libraries(duck_overlay_lifetime PRIVATE pthread m)
add_test(NAME callback_dispatch_after_free COMMAND duck_overlay_lifetime)
