#include "game.hpp"
#include "bulkAllocator.hpp"
#include "function.hpp"
#include "globals.hpp"
#include "graphics/overlay.hpp"
#include "number/random.hpp"
#include "path.hpp"
#include "script/lisp.hpp"
#include "string.hpp"
#include "util.hpp"
#include <algorithm>
#include <iterator>
#include <type_traits>


bool within_view_frustum(const Platform::Screen& screen,
                         const Vec2<Float>& pos);


bool Game::load_save_data(Platform& pfrm)
{
    alignas(PersistentData) u8 save_buffer[sizeof(PersistentData)] = {0};

    if (pfrm.read_save_data(save_buffer, sizeof(PersistentData), 0)) {
        PersistentData* loaded = (PersistentData*)save_buffer;
        if (loaded->magic_.get() == PersistentData::magic_val) {
            info(pfrm, "loaded existing save file");
            persistent_data_ = *loaded;

            return true;
        }
    }

    return false;
}


void newgame(Platform& pfrm, Game& game)
{
    info(pfrm, "constructing new game...");

    // Except for highscores and settings, we do not want to keep anything in
    // the old save data.
    PersistentData::HighScores highscores;
    std::copy(std::begin(game.persistent_data().highscores_),
              std::end(game.persistent_data().highscores_),
              std::begin(highscores));

    const auto settings = game.persistent_data().settings_;

    game.persistent_data() = PersistentData{};
    game.persistent_data().inventory_.push_item(
        pfrm, game, Item::Type::blaster, false);

    std::copy(std::begin(highscores),
              std::end(highscores),
              std::begin(game.persistent_data().highscores_));

    game.persistent_data().settings_ = settings;

    pfrm.write_save_data(&game.persistent_data(), sizeof(PersistentData), 0);

    game.player().set_health(game.persistent_data().player_health_.get());
    game.score() = 0;
    game.inventory() = game.persistent_data().inventory_;
    game.powerups().clear();
}


Game::Game(Platform& pfrm)
    : player_(pfrm),
      enemies_(std::get<BlindJumpGlobalData>(globals()).enemy_pool_,
               std::get<BlindJumpGlobalData>(globals()).enemy_node_pool_),
      details_(std::get<BlindJumpGlobalData>(globals()).detail_pool_,
               std::get<BlindJumpGlobalData>(globals()).detail_node_pool_),
      effects_(std::get<BlindJumpGlobalData>(globals()).effect_pool_,
               std::get<BlindJumpGlobalData>(globals()).effect_node_pool_),
      score_(0), next_state_(null_state()), state_(null_state()),
      boss_target_(0)
{
    if (not this->load_save_data(pfrm)) {
        info(pfrm, "no save file found");
        newgame(pfrm, *this);
        if (auto tm = pfrm.startup_time()) {
            persistent_data_.timestamp_ = *tm;
        }
    } else {
        if (auto tm = pfrm.startup_time()) {
            StringBuffer<80> str = "save file age: ";

            char buffer[30];
            english__to_string(
                time_diff(persistent_data_.timestamp_, *tm), buffer, 10);

            str += buffer;
            str += "sec";

            info(pfrm, str.c_str());
        }
    }

    player_.set_health(persistent_data_.player_health_.get());
    score_ = persistent_data_.score_.get();
    inventory_ = persistent_data_.inventory_;

    for (u32 i = 0; i < persistent_data_.powerup_count_.get(); ++i) {
        powerups_.push_back(persistent_data_.powerups_[i]);
    }

    rng::critical_state = persistent_data_.seed_.get();

    init_script(pfrm);

    if (auto eval_opt = pfrm.get_opt('e')) {
        lisp::dostring(eval_opt);
    }

    lisp::dostring(pfrm.load_file_contents("scripts", "init.lisp"));

    pfrm.logger().set_threshold(persistent_data_.settings_.log_severity_);

    pfrm.screen().enable_night_mode(persistent_data_.settings_.night_mode_);

    locale_set_language(persistent_data_.settings_.language_.get());

    state_ = State::initial(pfrm, *this);

    pfrm.load_overlay_texture("overlay");


    // NOTE: Because we're the initial state, unclear what to pass as a previous
    // state to the enter function, so, paradoxically, the initial state is it's
    // own parent.
    state_->enter(pfrm, *this, *state_);

    pfrm.on_watchdog_timeout([this](Platform& pfrm) {
        error(pfrm,
              "update loop stuck for unknown reason, system reset by watchdog");

        // Not sure what else to do... but at least if the code breaks because
        // we got stuck in a loop, we can return the user to where they left
        // off...
        pfrm.write_save_data(
            (byte*)&persistent_data_, sizeof persistent_data_, 0);
    });
}


class RemoteConsoleLispPrinter : public lisp::Printer {
public:
    RemoteConsoleLispPrinter(Platform& pfrm) : pfrm_(pfrm)
    {
    }

    void put_str(const char* str) override
    {
        fmt_ += str;
    }

    Platform::RemoteConsole::Line fmt_;
    Platform& pfrm_;
};


COLD void on_remote_console_text(Platform& pfrm,
                                 const Platform::RemoteConsole::Line& str)
{
    info(pfrm, str.c_str());

    RemoteConsoleLispPrinter printer(pfrm);

    lisp::read(str.c_str());
    lisp::eval(lisp::get_op(0));
    format(lisp::get_op(0), printer);

    lisp::pop_op();
    lisp::pop_op();


    pfrm.remote_console().printline(printer.fmt_.c_str());
}


HOT void Game::update(Platform& pfrm, Microseconds delta)
{
    if (next_state_) {
        next_state_->enter(pfrm, *this, *state_);

        state_ = std::move(next_state_);
    }

    rumble_.update(pfrm, delta);

    auto line = pfrm.remote_console().readline();
    if (UNLIKELY(static_cast<bool>(line))) {
        on_remote_console_text(pfrm, *line);
    }

    for (auto it = deferred_callbacks_.begin();
         it not_eq deferred_callbacks_.end();) {

        it->second -= delta;

        if (not(it->second > 0)) {
            it->first(pfrm, *this);
            it = deferred_callbacks_.erase(it);
        } else {
            ++it;
        }
    }

    pfrm.speaker().set_position(
        {camera_.center().x + pfrm.screen().size().x / 2,
         camera_.center().y + pfrm.screen().size().y / 2});

    next_state_ = state_->update(pfrm, *this, delta);

    if (next_state_) {
        state_->exit(pfrm, *this, *next_state_);
    }
}


void Game::rumble(Platform& pfrm, Microseconds duration)
{
    if (persistent_data_.settings_.rumble_enabled_) {
        rumble_.activate(pfrm, duration);
    }
}


static void show_offscreen_player_icon(Platform& pfrm, Game& game)
{
    // Basically, this code draws an imaginary line between the center of the
    // window, and the coordinate of the offscreen player character. The
    // intersection point between the edge of the screen and the imaginary line
    // represents the position to draw the arrow... more or less.

    const auto peer_pos = game.peer()->get_position().cast<int>();

    Sprite arrow_spr;
    arrow_spr.set_texture_index(119);
    arrow_spr.set_size(Sprite::Size::w16_h32);

    const auto view_size = pfrm.screen().get_view().get_size().cast<int>();

    const auto view_tl = pfrm.screen().get_view().get_center().cast<int>();
    const auto view_center = view_tl + view_size.cast<int>() / 2;

    const auto dy = view_center.y - peer_pos.y;
    const auto dx = view_center.x - peer_pos.x;

    if (dx == 0) {
        return;
    }

    const auto slope = Float(dy) / dx;

    const int center_y = view_size.y / 2;
    const int center_x = view_size.x / 2;

    if (-center_y <= slope * center_x and slope * center_x <= center_y) {
        if (view_center.x < peer_pos.x) {
            const auto y =
                clamp(peer_pos.y, view_tl.y, view_tl.y + view_size.y - 32);
            arrow_spr.set_position(
                {Float(view_tl.x + view_size.x - 24), Float(y)});
            // right edge
            arrow_spr.set_rotation((std::numeric_limits<s16>::max() * 3) / 4);
        } else {
            const auto y =
                clamp(peer_pos.y, view_tl.y, view_tl.y + view_size.y - 32);
            arrow_spr.set_position({Float(view_tl.x + 8), Float(y)});
            // left edge
            arrow_spr.set_rotation((std::numeric_limits<s16>::max() * 1) / 4);
        }
    } else if (-center_x <= center_y / slope and center_y / slope <= center_x) {
        if (view_center.y < peer_pos.y) {
            const auto x =
                clamp(peer_pos.x, view_tl.x, view_tl.x + view_size.x - 32);
            arrow_spr.set_position(
                {Float(x), Float(view_tl.y + view_size.y - 32)});
            arrow_spr.set_rotation(std::numeric_limits<s16>::max() / 2);
        } else {
            const auto x =
                clamp(peer_pos.x, view_tl.x, view_tl.x + view_size.x - 32);
            arrow_spr.set_position({Float(x), Float(view_tl.y)});
            arrow_spr.set_rotation(0);
        }
    }

    pfrm.screen().draw(arrow_spr);
}


