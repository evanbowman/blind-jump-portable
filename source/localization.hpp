#pragma once


enum class LocaleLanguage { null, english, count };


enum class LocaleString {
    intro_text_1,
    intro_text_2,
    map_legend_1,
    map_legend_2,
    map_legend_3,
    map_legend_4,
    waypoint_text,
    part_1_text,
    part_1_title,
    part_2_text,
    part_2_title,
    part_3_text,
    part_3_title,
    logbook_str_1,
    engineer_notebook_str,
    empty_inventory_str,
    old_poster_title,
    surveyor_logbook_title,
    engineer_notebook_title,
    blaster_title,
    accelerator_title,
    lethargy_title,
    map_system_title,
    explosive_rounds_title,
    signal_jammer_title,
    seed_packet_title,
    single_use_warning,
    locked,
    enemies_remaining_singular,
    enemies_remaining_plural,
    nothing,
    got_item_before,
    got_item_after,
    inventory_full,
    items,
    level_clear,
    you_died,
    score,
    high_score,
    items_collected_prefix,
    items_collected_suffix,
    waypoints,
    punctuation_period,
    menu_resume,
    menu_save_and_quit,
    goodbye_text,
    item_overheated,
    player_name,
    drone_name,
    dasher_name,

    count
};


void locale_set_language(LocaleLanguage ll);


const char* locale_string(LocaleString ls);


void locale_num2str(int num, char* buffer, int base);
