/*
 * SPDX-License-Identifier: MIT
 */

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

#include <sys/stat.h>

#include <wupsxx/logger.hpp>

#include "cfg.hpp"
#include "macro_sd.hpp"

namespace logger = wups::logger;

namespace {

    std::string g_last_error;

    void
    set_err(const char *msg)
        noexcept
    {
        g_last_error = (msg != nullptr) ? msg : "";
    }

    bool
    write_text_file(const char *path, const std::string &body)
        noexcept
    {
        FILE *f = fopen(path, "wb");
        if (f == nullptr) {
            set_err("fopen write failed");
            return false;
        }
        if (!body.empty() && fwrite(body.data(), 1, body.size(), f) != body.size()) {
            fclose(f);
            set_err("fwrite failed");
            return false;
        }
        fclose(f);
        return true;
    }

    bool
    read_text_file(const char *path, std::string *out)
        noexcept
    {
        if (out == nullptr) {
            return false;
        }
        FILE *f = fopen(path, "rb");
        if (f == nullptr) {
            return false;
        }
        out->clear();
        char buf[256];
        std::size_t n = 0;
        while ((n = fread(buf, 1, sizeof buf, f)) > 0) {
            out->append(buf, n);
        }
        fclose(f);
        while (!out->empty() && (out->back() == '\n' || out->back() == '\r')) {
            out->pop_back();
        }
        return true;
    }

    void
    trim(std::string &s)
    {
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
            s.erase(0, 1);
        }
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
            s.pop_back();
        }
    }

} // namespace


namespace macro_sd {

    bool
    ensure_directory()
        noexcept
    {
        mkdir("fs:/vol/external01/wiiu", 0777);
        if (mkdir(k_dir, 0777) != 0) {
            /* may already exist */
        }
        return true;
    }


    bool
    export_all()
        noexcept
    {
        set_err("");
        if (!ensure_directory()) {
            return false;
        }

        std::string body;
        body += "# GamePad Macro bank v1\n";
        body += "# Copy sd:/wiiu/gamepad_macro/ to share macros\n";
        body += "version=1\n";
        body += "post_menu_delay_ms=" + std::to_string(cfg::get_post_menu_delay_ms()) + "\n";
        body += "macro_slot_count=" + std::to_string(cfg::get_macro_slot_count()) + "\n";
        body += "macro_to_run=" + std::to_string(cfg::get_macro_to_run()) + "\n";
        body += "plugin_enabled=" + std::to_string(cfg::get_plugin_enabled() ? 1 : 0) + "\n";
        body += "language=" + std::to_string(cfg::get_language()) + "\n";

        for (int i = 0; i < 8; ++i) {
            body += "\n[macro";
            body += std::to_string(i + 1);
            body += "]\n";
            body += cfg::get_macro_slot(i);
            body += "\n";

            char slot_path[128];
            std::snprintf(slot_path, sizeof slot_path, "%s/macro%d.txt", k_dir, i + 1);
            if (!write_text_file(slot_path, cfg::get_macro_slot(i))) {
                logger::printf("macro_sd: slot %d write failed\n", i + 1);
            }
        }

        if (!write_text_file(k_bank_file, body)) {
            return false;
        }
        return true;
    }


    static bool
  parse_kv_line(const std::string &line, std::string *key, std::string *val)
    {
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
            return false;
        }
        *key = line.substr(0, eq);
        *val = line.substr(eq + 1);
        trim(*key);
        trim(*val);
        return !key->empty();
    }


    bool
    import_all()
        noexcept
    {
        set_err("");
        std::string bank;
        if (!read_text_file(k_bank_file, &bank)) {
            set_err("macros.ini not found");
            return false;
        }

        int current_slot = -1;
        std::size_t pos  = 0;
        while (pos < bank.size()) {
            std::size_t nl = bank.find('\n', pos);
            if (nl == std::string::npos) {
                nl = bank.size();
            }
            std::string line = bank.substr(pos, nl - pos);
            pos              = nl + 1;
            trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }
            if (line.size() > 2 && line.front() == '[' && line.back() == ']') {
                std::string sec = line.substr(1, line.size() - 2);
                if (sec.size() >= 5 && sec.rfind("macro", 0) == 0) {
                    const int n = std::atoi(sec.c_str() + 5);
                    if (n >= 1 && n <= 8) {
                        current_slot = n - 1;
                    }
                }
                continue;
            }

            std::string key;
            std::string val;
            if (!parse_kv_line(line, &key, &val)) {
                if (current_slot >= 0) {
                    cfg::set_macro_slot(current_slot, line);
                }
                continue;
            }

            if (key == "post_menu_delay_ms") {
                cfg::set_post_menu_delay_ms(std::atoi(val.c_str()));
            } else if (key == "macro_slot_count") {
                cfg::set_macro_slot_count(std::atoi(val.c_str()));
            } else if (key == "macro_to_run") {
                cfg::set_macro_to_run(std::atoi(val.c_str()));
            } else if (key == "plugin_enabled") {
                cfg::set_plugin_enabled(val != "0");
            } else if (key == "language") {
                cfg::set_language(std::atoi(val.c_str()));
            } else if (current_slot >= 0 && key == "script") {
                cfg::set_macro_slot(current_slot, val);
            }
        }

        for (int i = 0; i < 8; ++i) {
            char slot_path[128];
            std::snprintf(slot_path, sizeof slot_path, "%s/macro%d.txt", k_dir, i + 1);
            std::string slot_text;
            if (read_text_file(slot_path, &slot_text)) {
                cfg::set_macro_slot(i, slot_text);
            }
        }

        cfg::persist_all();
        return true;
    }


    std::string
    last_error()
        noexcept
    {
        return g_last_error;
    }

} // namespace macro_sd