HOT void Game::render(Platform& pfrm)
{
    Buffer<const Sprite*, Platform::Screen::sprite_limit> display_buffer;

    Buffer<const Sprite*, 30> shadows_buffer;

    auto show_sprite = [&](auto& e) {
        if (within_view_frustum(pfrm.screen(), e.get_sprite().get_position())) {
            using T = typename std::remove_reference<decltype(e)>::type;

            if constexpr (T::has_shadow) {
                if constexpr (T::multiface_shadow) {
                    for (const auto& spr : e.get_shadow()) {
                        shadows_buffer.push_back(spr);
                    }
                } else {
                    shadows_buffer.push_back(&e.get_shadow());
                }
            }

            if constexpr (T::multiface_sprite) {
                for (const auto& spr : e.get_sprites()) {
                    display_buffer.push_back(spr);
                }
            } else {
                display_buffer.push_back(&e.get_sprite());
            }

            e.mark_visible(true);
        } else {
            e.mark_visible(false);
        }
    };

    auto show_sprites = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end(); ++it) {
            show_sprite(**it);
        }
    };

    // Draw the effects first. Effects are not subject to z-ordering like
    // overworld objects, therefore, faster to draw.

    effects_.transform([&](auto& entity_buf) {
        using T = typename std::remove_reference<decltype(entity_buf)>::type;

        using VT = typename T::ValueType::element_type;

        if constexpr (not std::is_same<VT, DynamicEffect>() and
                      not std::is_same<VT, StaticEffect>()) {
            show_sprites(entity_buf);
        } else {
            for (auto& e : entity_buf) {
                if (e->is_backdrop()) {
                    // defer rendering...
                } else {
                    show_sprite(*e);
                }
            }
        }
    });

    // effects_.transform(show_sprites);
    for (auto spr : display_buffer) {
        pfrm.screen().draw(*spr);
    }
    display_buffer.clear();

    display_buffer.push_back(&player_.get_sprite());
    display_buffer.push_back(&player_.weapon().get_sprite());

    if (peer_player_) {
        if (within_view_frustum(pfrm.screen(), peer_player_->get_position())) {
            display_buffer.push_back(&peer_player_->get_sprite());
            display_buffer.push_back(peer_player_->get_sprites()[1]);
            display_buffer.push_back(&peer_player_->get_blaster_sprite());
            shadows_buffer.push_back(&peer_player_->get_shadow());
            peer_player_->mark_visible(true);
        } else {
            peer_player_->mark_visible(false);
        }
    }

    enemies_.transform(show_sprites);
    details_.transform(show_sprites);

    if (scavenger_) {
        show_sprite(*scavenger_);
    }

    std::sort(display_buffer.begin(),
              display_buffer.end(),
              [](const auto& l, const auto& r) {
                  return l->get_position().y > r->get_position().y;
              });

    for (auto& e : effects_.get<DynamicEffect>()) {
        if (e->is_backdrop()) {
            show_sprite(*e);
        }
    }

    for (auto& e : effects_.get<StaticEffect>()) {
        if (e->is_backdrop()) {
            show_sprite(*e);
        }
    }

    show_sprite(transporter_);

    display_buffer.push_back(&player_.get_shadow());

    if (peer_player_ and not peer_player_->visible()) {
        show_offscreen_player_icon(pfrm, *this);
    }

    for (auto spr : display_buffer) {
        pfrm.screen().draw(*spr);
    }

    for (auto spr : shadows_buffer) {
        pfrm.screen().draw(*spr);
    }

    display_buffer.clear();
}


static void cell_automata_advance(TileMap& map, TileMap& maptemp)
{
    auto& thresh = lisp::loadv<lisp::Cons>("cell-thresh");

    // At the start, whether each tile is filled or unfilled is completely
    // random. The cell_automata_advance function causes each tile to
    // appear/disappear based on how many neighbors that tile has, which
    // ultimately causes tiles to coalesce into blobs.
    map.for_each([&](const u8& tile, int x, int y) {
        uint8_t count = 0;
        auto collect = [&](int x, int y) {
            if (map.get_tile(x, y) == Tile::none) {
                count++;
            }
        };
        collect(x - 1, y - 1);
        collect(x + 1, y - 1);
        collect(x - 1, y + 1);
        collect(x + 1, y + 1);
        collect(x - 1, y);
        collect(x + 1, y);
        collect(x, y - 1);
        collect(x, y + 1);
        if (tile == Tile::none) {
            if (count < thresh.cdr()->expect<lisp::Integer>().value_) {
                maptemp.set_tile(x, y, Tile::plate);
            } else {
                maptemp.set_tile(x, y, Tile::none);
            }
        } else {
            if (count > thresh.car()->expect<lisp::Integer>().value_) {
                maptemp.set_tile(x, y, Tile::none);
            } else {
                maptemp.set_tile(x, y, Tile::plate);
            }
        }
    });
    maptemp.for_each(
        [&](const u8& tile, int x, int y) { map.set_tile(x, y, tile); });
}


using BossLevelMap = Bitmatrix<TileMap::width, TileMap::height>;


READ_ONLY_DATA
static constexpr const BossLevelMap level_0({{
    {},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
}});


READ_ONLY_DATA
static constexpr const BossLevelMap memorial_area({{
    {},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0},
}});


READ_ONLY_DATA
static constexpr const BossLevelMap memorial_area_gr({{
    {},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0},
    {0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
}});


READ_ONLY_DATA
static constexpr const BossLevelMap boss_level_0({{
    {},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
}});


READ_ONLY_DATA
static constexpr const BossLevelMap boss_level_1({{
    {},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
}});


READ_ONLY_DATA
static constexpr const BossLevelMap boss_level_2({{
    {},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
}});


READ_ONLY_DATA
static constexpr const BossLevelMap boss_level_2_gr({{
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0,
    },
    {
        0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    {
        0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
    },
    {
        0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
    },
    {
        0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    },
    {
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    },
    {
        0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    },
    {
        0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    },
    {
        0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0,
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0,
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
    },
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    },
}});


READ_ONLY_DATA
static constexpr const BossLevelMap boss_level_3({{
    {},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
}});


struct BossLevel {
    const BossLevelMap* map_;
    const char* spritesheet_;
    const BossLevelMap* grass_pattern_ = nullptr;
};


static const BossLevel* get_boss_level(Level current_level)
{
    switch (current_level) {
    case boss_0_level: {
        static constexpr const BossLevel ret{&boss_level_0,
                                             "spritesheet_boss0"};
        return &ret;
    }

    case boss_1_level: {
        static constexpr const BossLevel ret{&boss_level_1,
                                             "spritesheet_boss1"};
        return &ret;
    }

    case boss_2_level: {
        static constexpr const BossLevel ret{
            &boss_level_2, "spritesheet_boss2", &boss_level_2_gr};
        return &ret;
    }

    case boss_3_level: {
        static constexpr const BossLevel ret{&boss_level_3,
                                             "spritesheet_boss3"};
        return &ret;
    }

    default:
        return nullptr;
    }
}


bool is_boss_level(Level level)
{
    return get_boss_level(level) not_eq nullptr;
}


enum { star_empty = 60, star_1 = 70, star_2 = 71 };


struct StarFlickerAnimationInfo {
    Vec2<int> pos_;
    int tile_;
};

static Buffer<StarFlickerAnimationInfo, 3> active_star_anims;


void animate_starfield(Platform& pfrm, Microseconds delta)
{
    static Microseconds timer;

    timer += delta;

    if (timer > milliseconds(90)) {
        timer = 0;

        // Because a coord may have been selected twice, undo the flicker effect
        // in reverse order.
        for (auto& info : reversed(active_star_anims)) {
            pfrm.set_tile(
                Layer::background, info.pos_.x, info.pos_.y, info.tile_);
        }

        active_star_anims.clear();

        for (u32 i = 0; i < active_star_anims.capacity(); ++i) {

            const int x = rng::choice<32>(rng::utility_state);
            const int y = rng::choice<32>(rng::utility_state);

            const auto tile = pfrm.get_tile(Layer::background, x, y);
            if (active_star_anims.push_back({{x, y}, tile})) {
                if (tile == star_1 or tile == star_2) {
                    pfrm.set_tile(Layer::background, x, y, star_empty);
                }
            }
        }
    }
}


void draw_starfield(Platform& pfrm)
{
    active_star_anims.clear();

    // Redraw the starfield background. Due to memory constraints, the
    // background needs to source its 8x8 pixel tiles from the tile0 texture.
    for (int x = 0; x < 32; ++x) {
        for (int y = 0; y < 32; ++y) {
            pfrm.set_tile(Layer::background, x, y, [] {
                if (rng::choice<9>(rng::critical_state)) {
                    return star_empty;
                } else {
                    if (rng::choice<2>(rng::critical_state)) {
                        return star_1;
                    } else {
                        return star_2;
                    }
                }
            }());
        }
    }
}


static constexpr const ZoneInfo zone_1{
    LocaleString::part_1_text,
    LocaleString::part_1_title,
    "spritesheet",
    "tilesheet",
    "tilesheet_top",
    "midsommar",
    Microseconds{0},
    ColorConstant::electric_blue,
    ColorConstant::picton_blue,
    ColorConstant::aerospace_orange,
    custom_color(0x6bffff),
    [](Platform& pfrm, Game&) {
        draw_starfield(pfrm);

        const int x = 16;
        const int y = 16;

        draw_image(pfrm, 61, x, y, 3, 3, Layer::background);
    },
    [](int x, int y, const TileMap&) {
        if (rng::choice<7>(rng::critical_state) == 0) {
            return 18;
        }
        return 0;
    }};


