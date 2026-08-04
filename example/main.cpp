#define SDL_MAIN_HANDLED
#include "../imgui-wheels.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include <SDL.h>
#include <SDL_main.h>
#include <SDL_opengl.h>
#include <stdio.h>

SDL_GLContext gl_context;
void draw_knobs();
SDL_Window *init();
bool begin_frame(SDL_Window *window);
void end_frame(SDL_Window *window);
void teardown(SDL_Window *window);

int main(int, char **) {
    SDL_Window *window = init();
    if (window == nullptr) {
        fprintf(stderr, "Failed to initialize SDL/OpenGL/ImGui.\n");
        return 1;
    }

    if (ImGui::GetCurrentContext() == nullptr) {
        fprintf(stderr, "ImGui context was not created.\n");
        teardown(window);
        return 1;
    }

    ImGuiIO &io = ImGui::GetIO();

    // quick fix for hdpi screens
    io.FontGlobalScale = 2;

    while (begin_frame(window)) {
        {
            ImGui::Begin("Lighting Console Wheels");
            ImGui::TextUnformatted("Roll to shape fixture movement and atmosphere.");

            static float pan = 0.0f;
            if (ImGuiWheels::WheelFloatHorizontal("Pan", &pan, -270.0f, 270.0f, ImVec2(160.0f, 35.0f), "%.1f deg", 1.0f)) {
                // value was changed
            }

            static float tilt = 0.0f;
            if (ImGuiWheels::WheelFloatHorizontal("Tilt", &tilt, -135.0f, 135.0f, ImVec2(160.0f, 35.0f), "%.1f deg", 1.0f)) {
                // value was changed
            }

            if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) {
                tilt = 0.0f;
            }

            static float dimmer = 75.0f;
            if (ImGuiWheels::WheelFloatHorizontal("Dimmer", &dimmer, 0.0f, 100.0f, ImVec2(160.0f, 35.0f), "%.0f %%", 0.8f, true)) {
                // value was changed
            }

            static int gobo_index = 3;
            if (ImGuiWheels::WheelIntHorizontal("Gobo", &gobo_index, 1, 12, ImVec2(160.0f, 35.0f), "%d", 1.0f)) {
                // value was changed
            }

            ImGui::SeparatorText("Vertical roller");
            static float haze = 35.0f;
            if (ImGuiWheels::WheelFloatVertical("Haze", &haze, 0.0f, 100.0f, ImVec2(35.0f, 160.0f), "%.0f %%", 1.0f)) {
                // value was changed
            }

            ImGui::End();
        }

        end_frame(window);
    }

    teardown(window);

    return 0;
}

/******************
 * ImGui handling
 ******************/

SDL_Window *init() {
    // With SDL_MAIN_HANDLED we own the process entry point and must mark it ready.
    SDL_SetMainReady();

    // Setup SDL
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD)) {
        fprintf(stderr, "Error: SDL_Init(): %s\n", SDL_GetError());
        return NULL;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_WindowFlags window_flags =
            (SDL_WindowFlags) (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                       SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_Window *window =
            SDL_CreateWindow("Dear ImGui SDL3+OpenGL wheels example", 1280, 720, window_flags);
    if (window == nullptr) {
        fprintf(stderr, "Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        SDL_Quit();
        return NULL;
    }

    gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr) {
        fprintf(stderr, "Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }

    if (!SDL_GL_MakeCurrent(window, gl_context)) {
        fprintf(stderr, "Error: SDL_GL_MakeCurrent(): %s\n", SDL_GetError());
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }
    SDL_GL_SetSwapInterval(1);// Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGuiContext* imgui_context = ImGui::CreateContext();
    if (imgui_context == nullptr) {
        fprintf(stderr, "Error: ImGui::CreateContext() failed.\n");
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |=
            ImGuiConfigFlags_NavEnableKeyboard;// Enable Keyboard Controls
    io.ConfigFlags |=
            ImGuiConfigFlags_NavEnableGamepad;// Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    if (!ImGui_ImplSDL3_InitForOpenGL(window, gl_context)) {
        fprintf(stderr, "Error: ImGui_ImplSDL3_InitForOpenGL() failed: %s\n", SDL_GetError());
        ImGui::DestroyContext();
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 130")) {
        fprintf(stderr, "Error: ImGui_ImplOpenGL3_Init() failed.\n");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DestroyContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return NULL;
    }

    return window;
}

bool begin_frame(SDL_Window *window) {
    bool done = false;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT)
            done = true;
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
            event.window.windowID == SDL_GetWindowID(window))
            done = true;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    return !done;
}

void end_frame(SDL_Window *window) {
    static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO &io = ImGui::GetIO();
    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    // glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
}

void teardown(SDL_Window *window) {

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}