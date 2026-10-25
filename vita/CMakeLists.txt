cmake_minimum_required(VERSION 3.20)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "Vita toolchain")
endif()
project(DuckVitaBringup C CXX)
include("${VITASDK}/share/vita.cmake" REQUIRED)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(DUCK_GAME_SOURCE "" CACHE PATH "Private generated game project")
set(N64RECOMP_ROOT "" CACHE PATH "Pinned N64Recomp")
set(N64MODERN_ROOT "" CACHE PATH "Pinned N64ModernRuntime")
set(VITA_SAMPLES_ROOT "" CACHE PATH "VitaSDK samples for debug screen")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_headers.py"
  --recompiler "${N64RECOMP_ROOT}" --runtime "${N64MODERN_ROOT}"
  --output "${CMAKE_BINARY_DIR}/include" COMMAND_ERROR_IS_FATAL ANY)
add_compile_options(-mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard -ffunction-sections -fdata-sections)
include_directories("${CMAKE_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${N64MODERN_ROOT}/ultramodern/include" "${N64MODERN_ROOT}/thirdparty")
set(generated "${DUCK_GAME_SOURCE}/build")
file(GLOB module_c CONFIGURE_DEPENDS "${generated}/modules/funcs_*.c")
file(GLOB resident_c CONFIGURE_DEPENDS "${generated}/runtime/funcs_*.c")
if(NOT module_c OR NOT resident_c)
  message(FATAL_ERROR "Complete private generated game sources are required")
endif()
add_library(duck_vita_modules STATIC ${module_c})
target_include_directories(duck_vita_modules PRIVATE "${generated}/modules")
target_compile_options(duck_vita_modules PRIVATE -O1 -include "${generated}/modules/resident_refs.h")
add_library(duck_vita_resident STATIC ${resident_c})
target_include_directories(duck_vita_resident PRIVATE "${generated}/runtime" "${DUCK_GAME_SOURCE}/runtime")
target_compile_options(duck_vita_resident PRIVATE -O1)
# Native display presenter, intentionally separate from translated graphics tasks.
# Building this archive verifies VitaSDK header/API compatibility, not rendering.
add_library(duck_vita_display STATIC src/display.c src/framebuffer_convert.c)
add_library(duck_vita_guest_memory STATIC src/guest_memory.c)
add_library(duck_vita_dma STATIC src/dma.c)
target_include_directories(duck_vita_dma PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_dma PUBLIC duck_vita_guest_memory)
add_library(duck_vita_bus STATIC src/bus.c)
target_include_directories(duck_vita_bus PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_bus PUBLIC duck_vita_guest_memory)
target_include_directories(duck_vita_guest_memory PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(duck_vita_display PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(duck_vita_audio_rsp STATIC "${generated}/runtime/audio_rsp.cpp")
target_compile_options(duck_vita_audio_rsp PRIVATE -O1)

add_executable(duck_vita_probe src/main.c src/adapter_probe.c
  "${VITA_SAMPLES_ROOT}/common/debugScreen.c"
  "${DUCK_GAME_SOURCE}/tests/boot_smoke.c" "${generated}/boot/funcs_0.c"
  "${DUCK_GAME_SOURCE}/tests/module_execution.c" "${generated}/modules/funcs_0.c")
target_include_directories(duck_vita_probe PRIVATE "${VITA_SAMPLES_ROOT}/common" "${generated}/modules")
set_source_files_properties("${DUCK_GAME_SOURCE}/tests/boot_smoke.c" PROPERTIES COMPILE_DEFINITIONS "main=duck_boot_smoke_main")
set_source_files_properties("${DUCK_GAME_SOURCE}/tests/boot_smoke.c" PROPERTIES COMPILE_OPTIONS "-include;${generated}/boot/funcs.h")
set_source_files_properties("${DUCK_GAME_SOURCE}/tests/module_execution.c" PROPERTIES COMPILE_DEFINITIONS "main=duck_module_execution_main")
set_source_files_properties("${generated}/modules/funcs_0.c" PROPERTIES COMPILE_OPTIONS "-O1;-include;${generated}/modules/resident_refs.h")
target_link_options(duck_vita_probe PRIVATE -Wl,--gc-sections)
target_link_libraries(duck_vita_probe PRIVATE SceDisplay_stub SceCtrl_stub m)
vita_create_self(duck_vita_probe.self duck_vita_probe UNSAFE)
vita_create_vpk(DuckDodgers-Vita-Bringup.vpk DUCKVIT01 duck_vita_probe.self
  VERSION "00.01" NAME "Duck Recomp Vita Lab"
  FILE "${VITA_SAMPLES_ROOT}/hello_world/sce_sys/icon0.png" sce_sys/icon0.png
  FILE "${VITA_SAMPLES_ROOT}/hello_world/sce_sys/livearea/contents/bg.png" sce_sys/livearea/contents/bg.png
  FILE "${VITA_SAMPLES_ROOT}/hello_world/sce_sys/livearea/contents/startup.png" sce_sys/livearea/contents/startup.png
  FILE "${VITA_SAMPLES_ROOT}/hello_world/sce_sys/livearea/contents/template.xml" sce_sys/livearea/contents/template.xml)
