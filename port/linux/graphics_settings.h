#pragma once
#include <cstdint>
#include <filesystem>
#include <SDL.h>
#include "common/rt64_user_configuration.h"

namespace duck_graphics {
struct Settings {
    int width=1280, height=720;
    int scale=3; // native, fit window, 2x, 3x, 4.5x, 6x, 9x
    int msaa=1; // actual sample count
    int aspect=0; // original, window, 16:9, 21:9
    int filtering=2; // nearest, linear, antialiased pixel scaling
    bool fullscreen=false, vsync=true, three_point=true;
};
void initialize(SDL_Window* window, const std::filesystem::path& data_path);
bool handle_event(const SDL_Event& event);
void draw();
bool is_open();
void shutdown();
Settings initial();
bool take_request(uint64_t& revision, Settings& settings);
void set_capabilities(uint32_t samples, int effective_msaa);
RT64::UserConfiguration renderer_config(const Settings& settings);
}
