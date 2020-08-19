#include "localization.hpp"
#include "platform/platform.hpp"


// Each language should define a texture mapping, which tells the rendering code
// where in the texture data to look for the glyph corresponding to a given utf8
// codepoint. Keep in mind, that on some platforms, like the Gameboy Advance,
// there is very little texture memory available for displaying glyphs. You may
// only be able to display 80 or so unique characters on the screen at a time.
std::optional<Platform::TextureMapping>
english_spanish_french_texture_map(const utf8::Codepoint& cp)
{
    auto mapping = [&]() -> std::optional<u16> {
        switch (cp) {
        case '0':
            return 1;
        case '1':
            return 2;
        case '2':
            return 3;
        case '3':
            return 4;
        case '4':
            return 5;
        case '5':
            return 6;
        case '6':
            return 7;
        case '7':
            return 8;
        case '8':
            return 9;
        case '9':
            return 10;
        case 'a':
            return 11;
        case 'b':
            return 12;
        case 'c':
            return 13;
        case 'd':
            return 14;
        case 'e':
            return 15;
        case 'f':
            return 16;
        case 'g':
            return 17;
        case 'h':
            return 18;
        case 'i':
            return 19;
        case 'j':
            return 20;
        case 'k':
            return 21;
        case 'l':
            return 22;
        case 'm':
            return 23;
        case 'n':
            return 24;
        case 'o':
            return 25;
        case 'p':
            return 26;
        case 'q':
            return 27;
        case 'r':
            return 28;
        case 's':
            return 29;
        case 't':
            return 30;
        case 'u':
            return 31;
        case 'v':
            return 32;
        case 'w':
            return 33;
        case 'x':
            return 34;
        case 'y':
            return 35;
        case 'z':
            return 36;
        case '.':
            return 37;
        case ',':
            return 38;
        case 'A':
            return 39;
        case 'B':
            return 40;
        case 'C':
            return 41;
        case 'D':
            return 42;
        case 'E':
            return 43;
        case 'F':
            return 44;
        case 'G':
            return 45;
        case 'H':
            return 46;
        case 'I':
            return 47;
        case 'J':
            return 48;
        case 'K':
            return 49;
        case 'L':
            return 50;
        case 'M':
            return 51;
        case 'N':
            return 52;
        case 'O':
            return 53;
        case 'P':
            return 54;
        case 'Q':
            return 55;
        case 'R':
            return 56;
        case 'S':
            return 57;
        case 'T':
            return 58;
        case 'U':
            return 59;
        case 'V':
            return 60;
        case 'W':
            return 61;
        case 'X':
            return 62;
        case 'Y':
            return 63;
        case 'Z':
            return 64;
        case '"':
            return 65;
        case '\'':
            return 66;
        case '[':
            return 67;
        case ']':
            return 68;
        case '(':
            return 69;
        case ')':
            return 70;
        case ':':
            return 71;
        case ' ':
            return 72;
        case '%':
            return 93;
        case '!':
            return 94;
        case '?':
            return 95;
        case '+':
            return 98;
        case '-':
            return 99;
        case '/':
            return 100;
        case '*':
            return 101;
        case '=':
            return 102;
        default:
            if (cp == utf8::getc(u8"ñ")) {
                return 73;
            } else if (cp == utf8::getc(u8"á")) {
                return 74;
            } else if (cp == utf8::getc(u8"é")) {
                return 75;
            } else if (cp == utf8::getc(u8"í")) {
                return 76;
            } else if (cp == utf8::getc(u8"ó")) {
                return 77;
            } else if (cp == utf8::getc(u8"ú")) {
                return 78;
            } else if (cp == utf8::getc(u8"â")) {
                return 79;
            } else if (cp == utf8::getc(u8"ê")) {
                return 80;
            } else if (cp == utf8::getc(u8"î")) {
                return 81;
            } else if (cp == utf8::getc(u8"ô")) {
                return 82;
            } else if (cp == utf8::getc(u8"û")) {
                return 83;
            } else if (cp == utf8::getc(u8"à")) {
                return 84;
            } else if (cp == utf8::getc(u8"è")) {
                return 85;
            } else if (cp == utf8::getc(u8"ù")) {
                return 86;
            } else if (cp == utf8::getc(u8"ë")) {
                return 87;
            } else if (cp == utf8::getc(u8"ï")) {
                return 88;
            } else if (cp == utf8::getc(u8"ü")) {
                return 89;
            } else if (cp == utf8::getc(u8"ç")) {
                return 90;
            } else if (cp == utf8::getc(u8"Ç")) {
                return 91;
            } else if (cp == utf8::getc(u8"ö")) {
                return 92;
            } else if (cp == utf8::getc(u8"¡")) {
                return 96;
            } else if (cp == utf8::getc(u8"¿")) {
                return 97;
            }
            return std::nullopt;
        }
    }();
    if (mapping) {
        return Platform::TextureMapping{"charset_en_spn_fr", *mapping};
    } else {
        return {};
    }
}


