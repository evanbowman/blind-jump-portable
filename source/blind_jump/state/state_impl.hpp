#pragma once

#include "blind_jump/game.hpp"
#include "blind_jump/network_event.hpp"
#include "bulkAllocator.hpp"
#include "graphics/overlay.hpp"
#include "path.hpp"
#include "state.hpp"
#include "version.hpp"


class CommonNetworkListener : public net_event::Listener {
public:
    void receive(const net_event::PlayerEnteredGate&,
                 Platform& pfrm,
                 Game& game) override;

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

    void
    receive(const net_event::BossSwapTarget& t, Platform&, Game& game) override
    {
        game.set_boss_target(t.target_.get());
    }


    void
    receive(const net_event::PlayerDied&, Platform& pfrm, Game& game) override;

    void receive(const net_event::ProgramVersion& vn,
                 Platform& pfrm,
                 Game& game) override;

    void
    receive(const net_event::LethargyActivated&, Platform&, Game&) override;

    void receive(const net_event::Disconnect&, Platform&, Game&) override;
};


class OverworldState : public State, public CommonNetworkListener {
public:
    OverworldState(
        bool camera_tracking,
        std::optional<Settings::CameraMode> camera_mode_override = {})
        : camera_tracking_(camera_tracking),
          camera_mode_override_(camera_mode_override)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    virtual void display_time_remaining(Platform&, Game&);

    std::optional<Text> notification_text;
    NotificationStr notification_str;
    Microseconds notification_text_timer = 0;
    enum class NotificationStatus {
        flash,
        flash_animate,
        wait,
        display,
        exit_row2,
        exit,
        hidden
    } notification_status = NotificationStatus::hidden;

    using net_event::Listener::receive;

    void receive(const net_event::QuickChat&, Platform&, Game&) override;
    void receive(const net_event::NewLevelIdle&, Platform&, Game&) override;
    void receive(const net_event::PlayerSpawnLaser&, Platform&, Game&) override;
    void receive(const net_event::ItemChestShared&, Platform&, Game&) override;
    void receive(const net_event::EnemyStateSync&, Platform&, Game&) override;
    void receive(const net_event::SyncSeed&, Platform&, Game&) override;
    void receive(const net_event::PlayerInfo&, Platform&, Game&) override;
    void
    receive(const net_event::EnemyHealthChanged&, Platform&, Game&) override;
    void
    receive(const net_event::PlayerHealthChanged&, Platform&, Game&) override;

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

    std::optional<Settings::CameraMode> camera_mode_override_;
    Microseconds fps_timer_ = 0;
    int fps_frame_count_ = 0;
    std::optional<Text> fps_text_;
    std::optional<Text> network_tx_msg_text_;
    std::optional<Text> network_rx_msg_text_;
    std::optional<Text> network_tx_loss_text_;
    std::optional<Text> network_rx_loss_text_;
    std::optional<Text> link_saturation_text_;
    std::optional<Text> scratch_buf_avail_text_;
    std::optional<Text> time_remaining_text_;
    std::optional<SmallIcon> time_remaining_icon_;
    int idle_rx_count_ = 0;
};


class BossDeathSequenceState : public OverworldState {
public:
    BossDeathSequenceState(Game& game,
                           const Vec2<Float>& boss_position,
                           LocaleString boss_defeated_text)
        : OverworldState(false), boss_position_(boss_position),
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
        fade,
        endgame,
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


class TitleScreenState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    std::optional<Text> title_;
    std::optional<Text> options_[2];

    std::optional<Sidebar> sidebar_;
    std::optional<LeftSidebar> sidebar2_;

    int bossrush_cheat_counter_ = 0;

    enum class DisplayMode {
        sleep,
        fade_in,
        wait,
        select,
        image_animate_out,
        swap_image,
        image_animate_in,
        fade_out,
        pause,
    } display_mode_ = DisplayMode::sleep;

    Microseconds timer_ = 0;
    u8 cursor_index_ = 0;
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
    ActiveState(bool camera_tracking = true) : OverworldState(camera_tracking)
    {
    }
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    static constexpr const int boss_health_bar_count = 2;
    std::optional<BossHealthBar> boss_health_bar_[boss_health_bar_count];

private:
    std::optional<UIMetric> health_;
    std::optional<UIMetric> score_;