static constexpr const ZoneInfo zone_2{
    LocaleString::part_2_text,
    LocaleString::part_2_title,
    "spritesheet2",
    "tilesheet2",
    "tilesheet2_top",
    "computations",
    seconds(8) + milliseconds(700),
    ColorConstant::turquoise_blue,
    ColorConstant::maya_blue,
    ColorConstant::safety_orange,
    ColorConstant::turquoise_blue,
    [](Platform& pfrm, Game&) {
        draw_starfield(pfrm);

        const int x = 16;
        const int y = 16;

        draw_image(pfrm, 120, x, y, 5, 5, Layer::background);
    },
    [](int x, int y, const TileMap&) {
        if (rng::choice<16>(rng::critical_state) == 0) {
            return 17;
        }
        return 0;
    }};


static constexpr const ZoneInfo zone_3{
    LocaleString::part_3_text,
    LocaleString::part_3_title,
    "spritesheet3",
    "tilesheet3",
    "tilesheet3_top",
    "murmuration",
    Microseconds{0},
    ColorConstant::cerulean_blue,
    ColorConstant::picton_blue,
    ColorConstant::aerospace_orange,
    ColorConstant::cerulean_blue,
    [](Platform& pfrm, Game&) {
        draw_starfield(pfrm);

        const int x = 12;
        const int y = 12;

        draw_image(pfrm, 120, x, y, 9, 9, Layer::background);
    },
    [](int x, int y, const TileMap&) {
        if (rng::choice<14>(rng::critical_state) == 0) {
            switch (rng::choice<2>(rng::critical_state)) {
            case 0:
                return 18;
            case 1:
                return 19;
            }
        }
        return 0;
    }};


static constexpr const ZoneInfo zone_4{
    LocaleString::part_4_text,
    LocaleString::part_4_title,
    "spritesheet4",
    "tilesheet4",
    "tilesheet4_top",
    "chantiers_navals_412",
    Microseconds{0},
    ColorConstant::turquoise_blue,
    ColorConstant::maya_blue,
    ColorConstant::safety_orange,
    custom_color(0x6bffff),
    [](Platform& pfrm, Game&) {
        draw_starfield(pfrm);

        const int x = 12;
        const int y = 10;

        draw_image(pfrm, 120, x, y, 13, 13, Layer::background);
    },
    [](int, int, const TileMap&) { return 0; }};


const ZoneInfo& zone_info(Level level)
{
    if (UNLIKELY(level > boss_3_level)) {
        static const ZoneInfo null_zone{
            LocaleString::count,
            LocaleString::count,
            "",
            "",
            "",
            "",
            0,
            ColorConstant::null,
            ColorConstant::null,
            ColorConstant::null,
            ColorConstant::null,
            [](Platform&, Game&) {},
            [](int, int, const TileMap&) { return 0; }};
        return null_zone;
    } else if (level > boss_2_level) {
        return zone_4;
    } else if (level > boss_1_level) {
        return zone_3;
    } else if (level > boss_0_level) {
        return zone_2;
    } else {
        return zone_1;
    }
}


const ZoneInfo& current_zone(Game& game)
{
    return zone_info(game.level());
}


bool operator==(const ZoneInfo& lhs, const ZoneInfo& rhs)
{
    return lhs.title_line_2 == rhs.title_line_2;
}


static bool contains(lisp::Value* tiles_list, u8 t)
{
    while (tiles_list not_eq L_NIL) {
        if (tiles_list->type_ not_eq lisp::Value::Type::cons) {
            while (true)
                ; // TODO: raise error...
        }

        if (tiles_list->cons_.car()->type_ not_eq lisp::Value::Type::integer) {
            while (true)
                ; // TODO: raise error
        }

        if (static_cast<int>(t) == tiles_list->cons_.car()->integer_.value_) {
            return true;
        }

        tiles_list = tiles_list->cons_.cdr();
    }

    return false;
}


COLD void Game::next_level(Platform& pfrm, std::optional<Level> set_level)
{
    // Turn off rumble. Otherwise, if rumble happened to be active, the game
    // would continue rumbling until the next update() call, which could take a
    // moment, as we're generating a new level.
    pfrm.keyboard().rumble(false);

    if (set_level) {
        persistent_data_.level_.set(*set_level);
    } else {
        persistent_data_.level_.set(persistent_data_.level_.get() + 1);
    }

    if (level() == 0) {
        rng::critical_state = 9;
    }

    Entity::reset_ids();
    // These few entities are sort of a special case. All other entities are
    // erased during level transition, but the player and transporter are
    // persistent, and therefore, should be excluded from the id reset.
    player_.override_id(player_.id());
    transporter_.override_id(transporter_.id());

    persistent_data_.score_.set(score_);
    persistent_data_.player_health_.set(player_.get_health());
    persistent_data_.seed_.set(rng::critical_state);
    persistent_data_.inventory_ = inventory_;
    persistent_data_.store_powerups(powerups_);

    lisp::dostring(pfrm.load_file_contents("scripts", "pre_levelgen.lisp"));

    pfrm.load_tile0_texture(current_zone(*this).tileset0_name_);
    pfrm.load_tile1_texture(current_zone(*this).tileset1_name_);

    auto boss_level = get_boss_level(level());
    if (boss_level) {
        pfrm.speaker().stop_music();
        pfrm.load_sprite_texture(boss_level->spritesheet_);

    } else {
        if (level() == 0) {
            pfrm.load_sprite_texture("spritesheet_intro_cutscene");
        } else {
            pfrm.load_sprite_texture(current_zone(*this).spritesheet_name_);
        }
    }

RETRY:
    Game::regenerate_map(pfrm);

    if (not Game::respawn_entities(pfrm)) {
        warning(pfrm, "Map is too small, regenerating...");
        goto RETRY;
    }

    tiles_.for_each([&](u8 t, s8 x, s8 y) {
        pfrm.set_tile(Layer::map_0, x, y, static_cast<s16>(t));
    });

    current_zone(*this).generate_background_(pfrm, *this);

    lisp::dostring(pfrm.load_file_contents("scripts", "post_levelgen.lisp"));

    // We're doing this to speed up collision checking with walls. While it
    // might be nice to have more info about the tilemap, it's costly to check
    // all of the enumerations. At this point, we've already pushed the tilemap
    // to the platform for rendering, so we can simplify to just the data that
    // we absolutely need.
    auto wall_tiles = lisp::get_var("wall-tiles-list");
    auto edge_tiles = lisp::get_var("edge-tiles-list");

    tiles_.for_each([&](u8& tile, int, int) {
        if (not contains(wall_tiles, tile)) {
            if (contains(edge_tiles, tile)) {
                tile = Tile::plate;
            } else {
                tile = Tile::sand;
            }
        } else {
            tile = Tile::none;
        }
    });

    const auto player_pos = player_.get_position();
    const auto ssize = pfrm.screen().size();
    camera_.set_position(
        pfrm, {player_pos.x - ssize.x / 2, player_pos.y - float(ssize.y)});
}


COLD static u32
flood_fill(Platform& pfrm, TileMap& map, u8 replace, TIdx x, TIdx y)
{
    using Coord = Vec2<s8>;

    ScratchBufferBulkAllocator mem(pfrm);

    auto stack = mem.alloc<Buffer<Coord, TileMap::width * TileMap::height>>();

    if (UNLIKELY(not stack)) {
        pfrm.fatal("fatal error in floodfill");
    }

    const u8 target = map.get_tile(x, y);

    u32 count = 0;

    const auto action = [&](const Coord& c, TIdx x_off, TIdx y_off) {
        const TIdx x = c.x + x_off;
        const TIdx y = c.y + y_off;
        if (x > 0 and x < TileMap::width and y > 0 and y < TileMap::height) {
            if (map.get_tile(x, y) == target) {
                map.set_tile(x, y, replace);
                stack->push_back({x, y});
                count += 1;
            }
        }
    };

    action({x, y}, 0, 0);

    while (not stack->empty()) {
        Coord c = stack->back();
        stack->pop_back();
        action(c, -1, 0);
        action(c, 0, 1);
        action(c, 0, -1);
        action(c, 1, 0);
    }

    return count;
}


COLD void Game::seed_map(Platform& pfrm, TileMap& workspace)
{
    if (auto l = get_boss_level(level())) {
        for (int x = 0; x < TileMap::width; ++x) {
            for (int y = 0; y < TileMap::height; ++y) {
                tiles_.set_tile(x, y, l->map_->get(x, y));
            }
        }
    } else if (level() == 0) {
        for (int x = 0; x < TileMap::width; ++x) {
            for (int y = 0; y < TileMap::height; ++y) {
                tiles_.set_tile(x, y, level_0.get(x, y));
            }
        }
    } else if (level() == boss_0_level + 1) {
        for (int x = 0; x < TileMap::width; ++x) {
            for (int y = 0; y < TileMap::height; ++y) {
                tiles_.set_tile(x, y, memorial_area.get(x, y));
            }
        }
    } else {
        // Just for the sake of variety, intentionally generate
        // smaller maps sometimes.
        const bool small_map = rng::choice<100>(rng::critical_state) < 20 or
                               (not pfrm.network_peer().is_connected() and
                                (is_boss_level(level() - 1) or
                                 is_boss_level(level() - 2) or level() < 4));

        int count;

        const auto cell_iters =
            lisp::get_var("cell-iters")->expect<lisp::Integer>().value_;

        do {
            count = 0;
            tiles_.for_each([small_map](auto& t, int x, int y) {
                if (small_map and (x < 2 or x > TileMap::width - 2 or y < 3 or
                                   y > TileMap::height - 3)) {
                    t = 0;
                } else {
                    t = rng::choice<int(Tile::sand)>(rng::critical_state);
                }
            });

            for (int i = 0; i < cell_iters; ++i) {
                cell_automata_advance(tiles_, workspace);
            }

            tiles_.for_each([&count](u8 t, int, int) {
                if (t) {
                    ++count;
                }
            });

        } while (count == 0);
    }
}


