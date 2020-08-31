#pragma once

#include "bulkAllocator.hpp"
#include "game.hpp"
#include "graphics/overlay.hpp"
#include "network_event.hpp"
#include "path.hpp"
#include "state.hpp"


using DeferredState = Function<16, StatePtr()>;


class CommonNetworkListener : public net_event::Listener {
public:
    void receive(const net_event::PlayerEnteredGate&,
                 Platform& pfrm,
                 Game& game) override
    {
        if (game.peer()) {
            game.peer()->warping() = true;
        }

        push_notification(pfrm,
                          game.state(),
                          locale_string(LocaleString::peer_transport_waiting));
    }

    void receive(const net_event::ItemChestOpened& o,
                 Platform& pfrm,
                 Game& game) override
    {
        for (auto& chest : game.details().get<ItemChest>()) {
            if (chest->id() == o.id_.get()) {
                chest->sync(pfrm, o);
                return;
            }
        }
    }

    void receive(const net_event::PlayerDied&, Platform& pfrm, Game&) override
    {
        pfrm.network_peer().disconnect();
    }
};


class OverworldState : public State, public CommonNetworkListener {
public:
    OverworldState(Game&, bool camera_tracking)
        : camera_tracking_(camera_tracking)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;


    std::optional<Text> notification_text;
    NotificationStr notification_str;
    Microseconds notification_text_timer = 0;
    enum class NotificationStatus {
        flash,
        flash_animate,
        wait,
        display,
        exit,
        hidden
    } notification_status = NotificationStatus::hidden;

    using net_event::Listener::receive;

    void receive(const net_event::NewLevelIdle&, Platform&, Game&) override;
    void receive(const net_event::PlayerSpawnLaser&, Platform&, Game&) override;
    void receive(const net_event::ItemChestShared&, Platform&, Game&) override;
    void receive(const net_event::EnemyStateSync&, Platform&, Game&) override;
    void receive(const net_event::SyncSeed&, Platform&, Game&) override;
    void receive(const net_event::PlayerInfo&, Platform&, Game&) override;
    void
    receive(const net_event::EnemyHealthChanged&, Platform&, Game&) override;

protected:
    void hide_notifications(Platform& pfrm)
    {
        if (notification_status not_eq NotificationStatus::hidden) {
            for (int i = 0; i < 32; ++i) {
                pfrm.set_tile(Layer::overlay, i, 0, 0);
            }

            notification_status = NotificationStatus::hidden;
        }
    }

private:
    void show_stats(Platform& pfrm, Game& game, Microseconds delta);

    void multiplayer_sync(Platform& pfrm, Game& game, Microseconds delta);

    const bool camera_tracking_;
    Microseconds camera_snap_timer_ = 0;

    Microseconds fps_timer_ = 0;
    int fps_frame_count_ = 0;
    std::optional<Text> fps_text_;
    std::optional<Text> network_tx_msg_text_;
    std::optional<Text> network_rx_msg_text_;
    std::optional<Text> network_tx_loss_text_;
    std::optional<Text> network_rx_loss_text_;
    std::optional<Text> link_saturation_text_;
    std::optional<Text> scratch_buf_avail_text_;
    int idle_rx_count_ = 0;
};


class BossDeathSequenceState : public OverworldState {
public:
    BossDeathSequenceState(Game& game,
                           const Vec2<Float>& boss_position,
                           LocaleString boss_defeated_text)
        : OverworldState(game, false), boss_position_(boss_position),
          defeated_text_(boss_defeated_text)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    enum class AnimState {
        init,
        explosion_wait1,
        explosion_wait2,
        fade
    } anim_state_ = AnimState::init;

    const Vec2<Float> boss_position_;
    LocaleString defeated_text_;
    bool pushed_notification_ = false;
    Microseconds counter_ = 0;

    std::optional<UIMetric> health_;
    std::optional<UIMetric> score_;

    Buffer<UIMetric, Powerup::max_> powerups_;
};


// We want to make sure that we process network events while we have pause menus
// open...
class MenuState : public State, public CommonNetworkListener {
public:
    StatePtr update(Platform& pfrm, Game& game, Microseconds) override
    {
        if (pfrm.network_peer().is_connected()) {
            net_event::poll_messages(pfrm, game, *this);
        }

        return null_state();
    }
};


