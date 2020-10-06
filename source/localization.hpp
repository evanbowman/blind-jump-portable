#pragma once


#include "dateTime.hpp"
#include "string.hpp"
#include "bulkAllocator.hpp"


enum class LocaleString {
    empty,
    intro_text_1,
    intro_text_2,
    map_legend_1,
    map_legend_2,
    map_legend_3,
    map_legend_4,
    map_legend_5,
    waypoint_text,
    part_1_text,
    part_1_title,
    part_2_text,
    part_2_title,
    part_3_text,
    part_3_title,
    part_4_text,
    part_4_title,
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
    navigation_pamphlet_title,
    orange_title,
    orange_seeds_title,
    postal_advert_title,
    long_jump_z2_title,
    long_jump_z3_title,
    long_jump_z4_title,
    single_use_warning,
    locked,
    peer_too_close_to_item,
    enemies_remaining_singular,
    enemies_remaining_plural,
    nothing,
    got_item_before,
    got_item_after,
    inventory_full,
    items,
    level_clear,
    score,
    high_score,
    high_scores,
    items_collected_prefix,
    items_collected_suffix,
    items_collected_heading,
    waypoints,
    punctuation_period,
    menu_resume,
    menu_connect_peer,
    menu_settings,
    menu_console,
    menu_save_and_quit,
    goodbye_text,
    select_target_text,
    navigation_pamphlet,
    settings_show_stats,
    settings_language,
    settings_contrast,
    settings_night_mode,
    settings_swap_action_keys,
    settings_default,
    settings_difficulty,
    settings_difficulty_normal,
    settings_difficulty_hard,
    settings_difficulty_survival,
    settings_difficulty_err,
    settings_log_severity,
    yes,
    no,
    menu_disabled,
    distance_units_feet,
    launch,
    peer_connected,
    peer_connection_failed,
    peer_lost,
    peer_health_changed,
    level_transition_awaiting_peers,
    level_transition_synchronizing,
    fps_stats_suffix,
    network_tx_stats_suffix,
    network_rx_stats_suffix,
    network_tx_loss_stats_suffix,
    network_rx_loss_stats_suffix,
    scratch_buf_avail_stats_suffix,
    link_saturation_stats_suffix,
    map_required,
    boss0_defeated,
    boss1_defeated,
    boss2_defeated,
    peer_transport_waiting,
    severity_debug,
    severity_info,
    severity_warning,
    severity_error,
    power_surge_detected,
    store_buy_items,
    store_sell_items,
    store_buy,
    store_sell,
    store_info,
    scavenger_store,
    buy,
    sell,
    update_required,
    peer_requires_update,
    overall_heading,
    peer_used_lethargy,
    language_name,
    health_safety_notice,
    health_safety_text,
    count
};


void locale_set_language(int language_id);
void locale_set_language_english();
int locale_get_language();


using LocalizedStrBuffer = StringBuffer<1187>;
using LocalizedText = DynamicMemory<LocalizedStrBuffer>;
LocalizedText locale_string(Platform& pfrm, LocaleString ls);


// In most cases, you do not want to call this function directly, better to call
// the localized version, locale_num2str. Only call english__to_string for
// logging purposes, where the language is assumed to be english.
void english__to_string(int num, char* buffer, int base);


void locale_num2str(int num, char* buffer, int base);


template <u32 buffer_size>
void format_time(StringBuffer<buffer_size>& str, const DateTime& dt)
{
    char buffer[48];

    locale_num2str(dt.date_.month_, buffer, 10);
    str += buffer;
    str += "/";

    locale_num2str(dt.date_.day_, buffer, 10);
    str += buffer;
    str += "/";

    locale_num2str(dt.date_.year_, buffer, 10);
    str += buffer;
    str += " ";

    locale_num2str(dt.hour_, buffer, 10);
    str += buffer;
    str += ":";

    locale_num2str(dt.minute_, buffer, 10);
    str += buffer;
    str += ":";

    locale_num2str(dt.second_, buffer, 10);
    str += buffer;
}


template <u32 buffer_size>
void log_format_time(StringBuffer<buffer_size>& str, const DateTime& dt)
{
    const auto saved_language = locale_get_language();
    locale_set_language(1);

    format_time(str, dt);

    locale_set_language(saved_language);
}