static bool
is_center_tile(lisp::Value* wall_tiles_list, lisp::Value* edge_tiles_list, u8 t)
{
    return not contains(wall_tiles_list, t) and
           not contains(edge_tiles_list, t);
}


static bool
is_edge_tile(lisp::Value* wall_tiles_list, lisp::Value* edge_tiles_list, u8 t)
{
    return not contains(wall_tiles_list, t) and contains(edge_tiles_list, t);
}


static void add_map_decorations(Level level,
                                Platform& pfrm,
                                const TileMap& map,
                                TileMap& grass_overlay)
{
    auto adjacent_decor = [&](int x, int y) {
        for (int i = x - 1; i < x + 1; ++i) {
            for (int j = y - 1; j < y + 1; ++j) {
                if ((int)grass_overlay.get_tile(i, j) > 16) {
                    return true;
                }
            }
        }
        return false;
    };

    auto wall_tiles = lisp::get_var("wall-tiles-list");
    auto edge_tiles = lisp::get_var("edge-tiles-list");

    grass_overlay.for_each([&](u8 t, s8 x, s8 y) {
        pfrm.set_tile(Layer::map_1, x, y, t);
        if (t == Tile::none) {
            if (is_center_tile(wall_tiles, edge_tiles, map.get_tile(x, y))) {
                if (not adjacent_decor(x, y)) {
                    pfrm.set_tile(
                        Layer::map_1,
                        x,
                        y,
                        zone_info(level).generate_decoration_(x, y, map));
                }
            }
        }
    });
}


void debug_log_tilemap(Platform& pfrm, TileMap& map)
{
    debug(pfrm, "printing map...");
    for (int i = 0; i < TileMap::width; ++i) {
        StringBuffer<65> buffer;
        buffer.push_back('{');
        for (int j = 0; j < TileMap::height; ++j) {
            u8 tile = map.get_tile(i, j);
            if (tile) {
                buffer += "1, ";
            } else {
                buffer += "0, ";
            }
        }
        buffer += "},";
        debug(pfrm, buffer.c_str());
    }
}


using MapCoord = Vec2<TIdx>;
using MapCoordBuf = Buffer<MapCoord, TileMap::tile_count>;


static bool special_map(Level level)
{
    return level == 0 or level - 1 == boss_0_level;
}