std::optional<Platform::TextureMapping> null_texture_map(const utf8::Codepoint&)
{
    return {};
}


static const Platform::TextureCpMapper
    texture_codepoint_mappers[static_cast<int>(LocaleLanguage::count)] = {
        null_texture_map,
        english_spanish_french_texture_map};


static LocaleLanguage language = LocaleLanguage::null;


Platform::TextureCpMapper locale_texture_map()
{
    return texture_codepoint_mappers[static_cast<int>(language)];
}


void locale_set_language(LocaleLanguage ll)
{
    language = ll;
}


LocaleLanguage locale_get_language()
{
    return language;
}


StringBuffer<31> locale_language_name(LocaleLanguage ll)
{
    switch (language) {
    case LocaleLanguage::english:
        switch (ll) {
        case LocaleLanguage::english:
            return "english";
        case LocaleLanguage::null:
            return "none";
        case LocaleLanguage::count:
            return "error";
        }
        break;

    case LocaleLanguage::null:
        return "null";

    case LocaleLanguage::count:
        return "error";
    }
    return "";
}


const char* locale_string(LocaleString ls)
{
    static const char* empty_str = "NULL";

    // NOTE: Strings should be utf8.

    // clang-format off

    switch (language) {
    case LocaleLanguage::english:
        switch (ls) {
        case LocaleString::empty: return "";
        case LocaleString::intro_text_1: return "Unlicensed by Nintendo";
        case LocaleString::intro_text_2: return "Evan Bowman presents";
        case LocaleString::map_legend_1: return "you";
        case LocaleString::map_legend_2: return "enemy";
        case LocaleString::map_legend_3: return "exit";
        case LocaleString::map_legend_4: return "item";
        case LocaleString::waypoint_text: return "waypoint ";
        case LocaleString::part_1_text: return "part I:";
        case LocaleString::part_1_title: return "the arrival";
        case LocaleString::part_2_text: return "part II:";
        case LocaleString::part_2_title: return "the descent";
        case LocaleString::part_3_text: return "part III:";
        case LocaleString::part_3_title: return "by moonlight";
        case LocaleString::logbook_str_1: return
                "[2.11.2081]  We've been experiencing strange power surges lately on the "
                "station. The director reassured me that everthing will run smoothly from "
                "now on. The robots have been acting up ever since the last "
                "communications blackout though, I'm afraid next time they'll go mad...";
        case LocaleString::engineer_notebook_str: return
                "Power surges from the hyperspace gates seem to have an "
                "unusual effect on machinery, especially the robots on the "
                "stations. I've heard unconfirmed reports of violence that... "
                "if true... would be extremely distressing... The gate "
                "transit network was never really designed to be easily shut "
                "down, so concerned were we with creating a resiliant system "
                "that's always online. If you ever needed to shut down the "
                "network, you would need to chart a course to the gateway's "
                "central hub, on the lunar surface.";
        case LocaleString::navigation_pamphlet: return
            "No road map? No problem! The hyperspace gate network calculates a "
            "path to your destination by routing your journey through a "
            "sequence of waypoints. Tens of thousands of waypoints were "
            "contructed in orbit around Earth and its moon. Because the "
            "waypoints are constantly in motion during orbit, the gate network "
            "continually recalculates the path to your destination. You'll "
            "never visit the same waypoints twice! Due to the unpredictable "
            "nature of the hyperspace gates, passing through a gate is "
            "sometimes referred to as a blind jump.";
        case LocaleString::empty_inventory_str: return "Empty";
        case LocaleString::old_poster_title: return "Old poster (1)";
        case LocaleString::surveyor_logbook_title: return "Surveyor's logbook";
        case LocaleString::engineer_notebook_title: return "Engineer's notebook";
        case LocaleString::blaster_title: return "Blaster";
        case LocaleString::accelerator_title: return "Accelerator (60 shots)";
        case LocaleString::lethargy_title: return "Lethargy (18 sec)";
        case LocaleString::map_system_title: return "Map system";
        case LocaleString::map_required: return "required: Map system";
        case LocaleString::explosive_rounds_title: return "Explosive rounds (2)";
        case LocaleString::seed_packet_title: return "Seed packet";
        case LocaleString::navigation_pamphlet_title: return "Navigation pamphlet";
        case LocaleString::orange_title: return "Orange (+1 hp)";
        case LocaleString::orange_seeds_title: return "Orange seeds";
        case LocaleString::single_use_warning: return "(SINGLE USE)";
        case LocaleString::locked: return "locked, ";
        case LocaleString::peer_too_close_to_item: return "blocked, Peer is in the way!";
        case LocaleString::enemies_remaining_singular: return " enemy left";
        case LocaleString::enemies_remaining_plural: return " enemies left";
        case LocaleString::nothing: return "Nothing";
        case LocaleString::got_item_before: return "got \"";
        case LocaleString::got_item_after: return "\"";
        case LocaleString::inventory_full: return "Inventory full";
        case LocaleString::items: return "items";
        case LocaleString::level_clear: return "level clear";
        case LocaleString::you_died: return "you died";
        case LocaleString::score: return "score ";
        case LocaleString::items_collected_prefix: return "items collected % ";
        case LocaleString::items_collected_suffix: return "";
        case LocaleString::high_score: return "high score ";
        case LocaleString::waypoints: return "waypoints ";
        case LocaleString::punctuation_period: return ".";
        case LocaleString::menu_resume: return "Resume";
        case LocaleString::menu_connect_peer: return "Connect Peer";
        case LocaleString::menu_settings: return "Settings";
        case LocaleString::menu_save_and_quit: return "Save and Quit";
        case LocaleString::goodbye_text: return "See you later...";
        case LocaleString::signal_jammer_title: return "Signal jammer";
        case LocaleString::select_target_text: return "Choose a target:";
        case LocaleString::settings_show_stats: return "Show Stats: ";
        case LocaleString::settings_language: return "Language: ";
        case LocaleString::settings_contrast: return "Contrast: ";
        case LocaleString::settings_difficulty: return "Difficulty: ";
        case LocaleString::settings_difficulty_normal: return "normal";
        case LocaleString::settings_difficulty_hard: return "hard";
        case LocaleString::settings_difficulty_survival: return "survival";
        case LocaleString::settings_dynamic_camera: return "Sticky Camera: ";
        case LocaleString::settings_default: return "default";
        case LocaleString::settings_difficulty_err: return "level must be clear!";
        case LocaleString::yes: return "yes";
        case LocaleString::no: return "no";
        case LocaleString::menu_disabled: return "menu disabled";
        case LocaleString::distance_units_feet: return " ft";
        case LocaleString::launch: return "launch: ";
        case LocaleString::peer_connected: return "Peer connected!";
        case LocaleString::peer_connection_failed: return "Peer connection failed!";
        case LocaleString::peer_lost: return "Peer died!";
        case LocaleString::level_transition_awaiting_peers: return "Waiting for Peers...";
        case LocaleString::level_transition_synchronizing: return "Synchronizing Games...";
        case LocaleString::fps_stats_suffix: return " fps";
        case LocaleString::network_tx_stats_suffix: return " tx";
        case LocaleString::network_rx_stats_suffix: return " rx";
        case LocaleString::network_tx_loss_stats_suffix: return " tl";
        case LocaleString::network_rx_loss_stats_suffix: return " rl";
        case LocaleString::link_saturation_stats_suffix: return " lnsat";
        case LocaleString::scratch_buf_avail_stats_suffix: return " sbr";
        case LocaleString::boss0_defeated: return "The Wanderer Defeated";
        case LocaleString::boss1_defeated: return "Gatekeeper Destroyed";
        case LocaleString::peer_transport_waiting: return "Peer waiting in gate...";
        default: return empty_str;
        }
        break;

    default:
        return empty_str;
    }

    // clang-format on
}


void english__to_string(int num, char* buffer, int base)
{
    int i = 0;
    bool is_negative = false;

    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    // Based on the behavior of itoa()
    if (num < 0 && base == 10) {
        is_negative = true;
        num = -num;
    }

    while (num != 0) {
        int rem = num % base;
        buffer[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    str_reverse(buffer, i);

    return;
}


void locale_num2str(int num, char* buffer, int base)
{
    switch (language) {
    case LocaleLanguage::english:
        english__to_string(num, buffer, base);
        break;

    default:
    case LocaleLanguage::null:
        buffer[0] = '\0';
        break;
    }
}
