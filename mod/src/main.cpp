// DualSenseRDR2.asi - loaded by the ASI loader, runs a ScriptHookRDR2 script for game state.
#include "config.h"
#include "io_probe.h"
#include "common.h"
#include "core.h"
#include "log.h"
#include "shv.h"
#include "text_inject.h"

#include <atomic>

static HMODULE g_module = nullptr;

namespace dsr2 {
fs::path module_dir() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(g_module, path, MAX_PATH);
    return fs::path(path).parent_path();
}
}  // namespace dsr2

// Controller, audio and the XInput hook start right away (menus work too);
// game state comes from the ScriptHook script once a save is loaded.
static DWORD WINAPI boot_thread(LPVOID) {
    try {
        dsr2::core().init(true);
        if (dsr2::config().flag("General", "Probes", false)) dsr2::io_probe_start();   // research: which audio files the game reads
    } catch (const std::exception& e) {
        LOGE("init failed: %s", e.what());
    } catch (...) {
        LOGE("init failed: unknown exception");
    }
    return 0;
}

static void script_main() {
    while (true) {
        try {
            dsr2::core().script_tick();
        } catch (const std::exception& e) {
            LOGE("script tick: %s", e.what());
        }
        shv_scriptWait(0);
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        DisableThreadLibraryCalls(module);
        dsr2::log::open((dsr2::module_dir() / "DualSenseRDR2.log").wstring());
        LOGI("DualSense for RDR2 %s - loading", dsr2::kVersion);
        if (HANDLE t = CreateThread(nullptr, 0, boot_thread, nullptr, 0, nullptr)) CloseHandle(t);
        else LOGE("boot thread creation failed (%lu)", GetLastError());
        dsr2::text_inject_start();       // our Settings texts, in the game's own string table (even if init fails)
        shv_scriptRegister(module, script_main);
    } else if (reason == DLL_PROCESS_DETACH) {
        // Nothing is stopped here: joining our threads under the loader lock would hang (a thread can't finish
        // exiting without that lock). On process exit Windows ends them anyway.
        shv_scriptUnregister(module);
    }
    return TRUE;
}