COLD void Game::regenerate_map(Platform& pfrm)
{
    ScratchBufferBulkAllocator mem(pfrm);

    auto temporary = mem.alloc<TileMap>();
    if (not temporary) {
        pfrm.fatal("failed to create temporary tilemap");
    }

    seed_map(pfrm, *temporary);
    // debug_log_tilemap(pfrm, tiles_);

    // Remove tiles from edges of the map. Some platforms,
    // particularly consoles, have automatic tile-wrapping, and we
    // don't want to deal with having to support wrapping in all the
    // game logic.
    tiles_.for_each([&](u8& tile, int x, int y) {
        if (x == 0 or x == TileMap::width - 1 or y == 0 or
            y == TileMap::height - 1) {
            tile = Tile::none;
        }
    });

    auto wall_tiles = lisp::get_var("wall-tiles-list");
    auto edge_tiles = lisp::get_var("edge-tiles-list");

    // Create a mask of the tileset by filling the temporary tileset
    // with all walkable tiles from the tilemap.
    tiles_.for_each([&](const u8& tile, TIdx x, TIdx y) {
        if (not contains(wall_tiles, tile)) {
            temporary->set_tile(x, y, 1);
        } else {
            temporary->set_tile(x, y, 0);
        }
    });

    if (not special_map(level()) and not is_boss_level(level())) {
        // Pick a random filled tile, and flood-fill around that tile in
        // the map, to produce a single connected component.
        while (true) {
            const auto x = rng::choice(TileMap::width, rng::critical_state);
            const auto y = rng::choice(TileMap::height, rng::critical_state);
            if (temporary->get_tile(x, y) not_eq Tile::none) {
                flood_fill(pfrm, *temporary, 2, x, y);
                temporary->for_each([&](const u8& tile, TIdx x, TIdx y) {
                    if (tile not_eq 2) {
                        tiles_.set_tile(x, y, Tile::none);
                    }
                });
                break;
            } else {
                continue;
            }
        }
    }

    scavenger_.reset();

    const bool place_scavenger =
        level() > 3 and // at low levels, players will have low score anyway.
        not is_boss_level(level()) and
        (rng::choice<6>(rng::critical_state) > 1 or is_boss_level(level() - 1));

    // Now we want to generate a map region for our scavenger character. We want
    // to place a 3x3 block of tiles in some open location, and then connect
    // that location to the rest of the map.
    int tries = 0;
    while (place_scavenger and tries < 1000) {
        const auto x = rng::choice(TileMap::width, rng::critical_state);
        const auto y = rng::choice(TileMap::height, rng::critical_state);

        if (not(x > 2 and x < TileMap::width - 3 and y > 2 and
                y < TileMap::height - 3)) {
            continue;
        }

        bool free = true;
        for (int xx = x - 2; xx < x + 3; ++xx) {
            for (int yy = y - 2; yy < y + 3; ++yy) {
                const auto t = tiles_.get_tile(xx, yy);
                if (t) {
                    free = false;
                }
            }
        }

        if (free) {
            MapCoordBuf floor_tiles;
            tiles_.for_each([&floor_tiles](u8 tile, TIdx x, TIdx y) {
                if (tile) {
                    floor_tiles.push_back({x, y});
                }
            });

            if (floor_tiles.empty()) {
                break;
            }

            const Vec2<Float> seek{Float(x), Float(y)};
            std::sort(floor_tiles.begin(),
                      floor_tiles.end(),
                      [&](const MapCoord& lhs, const MapCoord& rhs) {
                          return distance(lhs.cast<Float>(), seek) <
                                 distance(rhs.cast<Float>(), seek);
                      });

            const auto nearest = *floor_tiles.begin();

            auto next_x = [&](int x) {
                if (seek.x < nearest.x) {
                    return x - 1;
                } else {
                    return x + 1;
                }
            };

            auto next_y = [&](int y) {
                if (seek.y < nearest.y) {
                    return y - 1;
                } else {
                    return y + 1;
                }
            };

            // Ok, so we could do fancy rasterization for a straight-line path,
            // but that would take some extra work, and the result, in some
            // cases, would look choppy, so let's use a sort of manhattan path.
            int write_y = nearest.y;
            int write_x = nearest.x;

            const bool x_first = rng::choice<2>(rng::critical_state);

            // Randomly flip the ordering of the x/y path rendering, so that the
            // path doesn't always connect to the generated region from the same
            // side.
            if (x_first) {
                for (; write_x not_eq seek.x; write_x = next_x(write_x)) {
                    tiles_.set_tile(write_x, write_y, 1);
                }
            }

            for (; write_y not_eq seek.y; write_y = next_y(write_y)) {
                tiles_.set_tile(write_x, write_y, 1);
            }

            if (not x_first) {
                for (; write_x not_eq seek.x; write_x = next_x(write_x)) {
                    tiles_.set_tile(write_x, write_y, 1);
                }
            }

            for (int xx = x - 1; xx < x + 2; ++xx) {
                for (int yy = y - 1; yy < y + 2; ++yy) {
                    tiles_.set_tile(xx, yy, 1);
                }
            }

            auto scavenger_wc = to_world_coord({s8(x), s8(y)});
            scavenger_wc.x += 16;
            scavenger_wc.y += 12;

            scavenger_.emplace(scavenger_wc);

            break;
        } else {
            ++tries;
        }
    }

    auto grass_overlay = mem.alloc<TileMap>([&](u8& t, int, int) {
        if (level() > boss_0_level) {
            if (rng::choice<3>(rng::critical_state)) {
                t = Tile::none;
            } else {
                t = Tile::plate;
            }
        } else {
            t = Tile::none;
        }
    });

    if (not grass_overlay) {
        pfrm.fatal("failed to alloc map1 workspace");
    }

    const auto cell_iters = lisp::loadv<lisp::Integer>("cell-iters").value_;

    for (int i = 0; i < cell_iters; ++i) {
        cell_automata_advance(*grass_overlay, *temporary);
    }

    // debug_log_tilemap(pfrm, *grass_overlay);

    if (auto info = get_boss_level(level())) {
        if (info->grass_pattern_) {
            for (int x = 0; x < TileMap::width; ++x) {
                for (int y = 0; y < TileMap::height; ++y) {
                    grass_overlay->set_tile(
                        x, y, info->grass_pattern_->get(x, y));
                }
            }
        }
    } else if (level() - 1 == boss_0_level) {
        for (int x = 0; x < TileMap::width; ++x) {
            for (int y = 0; y < TileMap::height; ++y) {
                grass_overlay->set_tile(x, y, memorial_area_gr.get(x, y));
            }
        }
    }

    // All tiles with four neighbors become sand tiles.
    tiles_.for_each([&](u8& tile, int x, int y) {
        if (tile == Tile::plate and
            tiles_.get_tile(x - 1, y) not_eq Tile::none and
            tiles_.get_tile(x + 1, y) not_eq Tile::none and
            tiles_.get_tile(x, y - 1) not_eq Tile::none and
            tiles_.get_tile(x, y + 1) not_eq Tile::none) {
            tile = Tile::sand;
        }
    });

    // Add ledge tiles to empty locations, where the y-1 tile is non-empty.
    tiles_.for_each([&](u8& tile, int x, int y) {
        auto above = tiles_.get_tile(x, y - 1);
        if (tile == Tile::none and
            (above == Tile::plate or above == Tile::damaged_plate)) {
            tile = Tile::ledge;
        }
    });

    // Crop the grass overlay tileset to fit the target tilemap.
    tiles_.for_each([&](u8& tile, int x, int y) {
        if (tile == Tile::none) {
            grass_overlay->set_tile(x, y, Tile::none);
        }
    });

    u8 bitmask[TileMap::width][TileMap::height];
    // After running the algorithm below, the bitmask will contain
    // correct tile indices, such that all the grass tiles are
    // smoothly connected.
    for (int x = 0; x < TileMap::width; ++x) {
        for (int y = 0; y < TileMap::height; ++y) {
            bitmask[x][y] = 0;
            bitmask[x][y] += 1 * int(grass_overlay->get_tile(x, y - 1));
            bitmask[x][y] += 2 * int(grass_overlay->get_tile(x + 1, y));
            bitmask[x][y] += 4 * int(grass_overlay->get_tile(x, y + 1));
            bitmask[x][y] += 8 * int(grass_overlay->get_tile(x - 1, y));
        }
    }

    if (zone_info(level()) == zone_4) {
        tiles_.for_each([&](u8 tile, int x, int y) {
            if (tile == Tile::ledge) {
                if (tiles_.get_tile(x + 1, y) == Tile::plate) {
                    grass_overlay->set_tile(x, y, 33);
                }
                if (tiles_.get_tile(x - 1, y) == Tile::plate) {
                    if (grass_overlay->get_tile(x, y) == 33) {
                        grass_overlay->set_tile(x, y, 32);
                    } else {
                        grass_overlay->set_tile(x, y, 34);
                    }
                }
                if (tiles_.get_tile(x, y + 1) == Tile::plate) {
                    switch (grass_overlay->get_tile(x, y)) {
                    case 33:
                        grass_overlay->set_tile(x, y, 30);
                        break;

                    case 34:
                        grass_overlay->set_tile(x, y, 29);
                        break;

                    case 32:
                        grass_overlay->set_tile(x, y, 28);
                        break;
                    }
                }
            }
            if (tile == Tile::none) {
                if (tiles_.get_tile(x + 1, y) == Tile::plate and
                    tiles_.get_tile(x, y + 1) == Tile::plate) {
                    grass_overlay->set_tile(x, y, 36);
                }
                if (tiles_.get_tile(x - 1, y) == Tile::plate and
                    tiles_.get_tile(x, y + 1) == Tile::plate) {
                    if (grass_overlay->get_tile(x, y) == 36) {
                        grass_overlay->set_tile(x, y, 31);
                    } else {
                        grass_overlay->set_tile(x, y, 35);
                    }
                }
            }
            if (tile == Tile::sand) {
                if (grass_overlay->get_tile(x, y)) {
                    return;
                }
                if (tiles_.get_tile(x - 1, y) == Tile::plate and
                    tiles_.get_tile(x, y - 1) == Tile::plate and
                    tiles_.get_tile(x - 1, y - 1) not_eq Tile::plate) {

                    grass_overlay->set_tile(x, y, 34);
                }
                if (tiles_.get_tile(x + 1, y) == Tile::plate and
                    tiles_.get_tile(x, y - 1) == Tile::plate and
                    tiles_.get_tile(x + 1, y - 1) not_eq Tile::plate) {

                    if (grass_overlay->get_tile(x, y) == 34) {
                        grass_overlay->set_tile(x, y, 32);
                    } else {
                        grass_overlay->set_tile(x, y, 33);
                    }
                }
                if (tiles_.get_tile(x - 1, y) == Tile::plate and
                    tiles_.get_tile(x, y + 1) == Tile::plate and
                    tiles_.get_tile(x - 1, y + 1) not_eq Tile::plate) {

                    if (grass_overlay->get_tile(x, y) == 34) {
                        grass_overlay->set_tile(x, y, 29);
                    } else if (grass_overlay->get_tile(x, y) == 32) {
                        grass_overlay->set_tile(x, y, 28);
                    } else {
                        grass_overlay->set_tile(x, y, 35);
                    }
                }
                if (tiles_.get_tile(x + 1, y) == Tile::plate and
                    tiles_.get_tile(x, y + 1) == Tile::plate and
                    tiles_.get_tile(x + 1, y + 1) not_eq Tile::plate) {

                    if (grass_overlay->get_tile(x, y) == 33) {
                        grass_overlay->set_tile(x, y, 30);
                    } else if (grass_overlay->get_tile(x, y) == 35) {
                        grass_overlay->set_tile(x, y, 31);
                    } else if (grass_overlay->get_tile(x, y) == 29 or
                               grass_overlay->get_tile(x, y) == 28 or
                               grass_overlay->get_tile(x, y) == 32 or
                               grass_overlay->get_tile(x, y) == 31) {
                        grass_overlay->set_tile(x, y, 28);
                    } else {
                        grass_overlay->set_tile(x, y, 36);
                    }
                }
            }
        });
    }

    grass_overlay->for_each([&](u8& tile, int x, int y) {
        if (tile == Tile::plate) {
            auto match = tiles_.get_tile(x, y);
            switch (match) {
            case Tile::plate:
            case Tile::sand:
                grass_overlay->set_tile(
                    x, y, Tile::grass_start + bitmask[x][y]);
                break;

            case Tile::ledge:
                tiles_.set_tile(x,
                                y,
                                (rng::choice<2>(rng::critical_state))
                                    ? Tile::grass_ledge
                                    : Tile::grass_ledge_vines);
                grass_overlay->set_tile(x, y, Tile::none);
                break;

            default:
                break;
            }
        }
    });

    if (zone_info(level()) == zone_1) {
        tiles_.for_each([&](u8& tile, int x, int y) {
            const auto up = tiles_.get_tile(x, y + 1);
            const auto down = tiles_.get_tile(x, y - 1);
            const auto left = tiles_.get_tile(x - 1, y);
            const auto right = tiles_.get_tile(x + 1, y);
            if (tile == 0 and not contains(wall_tiles, left) and
                (up == 0 or up == 18 or down == 0 or down == 18) and
                (tiles_.get_tile(x - 2, y) == 0 or
                 tiles_.get_tile(x - 2, y) == 19)) {
                tiles_.set_tile(x, y, 18);
            }
            if (tile == 0 and not contains(wall_tiles, right) and
                (up == 0 or up == 19 or down == 0 or down == 19) and
                (tiles_.get_tile(x + 2, y) == 0 or
                 tiles_.get_tile(x + 2, y) == 18)) {
                tiles_.set_tile(x, y, 19);
            }
        });

        tiles_.for_each([&](u8& tile, int x, int y) {
            if (tile == Tile::ledge) {
                if (tiles_.get_tile(x + 1, y) == Tile::plate) {
                    tile = Tile::beam_br;
                } else if (tiles_.get_tile(x - 1, y) == Tile::plate) {
                    tile = Tile::beam_bl;
                }
            } else if (tile == Tile::none and
                       tiles_.get_tile(x, y + 1) == Tile::plate) {
                if (tiles_.get_tile(x + 1, y) == Tile::plate) {
                    tile = Tile::beam_ur;
                } else if (tiles_.get_tile(x - 1, y) == Tile::plate) {
                    tile = Tile::beam_ul;
                }
            }
        });
    }

    if (zone_info(level()) == zone_3) {
        tiles_.for_each([&](u8& tile, int x, int y) {
            if (contains(wall_tiles, tile)) {
                if (not contains(wall_tiles, tiles_.get_tile(x, y + 1))) {
                    grass_overlay->set_tile(x, y, 17);
                }
            }
        });
    }

    if (zone_info(level()) == zone_1) {
        tiles_.for_each([&](u8& tile, int x, int y) {
            const auto up = tiles_.get_tile(x, y + 1);
            const auto down = tiles_.get_tile(x, y - 1);
            const auto left = tiles_.get_tile(x - 1, y);
            const auto right = tiles_.get_tile(x + 1, y);
            if (tile == Tile::plate and
                grass_overlay->get_tile(x, y) == Tile::none) {

                if (is_center_tile(
                        wall_tiles, edge_tiles, tiles_.get_tile(x + 1, y)) and
                    not contains(wall_tiles, up) and
                    not contains(wall_tiles, down) and
                    not(is_center_tile(wall_tiles, edge_tiles, up) and
                        is_center_tile(wall_tiles, edge_tiles, down))) {

                    tiles_.set_tile(x, y, Tile::plate_left);
                }
                if (is_center_tile(
                        wall_tiles, edge_tiles, tiles_.get_tile(x - 1, y)) and
                    not contains(wall_tiles, up) and
                    not contains(wall_tiles, down) and
                    not(is_center_tile(wall_tiles, edge_tiles, up) and
                        is_center_tile(wall_tiles, edge_tiles, down))) {

                    tiles_.set_tile(x, y, Tile::plate_right);
                }
                if (is_center_tile(
                        wall_tiles, edge_tiles, tiles_.get_tile(x, y + 1)) and
                    not contains(wall_tiles, right) and
                    not contains(wall_tiles, left) and
                    not(is_center_tile(wall_tiles, edge_tiles, left) and
                        is_center_tile(wall_tiles, edge_tiles, right))) {

                    tiles_.set_tile(x, y, Tile::plate_top);
                }
                if (is_center_tile(
                        wall_tiles, edge_tiles, tiles_.get_tile(x, y - 1)) and
                    not contains(wall_tiles, right) and
                    not contains(wall_tiles, left) and
                    not(is_center_tile(wall_tiles, edge_tiles, left) and
                        is_center_tile(wall_tiles, edge_tiles, right))) {

                    tiles_.set_tile(x, y, Tile::plate_bottom);
                }
            }
        });

        // tiles_.for_each([&](u8& tile, int x, int y) {
        //     if (tile == 18 and (tiles_.get_tile(x, y - 1) not_eq 18 and
        //                         tiles_.get_tile(x, y + 1) not_eq 18)) {
        //         tile = 0;
        //     }
        //     if (tile == 19 and (tiles_.get_tile(x, y - 1) not_eq 19 and
        //                         tiles_.get_tile(x, y + 1) not_eq 19)) {
        //         tile = 0;
        //     }

        // });
    }

    grass_overlay->for_each([&](u8& tile, int x, int y) {
        if (level() <= boss_0_level or level() > boss_1_level) {
            return;
        }
        if (tile == 16 and rng::choice<2>(rng::critical_state)) {
            tile = 19;
        }
    });

    add_map_decorations(level(), pfrm, tiles_, *grass_overlay);

    tiles_.for_each([&](u8& tile, int x, int y) {
        if (tile == Tile::plate) {
            if (rng::choice<4>(rng::critical_state) == 0) {
                tile = Tile::damaged_plate;
            }
        } else if (tile == Tile::sand) {
            if (rng::choice<4>(rng::critical_state) == 0 and
                grass_overlay->get_tile(x, y) == Tile::none) {
                tile = Tile::sand_sprouted;
            }
        }
    });
}


