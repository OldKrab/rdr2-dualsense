// DualSenseRDR2.ini - every feature can be switched and tuned without rebuilding.
#pragma once

#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <tuple>
#include <vector>

namespace dsr2 {

class Config {
public:
    // Loads the file; writes the documented default file first if it does not exist.
    bool load(const std::wstring& path);

    std::string str(const std::string& section, const std::string& key, const std::string& def) const;
    float num(const std::string& section, const std::string& key, float def) const;
    int integer(const std::string& section, const std::string& key, int def) const;
    bool flag(const std::string& section, const std::string& key, bool def) const;

    static const char* default_text();
    // Writes one or more values into the ini file atomically (keeps comments, order and everything else), then reloads.
    bool set(const std::string& section, const std::string& key, const std::string& value);
    bool set_many(const std::vector<std::tuple<std::string, std::string, std::string>>& updates);

private:
    void migrate();                                // older defaults still untouched -> the current ones
    bool migrating_ = false;
    mutable std::shared_mutex mu_;                 // the settings page writes while other threads read
    std::mutex write_mu_;                          // serializes read-modify-write in set()/set_many()
    std::map<std::string, std::string> values_;
    std::wstring path_;   // "section.key" lowercase -> value
};

Config& config();

}  // namespace dsr2