class LaunchCutsceneState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Float camera_offset_ = 0.f;
    Microseconds timer_ = 0;

    int anim_index_ = 0;
    Microseconds anim_timer_ = 0;

    Microseconds camera_shake_timer_ = milliseconds(400);

    Float speed_ = 0.f; // feet per microsecond
    int altitude_update_ = 1;
    int altitude_ = 100;

    Microseconds cloud_spawn_timer_ = milliseconds(100);
    bool cloud_lane_ = 0;

    std::optional<Text> altitude_text_;

    enum class Scene {
        fade_in0,
        wait,
        fade_transition0,
        fade_in,
        rising,
        enter_clouds,
        within_clouds,
        exit_clouds,
        scroll,
        fade_out
    } scene_ = Scene::fade_in0;
};


class ActiveState : public OverworldState {
public:
    ActiveState(Game& game, bool camera_tracking = true)
        : OverworldState(game, camera_tracking)
    {
    }
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    std::optional<BossHealthBar> boss_health_bar_;

private:
    std::optional<UIMetric> health_;
    std::optional<UIMetric> score_;

    Buffer<UIMetric, Powerup::max_> powerups_;

    bool pixelated_ = false;
};


class FadeInState : public OverworldState {
public:
    FadeInState(Game& game) : OverworldState(game, false)
    {
    }
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
};


class WarpInState : public OverworldState {
public:
    WarpInState(Game& game) : OverworldState(game, true)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
    bool shook_ = false;
};


class PreFadePauseState : public OverworldState {
public:
    PreFadePauseState(Game& game) : OverworldState(game, false)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;
};


class GlowFadeState : public OverworldState {
public:
    GlowFadeState(Game& game, ColorConstant color)
        : OverworldState(game, false), color_(color)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
    ColorConstant color_;
};


class FadeOutState : public OverworldState {
public:
    FadeOutState(Game& game, ColorConstant color)
        : OverworldState(game, false), color_(color)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
    ColorConstant color_;
};


class RespawnWaitState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
};


class DeathFadeState : public OverworldState {
public:
    DeathFadeState(Game& game) : OverworldState(game, false)
    {
    }
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    Microseconds counter_ = 0;
};


class DeathContinueState : public State {
public:
    DeathContinueState()
    {
    }
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    std::optional<Text> score_;
    std::optional<Text> highscore_;
    std::optional<Text> level_;
    std::optional<Text> items_collected_;
    Microseconds counter_ = 0;
    Microseconds counter2_ = 0;
};


class InventoryState : public MenuState {
public:
    InventoryState(bool fade_in);

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    static int page_;
    static Vec2<u8> selector_coord_;

private:
    static constexpr const Microseconds fade_duration_ = milliseconds(400);

    void update_arrow_icons(Platform& pfrm);
    void update_item_description(Platform& pfrm, Game& game);
    void clear_items();
    void display_items(Platform& pfrm, Game& game);

    std::optional<Border> selector_;
    std::optional<SmallIcon> left_icon_;
    std::optional<SmallIcon> right_icon_;
    std::optional<Text> page_text_;
    std::optional<Text> item_description_;
    std::optional<Text> item_description2_;
    std::optional<Text> label_;
    std::optional<MediumIcon> item_icons_[Inventory::cols][Inventory::rows];

    Microseconds selector_timer_ = 0;
    Microseconds fade_timer_ = 0;
    bool selector_shaded_ = false;
};


// Because pause menus are disabled in multiplayer, at least give players an
// option to select powerups, through a quick-select item sidebar.
class QuickSelectInventoryState : public OverworldState {
public:
    QuickSelectInventoryState(Game& game) : OverworldState(game, true)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    void draw_items(Platform& pfrm, Game& game);

    std::optional<Sidebar> sidebar_;
    std::optional<Border> selector_;

    std::optional<UIMetric> health_;
    std::optional<UIMetric> score_;
    Buffer<UIMetric, Powerup::max_> powerups_;

    static constexpr const int item_display_count_ = 3;
    Buffer<MediumIcon, item_display_count_> item_icons_;
    Buffer<Item::Type, item_display_count_> items_;

    enum class DisplayMode {
        enter,
        show,
        used_item,
        exit
    } display_mode_ = DisplayMode::enter;

    void show_sidebar(Platform& pfrm);

    int used_item_anim_index_ = 0;

    Microseconds timer_ = 0;

    u32 selector_pos_ = 0;

    bool selector_shaded_ = false;

    int page_ = 0;
    bool more_pages_ = false;
};


class NotebookState : public MenuState {
public:
    // NOTE: The NotebookState class does not store a local copy of the text
    // string! Do not pass in pointers to a local buffer, only static strings!
    NotebookState(const char* text);

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    void repaint_page(Platform& pfrm);
    void repaint_margin(Platform& pfrm);