    Buffer<UIMetric, Powerup::max_> powerups_;
    std::optional<MediumIcon> dodge_ready_;

    bool pixelated_ = false;
};


class FadeInState : public OverworldState {
public:
    FadeInState(Game& game) : OverworldState(false)
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
    WarpInState(Game& game) : OverworldState(true)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
    bool shook_ = false;
};


class PreFadePauseState : public OverworldState {
public:
    PreFadePauseState(Game& game, ColorConstant fade_color)
        : OverworldState(false), fade_color_(fade_color)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    ColorConstant fade_color_;
    Microseconds timer_ = 0;
};


class GlowFadeState : public OverworldState {
public:
    GlowFadeState(Game& game, ColorConstant color)
        : OverworldState(false), color_(color)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
    Microseconds timer_ = 0;
    ColorConstant color_;
};


class FadeOutState : public OverworldState {
public:
    FadeOutState(Game& game, ColorConstant color)
        : OverworldState(false), color_(color)
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
    DeathFadeState(Game& game) : OverworldState(false)
    {
    }
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    Microseconds counter_ = 0;
};


class ScoreScreenState : public State {
public:
    ScoreScreenState(s8 metrics_y_offset = 0,
                     const Text::OptColors& metric_font_colors = {})
        : metrics_y_offset_(metrics_y_offset),
          metric_font_colors_(metric_font_colors)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

protected:
    void repaint_stats(Platform&, Game&);

    void clear_stats(Platform&);

    bool locked() const
    {
        return locked_;
    }

    bool has_lines() const
    {
        return lines_.empty();
    }

private:
    s8 metrics_y_offset_;
    bool locked_ = false;

    Buffer<Text, 5> lines_;

    int page_ = 0;
    u32 playtime_seconds_ = 0;
    Level max_level_ = 0;

    Text::OptColors metric_font_colors_;
};


class DeathContinueState : public ScoreScreenState {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
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
    QuickSelectInventoryState(Game& game)
        : OverworldState(true, Settings::CameraMode::tracking_strong)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    void display_time_remaining(Platform&, Game&) override;

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

    u8 used_item_anim_index_ = 0;
    u8 selector_pos_ = 0;
    u8 page_ = 0;
    bool selector_shaded_ = false;
    bool more_pages_ = false;

    Microseconds timer_ = 0;
};


class QuickChatState : public OverworldState {
public:
    QuickChatState() : OverworldState(true)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    void display_time_remaining(Platform&, Game&) override;

private:
    void update_text(Platform& pfrm, Game& game);

    int msg_index_ = 0;
    std::optional<Text> text_;
};


class NotebookState : public MenuState {
public:
    NotebookState(LocalizedText&& str);

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
    LocalizedText str_;
    int page_;
};


class ImageViewState : public MenuState {
public:
    ImageViewState(const StringBuffer<48>& image_name,
                   ColorConstant background_color);

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    StringBuffer<48> image_name_;
    ColorConstant background_color_;
};


class EndingCutsceneState : public ScoreScreenState {
public:
    EndingCutsceneState() : ScoreScreenState(-4)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    enum class AnimState {
        fade_in,
        wait1,
        hold,
        wait2,
        fade_out,
    } anim_state_ = AnimState::fade_in;

    Microseconds counter_ = 0;
    Microseconds anim_counter_ = 0;
    int anim_index_ = 0;
};


class MapSystemState : public MenuState {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    static constexpr const std::array<LocaleString, 5> legend_strings = {
        LocaleString::map_legend_1,
        LocaleString::map_legend_2,
        LocaleString::map_legend_3,
        LocaleString::map_legend_4,
        LocaleString::map_legend_5};

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
    Microseconds map_enter_duration_ = 0;

    std::optional<DynamicMemory<IncrementalPathfinder>> path_finder_;
    std::optional<DynamicMemory<PathBuffer>> path_;
};


