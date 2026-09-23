#include <obs-module.h>
#include <obs-frontend-api.h>
#include "ui/hider-dock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-hidder-win", "en-US")

static HiderDockWidget* s_dock = nullptr;

static void OnToggleActiveHotkey(void* data, obs_hotkey_id id, obs_hotkey_t* hotkey, bool pressed) {
    if (pressed && s_dock) {
        s_dock->ToggleActiveWindow();
    }
}

bool obs_module_load(void) {
    blog(LOG_INFO, "[obs-hidder-win] Loading Window Hider plugin...");

    // Create the native Qt6 dock
    QMainWindow* mainWindow = reinterpret_cast<QMainWindow*>(obs_frontend_get_main_window());
    s_dock = new HiderDockWidget(mainWindow);

    // Register dock with OBS frontend
    obs_frontend_add_custom_qdock("obs_hidder_win_dock", s_dock);

    // Register global hotkey
    obs_hotkey_register_frontend(
        "hidder_toggle_active",
        obs_module_text("Toggle Hide Active Window from Capture"),
        OnToggleActiveHotkey,
        nullptr
    );

    blog(LOG_INFO, "[obs-hidder-win] Plugin loaded successfully.");
    return true;
}

void obs_module_unload(void) {
    blog(LOG_INFO, "[obs-hidder-win] Unloading plugin...");
    s_dock = nullptr;
}

MODULE_EXPORT const char* obs_module_name(void) {
    return "Window Hider (Capture Cloaker)";
}

MODULE_EXPORT const char* obs_module_description(void) {
    return "Excludes selected application windows from Display Capture while keeping them visible on your physical screen.";
}
