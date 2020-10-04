#include "localization.hpp"
#include "platform/platform.hpp"
#include "script/lisp.hpp"


// Each language should define a texture mapping, which tells the rendering code
// where in the texture data to look for the glyph corresponding to a given utf8
// codepoint. Keep in mind, that on some platforms, like the Gameboy Advance,
// there is very little texture memory available for displaying glyphs. You may
// only be able to display 80 or so unique characters on the screen at a time.
std::optional<Platform::TextureMapping>
standard_texture_map(const utf8::Codepoint& cp)
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
        case '<':
            return 103;
        case '>':
            return 104;
        case '#':
            return 105;
        default:
            // extended spanish and french characters
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
            // katakana
            else if (cp == utf8::getc(u8"ア")) {
                return 106;
            } else if (cp == utf8::getc(u8"イ")) {
                return 107;
            } else if (cp == utf8::getc(u8"ウ")) {
                return 108;
            } else if (cp == utf8::getc(u8"エ")) {
                return 109;
            } else if (cp == utf8::getc(u8"オ")) {
                return 110;
            } else if (cp == utf8::getc(u8"カ")) {
                return 111;
            } else if (cp == utf8::getc(u8"キ")) {
                return 112;
            } else if (cp == utf8::getc(u8"ク")) {
                return 113;
            } else if (cp == utf8::getc(u8"ケ")) {
                return 114;
            } else if (cp == utf8::getc(u8"コ")) {
                return 115;
            } else if (cp == utf8::getc(u8"サ")) {
                return 116;
            } else if (cp == utf8::getc(u8"シ")) {
                return 117;
            } else if (cp == utf8::getc(u8"ス")) {
                return 118;
            } else if (cp == utf8::getc(u8"セ")) {
                return 119;
            } else if (cp == utf8::getc(u8"ソ")) {
                return 120;
            } else if (cp == utf8::getc(u8"タ")) {
                return 121;
            } else if (cp == utf8::getc(u8"チ")) {
                return 122;
            } else if (cp == utf8::getc(u8"ツ")) {
                return 123;
            } else if (cp == utf8::getc(u8"テ")) {
                return 124;
            } else if (cp == utf8::getc(u8"ト")) {
                return 125;
            } else if (cp == utf8::getc(u8"ナ")) {
                return 126;
            } else if (cp == utf8::getc(u8"ニ")) {
                return 127;
            } else if (cp == utf8::getc(u8"ヌ")) {
                return 128;
            } else if (cp == utf8::getc(u8"ネ")) {
                return 129;
            } else if (cp == utf8::getc(u8"ノ")) {
                return 130;
            } else if (cp == utf8::getc(u8"ハ")) {
                return 131;
            } else if (cp == utf8::getc(u8"ヒ")) {
                return 132;
            } else if (cp == utf8::getc(u8"フ")) {
                return 133;
            } else if (cp == utf8::getc(u8"ヘ")) {
                return 134;
            } else if (cp == utf8::getc(u8"ホ")) {
                return 135;
            } else if (cp == utf8::getc(u8"マ")) {
                return 136;
            } else if (cp == utf8::getc(u8"ミ")) {
                return 137;
            } else if (cp == utf8::getc(u8"ム")) {
                return 138;
            } else if (cp == utf8::getc(u8"メ")) {
                return 139;
            } else if (cp == utf8::getc(u8"モ")) {
                return 140;
            } else if (cp == utf8::getc(u8"ヤ")) {
                return 141;
            } else if (cp == utf8::getc(u8"ユ")) {
                return 142;
            } else if (cp == utf8::getc(u8"ヨ")) {
                return 143;
            } else if (cp == utf8::getc(u8"ラ")) {
                return 144;
            } else if (cp == utf8::getc(u8"リ")) {
                return 145;
            } else if (cp == utf8::getc(u8"ル")) {
                return 146;
            } else if (cp == utf8::getc(u8"レ")) {
                return 147;
            } else if (cp == utf8::getc(u8"ロ")) {
                return 148;
            } else if (cp == utf8::getc(u8"ワ")) {
                return 149;
            } else if (cp == utf8::getc(u8"ヲ")) {
                return 150;
            } else if (cp == utf8::getc(u8"ン")) {
                return 151;
            } else if (cp == utf8::getc(u8"ガ")) {
                return 152;
            } else if (cp == utf8::getc(u8"ギ")) {
                return 153;
            } else if (cp == utf8::getc(u8"グ")) {
                return 154;
            } else if (cp == utf8::getc(u8"ゲ")) {
                return 155;
            } else if (cp == utf8::getc(u8"ゴ")) {
                return 156;
            } else if (cp == utf8::getc(u8"ゲ")) {
                return 157;
            } else if (cp == utf8::getc(u8"ジ")) {
                return 158;
            } else if (cp == utf8::getc(u8"ズ")) {
                return 159;
            } else if (cp == utf8::getc(u8"ゼ")) {
                return 160;
            } else if (cp == utf8::getc(u8"ゾ")) {
                return 161;
            } else if (cp == utf8::getc(u8"ダ")) {
                return 162;
            } else if (cp == utf8::getc(u8"ヂ")) {
                return 163;
            } else if (cp == utf8::getc(u8"ヅ")) {
                return 164;
            } else if (cp == utf8::getc(u8"デ")) {
                return 165;
            } else if (cp == utf8::getc(u8"ド")) {
                return 166;
            } else if (cp == utf8::getc(u8"バ")) {
                return 167;
            } else if (cp == utf8::getc(u8"パ")) {
                return 168;
            } else if (cp == utf8::getc(u8"ビ")) {
                return 169;
            } else if (cp == utf8::getc(u8"ピ")) {
                return 170;
            } else if (cp == utf8::getc(u8"ブ")) {
                return 171;
            } else if (cp == utf8::getc(u8"プ")) {
                return 172;
            } else if (cp == utf8::getc(u8"ベ")) {
                return 173;
            } else if (cp == utf8::getc(u8"ペ")) {
                return 174;
            } else if (cp == utf8::getc(u8"ボ")) {
                return 175;
            } else if (cp == utf8::getc(u8"ポ")) {
                return 176;
            } else if (cp == utf8::getc(u8"ー")) {
                return 177;
            } else if (cp == utf8::getc(u8"ヴ")) {
                return 178;
            } else if (cp == utf8::getc(u8"ァ")) {
                return 179;
            } else if (cp == utf8::getc(u8"ィ")) {
                return 180;
            } else if (cp == utf8::getc(u8"ゥ")) {
                return 181;
            } else if (cp == utf8::getc(u8"ェ")) {
                return 182;
            } else if (cp == utf8::getc(u8"ォ")) {
                return 183;
            } else if (cp == utf8::getc(u8"・")) {
                return 184;
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



static int language_id = 0;


Platform::TextureCpMapper locale_texture_map()
{
    return standard_texture_map;
}


void locale_set_language(int language_id)
{
    ::language_id = language_id;
}


int locale_get_language()
{
    return ::language_id;
}


StringBuffer<31> locale_language_name(int language)
{
    auto languages = lisp::get_var("languages");

    auto lang = lisp::get_list(languages, language);

    return lang->expect<lisp::Symbol>().name_;
}


LocalizedText locale_string(Platform& pfrm, LocaleString ls)
{
    auto result = allocate_dynamic<LocalizedStrBuffer>(pfrm);

    auto languages = lisp::get_var("languages");

    auto lang = lisp::get_list(languages, ::language_id);

    StringBuffer<31> fname = lang->expect<lisp::Symbol>().name_
        ;
    fname += ".txt";

    if (auto data = pfrm.load_file_contents("strings", fname.c_str())) {
        const int target_line = static_cast<int>(ls);

        int index = 0;
        while (index not_eq target_line) {
            while (*data not_eq '\n') {
                if (*data == '\0') {
                    error(pfrm, "blah");
                    while (true) ; // FIXME: raise error...
                }
                ++data;
            }
            ++data;

            ++index;
        }

        while (*data not_eq '\0' and *data not_eq '\n') {
            result.obj_->push_back(*data);
            ++data;
        }

        return result;
    } else {
        error(pfrm, "strings file for language does not exist");
        while (true) ;
    }

    // static const char* empty_str = "NULL";

    // // NOTE: Strings should be utf8.

    // // clang-format off

    // switch (language) {
    // case LocaleLanguage::spanish:
    //     switch (ls) {
    //     case LocaleString::empty: return "";
    //     case LocaleString::intro_text_2: return "Evan Bowman presenta";
    //     case LocaleString::map_legend_1: return "tú";
    //     case LocaleString::map_legend_2: return "enemigo";
    //     case LocaleString::map_legend_3: return "salida";
    //     case LocaleString::map_legend_4: return "artículo";
    //     case LocaleString::waypoint_text: return "waypoint ";
    //     case LocaleString::part_1_text: return "parte I:";
    //     case LocaleString::part_1_title: return "el arribo ";
    //     case LocaleString::part_2_text: return "parte II:";
    //     case LocaleString::part_2_title: return "el descenso ";
    //     case LocaleString::part_3_text: return "parte III:";
    //     case LocaleString::part_3_title: return "Hacia el Páramo";
    //     case LocaleString::part_4_text: return "parte IV:";
    //     case LocaleString::part_4_title: return "en la luz de la luna";
    //     case LocaleString::settings_show_stats: return "Mostrar estadísticas: ";
    //     case LocaleString::settings_language: return "Idioma: ";
    //     case LocaleString::settings_contrast: return "Contraste: ";
    //     case LocaleString::settings_difficulty: return "Dificultad: ";
    //     case LocaleString::settings_difficulty_normal: return "normal";
    //     case LocaleString::settings_difficulty_hard: return "difícil";
    //     case LocaleString::settings_difficulty_survival: return "sobrevivencia";
    //     case LocaleString::settings_night_mode: return "Modo Nocturno: ";
    //     case LocaleString::settings_swap_action_keys: return "Transponer A/B: ";
    //     case LocaleString::settings_default: return "por defecto";
    //     case LocaleString::settings_difficulty_err: return "level must be clear!";
    //     case LocaleString::yes: return "sí";
    //     case LocaleString::no: return "no";
    //     case LocaleString::menu_resume: return "Reanudar";
    //     case LocaleString::menu_connect_peer: return "Connectar el Par";
    //     case LocaleString::menu_console: return "Script Console";
    //     case LocaleString::menu_settings: return "Configuración";
    //     case LocaleString::menu_save_and_quit: return "Guardar y Abandonar";
    //     case LocaleString::distance_units_feet: return " ft";
    //     case LocaleString::empty_inventory_str: return "Vacía";
    //     case LocaleString::old_poster_title: return "Póster viejo (1)";
    //     case LocaleString::surveyor_logbook_title: return "Cuaderno (1/2)";
    //     case LocaleString::engineer_notebook_title: return "Cuaderno (2/2)";
    //     case LocaleString::blaster_title: return "Pistola";
    //     case LocaleString::accelerator_title: return "Acelerador (60 balas)";
    //     case LocaleString::postal_advert_title: return "...";
    //     case LocaleString::long_jump_z2_title: return "Long jump drive (pt. II)";
    //     case LocaleString::long_jump_z3_title: return "Long jump drive (pt. III)";
    //     case LocaleString::long_jump_z4_title: return "Long jump drive (pt. IV)";
    //     case LocaleString::lethargy_title: return "Letargo (18 seg)";
    //     case LocaleString::map_system_title: return "Sistema de mapa";
    //     case LocaleString::map_required: return "necesario: Sistema de mapa";
    //     case LocaleString::explosive_rounds_title: return "Balas explosivos (2)";
    //     case LocaleString::seed_packet_title: return "Paquete de semillas";
    //     case LocaleString::navigation_pamphlet_title: return "Folleto de navegación";
    //     case LocaleString::orange_title: return "Naranja (+1 hp)";
    //     case LocaleString::orange_seeds_title: return "Semillas de naranja";
    //     case LocaleString::single_use_warning: return "(DE UN SOLO USO)";
    //     case LocaleString::locked: return "cerrado, ";
    //     case LocaleString::enemies_remaining_singular: return " enemigo restante";
    //     case LocaleString::enemies_remaining_plural: return " enemigos restantes";
    //     case LocaleString::nothing: return "Nada";
    //     case LocaleString::got_item_before: return "hallas \"";
    //     case LocaleString::got_item_after: return "\"";
    //     case LocaleString::level_clear: return "waypoint completado";
    //     case LocaleString::items: return "artículos";
    //     case LocaleString::score: return "puntuación";
    //     case LocaleString::high_score: return "puntuación alta";
    //     case LocaleString::high_scores: return "puntuaciónes altas";
    //     case LocaleString::waypoints: return "waypoints";
    //     case LocaleString::punctuation_period: return ".";
    //     case LocaleString::goodbye_text: return "Hasta luego...";
    //     case LocaleString::signal_jammer_title: return "Bloqueador de señales";
    //     case LocaleString::inventory_full:
    //     case LocaleString::peer_too_close_to_item:
    //     case LocaleString::items_collected_prefix:
    //     case LocaleString::items_collected_suffix:
    //     case LocaleString::items_collected_heading:
    //     case LocaleString::select_target_text:
    //     case LocaleString::settings_log_severity:
    //     case LocaleString::menu_disabled:
    //     case LocaleString::launch:
    //     case LocaleString::peer_connected:
    //     case LocaleString::peer_connection_failed:
    //     case LocaleString::peer_lost:
    //     case LocaleString::peer_health_changed:
    //     case LocaleString::level_transition_awaiting_peers:
    //     case LocaleString::level_transition_synchronizing:
    //     case LocaleString::fps_stats_suffix:
    //     case LocaleString::network_tx_stats_suffix:
    //     case LocaleString::network_rx_stats_suffix:
    //     case LocaleString::network_tx_loss_stats_suffix:
    //     case LocaleString::network_rx_loss_stats_suffix:
    //     case LocaleString::link_saturation_stats_suffix:
    //     case LocaleString::scratch_buf_avail_stats_suffix:
    //     case LocaleString::boss0_defeated:
    //     case LocaleString::boss1_defeated:
    //     case LocaleString::boss2_defeated:
    //     case LocaleString::peer_transport_waiting:
    //     case LocaleString::severity_debug:
    //     case LocaleString::severity_info:
    //     case LocaleString::severity_warning:
    //     case LocaleString::severity_error:
    //     case LocaleString::power_surge_detected:
    //     case LocaleString::store_buy_items:
    //     case LocaleString::store_sell_items:
    //     case LocaleString::store_buy:
    //     case LocaleString::store_sell:
    //     case LocaleString::store_info:
    //     case LocaleString::scavenger_store:
    //     case LocaleString::buy:
    //     case LocaleString::sell:
    //     case LocaleString::update_required:
    //     case LocaleString::peer_requires_update:
    //     case LocaleString::overall_heading:
    //     case LocaleString::peer_used_lethargy:
    //     default:
    //         return empty_str;
    //     }
    //     break;

    // case LocaleLanguage::english:
    //     switch (ls) {
    //     case LocaleString::empty: return "";
    //     case LocaleString::intro_text_1: return "";
    //     case LocaleString::intro_text_2: return "Evan Bowman presents";
    //     case LocaleString::map_legend_1: return "you";
    //     case LocaleString::map_legend_2: return "enemy";
    //     case LocaleString::map_legend_3: return "exit";
    //     case LocaleString::map_legend_4: return "item";
    //     case LocaleString::map_legend_5: return "shop";
    //     case LocaleString::waypoint_text: return "waypoint ";
    //     case LocaleString::part_1_text: return "part I:";
    //     case LocaleString::part_1_title: return "the arrival ";
    //     case LocaleString::part_2_text: return "part II:";
    //     case LocaleString::part_2_title: return "the descent ";
    //     case LocaleString::part_3_text: return "part III:";
    //     case LocaleString::part_3_title: return "into the waste";
    //     case LocaleString::part_4_text: return "part IV:";
    //     case LocaleString::part_4_title: return "by moonlight";
    //     case LocaleString::logbook_str_1: return
    //             "[2.11.2081]  We've been experiencing strange power surges lately on the "
    //             "station. The director reassured me that everthing will run smoothly from "
    //             "now on. The robots have been acting up ever since the last "
    //             "communications blackout though, I'm afraid next time they'll go mad...";
    //     case LocaleString::engineer_notebook_str: return
    //             "Power surges emanating from the hyperspace gates seem to have an "
    //             "unusual effect on machinery, especially the robots on the "
    //             "stations. We've observed some unfortunate violent reactions from "
    //             "the machines recently--this is all starting to get out of control. "
    //             "We long suspected that there was something... else... out there in "
    //             "hyperspace, something that we, perhaps, should never have disturbed... "
    //             "It's time that we sealed up the transport gates for good. The best way, "
    //             "would be to disable the transit network's central hub, on the lunar "
    //             "surface...";
    //     case LocaleString::navigation_pamphlet: return
    //         "No road map? No problem! The hyperspace gate network calculates a "
    //         "path to your destination by routing your journey through a "
    //         "sequence of waypoints. Tens of thousands of waypoints were "
    //         "contructed in orbit around Earth and its moon. Because the "
    //         "waypoints are constantly in motion during orbit, the gate network "
    //         "continually recalculates the path to your destination. You'll "
    //         "never visit the same waypoints twice! Due to the unpredictable "
    //         "nature of the hyperspace gates, passing through a gate is "
    //         "sometimes referred to as a blind jump.";
    //     case LocaleString::empty_inventory_str: return "Empty";
    //     case LocaleString::old_poster_title: return "Old poster (1)";
    //     case LocaleString::surveyor_logbook_title: return "Notebook (1/2)";
    //     case LocaleString::engineer_notebook_title: return "Notebook (2/2)";
    //     case LocaleString::blaster_title: return "Blaster";
    //     case LocaleString::accelerator_title: return "Accelerator (60 shots)";
    //     case LocaleString::postal_advert_title: return "Low Orbit Postal Advert";
    //     case LocaleString::long_jump_z2_title: return "Long jump drive (pt. II)";
    //     case LocaleString::long_jump_z3_title: return "Long jump drive (pt. III)";
    //     case LocaleString::long_jump_z4_title: return "Long jump drive (pt. IV)";
    //     case LocaleString::lethargy_title: return "Lethargy (18 sec)";
    //     case LocaleString::map_system_title: return "Map system";
    //     case LocaleString::map_required: return "required: Map system";
    //     case LocaleString::explosive_rounds_title: return "Explosive rounds (2)";
    //     case LocaleString::seed_packet_title: return "Seed packet";
    //     case LocaleString::navigation_pamphlet_title: return "Navigation pamphlet";
    //     case LocaleString::orange_title: return "Orange (+1 hp)";
    //     case LocaleString::orange_seeds_title: return "Orange seeds";
    //     case LocaleString::single_use_warning: return "(SINGLE USE)";
    //     case LocaleString::locked: return "locked, ";
    //     case LocaleString::peer_too_close_to_item: return "blocked, Peer is in the way!";
    //     case LocaleString::enemies_remaining_singular: return " enemy left";
    //     case LocaleString::enemies_remaining_plural: return " enemies left";
    //     case LocaleString::nothing: return "Nothing";
    //     case LocaleString::got_item_before: return "got \"";
    //     case LocaleString::got_item_after: return "\"";
    //     case LocaleString::inventory_full: return "Inventory full";
    //     case LocaleString::items: return "items";
    //     case LocaleString::level_clear: return "waypoint clear";
    //     case LocaleString::score: return "score ";
    //     case LocaleString::items_collected_prefix: return "items collected % ";
    //     case LocaleString::items_collected_suffix: return "";
    //     case LocaleString::items_collected_heading: return "items collected %";
    //     case LocaleString::high_score: return "highscore ";
    //     case LocaleString::high_scores: return "highscores";
    //     case LocaleString::waypoints: return "waypoints ";
    //     case LocaleString::punctuation_period: return ".";
    //     case LocaleString::menu_resume: return "Resume";
    //     case LocaleString::menu_connect_peer: return "Connect Peer";
    //     case LocaleString::menu_console: return "Script Console";
    //     case LocaleString::menu_settings: return "Settings";
    //     case LocaleString::menu_save_and_quit: return "Save and Quit";
    //     case LocaleString::goodbye_text: return "See you later...";
    //     case LocaleString::signal_jammer_title: return "Signal jammer";
    //     case LocaleString::select_target_text: return "Choose a target:";
    //     case LocaleString::settings_show_stats: return "Show Stats: ";
    //     case LocaleString::settings_language: return "Language: ";
    //     case LocaleString::settings_contrast: return "Contrast: ";
    //     case LocaleString::settings_difficulty: return "Difficulty: ";
    //     case LocaleString::settings_difficulty_normal: return "normal";
    //     case LocaleString::settings_difficulty_hard: return "hard";
    //     case LocaleString::settings_difficulty_survival: return "survival";
    //     case LocaleString::settings_night_mode: return "Night Mode: ";
    //     case LocaleString::settings_swap_action_keys: return "Swap A/B: ";
    //     case LocaleString::settings_default: return "default";
    //     case LocaleString::settings_difficulty_err: return "level must be clear!";
    //     case LocaleString::settings_log_severity: return "Log Severity: ";
    //     case LocaleString::yes: return "yes";
    //     case LocaleString::no: return "no";
    //     case LocaleString::menu_disabled: return "menu disabled";
    //     case LocaleString::distance_units_feet: return " ft";
    //     case LocaleString::launch: return "launch: ";
    //     case LocaleString::peer_connected: return "Peer connected!";
    //     case LocaleString::peer_connection_failed: return "Peer connection failed!";
    //     case LocaleString::peer_lost: return "Peer died!";
    //     case LocaleString::peer_health_changed: return "Peer health changed to ";
    //     case LocaleString::level_transition_awaiting_peers: return "Waiting for Peers...";
    //     case LocaleString::level_transition_synchronizing: return "Synchronizing Games...";
    //     case LocaleString::fps_stats_suffix: return " fps";
    //     case LocaleString::network_tx_stats_suffix: return " tx";
    //     case LocaleString::network_rx_stats_suffix: return " rx";
    //     case LocaleString::network_tx_loss_stats_suffix: return " tl";
    //     case LocaleString::network_rx_loss_stats_suffix: return " rl";
    //     case LocaleString::link_saturation_stats_suffix: return " lnsat";
    //     case LocaleString::scratch_buf_avail_stats_suffix: return " sbr";
    //     case LocaleString::boss0_defeated: return "The Wanderer Defeated";
    //     case LocaleString::boss1_defeated: return "Gatekeeper Destroyed";
    //     case LocaleString::boss2_defeated: return "Defeated the Twins";
    //     case LocaleString::peer_transport_waiting: return "Peer waiting in gate...";
    //     case LocaleString::severity_debug: return "debug";
    //     case LocaleString::severity_info: return "info";
    //     case LocaleString::severity_warning: return "warning";
    //     case LocaleString::severity_error: return "error";
    //     case LocaleString::power_surge_detected: return "Power Surge detected!";
    //     case LocaleString::store_buy_items: return "Buy items";
    //     case LocaleString::store_sell_items: return "Sell items";
    //     case LocaleString::store_buy: return "Buy, price: ";
    //     case LocaleString::store_sell: return "Sell, price: ";
    //     case LocaleString::store_info: return "Item info";
    //     case LocaleString::scavenger_store: return "Scavenger's shop";
    //     case LocaleString::buy: return "buy";
    //     case LocaleString::sell: return "sell";
    //     case LocaleString::update_required: return "Update required!";
    //     case LocaleString::peer_requires_update: return "Peer requires update!";
    //     case LocaleString::overall_heading: return "overall";
    //     case LocaleString::peer_used_lethargy: return "Peer used lethargy!";
    //     default: return empty_str;
    //     }
    //     break;

    // default:
    //     return empty_str;
    // }

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
    // FIXME!!!

    // switch (language) {
    // case LocaleLanguage::spanish:
    // case LocaleLanguage::english:
        english__to_string(num, buffer, base);
    //     break;

    // default:
    // case LocaleLanguage::null:
        // buffer[0] = '\0';
    //     break;
    // }
}