class QuickMapState : public OverworldState {
public:
    QuickMapState(Game& game)
        : OverworldState(true, Settings::CameraMode::tracking_strong)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    void display_time_remaining(Platform& pfrm, Game& game) override;

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


class HealthAndSafetyWarningState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    enum class DisplayMode {
        wait,
        fade_out,
        swap_texture,
        exit,
    } display_mode_ = DisplayMode::wait;

    Microseconds timer_ = 0;

    std::optional<Text> text_;
    std::optional<Text> continue_text_;
    std::optional<TextView> tv_;
};


class IntroCreditsState : public State {
public:
    IntroCreditsState(LocalizedText&& str) : str_(std::move(str))
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    virtual StatePtr next_state(Platform& pfrm, Game& game);

    static Microseconds music_offset();

private:
    void center(Platform& pfrm);

    void show_version(Platform& pfrm, Game& game);

    LocalizedText str_;
    std::optional<Text> creator_;
    std::optional<Text> text_;
    std::optional<Text> translator_;
    std::optional<Text> version_;
    Microseconds timer_ = 0;
};


class EndingCreditsState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    enum class DisplayMode {
        scroll,
        scroll2,
        draw_image,
        fade_show_image,
        show_image,
        fade_out,
        done
    } display_mode_ = DisplayMode::scroll;

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
    void exit(Platform& pfrm, Game& game, State& next_state) override;

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

    void erase_cursor(Platform& pfrm);
    void draw_cursor_image(Platform& pfrm, Text* target, int tile1, int tile2);

    bool connect_peer_option_available(Game& game) const;

    void repaint_text(Platform&, Game&);

    Microseconds fade_timer_ = 0;
    Microseconds log_timer_ = 0;
    static int cursor_loc_;
    int anim_index_ = 0;
    int developer_mode_activation_counter_ = 0;
    Microseconds anim_timer_ = 0;

    Buffer<LocaleString, 5> strs_;
    Buffer<Text, 5> texts_;
    Float y_offset_ = 0;
};


class LispReplState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    using Command = StringBuffer<256>;

private:
    enum class DisplayMode {
        entry,
        show_result,
        completion_list,
    } display_mode_ = DisplayMode::entry;

    Vec2<int> keyboard_cursor_;

    void repaint_entry(Platform& pfrm, bool show_cursor = true);

    void repaint_completions(Platform& pfrm);


    Command command_;

    std::optional<Text> keyboard_top_;
    std::optional<Text> keyboard_bottom_;
    Buffer<Text, 7> keyboard_;

    std::optional<Text> version_text_;

    static constexpr const int completion_count = 10;
    Buffer<const char*, completion_count> completion_strs_;
    Buffer<Text, completion_count> completions_;
    u8 completion_cursor_ = 0;
    u8 completion_prefix_len_ = 0;

    Microseconds timer_ = 0;

    std::optional<Text> entry_;
};


class RemoteReplState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;
};


// This is a hidden game state intended for debugging. The user can enter
// various numeric codes, which trigger state changes within the game
// (e.g. jumping to a boss fight/level, spawing specific enemies, setting the
// random seed, etc.)
//
// Update: The game now has a lisp interpreter built in, so the command code
// state is currently unused. A neat relic from early in the project's
// development.
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

    Enemy* make_selector_target(Platform&, Game& game);

    void print(Platform& pfrm, const char* text);

private:
    enum class Mode {
        fade_in,
        update_selector,
        active,
        selected
    } mode_ = Mode::fade_in;
    int selector_index_ = 0;
    Microseconds timer_ = 0;
    Vec2<Float> selector_start_pos_;
    Enemy* target_ = nullptr;
    Camera cached_camera_;
    std::optional<Text> text_;
    int flicker_anim_index_ = 0;
};


class DialogState : public OverworldState {
public:
    DialogState(DeferredState exit_state, const LocaleString* text)
        : OverworldState(true), exit_state_(exit_state), text_(text)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    void display_time_remaining(Platform& pfrm, Game& game) override;

private:
    DeferredState exit_state_;

    const LocaleString* text_;

