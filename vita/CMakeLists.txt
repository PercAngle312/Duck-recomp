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
add_library(duck_vita_event_bridge STATIC src/event_bridge.c)
target_include_directories(duck_vita_event_bridge PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_event_bridge PUBLIC duck_vita_scheduler duck_vita_cpu_irq)
add_library(duck_vita_dispatch STATIC src/dispatch.c)
target_include_directories(duck_vita_dispatch PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_dispatch PUBLIC duck_vita_recomp_bridge)
# Opt-in private game function map. Generated output stays in build/ and
# contains proprietary-derived entrypoints; no map is shipped in this repo.
set(DUCK_VITA_FUNCTION_MAP "" CACHE FILEPATH "Private verified vram,symbol CSV")
if(DUCK_VITA_FUNCTION_MAP)
  if(NOT EXISTS "${DUCK_VITA_FUNCTION_MAP}")
    message(FATAL_ERROR "Private function map does not exist")
  endif()
  set(duck_private_registry "${CMAKE_BINARY_DIR}/private/duck_game_dispatch.c")
  add_custom_command(OUTPUT "${duck_private_registry}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/generate_dispatch.py"
      --map "${DUCK_VITA_FUNCTION_MAP}" --output "${duck_private_registry}"
      --source-root "${generated}"
    DEPENDS "${DUCK_VITA_FUNCTION_MAP}" "${CMAKE_CURRENT_SOURCE_DIR}/generate_dispatch.py" ${module_c} ${resident_c}
    VERBATIM)
  add_library(duck_vita_game_registry STATIC "${duck_private_registry}")
  target_include_directories(duck_vita_game_registry PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_BINARY_DIR}/include")
  # The registry references the generated functions. This archive intentionally
  # does not constitute a linked or runnable game.
  target_link_libraries(duck_vita_game_registry PUBLIC duck_vita_dispatch
    duck_vita_modules duck_vita_resident)
endif()
add_library(duck_vita_recomp_bridge STATIC src/recomp_bridge.c)
target_include_directories(duck_vita_recomp_bridge PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_recomp_bridge PUBLIC duck_vita_scheduler)
add_library(duck_vita_executor STATIC src/executor.c)
target_include_directories(duck_vita_executor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_executor PUBLIC duck_vita_context duck_vita_scheduler)
add_library(duck_vita_context STATIC src/context.c)
target_include_directories(duck_vita_context PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_context PUBLIC duck_vita_scheduler)
add_library(duck_vita_scheduler STATIC src/scheduler.c)
target_include_directories(duck_vita_scheduler PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_scheduler PUBLIC duck_vita_queue)
add_library(duck_vita_queue STATIC src/queue.c)
target_include_directories(duck_vita_queue PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(duck_vita_cpu_irq STATIC src/cpu_irq.c)
target_include_directories(duck_vita_cpu_irq PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(duck_vita_mi STATIC src/mi.c)
target_include_directories(duck_vita_mi PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_mi PUBLIC duck_vita_bus)
# SP task callbacks and guest SP/DP events are kept in a reusable runtime
# archive; the snapshot replay does not initialize registered guest OS events.
add_library(duck_vita_sp_runtime STATIC src/sp.c src/os_sp.c src/sp_events.c src/sp_runtime.c src/game_loop.c)
target_include_directories(duck_vita_sp_runtime PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_sp_runtime PUBLIC duck_vita_dispatch duck_vita_recomp_bridge duck_vita_scheduler duck_vita_os_io)
add_library(duck_vita_os_io STATIC src/os_io.c src/os_core.c src/os_queue.c)
target_include_directories(duck_vita_os_io PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
# Fiber runtime installs queue wait/wake hooks; standalone queue use remains nonblocking.
add_library(duck_vita_os_time STATIC src/os_time.c src/os_vi.c src/os_vi_config.c src/clock_vita.c)
target_include_directories(duck_vita_os_time PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_os_time PUBLIC duck_vita_os_io m)
add_library(duck_vita_os_threads STATIC src/os_threads.c src/fiber_vita.c src/os_interrupt.c)
target_include_directories(duck_vita_os_threads PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_os_threads PUBLIC duck_vita_os_io SceFiber_stub SceSysmodule_stub)
# Runtime dispatch catalog is generated privately by generate_overlay_catalog.py.
# Linking these archives alone does not execute graphics or produce a game VPK.
add_library(duck_vita_overlays STATIC src/os_overlays.c)
target_include_directories(duck_vita_overlays PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(duck_vita_os_input STATIC src/os_input.c src/input_vita.c)
target_include_directories(duck_vita_os_input PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_os_input PUBLIC duck_vita_os_io SceCtrl_stub)
add_library(duck_vita_pi STATIC src/pi.c)
target_include_directories(duck_vita_pi PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(duck_vita_pi PUBLIC duck_vita_bus duck_vita_dma)
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
set(duck_original_vpk_flags "${VITA_PACK_VPK_FLAGS}")
vita_create_vpk(DuckDodgers-Vita-Bringup.vpk DUCKVIT01 duck_vita_probe.self
  VERSION "00.01" NAME "Duck Recomp Vita Lab"
  FILE "${VITA_SAMPLES_ROOT}/hello_world/sce_sys/icon0.png" sce_sys/icon0.png
  FILE "${VITA_SAMPLES_ROOT}/hello_world/sce_sys/livearea/contents/bg.png" sce_sys/livearea/contents/bg.png
  FILE "${VITA_SAMPLES_ROOT}/hello_world/sce_sys/livearea/contents/startup.png" sce_sys/livearea/contents/startup.png
  FILE "${VITA_SAMPLES_ROOT}/hello_world/sce_sys/livearea/contents/template.xml" sce_sys/livearea/contents/template.xml)

# Separately labeled hardware test. Never presented as the playable game.
add_executable(duck_vita_thread_probe src/thread_probe.c tests/os_threads.c
  "${VITA_SAMPLES_ROOT}/common/debugScreen.c")
target_include_directories(duck_vita_thread_probe PRIVATE "${VITA_SAMPLES_ROOT}/common")
set_source_files_properties(tests/os_threads.c PROPERTIES COMPILE_DEFINITIONS "main=duck_threads_test_main")
# Keep assertions active: Release flags otherwise erase the actual test calls.
target_compile_options(duck_vita_thread_probe PRIVATE -UNDEBUG)
target_link_libraries(duck_vita_thread_probe PRIVATE duck_vita_os_threads SceDisplay_stub SceCtrl_stub m)
vita_create_self(duck_vita_thread_probe.self duck_vita_thread_probe UNSAFE)
# VitaSDK uses a macro that mutates this variable; isolate the second package.
set(VITA_PACK_VPK_FLAGS "${duck_original_vpk_flags}")
vita_create_vpk(DuckDodgers-Vita-Thread-Test.vpk DUCKVIT02 duck_vita_thread_probe.self
  VERSION "00.01" NAME "Duck Vita Thread Test"
  FILE "${VITA_SAMPLES_ROOT}/hello_world/sce_sys/icon0.png" sce_sys/icon0.png
  FILE "${VITA_SAMPLES_ROOT}/hello_world/sce_sys/livearea/contents/bg.png" sce_sys/livearea/contents/bg.png
  FILE "${VITA_SAMPLES_ROOT}/hello_world/sce_sys/livearea/contents/startup.png" sce_sys/livearea/contents/startup.png
  FILE "${VITA_SAMPLES_ROOT}/hello_world/sce_sys/livearea/contents/template.xml" sce_sys/livearea/contents/template.xml)