    Microseconds timer_ = 0;

    enum class DisplayMode {
        fade_in,
        show,
        fade_out,
        transition,
        after_transition,
    } display_mode_ = DisplayMode::transition;

    std::optional<TextView> text_;
    std::optional<Text> page_number_;
    const char* str_;
    int page_;
};


class ImageViewState : public MenuState {
public:
    ImageViewState(const char* image_name, ColorConstant background_color);

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    const char* image_name_;
    ColorConstant background_color_;
};


class MapSystemState : public MenuState {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    static constexpr const std::array<LocaleString, 4> legend_strings = {
        LocaleString::map_legend_1,
        LocaleString::map_legend_2,
        LocaleString::map_legend_3,
        LocaleString::map_legend_4};

    Microseconds timer_ = 0;
    enum class AnimState {
        map_enter,
        wp_text,
        legend,
        path_wait,
        wait
    } anim_state_ = AnimState::map_enter;
    std::optional<Text> level_text_;
    int last_column_ = -1;
    std::array<std::optional<Text>, legend_strings.size()> legend_text_;
    std::optional<Border> legend_border_;
    Microseconds map_enter_duration_;

    std::optional<DynamicMemory<IncrementalPathfinder>> path_finder_;
    std::optional<DynamicMemory<PathBuffer>> path_;
};


class QuickMapState : public OverworldState {
public:
    QuickMapState(Game& game) : OverworldState(game, true)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    enum class DisplayMode {
        enter,
        draw,
        path_wait,
        show,
        exit
    } display_mode_ = DisplayMode::enter;

    Microseconds timer_ = 0;

    std::optional<UIMetric> health_;
    std::optional<UIMetric> score_;
    Buffer<UIMetric, Powerup::max_> powerups_;

    int last_map_column_ = -1;

    std::optional<LeftSidebar> sidebar_;
    std::optional<Text> level_text_;

    std::optional<DynamicMemory<IncrementalPathfinder>> path_finder_;
    std::optional<DynamicMemory<PathBuffer>> path_;
};


class IntroCreditsState : public State {
public:
    IntroCreditsState(const char* str) : str_(str)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    virtual StatePtr next_state(Platform& pfrm, Game& game);

private:
    void center(Platform& pfrm);

    const char* str_;
    std::optional<Text> text_;
    Microseconds timer_ = 0;
};


class IntroLegalMessage : public IntroCreditsState {
public:
    IntroLegalMessage()
        : IntroCreditsState(locale_string(LocaleString::intro_text_1))
    {
    }

    StatePtr next_state(Platform& pfrm, Game& game) override;
};


class EndingCreditsState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds timer_ = 0;
    Buffer<Text, 12> lines_; // IMPORTANT: If you're porting this code to a
                             // platform with a taller screen size, you may need
                             // to increase the line capacity here.
    int scroll_ = 0;
    int next_ = 0;
    int next_y_ = 0;
};


class NewLevelState : public State {
public:
    NewLevelState(Level next_level) : timer_(0), next_level_(next_level)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    enum DisplayMode { wait_1, wait_2 } display_mode_ = wait_1;

    std::optional<Text> text_[2];
    Microseconds timer_;
    OverlayCoord pos_;
    Level next_level_;
};


class GoodbyeState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds wait_timer_ = 0;
    std::optional<Text> text_;
};


class PauseScreenState : public MenuState {
public:
    static constexpr const auto fade_duration = milliseconds(400);

    PauseScreenState(bool fade_in = true)
    {
        if (not fade_in) {
            fade_timer_ = fade_duration - milliseconds(1);
        }
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    void draw_cursor(Platform& pfrm);

    bool connect_peer_option_available(Game& game) const;

    Microseconds fade_timer_ = 0;
    Microseconds log_timer_ = 0;
    static int cursor_loc_;
    int anim_index_ = 0;
    Microseconds anim_timer_ = 0;
    std::optional<Text> resume_text_;
    std::optional<Text> connect_peer_text_;
    std::optional<Text> settings_text_;
    std::optional<Text> save_and_quit_text_;
    std::optional<Text> console_text_;
    Float y_offset_ = 0;
};


class LispReplState : public State {
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    enum class DisplayMode {
        entry,
        show_result
    } display_mode_ = DisplayMode::entry;

    Vec2<int> keyboard_cursor_;

    void repaint_entry(Platform& pfrm);

    StringBuffer<28> command_;