    struct TextWriterState {
        std::optional<LocalizedText> text_;
        const char* current_word_;
        Microseconds timer_;
        u8 line_;
        u8 pos_;
        u8 current_word_remaining_;
    };

    TextWriterState text_state_;

    void clear_textbox(Platform& pfrm);

    // Return false when the textbox has no more room to print additional
    // glyphs, otherwise, return true.
    bool advance_text(Platform& pfrm,
                      Game& game,
                      Microseconds delta,
                      bool sfx = true);

    bool advance_asian_text(Platform& pfrm,
                            Game& game,
                            Microseconds delta,
                            bool sfx);

    void init_text(Platform& pfrm, LocaleString str);

    enum class DisplayMode {
        animate_in,
        busy,
        key_released_check1,
        key_released_check2,
        wait,
        done,
        animate_out,
        clear,
    } display_mode_ = DisplayMode::animate_in;

    // NOTE: We needed to add this code when we added support for
    // Chinese. Chinese is not space-delimited like english, so we need
    // completely different logic to print glyphs.
    bool asian_language_ = false;
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

    DeferredState exit_state_;

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
        Result update(Platform& pfrm, Game& game, int dir) override
        {
            bool& show = game.persistent_data().settings_.show_stats_;
            if (dir not_eq 0) {
                show = not show;
            }
            if (show) {
                return locale_string(pfrm, LocaleString::yes)->c_str();
            } else {
                return locale_string(pfrm, LocaleString::no)->c_str();
            }
        }
    } show_stats_line_updater_;

    class RumbleEnabledLineUpdater : public LineUpdater {
        Result update(Platform& pfrm, Game& game, int dir) override
        {
            bool& enabled = game.persistent_data().settings_.rumble_enabled_;
            if (dir not_eq 0) {
                enabled = not enabled;
            }
            if (enabled) {
                return locale_string(pfrm, LocaleString::yes)->c_str();
            } else {
                return locale_string(pfrm, LocaleString::no)->c_str();
            }
        }
    } rumble_enabled_line_updater_;

    class SpeedrunClockLineUpdater : public LineUpdater {
        Result update(Platform& pfrm, Game& game, int dir) override
        {
            bool& show = game.persistent_data().settings_.show_speedrun_clock_;
            if (dir not_eq 0) {
                show = not show;
            }
            if (show) {
                return locale_string(pfrm, LocaleString::yes)->c_str();
            } else {
                return locale_string(pfrm, LocaleString::no)->c_str();
            }
        }
    } speedrun_clock_line_updater_;

    class NightModeLineUpdater : public LineUpdater {
        Result update(Platform& pfrm, Game& game, int dir) override
        {
            bool& enabled = game.persistent_data().settings_.night_mode_;
            if (dir not_eq 0) {
                enabled = not enabled;
                pfrm.screen().enable_night_mode(enabled);
            }
            if (enabled) {
                return locale_string(pfrm, LocaleString::yes)->c_str();
            } else {
                return locale_string(pfrm, LocaleString::no)->c_str();
            }
        }
    } night_mode_line_updater_;

    class SwapActionKeysLineUpdater : public LineUpdater {
        Result update(Platform& pfrm, Game& game, int dir) override
        {
            if (dir not_eq 0) {
                std::swap(game.persistent_data().settings_.action1_key_,
                          game.persistent_data().settings_.action2_key_);
            }
            if (game.persistent_data().settings_.action1_key_ ==
                Settings::default_action1_key) {
                return locale_string(pfrm, LocaleString::no)->c_str();
            } else {
                return locale_string(pfrm, LocaleString::yes)->c_str();
            }
        }
    } swap_action_keys_line_updater_;

    class CameraModeLineUpdater : public LineUpdater {
        Result update(Platform& pfrm, Game& game, int dir) override
        {
            auto& current = game.persistent_data().settings_.camera_mode_;

            auto val = static_cast<int>(current);

            if (dir > 0) {
                val = val + 1;
                if (val == static_cast<int>(Settings::CameraMode::count)) {
                    val = 0;
                }
            } else if (dir < 0) {
                val = std::max(0, val - 1);
            }

            current = static_cast<Settings::CameraMode>(val);

            switch (current) {
            case Settings::CameraMode::count:
                break;

            case Settings::CameraMode::fixed:
                return locale_string(pfrm, LocaleString::settings_camera_fixed)
                    ->c_str();
                break;

            case Settings::CameraMode::tracking_weak:
                return locale_string(
                           pfrm, LocaleString::settings_camera_tracking_weak)
                    ->c_str();
                break;

            case Settings::CameraMode::tracking_strong:
                return locale_string(
                           pfrm, LocaleString::settings_camera_tracking_strong)
                    ->c_str();
                break;
            }

            return "LOGIC ERR";
        }
    } camera_mode_line_updater_;

    class StrafeModeLineUpdater : public LineUpdater {
        Result update(Platform& pfrm, Game& game, int dir) override
        {
            auto& current = game.persistent_data().settings_.button_mode_;

            auto val = static_cast<int>(current);

            if (dir > 0) {
                val = std::min(
                    static_cast<int>(Settings::ButtonMode::count) - 1, val + 1);
            } else if (dir < 0) {
                val = std::max(0, val - 1);
            }

            current = static_cast<Settings::ButtonMode>(val);

            switch (current) {
            case Settings::ButtonMode::count:
            case Settings::ButtonMode::strafe_separate:
                return locale_string(pfrm,
                                     LocaleString::settings_strafe_key_separate)
                    ->c_str();

            case Settings::ButtonMode::strafe_combined:
                return locale_string(pfrm,
                                     LocaleString::settings_strafe_key_combined)
                    ->c_str();
            }

            return "";
        }
    } strafe_mode_line_updater_;

    class LanguageLineUpdater : public LineUpdater {
        Result update(Platform& pfrm, Game& game, int dir) override;

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
                return locale_string(pfrm, LocaleString::settings_default)
                    ->c_str();
            }
        }
    } contrast_line_updater_;

    class DifficultyLineUpdater : public LineUpdater {

        Result update(Platform& pfrm, Game& game, int dir) override
        {
            auto difficulty =
                static_cast<int>(game.persistent_data().settings_.difficulty_);

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

            game.persistent_data().settings_.difficulty_ =
                static_cast<Settings::Difficulty>(difficulty);

            switch (static_cast<Settings::Difficulty>(difficulty)) {
            case Settings::Difficulty::easy:
                return locale_string(pfrm,
                                     LocaleString::settings_difficulty_easy)
                    ->c_str();

            case Settings::Difficulty::normal:
                return locale_string(pfrm,
                                     LocaleString::settings_difficulty_normal)
                    ->c_str();

            case Settings::Difficulty::hard:
                return locale_string(pfrm,
                                     LocaleString::settings_difficulty_hard)
                    ->c_str();

            case Settings::Difficulty::survival:
                return locale_string(pfrm,
                                     LocaleString::settings_difficulty_survival)
                    ->c_str();

            case Settings::Difficulty::count:
                break;
            }
            return "";
        }

        void complete(Platform& pfrm, Game& game, EditSettingsState& s) override
        {
        }
    } difficulty_line_updater_;


    struct LineInfo {
        LineUpdater& updater_;
        std::optional<Text> text_ = {};
        int cursor_begin_ = 0;
        int cursor_end_ = 0;
    };

    static constexpr const int line_count_ = 9;

    std::array<LineInfo, line_count_> lines_;

    static constexpr const LocaleString strings[line_count_] = {
        LocaleString::settings_language,
        LocaleString::settings_swap_action_keys,
        LocaleString::settings_strafe_key,
        LocaleString::settings_camera,
        LocaleString::settings_difficulty,
        LocaleString::settings_contrast,
        LocaleString::settings_night_mode,
        LocaleString::settings_speedrun_clock,
        LocaleString::settings_rumble_enabled};

    int select_row_ = 0;
    int anim_index_ = 0;
    Microseconds anim_timer_ = 0;
    Float y_offset_ = 0;

    bool exit_ = false;
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