COLD static MapCoordBuf get_free_map_slots(const TileMap& map)
{
    MapCoordBuf output;

    auto wall_tiles = lisp::get_var("wall-tiles-list");

    map.for_each([&](const u8& tile, TIdx x, TIdx y) {
        if (not contains(wall_tiles, tile)) {
            output.push_back({x, y});
        }
    });

    for (auto it = output.begin(); it not_eq output.end();) {
        const u8 tile = map.get_tile(it->x, it->y);
        if (not(tile == Tile::sand or tile == Tile::sand_sprouted)) {
            output.erase(it);
        } else {
            ++it;
        }
    }

    return output;
}


COLD static std::optional<MapCoord> select_coord(MapCoordBuf& free_spots)
{
    if (not free_spots.empty()) {
        auto choice = rng::choice(free_spots.size(), rng::critical_state);
        auto it = &free_spots[choice];
        const auto result = *it;
        free_spots.erase(it);
        return result;
    } else {
        return {};
    }
}


template <typename T> struct Type {
    using value = T;
};


static Vec2<Float> world_coord(const MapCoord& c)
{
    return Vec2<Float>{static_cast<Float>(c.x * 32) + 16,
                       static_cast<Float>(c.y * 24)};
}


template <typename Type, typename Group, typename... Params>
void spawn_entity(Platform& pf,
                  MapCoordBuf& free_spots,
                  Group& group,
                  Params&&... params)
{
    if (const auto c = select_coord(free_spots)) {
        if (not group.template spawn<Type>(world_coord(*c), params...)) {
            warning(pf, "spawn failed: entity buffer full");
        }
    } else {
        warning(pf, "spawn failed: out of coords");
    }
}


COLD static void
spawn_compactors(Platform& pfrm, Game& game, MapCoordBuf& free_spots)
{
    if (game.level() > boss_0_level + 4 and game.level() < boss_2_level) {
        const int count = std::min(u32([&] {
                                       if (free_spots.size() < 65) {
                                           return 2;
                                       } else {
                                           return 3;
                                       }
                                   }()),
                                   free_spots.size() / 25);

        auto wall_tiles = lisp::get_var("wall-tiles-list");
        auto edge_tiles = lisp::get_var("edge-tiles-list");

        for (int i = 0; i < count and free_spots.size() > 0; ++i) {
            if (rng::choice<2>(rng::critical_state)) {
                int tries = 0;

                static const int max_tries = 512;

                while (tries < max_tries) {
                    auto selected = &free_spots[rng::choice(
                        free_spots.size(), rng::critical_state)];
                    const int x = selected->x;
                    const int y = selected->y;

                    tries++;

                    const auto t = game.tiles().get_tile(x, y);
                    int edge_count = 0;
                    auto detect_edge = [&](int x, int y) {
                        auto tile = game.tiles().get_tile(x, y);
                        if (is_edge_tile(wall_tiles, edge_tiles, tile)) {
                            ++edge_count;
                        }
                    };
                    if (not is_edge_tile(wall_tiles, edge_tiles, t)) {
                        detect_edge(x - 1, y);
                        detect_edge(x + 1, y);
                        detect_edge(x, y - 1);
                        detect_edge(x, y + 1);

                        if (edge_count > 2) {
                            game.enemies().spawn<Compactor>(
                                world_coord(*selected));
                            free_spots.erase(selected);
                            break;
                        }
                    }
                }

                // We tried to find a little nook to place the compactor, but
                // failed, let's just put it on any edge tile.
                if (tries == max_tries) {
                    const s8 x =
                        rng::choice<TileMap::width>(rng::critical_state);
                    const s8 y =
                        rng::choice<TileMap::height>(rng::critical_state);

                    const auto t = game.tiles().get_tile(x, y);
                    if (is_edge_tile(wall_tiles, edge_tiles, t)) {
                        const auto wc = to_world_coord(Vec2<TIdx>{x, y});
                        game.enemies().spawn<Compactor>(wc);
                    }
                }
            }
        }
    }
}


COLD static void
spawn_enemies(Platform& pfrm, Game& game, MapCoordBuf& free_spots)
{
    // We want to spawn enemies based on both the difficulty of level, and the
    // number of free spots that are actually available on the map. Some
    // enemies, like snakes, take up a lot of resources on constrained systems,
    // so we can only display one or _maybe_ two.
    //
    // Some other enemies require a lot of map space to fight effectively, so
    // they are banned from tiny maps.

    const auto max_density = 119;

    const auto min_density = 70;

    const auto density_incr = 4;


    const auto density = std::min(
        Float(max_density) / 1000,
        Float(min_density) / 1000 + game.level() * Float(density_incr) / 1000);


    const auto max_enemies = 6;

    const int spawn_count =
        std::max(std::min(max_enemies, int(free_spots.size() * density)), 1);

    struct EnemyInfo {
        Function<64, void()> spawn_;
        bool (*level_in_range_)(Level) = [](Level) { return true; };
        int max_allowed_ = 1000;
        bool (*ignore_)(Level) = [](Level) { return false; };
    } info[] = {
        {[&]() {
             spawn_entity<Drone>(pfrm, free_spots, game.enemies());
             if (game.level() > 6 and game.level() < boss_0_level) {
                 spawn_entity<Drone>(pfrm, free_spots, game.enemies());
             }
         },
         [](Level l) { return l < boss_0_level or l > boss_1_level; }},
        {[&]() { spawn_entity<Dasher>(pfrm, free_spots, game.enemies()); },
         [](Level l) { return l >= 4; }},
        {[&]() {
             spawn_entity<SnakeHead>(pfrm, free_spots, game.enemies(), game);
         },
         [](Level l) { return l >= 6 and l < boss_0_level; },
         1},
        {[&]() { spawn_entity<Turret>(pfrm, free_spots, game.enemies()); },
         [](Level l) { return l > 0; }},
        {[&]() { spawn_entity<Scarecrow>(pfrm, free_spots, game.enemies()); },
         [](Level l) { return l > boss_0_level and l < boss_3_level; }},
        {[&]() { spawn_entity<Golem>(pfrm, free_spots, game.enemies()); },
         [](Level l) { return l > boss_0_level + 4; },
         1,
         [](Level l) -> bool {
             if (l < boss_1_level and l > boss_0_level + 4) {
                 return rng::choice<6>(rng::critical_state);
             } else if (l < boss_2_level) {
                 return rng::choice<4>(rng::critical_state);
             }
             return false;
         }}};

    Buffer<EnemyInfo*, 100> distribution;

    while (not distribution.full()) {
        auto selected = &info[rng::choice<sizeof info / sizeof(EnemyInfo)>(
            rng::critical_state)];

        if (not selected->level_in_range_(game.level())) {
            continue;
        }

        distribution.push_back(selected);
    }

    int i = 0;
    while (i < spawn_count) {

        auto choice = distribution[rng::choice<distribution.capacity()>(
            rng::critical_state)];

        if (choice->max_allowed_ == 0) {
            continue;
        }

        choice->max_allowed_--;
        if (not choice->ignore_(game.level())) {
            choice->spawn_();
            ++i;
        }
    }

    spawn_compactors(pfrm, game, free_spots);
}