    std::optional<Text> keyboard_top_;
    std::optional<Text> keyboard_bottom_;
    Buffer<Text, 7> keyboard_;

    std::optional<Text> version_text_;

    Microseconds timer_ = 0;

    std::optional<Text> entry_;
};


// This is a hidden game state intended for debugging. The user can enter
// various numeric codes, which trigger state changes within the game
// (e.g. jumping to a boss fight/level, spawing specific enemies, setting the
// random seed, etc.)
class CommandCodeState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    bool handle_command_code(Platform& pfrm, Game& game);

    void push_char(Platform& pfrm, Game& game, char c);

    void update_selector(Platform& pfrm, Microseconds dt);

    StringBuffer<10> input_;
    std::optional<Text> input_text_;
    std::optional<Text> numbers_;
    std::optional<Border> entry_box_;
    std::optional<Border> selector_;
    Microseconds selector_timer_ = 0;
    bool selector_shaded_ = false;
    u8 selector_index_ = 0;
    Level next_level_;
};


class LogfileViewerState : public MenuState {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    void repaint(Platform& pfrm, int offset);
    int offset_ = 0;
};


class SignalJammerSelectorState : public MenuState {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    Enemy* make_selector_target(Game& game);

    void print(Platform& pfrm, const char* text);

private:
    enum class Mode {
        fade_in,
        update_selector,
        active,
        selected
    } mode_ = Mode::fade_in;
    int selector_index_ = 0;
    Microseconds timer_;
    Vec2<Float> selector_start_pos_;
    Enemy* target_;
    Camera cached_camera_;
    std::optional<Text> text_;
    int flicker_anim_index_ = 0;
};


class EditSettingsState : public MenuState {
public:
    EditSettingsState(DeferredState exit_state);

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    void refresh(Platform& pfrm, Game& game);

    void draw_line(Platform& pfrm, int row, const char* value);

    std::optional<HorizontalFlashAnimation> message_anim_;
    const char* str_ = nullptr;

    DeferredState exit_state_;

    void message(Platform& pfrm, const char* str);

    class LineUpdater {
    public:
        using Result = StringBuffer<31>;

        virtual ~LineUpdater()
        {
        }
        virtual Result update(Platform&, Game& game, int dir) = 0;
        virtual void complete(Platform&, Game&, EditSettingsState&)
        {
        }
    };

    class ShowStatsLineUpdater : public LineUpdater {
        Result update(Platform&, Game& game, int dir) override
        {
            bool& show = game.persistent_data().settings_.show_stats_;
            if (dir not_eq 0) {
                show = not show;
            }
            if (show) {
                return locale_string(LocaleString::yes);
            } else {
                return locale_string(LocaleString::no);
            }
        }
    } show_stats_line_updater_;

    class NightModeLineUpdater : public LineUpdater {
        Result update(Platform& pfrm, Game& game, int dir) override
        {
            bool& enabled = game.persistent_data().settings_.night_mode_;
            if (dir not_eq 0) {
                enabled = not enabled;
                pfrm.screen().enable_night_mode(enabled);
            }
            if (enabled) {
                return locale_string(LocaleString::yes);
            } else {
                return locale_string(LocaleString::no);
            }
        }
    } night_mode_line_updater_;

    class SwapActionKeysLineUpdater : public LineUpdater {
        Result update(Platform&, Game& game, int dir) override
        {
            if (dir not_eq 0) {
                std::swap(game.persistent_data().settings_.action1_key_,
                          game.persistent_data().settings_.action2_key_);
            }
            if (game.persistent_data().settings_.action1_key_ ==
                Settings::default_action1_key) {
                return locale_string(LocaleString::no);
            } else {
                return locale_string(LocaleString::yes);
            }
        }
    } swap_action_keys_line_updater_;

    class LanguageLineUpdater : public LineUpdater {
        Result update(Platform&, Game& game, int dir) override
        {
            auto& language = game.persistent_data().settings_.language_;
            int l = static_cast<int>(language);

            if (dir > 0) {
                l += 1;
                l %= static_cast<int>(LocaleLanguage::count);
                if (l == 0) {
                    l = 1;
                }
            } else if (dir < 0) {
                if (l > 1) {
                    l -= 1;
                } else if (l == 1) {
                    l = static_cast<int>(LocaleLanguage::count) - 1;
                }
            }

            language = static_cast<LocaleLanguage>(l);

            locale_set_language(language);

            return locale_language_name(language);
        }