class MultiplayerReviveWaitingState : public OverworldState {
public:
    MultiplayerReviveWaitingState(const Vec2<Float>& player_death_pos)
        : OverworldState(true), player_death_pos_(player_death_pos), timer_(0),
          display_mode_(DisplayMode::camera_pan)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    using net_event::Listener::receive;

    void receive(const net_event::PlayerEnteredGate&,
                 Platform& pfrm,
                 Game& game) override;

    void receive(const net_event::HealthTransfer&,
                 Platform& pfrm,
                 Game& game) override;

private:
    Vec2<Float> player_death_pos_;
    Camera camera_;
    Microseconds timer_;
    enum class DisplayMode {
        camera_pan,
        track_peer,
    } display_mode_;
};


class MultiplayerReviveState : public OverworldState {
public:
    MultiplayerReviveState() : OverworldState(true)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    void show_menu(Platform& pfrm);

    enum class DisplayMode {
        fade_in,
        menu,
        fade_out
    } display_mode_ = DisplayMode::fade_in;

    std::optional<SmallIcon> heart_icon_;
    std::optional<Text> text_;
    std::optional<UIMetric> health_;
    std::optional<UIMetric> score_;
    Buffer<UIMetric, Powerup::max_> powerups_;
    int donate_health_count_ = 0;
};


