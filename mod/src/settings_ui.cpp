// The "DualSense" page in the game's Settings (Settings > Controls > Controller > DualSense) - the LML part
// of the mod adds the page (tools/rpf8/build_dualsense_page.py). Each row is one of the game's own option
// steppers bound to ScriptDatastoreRoot.DualSenseSettings.<row id>; the UI changes "value" itself (two-way
// binding), because no script runs while the pause menu is open. So: keep that data store filled with the ini
// values while the game runs, and when the menu is gone, save whatever the page changed.
// Script thread only (called from World::tick).
#include "settings_ui.h"

#include "config.h"
#include "core.h"
#include "log.h"
#include "natives.h"
#include "settings_menu.h"

#include <string>
#include <vector>

namespace dsr2 {

using namespace nat;

namespace {

struct PageRow {
    const MenuEntry* entry;
    Hash store;                          // its data container
    int shown;                           // the option index we last put in / read from the store
};

struct Page {
    Hash root = 0;
    std::vector<PageRow> rows;
} page;

void create_store() {
    page.root = DATABINDING_ADD_DATA_CONTAINER_FROM_PATH("", "DualSenseSettings");
    page.rows.clear();
    if (!page.root) return;
    for (const MenuEntry& e : SettingsMenu::entries()) {
        if (e.kind == MenuEntry::Header) continue;
        // the data context of the row's stepper: value (two-way), prompt, enabled, visible, and
        //   levels  (our percent stepper = the game's list_item_int_stepper): minimum, maximum, step
        //   others  (list_item_option_stepper): options = text labels, value = index
        Hash h = DATABINDING_ADD_DATA_CONTAINER(page.root, e.id);
        if (!h) continue;
        int v = SettingsMenu::page_value(e, config());
        if (e.kind == MenuEntry::Number) {
            DATABINDING_ADD_DATA_INT(h, "minimum", 0);
            DATABINDING_ADD_DATA_INT(h, "maximum", SettingsMenu::page_max(e));
            DATABINDING_ADD_DATA_INT(h, "step", SettingsMenu::kPercentStep);
        } else {
            Hash options = DATABINDING_ADD_HASH_ARRAY(h, "options");
            for (const std::string& label : SettingsMenu::option_labels(e))
                DATABINDING_INSERT_HASH_TO_ARRAY(options, joaat(label.c_str()), -1);
        }
        DATABINDING_ADD_DATA_INT(h, "value", v);
        DATABINDING_ADD_DATA_STRING(h, "prompt", "");
        DATABINDING_ADD_DATA_BOOL(h, "enabled", true);
        DATABINDING_ADD_DATA_BOOL(h, "visible", true);
        page.rows.push_back({&e, h, v});
    }
    // the "DualSense" row in Settings > Controls shows only while this is 1 (controls.ymt,
    // uiConditionCheckGenericDatastoreInt) - i.e. once the page has its values (not on the title screen)
    DATABINDING_ADD_DATA_INT(page.root, "ready", 1);
    LOGD("settings page: data store ready (0x%08X, %zu rows)", page.root, page.rows.size());
}

}  // namespace

void settings_ui_tick(double now) {
    if (!page.root || !DATABINDING_IS_ENTRY_VALID(page.root)) {
        create_store();
        return;
    }
    // a row the game dropped (while the root stayed) would be skipped for good - the page then shows nothing
    // of the ini. Build the whole store again (at most once a second, in case the game keeps dropping it).
    static double rebuilt_t = -10;
    for (const PageRow& r : page.rows) {
        if (r.store && DATABINDING_IS_ENTRY_VALID(r.store)) continue;
        if (now - rebuilt_t < 1.0) return;
        LOGI("settings page: row %s lost its data - building the page again", r.entry->id);
        rebuilt_t = now;
        DATABINDING_REMOVE_DATA_ENTRY(page.root);
        create_store();
        return;
    }
    std::vector<std::tuple<std::string, std::string, std::string>> updates;
    for (PageRow& r : page.rows) {
        const MenuEntry& e = *r.entry;
        int v = DATABINDING_READ_INT_FROM_PARENT(r.store, "value");
        if (v != r.shown) {                                   // the page changed it while the menu was open
            std::string val = SettingsMenu::page_ini(e, v);
            if (!val.empty()) {
                updates.emplace_back(e.section, e.key, val);
                LOGI("settings page: %s.%s = %s", e.section, e.key, val.c_str());
            }
            r.shown = v;
            continue;
        }
        int want = SettingsMenu::page_value(e, config());  // the ini changed elsewhere (edited, F9)
        if (want != v) {
            DATABINDING_WRITE_DATA_INT_FROM_PARENT(r.store, "value", want);
            r.shown = want;
        }
    }
    if (!updates.empty()) {
        if (config().set_many(updates)) core().request_reload();
    }
}

}  // namespace dsr2