        void complete(Platform& pfrm, Game& game, EditSettingsState& s) override
        {
            s.refresh(pfrm, game);
        }
    } language_line_updater_;

    class ContrastLineUpdater : public LineUpdater {
        Result update(Platform& pfrm, Game& game, int dir) override
        {
            auto& contrast = game.persistent_data().settings_.contrast_;

            if (dir > 0) {
                contrast += 1;
            } else if (dir < 0) {
                contrast -= 1;
            }

            contrast = clamp(contrast, Contrast{-25}, Contrast{25});

            pfrm.screen().set_contrast(contrast);

            if (contrast not_eq 0) {
                char buffer[30];
                locale_num2str(contrast, buffer, 10);
                if (contrast > 0) {
                    Result result;
                    result += "+";
                    result += buffer;
                    return result;
                } else {
                    return buffer;
                }
            } else {
                return locale_string(LocaleString::settings_default);
            }
        }
    } contrast_line_updater_;

    class DifficultyLineUpdater : public LineUpdater {

        Result update(Platform&, Game& game, int dir) override
        {
            auto difficulty =
                static_cast<int>(game.persistent_data().settings_.difficulty_);

            // Normally we require all enemies to be defeated to mess with the
            // difficulty. But we don't want the player to get stuck a situation
            // where he/she cannot switch back to easy mode because they're
            // unable to beat the first level on hard mode. So for level zero,
            // allow modifying difficulty.
            if (game.level() == 0 or enemies_remaining(game) == 0) {
                if (dir > 0) {
                    if (difficulty <
                        static_cast<int>(Settings::Difficulty::count) - 1) {
                        difficulty += 1;
                    }
                } else if (dir < 0) {
                    if (difficulty > 0) {
                        difficulty -= 1;
                    }
                }
            }

            game.persistent_data().settings_.difficulty_ =
                static_cast<Settings::Difficulty>(difficulty);

            switch (static_cast<Settings::Difficulty>(difficulty)) {
            case Settings::Difficulty::normal:
                return locale_string(LocaleString::settings_difficulty_normal);

            case Settings::Difficulty::hard:
                return locale_string(LocaleString::settings_difficulty_hard);

            case Settings::Difficulty::survival:
                return locale_string(
                    LocaleString::settings_difficulty_survival);

            case Settings::Difficulty::count:
                break;
            }
            return "";
        }

        void complete(Platform& pfrm, Game& game, EditSettingsState& s) override
        {
            if (game.level() not_eq 0 and enemies_remaining(game)) {
                s.message(pfrm,
                          locale_string(LocaleString::settings_difficulty_err));
            }
        }
    } difficulty_line_updater_;


    struct LineInfo {
        LineUpdater& updater_;
        std::optional<Text> text_ = {};
        int cursor_begin_ = 0;
        int cursor_end_ = 0;
    };

    static constexpr const int line_count_ = 6;

    std::array<LineInfo, line_count_> lines_;

    static constexpr const LocaleString strings[line_count_] = {
        LocaleString::settings_swap_action_keys,
        LocaleString::settings_difficulty,
        LocaleString::settings_language,
        LocaleString::settings_contrast,
        LocaleString::settings_night_mode,
        LocaleString::settings_show_stats,
    };

    std::optional<Text> message_;

    int select_row_ = 0;
    int anim_index_ = 0;
    Microseconds anim_timer_ = 0;
    Float y_offset_ = 0;
};


class NewLevelIdleState : public State, public net_event::Listener {
public:
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    void receive(const net_event::NewLevelIdle&, Platform&, Game&) override;
    void receive(const net_event::NewLevelSyncSeed&, Platform&, Game&) override;

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

private:
    void display_text(Platform& pfrm, LocaleString ls);

    Microseconds timer_ = 0;
    bool peer_ready_ = false;
    int matching_syncs_received_ = 0;
    bool ready_ = false;
    std::optional<Text> text_;
    std::optional<LoadingBar> loading_bar_;
};


class NetworkConnectSetupState : public MenuState {
public:
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds timer_;
};


class NetworkConnectWaitState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    bool ready_ = false;
};


void state_deleter(State* s);


template <typename... States> class StatePool {
public:
    template <typename TState, typename... Args> StatePtr create(Args&&... args)
    {
        static_assert(std::disjunction<std::is_same<TState, States>...>(),
                      "State missing from state pool");

        if (auto mem = pool_.get()) {
            new (mem) TState(std::forward<Args>(args)...);

            return {reinterpret_cast<TState*>(mem), state_deleter};
        } else {

            return null_state();
        }
    }

    Pool<std::max({sizeof(States)...}),
         // We should only need memory for two states at any given time: the
         // current state, and the next state.
         3,
         std::max({alignof(States)...})>
        pool_;
};