class ItemShopState : public OverworldState {
private:
    enum class DisplayMode {
        wait,
        animate_in_buy,
        animate_in_sell,
        show_buy,
        inflate_buy_options,
        show_buy_options,
        deflate_buy_options,
        to_sell_menu,
        show_sell,
        inflate_sell_options,
        show_sell_options,
        deflate_sell_options,
        to_buy_menu,
        exit_left,
        exit_right,
    };

public:
    ItemShopState() : OverworldState(false)
    {
    }

    // When the users selects the item info button, the item shop state exits,
    // and transitions to a dialog state, which holds the parameters needed to
    // recreate the item shop state when the dialog finishes.
    ItemShopState(DisplayMode mode, int selector_pos, int page)
        : OverworldState(false)
    {
        display_mode_ = mode;
        selector_pos_ = selector_pos;

        if (mode == DisplayMode::animate_in_buy) {
            buy_page_num_ = page;
        } else if (mode == DisplayMode::animate_in_sell) {
            sell_page_num_ = page;
        } else {
            // error in program logic.
            while (true)
                ;
        }
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    virtual void display_time_remaining(Platform&, Game&) override;

private:
    DisplayMode display_mode_ = DisplayMode::wait;

    void show_sidebar(Platform& pfrm);

    void show_sell_icons(Platform& pfrm, Game& game);
    void show_buy_icons(Platform& pfrm, Game& game);

    void show_label(Platform& pfrm,
                    Game& game,
                    const char* str,
                    bool anchor_right,
                    bool append_coin_icon = false);

    void hide_label(Platform& pfrm);

    void show_score(Platform& pfrm, Game& game, UIMetric::Align align);

    void show_sell_option_label(Platform& pfrm, Game& game);
    void show_buy_option_label(Platform& pfrm, Game& game);

    Microseconds timer_ = 0;

    bool selector_shaded_ = false;

    int selector_pos_ = 0;
    int selector_x_ = 0;
    std::optional<Border> selector_;

    std::optional<UIMetric> score_;

    int buy_page_num_ = 0;
    int sell_page_num_ = 0;

    bool sell_items_remaining_ = false;
    bool buy_items_remaining_ = false;

    std::optional<LeftSidebar> buy_item_bar_;
    std::optional<Sidebar> sell_item_bar_;

    std::optional<LeftSidebar> buy_options_bar_;
    std::optional<Sidebar> sell_options_bar_;

    std::optional<Text> buy_sell_text_;

    std::optional<Text> heading_text_;

    std::optional<SmallIcon> coin_icon_;

    static constexpr const int item_display_count_ = 3;
    Buffer<MediumIcon, item_display_count_> buy_item_icons_;
    Buffer<MediumIcon, item_display_count_> sell_item_icons_;

    std::optional<MediumIcon> buy_sell_icon_;
    std::optional<MediumIcon> info_icon_;
};


class LanguageSelectionState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Buffer<Text, 10> languages_;
    int cursor_loc_ = 0;
    Microseconds cursor_anim_timer_;
    int anim_index_ = 0;
    Float y_offset_ = 0;
};


