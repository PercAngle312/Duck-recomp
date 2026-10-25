#include "graphics_settings.h"
#include "imgui.h"
#include "imgui/imgui_impl_sdl2_custom.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <string>

namespace duck_graphics {
namespace {
std::mutex mutex;
Settings requested, edited;
uint64_t revision=1;
uint32_t supported_msaa=1;
bool capabilities_known=false;
std::atomic<bool> opened=false;
std::atomic<int> pending_effective_msaa{-1};
SDL_Window *game_window=nullptr, *panel=nullptr;
SDL_Renderer* renderer=nullptr;
ImGuiContext* context=nullptr;
bool platform_initialized=false,renderer_initialized=false;
std::filesystem::path path;
std::string status="F5 opens graphics settings. Apply saves changes.";
bool startup_vsync=true;
Uint64 last_draw=0;
const double scales[]={1.0,0.0,2.0,3.0,4.5,6.0,9.0};
const char* scale_names[]={"Native","Fit window","2x (~480p)","3x (~720p)","4.5x (~1080p)","6x (~1440p)","9x (~2160p)"};

Settings sanitize(Settings s) {
    if(s.width<640 || s.width>7680)s.width=1280;
    if(s.height<480 || s.height>4320)s.height=720;
    if(s.scale<0 || s.scale>6)s.scale=3;
    if(s.aspect<0 || s.aspect>3)s.aspect=0;
    if(s.filtering<0 || s.filtering>2)s.filtering=2;
    if(s.msaa!=1 && s.msaa!=2 && s.msaa!=4 && s.msaa!=8)s.msaa=1;
    return s;
}
json serialize(const Settings& s) {
    return {{"version",1},{"window_width",s.width},{"window_height",s.height},
        {"render_scale",s.scale},{"msaa_samples",s.msaa},{"aspect_ratio",s.aspect},
        {"output_filter",s.filtering},{"fullscreen",s.fullscreen},
        {"vsync",s.vsync},{"three_point_filtering",s.three_point}};
}
bool save(const Settings& s) {
    // Write a temporary file, then replace only after the stream closed cleanly.
    auto tmp=path;tmp+=".tmp";
    std::ofstream f(tmp);f<<serialize(s).dump(2)<<'\n';f.close();
    if(!f) {status="Could not write graphics settings. Previous settings are intact.";return false;}
    std::error_code error;std::filesystem::rename(tmp,path,error);
    if(error) {status="Could not replace graphics settings: "+error.message();return false;}
    return true;
}
void apply() {
    Settings next=sanitize(edited);
    {
        std::lock_guard lock(mutex);
        if(capabilities_known && !(supported_msaa & next.msaa)) {
            for(int candidate:{8,4,2,1})if(candidate<=next.msaa && (supported_msaa & candidate)) {next.msaa=candidate;break;}
        }
    }
    if(SDL_SetWindowFullscreen(game_window,next.fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0)!=0) {
        status=std::string("Display change failed: ")+SDL_GetError();return;
    }
    if(!next.fullscreen)SDL_SetWindowSize(game_window,next.width,next.height);
    if(opened && panel)SDL_RaiseWindow(panel);
    {
        std::lock_guard lock(mutex);requested=next;revision++;
    }
    edited=next;
    if(save(next))status=next.vsync!=startup_vsync?"Saved. Restart the game to apply the V-sync change.":"Saved and applied. F5 or Escape returns to the game.";
    fprintf(stderr,"[settings] Requested %dx%d scale=%g MSAA=%dx aspect=%d fullscreen=%d vsync=%d\n",
        next.width,next.height,scales[next.scale],next.msaa,next.aspect,next.fullscreen,next.vsync);
}
void close() {
    opened=false;
    if(panel)SDL_HideWindow(panel);
    SDL_RaiseWindow(game_window);
}
void show() {
    if(!panel) {
        panel=SDL_CreateWindow("Duck Dodgers - Graphics",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,800,690,SDL_WINDOW_ALLOW_HIGHDPI);
        if(!panel) {fprintf(stderr,"[settings] Window creation failed: %s\n",SDL_GetError());return;}
        // A software UI avoids creating another GPU context beside RT64 Vulkan.
        renderer=SDL_CreateRenderer(panel,-1,SDL_RENDERER_SOFTWARE);
        if(!renderer) {SDL_DestroyWindow(panel);panel=nullptr;return;}
        IMGUI_CHECKVERSION();context=ImGui::CreateContext();
        auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
        io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
        io.FontGlobalScale=1.3f;
        ImGui::StyleColorsDark();auto& style=ImGui::GetStyle();style.ScaleAllSizes(1.15f);
        style.WindowRounding=0;style.FrameRounding=5;style.GrabRounding=5;
        style.Colors[ImGuiCol_WindowBg]=ImVec4(.055f,.065f,.09f,1);
        style.Colors[ImGuiCol_Button]=ImVec4(.16f,.25f,.35f,1);
        style.Colors[ImGuiCol_Header]=ImVec4(.16f,.28f,.4f,1);
        platform_initialized=ImGui_ImplSDL2_InitForSDLRenderer(panel,renderer);
        if(platform_initialized)renderer_initialized=ImGui_ImplSDLRenderer2_Init(renderer);
        if(!platform_initialized || !renderer_initialized) {
            fprintf(stderr,"[settings] UI initialization failed\n");shutdown();return;
        }
    }
    {std::lock_guard lock(mutex);edited=requested;}
    opened=true;SDL_ShowWindow(panel);SDL_RaiseWindow(panel);
}
}

void initialize(SDL_Window* window,const std::filesystem::path& data_path) {
    game_window=window;path=data_path/"graphics.json";
    Settings value;
    if(std::getenv("DUCK_GRAPHICS_SAFE")) {
        value.width=960;value.height=720;value.scale=0;value.msaa=1;
        status="Safe mode: native rendering. Apply to save a new configuration.";
    } else if(std::ifstream file(path);file) {
        try {
            json j;file>>j;
            value.width=j.value("window_width",value.width);value.height=j.value("window_height",value.height);
            value.scale=j.value("render_scale",value.scale);value.msaa=j.value("msaa_samples",value.msaa);
            value.aspect=j.value("aspect_ratio",value.aspect);value.filtering=j.value("output_filter",value.filtering);
            value.fullscreen=j.value("fullscreen",value.fullscreen);value.vsync=j.value("vsync",value.vsync);
            value.three_point=j.value("three_point_filtering",value.three_point);
        } catch(const std::exception& error) {
            fprintf(stderr,"[settings] Invalid graphics.json, using defaults: %s\n",error.what());
            value=Settings{};status="Could not read graphics.json. Defaults loaded; Apply replaces it.";
        }
    }
    value=sanitize(value);
    {std::lock_guard lock(mutex);requested=edited=value;}
    startup_vsync=value.vsync;
    SDL_SetWindowSize(game_window,value.width,value.height);
    if(value.fullscreen && SDL_SetWindowFullscreen(game_window,SDL_WINDOW_FULLSCREEN_DESKTOP)!=0) {
        fprintf(stderr,"[settings] Fullscreen failed: %s\n",SDL_GetError());
        std::lock_guard lock(mutex);requested.fullscreen=edited.fullscreen=false;
    }
    fprintf(stderr,"[settings] Loaded %s: %dx%d scale=%g MSAA=%dx\n",path.c_str(),value.width,value.height,scales[value.scale],value.msaa);
    if(std::getenv("DUCK_SHOW_GRAPHICS"))show();
}
bool handle_event(const SDL_Event& event) {
    if(event.type==SDL_KEYDOWN && !event.key.repeat) {
        if(event.key.keysym.scancode==SDL_SCANCODE_F5) {if(opened)close();else show();return true;}
        if(event.key.keysym.scancode==SDL_SCANCODE_F11) {
            {std::lock_guard lock(mutex);edited=requested;}
            edited.fullscreen=!edited.fullscreen;apply();return true;
        }
        if(opened && event.key.keysym.scancode==SDL_SCANCODE_ESCAPE) {close();return true;}
    }
    if(panel && event.type==SDL_WINDOWEVENT && event.window.windowID==SDL_GetWindowID(panel) && event.window.event==SDL_WINDOWEVENT_CLOSE) {close();return true;}
    if(opened) {
        ImGui::SetCurrentContext(context);ImGui_ImplSDL2_ProcessEvent(&event);
        if(event.type==SDL_KEYDOWN && event.key.keysym.scancode==SDL_SCANCODE_RETURN && (event.key.keysym.mod & KMOD_CTRL)) {apply();return true;}
    }
    return false;
}
void draw() {
    if(!opened || SDL_GetTicks64()-last_draw<16)return;
    if(int effective=pending_effective_msaa.exchange(-1);effective>=0) {
        edited.msaa=effective;status="Requested MSAA is unavailable on this GPU; using a supported setting.";
    }
    last_draw=SDL_GetTicks64();ImGui::SetCurrentContext(context);
    ImGui_ImplSDLRenderer2_NewFrame();ImGui_ImplSDL2_NewFrame();ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0,0));ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Graphics",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextColored(ImVec4(1,.77f,.25f,1),"DUCK DODGERS");ImGui::SameLine();ImGui::TextUnformatted(" / Graphics settings");
    ImGui::TextDisabled("F5 / Escape: close     F11: fullscreen     Ctrl+Enter: apply");
    ImGui::Separator();
    if(ImGui::Button("Native")) {edited=Settings{};edited.width=960;edited.height=720;edited.scale=0;}
    ImGui::SameLine();if(ImGui::Button("Balanced"))edited=Settings{};
    ImGui::SameLine();if(ImGui::Button("High")) {edited=Settings{};edited.width=1920;edited.height=1080;edited.scale=4;edited.msaa=4;}
    ImGui::SameLine();if(ImGui::Button("Ultra")) {edited=Settings{};edited.width=3840;edited.height=2160;edited.scale=6;edited.msaa=8;}
    ImGui::Spacing();
    ImGui::SetNextItemWidth(340);
    const std::string window_label=std::to_string(edited.width)+" x "+std::to_string(edited.height);
    if(ImGui::BeginCombo("Window resolution",window_label.c_str())) {
        const int sizes[][2]={{960,720},{1280,720},{1920,1080},{2560,1440},{3840,2160},{2560,1080},{3440,1440}};
        for(const auto& size:sizes) {
            std::string label=std::to_string(size[0])+" x "+std::to_string(size[1]);
            if(ImGui::Selectable(label.c_str(),edited.width==size[0] && edited.height==size[1])) {edited.width=size[0];edited.height=size[1];}
        }ImGui::EndCombo();
    }
    ImGui::Checkbox("Borderless fullscreen (uses desktop resolution)",&edited.fullscreen);
    ImGui::SetNextItemWidth(340);ImGui::Combo("Internal rendering",&edited.scale,scale_names,7);
    ImGui::TextDisabled("Internal scaling improves 3D detail. Window size controls output.");
    ImGui::SetNextItemWidth(340);
    std::string aa=edited.msaa==1?"Off":std::to_string(edited.msaa)+"x MSAA";
    if(ImGui::BeginCombo("Anti-aliasing",aa.c_str())) {
        uint32_t mask;bool known;{std::lock_guard lock(mutex);mask=supported_msaa;known=capabilities_known;}
        for(int samples:{1,2,4,8}) {
            bool supported=!known || (mask & samples);
            std::string label=samples==1?"Off":std::to_string(samples)+"x MSAA";
            if(!supported)label+=" (unavailable on this GPU)";
            ImGui::BeginDisabled(!supported);
            if(ImGui::Selectable(label.c_str(),edited.msaa==samples))edited.msaa=samples;
            ImGui::EndDisabled();
        }ImGui::EndCombo();
    }
    ImGui::SetNextItemWidth(340);
    const char* aspect_names[]={"Original 4:3","Fit window (widescreen)","16:9 widescreen","21:9 ultrawide"};
    ImGui::Combo("Aspect ratio",&edited.aspect,aspect_names,4);
    ImGui::TextDisabled("Widescreen is experimental: check HUD placement and scene edges.");
    ImGui::SetNextItemWidth(340);ImGui::Combo("Output filter",&edited.filtering,"Nearest\0Linear\0Antialiased pixel scaling\0");
    ImGui::Checkbox("Original N64 three-point texture filtering",&edited.three_point);
    ImGui::Checkbox("V-sync (change takes effect after restart)",&edited.vsync);
    ImGui::Spacing();ImGui::Separator();
    if(ImGui::Button("Apply & save",ImVec2(180,38)))apply();
    ImGui::SameLine();if(ImGui::Button("Close",ImVec2(100,38)))close();
    ImGui::TextWrapped("%s",status.c_str());
    ImGui::TextDisabled("Settings are stored in userdata/graphics.json. Game speed is unchanged.");
    ImGui::TextDisabled("The game continues while this window is open; pause in-game first if needed.");
    ImGui::End();ImGui::Render();
    SDL_SetRenderDrawColor(renderer,14,17,23,255);SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(),renderer);SDL_RenderPresent(renderer);
}
bool is_open() {return opened.load();}
Settings initial() {std::lock_guard lock(mutex);return requested;}
bool take_request(uint64_t& seen,Settings& value) {
    std::lock_guard lock(mutex);if(seen==revision)return false;seen=revision;value=requested;return true;
}
void set_capabilities(uint32_t samples,int effective_msaa) {
    std::lock_guard lock(mutex);supported_msaa=samples;capabilities_known=true;
    if(!(samples & requested.msaa)) {requested.msaa=effective_msaa;pending_effective_msaa=effective_msaa;}
}
RT64::UserConfiguration renderer_config(const Settings& settings) {
    auto s=sanitize(settings);RT64::UserConfiguration c;
    c.graphicsAPI=RT64::UserConfiguration::GraphicsAPI::Vulkan;
    c.resolution=s.scale==0?RT64::UserConfiguration::Resolution::Original:s.scale==1?RT64::UserConfiguration::Resolution::WindowIntegerScale:RT64::UserConfiguration::Resolution::Manual;
    c.resolutionMultiplier=scales[s.scale];
    c.antialiasing=s.msaa==2?RT64::UserConfiguration::Antialiasing::MSAA2X:s.msaa==4?RT64::UserConfiguration::Antialiasing::MSAA4X:s.msaa==8?RT64::UserConfiguration::Antialiasing::MSAA8X:RT64::UserConfiguration::Antialiasing::None;
    c.aspectRatio=s.aspect==0?RT64::UserConfiguration::AspectRatio::Original:s.aspect==1?RT64::UserConfiguration::AspectRatio::Expand:RT64::UserConfiguration::AspectRatio::Manual;
    c.aspectTarget=s.aspect==3?21.0/9.0:16.0/9.0;
    c.filtering=static_cast<RT64::UserConfiguration::Filtering>(s.filtering);
    c.threePointFiltering=s.three_point;c.developerMode=false;
    // Keep original game timing; display refresh is not an FPS unlock.
    c.refreshRate=RT64::UserConfiguration::RefreshRate::Original;
    c.validate();return c;
}
void shutdown() {
    opened=false;
    if(context) {
        ImGui::SetCurrentContext(context);
        if(renderer_initialized)ImGui_ImplSDLRenderer2_Shutdown();
        if(platform_initialized)ImGui_ImplSDL2_Shutdown();
        renderer_initialized=platform_initialized=false;
        ImGui::DestroyContext(context);context=nullptr;
    }
    if(renderer) {SDL_DestroyRenderer(renderer);renderer=nullptr;}
    if(panel) {SDL_DestroyWindow(panel);panel=nullptr;}
}
}