using StatePoolInst = StatePool<ActiveState,
                                FadeInState,
                                WarpInState,
                                PreFadePauseState,
                                GlowFadeState,
                                FadeOutState,
                                GoodbyeState,
                                DeathFadeState,
                                InventoryState,
                                NotebookState,
                                ImageViewState,
                                NewLevelState,
                                CommandCodeState,
                                PauseScreenState,
                                LispReplState,
                                QuickMapState,
                                MapSystemState,
                                NewLevelIdleState,
                                EditSettingsState,
                                IntroCreditsState,
                                IntroLegalMessage,
                                DeathContinueState,
                                RespawnWaitState,
                                LogfileViewerState,
                                EndingCreditsState,
                                NetworkConnectSetupState,
                                NetworkConnectWaitState,
                                LaunchCutsceneState,
                                BossDeathSequenceState,
                                QuickSelectInventoryState,
                                SignalJammerSelectorState>;


StatePoolInst& state_pool();


[[noreturn]] void factory_reset(Platform& pfrm);


void repaint_powerups(Platform& pfrm,
                      Game& game,
                      bool clean,
                      std::optional<UIMetric>* health,
                      std::optional<UIMetric>* score,
                      Buffer<UIMetric, Powerup::max_>* powerups,
                      UIMetric::Align align);


void repaint_health_score(Platform& pfrm,
                          Game& game,
                          std::optional<UIMetric>* health,
                          std::optional<UIMetric>* score,
                          UIMetric::Align align);


void update_powerups(Platform& pfrm,
                     Game& game,
                     std::optional<UIMetric>* health,
                     std::optional<UIMetric>* score,
                     Buffer<UIMetric, Powerup::max_>* powerups,
                     UIMetric::Align align);


void update_ui_metrics(Platform& pfrm,
                       Game& game,
                       Microseconds delta,
                       std::optional<UIMetric>* health,
                       std::optional<UIMetric>* score,
                       Buffer<UIMetric, Powerup::max_>* powerups,
                       Entity::Health last_health,
                       Score last_score,
                       UIMetric::Align align);


Vec2<s8> get_constrained_player_tile_coord(Game& game);


// Return true when done drawing the map. Needs lastcolumn variable to store
// display progress. A couple different states share this code--the map system
// state in the pause screen, and the simpler overworld minimap.
//
// FIXME: This function has too many inputs, I wouldn't call this particularly
// great code.
bool draw_minimap(Platform& pfrm,
                  Game& game,
                  Float percentage,
                  int& last_column,
                  int x_start = 1,
                  int y_skip_top = 0,
                  int y_skip_bot = 0,
                  bool force_icons = false,
                  PathBuffer* path = nullptr);


struct InventoryItemHandler {
    Item::Type type_;
    int icon_;
    StatePtr (*callback_)(Platform& pfrm, Game& game);
    LocaleString description_;
    enum { no = 0, yes, custom } single_use_ = no;
};


const InventoryItemHandler* inventory_item_handler(Item::Type type);


void consume_selected_item(Game& game);


static constexpr auto inventory_key = Key::select;
static constexpr auto quick_select_inventory_key = Key::alt_2;
static constexpr auto quick_map_key = Key::alt_1;


// FIXME!!!
extern std::optional<Platform::Keyboard::RestoreState> restore_keystates;
extern Bitmatrix<TileMap::width, TileMap::height> visited;


void player_death(Platform& pfrm, Game& game, const Vec2<Float>& position);


// Sometimes, the creator of a new state wants to inject its own next state into
// the newly created state. For example, let's say we have a credits screen,
// which we want to show if a user selects an option from a pause menu, but also
// when a user beats the game. In once secenario, the next state after the
// credits screen is once again the pause menu state, but in the other scenario,
// the next state is not the pause menu state, becaues we did not arrive in the
// credits screen from the pause menu. Now we could start adding all sorts of
// enumerations to various states, to coordinate which state the next state
// should ultimately produce, but seems easier to simply pass the next state in
// as an argument...
template <typename S, typename... Args>
DeferredState make_deferred_state(Args&&... args)
{
    return [args = std::make_tuple(std::forward<Args>(args)...)] {
        return std::apply(
            [](auto&&... args) { return state_pool().create<S>(args...); },
            std::move(args));
    };
}