void state_deleter(State* s);


template <typename... States> class StatePool {
public:
    template <typename TState, typename... Args> StatePtr create(Args&&... args)
    {
        static_assert(std::disjunction<std::is_same<TState, States>...>(),
                      "State missing from state pool");

#ifdef __GBA__
        static_assert(std::max({sizeof(States)...}) < 800,
                      "Note: this is merely a warning. You are welcome to "
                      "increase the overall size of the state pool, just be "
                      "careful, as the state cells are already quite large, in"
                      " number of bytes.");
#endif // __GBA__

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
                                DialogState,
                                PreFadePauseState,
                                GlowFadeState,
                                FadeOutState,
                                GoodbyeState,
                                DeathFadeState,
                                InventoryState,
                                QuickChatState,
                                NotebookState,
                                ImageViewState,
                                NewLevelState,
                                CommandCodeState,
                                TitleScreenState,
                                PauseScreenState,
                                LispReplState,
                                RemoteReplState,
                                ItemShopState,
                                QuickMapState,
                                MapSystemState,
                                NewLevelIdleState,
                                EditSettingsState,
                                IntroCreditsState,
                                DeathContinueState,
                                RespawnWaitState,
                                LogfileViewerState,
                                EndingCreditsState,
                                NetworkConnectSetupState,
                                NetworkConnectWaitState,
                                LaunchCutsceneState,
                                EndingCutsceneState,
                                BossDeathSequenceState,
                                QuickSelectInventoryState,
                                SignalJammerSelectorState,
                                HealthAndSafetyWarningState,
                                MultiplayerReviveState,
                                MultiplayerReviveWaitingState,
                                LanguageSelectionState>;


StatePoolInst& state_pool();


[[noreturn]] void factory_reset(Platform& pfrm);


void repaint_powerups(Platform& pfrm,
                      Game& game,
                      bool clean,
                      std::optional<UIMetric>* health,
                      std::optional<UIMetric>* score,
                      std::optional<MediumIcon>* dodge,
                      Buffer<UIMetric, Powerup::max_>* powerups,
                      UIMetric::Align align);


void repaint_health_score(Platform& pfrm,
                          Game& game,
                          std::optional<UIMetric>* health,
                          std::optional<UIMetric>* score,
                          std::optional<MediumIcon>* dodge,
                          UIMetric::Align align);


void update_powerups(Platform& pfrm,
                     Game& game,
                     std::optional<UIMetric>* health,
                     std::optional<UIMetric>* score,
                     std::optional<MediumIcon>* dodge,
                     Buffer<UIMetric, Powerup::max_>* powerups,
                     UIMetric::Align align);


void update_ui_metrics(Platform& pfrm,
                       Game& game,
                       Microseconds delta,
                       std::optional<UIMetric>* health,
                       std::optional<UIMetric>* score,
                       std::optional<MediumIcon>* dodge,
                       Buffer<UIMetric, Powerup::max_>* powerups,
                       Entity::Health last_health,
                       Score last_score,
                       bool dodge_was_ready,
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
                  int y_start = 1,
                  int y_skip_top = 0,
                  int y_skip_bot = 0,
                  bool force_icons = false,
                  PathBuffer* path = nullptr);


StringBuffer<32> format_time(u32 seconds, bool include_hours = true);


struct InventoryItemHandler {
    Item::Type type_;
    int icon_;
    StatePtr (*callback_)(Platform& pfrm, Game& game);
    LocaleString description_;
    LocaleString scavenger_dialog_[2] = {LocaleString::sc_dialog_skip,
                                         LocaleString::empty};
    enum { no = 0, yes, custom } single_use_ = no;
};


const InventoryItemHandler* inventory_item_handler(Item::Type type);


void consume_selected_item(Game& game);


static constexpr auto inventory_key = Key::select;
static constexpr auto quick_select_inventory_key = Key::alt_2;
static constexpr auto quick_map_key = Key::alt_1;


// FIXME!!!
extern std::optional<Platform::Keyboard::RestoreState> restore_keystates;


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