LevelRange level_range(Item::Type item)
{
    static const auto max = std::numeric_limits<Level>::max();
    static const auto min = std::numeric_limits<Level>::min();

    switch (item) {
    case Item::Type::old_poster_1:
    case Item::Type::worker_notebook_1:
    case Item::Type::engineer_notebook_1:
        return {min, boss_0_level};

    case Item::Type::worker_notebook_2:
    case Item::Type::engineer_notebook_2:
    case Item::Type::signal_jammer:
        return {boss_0_level, max};

    case Item::Type::seed_packet:
        return {0, boss_2_level};

    default:
        return {min, max};
    }
}


using ItemRarity = int;


// Base price of an item, for use with in-game shops. The base price will
// generally represent what the shopkeeper is willing to buy the item for, and
// when buying an item from the shopkeeper, a markup will be applied to the base
// price. Prices should generally be configured such that items are expensive,
// and you actually need to play the game in order to buy stuff.
int base_price(Item::Type item)
{
    switch (item) {
    case Item::Type::count:
    case Item::Type::null:
        return 0;

    case Item::Type::lethargy:
        return 400;

    case Item::Type::explosive_rounds_2:
        return 310;

    case Item::Type::accelerator:
        return 280;

    case Item::Type::orange_seeds:
        return 5;

    case Item::Type::orange:
        return 230;

    case Item::Type::signal_jammer:
        return 320;

    case Item::Type::long_jump_z2:
    case Item::Type::long_jump_z3:
    case Item::Type::long_jump_z4:
        return 1150;

    case Item::Type::map_system:
        return 100;

    default:
        return 0;
    }
}


// Higher rarity value makes an item more common
ItemRarity rarity(Item::Type item)
{
    switch (item) {
    case Item::Type::heart:
    case Item::Type::coin:
    case Item::Type::blaster:
    case Item::Type::count:
    case Item::Type::orange_seeds:
    case Item::Type::long_jump_z2:
    case Item::Type::long_jump_z3:
    case Item::Type::long_jump_z4:
    case Item::Type::long_jump_home:
        return 0;

    case Item::Type::null:
        return 1;

    case Item::Type::orange:
        return 2;

    case Item::Type::engineer_notebook_1:
    case Item::Type::engineer_notebook_2:
        return 3;

    case Item::Type::postal_advert:
        return 0;

    case Item::Type::worker_notebook_1:
    case Item::Type::worker_notebook_2:
        return 3;

    case Item::Type::lethargy:
        return 2;

    case Item::Type::old_poster_1:
        return 3;

    case Item::Type::seed_packet:
        return 2;

    case Item::Type::navigation_pamphlet:
        return 2;

    case Item::Type::signal_jammer:
        return 3;

    case Item::Type::accelerator:
        return 4;

    case Item::Type::explosive_rounds_2:
        return 4;

    case Item::Type::map_system:
        return 7;
    }
    return 0;
}


bool level_in_range(Level level, LevelRange range)
{
    return level >= range[0] and level < range[1];
}


static void
spawn_item_chest(Platform& pfrm, Game& game, MapCoordBuf& free_spots)
{
    Buffer<Item::Type, static_cast<int>(Item::Type::count)> items_in_range;

    for (int i = 0; i < static_cast<int>(Item::Type::count); ++i) {
        auto item = static_cast<Item::Type>(i);

        if (level_in_range(game.level(), level_range(item)) and
            not(game.inventory().item_count(item) and
                item_is_persistent(item)) and
            not(item == Item::Type::lethargy and
                game.inventory().item_count(Item::Type::lethargy)) and
            not(item == Item::Type::signal_jammer and
                game.inventory().item_count(Item::Type::signal_jammer) > 2)) {

            if (pfrm.network_peer().is_connected() and
                item == Item::Type::signal_jammer) {
                continue;
            }

            items_in_range.push_back(item);
        }
    }

    Buffer<Item::Type, 300> distribution;

    ItemRarity cumulative_rarity = 0;
    for (auto item : items_in_range) {
        cumulative_rarity += rarity(item);
    }

    if (cumulative_rarity == 0) {
        error(pfrm,
              "logic error when spawning item chest, "
              "exit to avoid possible division by zero");
        return;
    }

    for (auto item : items_in_range) {
        const auto iters =
            distribution.capacity() * (Float(rarity(item)) / cumulative_rarity);

        for (int i = 0; i < iters; ++i) {
            distribution.push_back(item);
        }
    }

    spawn_entity<ItemChest>(
        pfrm,
        free_spots,
        game.details(),
        distribution[rng::choice(distribution.size(), rng::critical_state)]);
}


COLD bool Game::respawn_entities(Platform& pfrm)
{
    auto clear_entities = [&](auto& buf) { buf.clear(); };

    auto wall_tiles = lisp::get_var("wall-tiles-list");
    auto edge_tiles = lisp::get_var("edge-tiles-list");

    enemies_.transform(clear_entities);
    details_.transform(clear_entities);
    effects_.transform(clear_entities);

    if (scavenger_) {
        if (is_boss_level(level() - 1)) {
            switch (level() - 1) {
            case boss_0_level:
                scavenger_->inventory_.push_back(Item::Type::long_jump_z2);
                break;

            case boss_1_level:
                scavenger_->inventory_.push_back(Item::Type::long_jump_z3);
                break;

            case boss_2_level:
                scavenger_->inventory_.push_back(Item::Type::long_jump_z4);
                break;
            }
        }

        auto& sc_inventory = scavenger_->inventory_;
        const u32 item_count = rng::choice<7>(rng::critical_state);
        while (sc_inventory.size() < item_count) {
            auto item = static_cast<Item::Type>(
                rng::choice<static_cast<int>(Item::Type::count)>(
                    rng::critical_state));

            if ((int)item < (int)Item::Type::inventory_item_start or
                item_is_persistent(item) or item == Item::Type::long_jump_z2 or
                item == Item::Type::long_jump_z3 or
                item == Item::Type::long_jump_z4 or
                item == Item::Type::long_jump_home) {

                continue;
            }

            sc_inventory.push_back(item);
        }

        sc_inventory.push_back(Item::Type::orange);

        if (rng::choice<2>(rng::critical_state)) {
            sc_inventory.push_back(Item::Type::orange);
        }

        rng::shuffle(sc_inventory, rng::critical_state);
    }

    if (level() == 0) {
        details().spawn<Lander>(Vec2<Float>{409.f, 112.f});
        details().spawn<Signpost>(Vec2<Float>{403.f, 112.f},
                                  Signpost::Type::lander);
        details().spawn<ItemChest>(Vec2<Float>{348, 154},
                                   Item::Type::explosive_rounds_2);
        enemies().spawn<Drone>(Vec2<Float>{159, 275});
        if (static_cast<int>(difficulty()) <
            static_cast<int>(Settings::Difficulty::hard)) {
            details().spawn<Item>(
                Vec2<Float>{80, 332}, pfrm, Item::Type::heart);
        }
        tiles_.set_tile(12, 4, Tile::none);
        player_.init({409.1f, 167.2f});
        transporter_.set_position({110, 306});
        return true;
    } else if (level() == boss_0_level + 1) {

        player_.init({380.1f, 100.2f});

        details().spawn<Signpost>(Vec2<Float>{430, 100},
                                  Signpost::Type::memorial);

        transporter_.set_position({431, 277});

        return true;
    }

    auto free_spots = get_free_map_slots(tiles_);

    // Because the scavenger sits in a specially generated secluded part of the
    // map, Game::respawn_entities() is not responsible for creating the
    // scavenger entity. But we do not want to spawn any entities on top of him,
    // so consume nearby free map slots.
    if (scavenger_) {
        for (auto it = free_spots.begin(); it not_eq free_spots.end();) {
            if (distance(to_world_coord(*it), scavenger_->get_position()) <
                40) {
                it = free_spots.erase(it);
            } else {
                ++it;
            }
        }
    }

    const size_t initial_free_spaces = free_spots.size();

    if (free_spots.size() < 6) {
        // The randomly generated map is unacceptably tiny! Try again...
        return false;
    }

    auto player_coord = select_coord(free_spots);
    if (player_coord) {

        const auto wc = world_coord(*player_coord);

        player_.init({wc.x + 0.1f, wc.y + 0.1f});

        // We want to remove adjacent free spaces, so that the player doesn't
        // spawn on top of enemies, and get injured upon entry into a level. The
        // game is meant to be difficult, but not cruel!
        for (auto it = free_spots.begin(); it not_eq free_spots.end();) {
            if (abs(it->x - player_coord->x) < 2 and
                abs(it->y - player_coord->y) < 2) {

                it = free_spots.erase(it);
            } else {
                ++it;
            }
        }

    } else {
        return false;
    }

    auto place_transporter = [&] {
        if (not free_spots.empty()) {
            MapCoord* farthest = free_spots.begin();
            for (auto& elem : free_spots) {
                if (manhattan_length(elem, *player_coord) >
                    manhattan_length(*farthest, *player_coord)) {
                    farthest = &elem;
                }
            }
            const auto target = world_coord(*farthest);
            transporter_.set_position(
                rng::sample<3>({target.x, target.y + 16}, rng::critical_state));
            free_spots.erase(farthest);

            return true;

        } else {
            return false;
        }
    };

    // When the current level is one of the pre-generated boss levels, we do not
    // want to spawn all of the other stuff in the level, just the player.
    const auto boss_level = get_boss_level(level());

    if (boss_level) {

        transporter_.set_position({0, 0});

        MapCoord* farthest = free_spots.begin();
        for (auto& elem : free_spots) {
            if (manhattan_length(elem, *player_coord) >
                manhattan_length(*farthest, *player_coord)) {
                farthest = &elem;
            }
        }
        const auto target = world_coord(*farthest);

        switch (level()) {
        case boss_0_level:
            enemies_.spawn<Wanderer>(target);
            break;

        case boss_1_level:
            enemies_.spawn<Gatekeeper>(target, *this);
            break;

        case boss_2_level:
            enemies_.spawn<Twin>(target);
            free_spots.erase(farthest);
            farthest = free_spots.begin();
            for (auto& elem : free_spots) {
                if (manhattan_length(elem, *player_coord) >
                    manhattan_length(*farthest, *player_coord)) {
                    farthest = &elem;
                }
            }
            enemies_.spawn<Twin>(target);
            break;

        case boss_3_level: {
            enemies_.spawn<InfestedCore>(to_world_coord(Vec2<TIdx>{10, 8}));
            enemies_.spawn<InfestedCore>(to_world_coord(Vec2<TIdx>{5, 8}));
            enemies_.spawn<InfestedCore>(to_world_coord(Vec2<TIdx>{10, 12}));
            enemies_.spawn<InfestedCore>(to_world_coord(Vec2<TIdx>{5, 12}));
            const auto p = to_world_coord(Vec2<TIdx>{8, 11});
            player_.init({p.x - 0.1f, p.y - 0.1f});
        } break;
        }

        int heart_count = 2;

        if (difficulty() == Settings::Difficulty::hard) {
            heart_count = 0;
        }

        while (heart_count > 0) {
            const s8 x = rng::choice<TileMap::width>(rng::critical_state);
            const s8 y = rng::choice<TileMap::height>(rng::critical_state);

            if (is_edge_tile(wall_tiles, edge_tiles, tiles_.get_tile(x, y))) {

                auto wc = to_world_coord({x, y});
                wc.x += 16;

                details_.spawn<Item>(wc, pfrm, Item::Type::heart);

                if (--heart_count == 0) {
                    break;
                }
            }
        }

        return true;
    }

    if (not place_transporter()) {
        return false;
    }

    // if (auto path = find_path(
    //         pfrm,
    //         tiles_,
    //         to_tile_coord(player_.get_position().cast<s32>()).cast<u8>(),
    //         to_tile_coord(transporter_.get_position().cast<s32>())
    //             .cast<u8>())) {
    //     // ...
    // }


    // Sometimes for small maps, and always for large maps, place an item chest
    if (rng::choice<2>(rng::critical_state) or (int) initial_free_spaces > 25) {
        spawn_item_chest(pfrm, *this, free_spots);
    }

    spawn_enemies(pfrm, *this, free_spots);

    // Potentially hide some items in far crannies of the map. If
    // there's no sand nearby, and no items eiher, potentially place
    // an item.
    tiles_.for_each([&](u8 t, s8 x, s8 y) {
        if (is_edge_tile(wall_tiles, edge_tiles, t)) {
            for (int i = x - 1; i < x + 2; ++i) {
                for (int j = y - 1; j < y + 2; ++j) {
                    const auto curr = tiles_.get_tile(i, j);
                    if (is_center_tile(wall_tiles, edge_tiles, curr)) {
                        return;
                    }
                }
            }
            for (auto& item : details_.get<Item>()) {
                if (manhattan_length(item->get_position(),
                                     to_world_coord({x, y})) < 64) {
                    return;
                }
            }
            if (rng::choice<3>(rng::critical_state)) {
                MapCoord c{x, y};

                // NOTE: We want hearts to become less available at higher
                // levels, so that the game naturally becomes more
                // challenging. But for the first few levels, do not make hearts
                // more scarce.
                const auto heart_chance = [&]() -> int {
                    if (difficulty() == Settings::Difficulty::hard) {
                        return 1;
                    } else {
                        return 3 + std::max(level() - 4, Level(0)) * 0.2f;
                    }
                }();


                if (not details_.spawn<Item>(world_coord(c), pfrm, [&] {
                        if (rng::choice(heart_chance, rng::critical_state)) {
                            return Item::Type::coin;
                        } else {
                            return Item::Type::heart;
                        }
                    }())) {
                    // TODO: log error?
                }
            }
        }
    });

    // For map locations with nothing nearby, potentially place an item or
    // something
    tiles_.for_each([&](u8 t, s8 x, s8 y) {
        if (is_center_tile(wall_tiles, edge_tiles, t)) {
            const auto pos = to_world_coord({x, y});

            bool entity_nearby = false;
            auto check_nearby = [&](auto& entity) {
                if (entity_nearby) {
                    return;
                }
                if (distance(entity.get_position(), pos) < 132) {
                    entity_nearby = true;
                }
            };

            auto check_entities = [&](auto& buf) {
                for (auto& entity : buf) {
                    check_nearby(*entity);
                }
            };

            enemies_.transform(check_entities);
            check_entities(details_.get<Item>());
            check_entities(details_.get<ItemChest>());
            check_nearby(transporter_);

            if (not entity_nearby) {
                int adj_sand_tiles = 0;
                for (int i = x - 1; i < x + 1; ++i) {
                    for (int j = y - 1; j < y + 1; ++j) {
                        if (is_center_tile(wall_tiles,
                                           edge_tiles,
                                           tiles_.get_tile(i, j))) {
                            adj_sand_tiles++;
                        }
                    }
                }

                auto wc = to_world_coord({x, y});
                wc.x += 16; // center the item over the tile

                if (adj_sand_tiles < 1) {
                    // If there are no other adjacent non-edge
                    // tiles, definitely place an item
                    details_.spawn<Item>(wc, pfrm, Item::Type::heart);

                } else if (adj_sand_tiles < 3) {
                    // If there is a low number of adjacent
                    // non-edge tiles, possibly place an item
                    if (rng::choice<3>(rng::critical_state) == 0) {
                        details_.spawn<Item>(wc, pfrm, Item::Type::heart);
                    }
                } else {
                    if (rng::choice<5>(rng::critical_state) == 0) {
                        details_.spawn<Item>(wc, pfrm, Item::Type::heart);
                    }
                }
            }
        }
    });

    int heart_count = 0;
    for (auto& item : details_.get<Item>()) {
        if (item->get_type() == Item::Type::heart) {
            ++heart_count;
        }
    }

    const int max_hearts = [&] {
        switch (difficulty()) {
        case Settings::Difficulty::easy:
            return 3;
        case Settings::Difficulty::survival:
        case Settings::Difficulty::hard:
            return 0;
        default:
        case Settings::Difficulty::normal:
            return 2;
        }
    }();

    if (heart_count > max_hearts) {
        const auto item_count = length(details_.get<Item>());

        while (heart_count > max_hearts) {
            auto choice = rng::choice(item_count, rng::critical_state);

            if (auto item = list_ref(details_.get<Item>(), choice)) {
                if ((*item)->get_type() == Item::Type::heart) {
                    (*item)->set_type(Item::Type::coin);
                    --heart_count;
                }
            }
        }
    }

    return true;
}


bool within_view_frustum(const Platform::Screen& screen, const Vec2<Float>& pos)
{
    // FIXME: I thought I had the math correct, but apparantly the view center
    // points to the top left corner of the screen. Ah well...
    const auto view_center = screen.get_view().get_center().cast<s32>();
    const auto view_half_extent = screen.size().cast<s32>() / s32(2);
    Vec2<s32> view_br = {view_center.x + view_half_extent.x * 2,
                         view_center.y + view_half_extent.y * 2};

    return pos.x > view_center.x - 32 and pos.x < view_br.x + 32 and
           pos.y > view_center.y - 32 and pos.y < view_br.y + 32;
}


bool share_item(Platform& pfrm,
                Game& game,
                const Vec2<Float>& position,
                Item::Type item)
{
    if (create_item_chest(game, position, item, false)) {
        net_event::ItemChestShared s;
        s.id_.set((*game.details().get<ItemChest>().begin())->id());
        s.item_ = item;

        net_event::transmit(pfrm, s);

        pfrm.sleep(
            20); // Wait for the item to arrive at the other player's game
        return true;
    }
    return false;
}


SimulatedMiles distance_travelled(Level level)
{
    static const SimulatedMiles low_earth_orbit = 1200;
    static const SimulatedMiles distance_to_moon = 238900;
    static const auto total_distance = distance_to_moon - low_earth_orbit;

    if (level == 0) {
        return low_earth_orbit;
    } else {
        constexpr Level num_levels = boss_max_level - 1;
        static_assert(num_levels not_eq 0);
        const auto fraction_levels_done = level / Float(num_levels);
        return total_distance * fraction_levels_done;
    }
}


void safe_disconnect(Platform& pfrm)
{
    net_event::Disconnect d;
    net_event::transmit(pfrm, d);

    for (int i = 0; i < 10; ++i) {
        pfrm.network_peer().update();
        pfrm.sleep(1);
    }

    pfrm.network_peer().disconnect();
}


Entity* get_entity_by_id(Game& game, Entity::Id id)
{
    if (id == 0) {
        return nullptr;
    }

    if (id == game.player().id()) {
        return &game.player();
    }
    if (id == game.transporter().id()) {
        return &game.transporter();
    }

    Entity* result = nullptr;

    auto match_id = [&](auto& buf) {
        for (auto& entity : buf) {
            if (entity->id() == id) {
                result = entity.get();
            }
        }
    };

    game.enemies().transform(match_id);
    if (result) {
        return result;
    }

    game.details().transform(match_id);
    game.effects().transform(match_id);

    return result;
}
