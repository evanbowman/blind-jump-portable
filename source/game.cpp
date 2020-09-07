#include "game.hpp"
#include "bulkAllocator.hpp"
#include "function.hpp"
#include "graphics/overlay.hpp"
#include "number/random.hpp"
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

    if (pfrm.read_save_data(save_buffer, sizeof(PersistentData))) {
        PersistentData* loaded = (PersistentData*)save_buffer;
        if (loaded->magic_ == PersistentData::magic_val) {
            info(pfrm, "loaded existing save file");
            persistent_data_ = *loaded;

            if (level() > 0) {
                loaded->reset(pfrm);
                pfrm.write_save_data(save_buffer, sizeof(PersistentData));
            }

            return true;
        }
    }

    return false;
}


Game::Game(Platform& pfrm)
    : player_(pfrm), score_(0), next_state_(null_state()), state_(null_state())
{
    if (not this->load_save_data(pfrm)) {
        persistent_data_.reset(pfrm);
        info(pfrm, "no save file found");
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

    player_.set_health(persistent_data_.player_health_);
    score_ = persistent_data_.score_;
    inventory_ = persistent_data_.inventory_;

    for (u32 i = 0; i < persistent_data_.powerup_count_; ++i) {
        powerups_.push_back(persistent_data_.powerups_[i]);
    }

    rng::critical_state = persistent_data_.seed_;

    init_script(pfrm);

    pfrm.logger().set_threshold(persistent_data_.settings_.log_severity_);

    pfrm.screen().enable_night_mode(persistent_data_.settings_.night_mode_);

    if (persistent_data_.settings_.language_ not_eq LocaleLanguage::null) {
        locale_set_language(persistent_data_.settings_.language_);
    } else {
        const auto lang = lisp::get_var("default-lang");

        if (lang->type_ not_eq lisp::Value::Type::symbol) {
            while (true)
                ; // TODO: fatal error
        }

        const auto lang_enum = [&] {
            if (str_cmp(lang->symbol_.name_, "english") == 0) {
                return LocaleLanguage::english;
            }
            return LocaleLanguage::null;
        }();

        if (lang_enum not_eq LocaleLanguage::null) {
            locale_set_language(lang_enum);
            persistent_data_.settings_.language_ = lang_enum;

            StringBuffer<64> buf;
            buf += "saved default language as ";
            buf += lang->symbol_.name_;

            info(pfrm, buf.c_str());
        }
    }

    state_ = State::initial();

    pfrm.load_overlay_texture("overlay");

    if (inventory().item_count(Item::Type::blaster) == 0) {
        inventory().push_item(pfrm, *this, Item::Type::blaster);
    }

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
        pfrm.write_save_data((byte*)&persistent_data_, sizeof persistent_data_);
    });
}


// We're storing a pointer to the C++ game class instance in a userdata object,
// so that routines registered with the lisp interpreter can change game state.
static Game* interp_get_game()
{
    auto game = lisp::get_var("*game*");
    if (game->type_ not_eq lisp::Value::Type::user_data) {
        return nullptr;
    }
    return (Game*)game->user_data_.obj_;
}


static Platform* interp_get_pfrm()
{
    auto pfrm = lisp::get_var("*pfrm*");
    if (pfrm->type_ not_eq lisp::Value::Type::user_data) {
        return nullptr;
    }
    return (Platform*)pfrm->user_data_.obj_;
}


static Entity* get_entity_by_id(Game& game, Entity::Id id)
{
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

    game.effects().transform(match_id);
    game.details().transform(match_id);
    game.enemies().transform(match_id);

    return result;
}


HOT void Game::update(Platform& pfrm, Microseconds delta)
{
    if (next_state_) {
        next_state_->enter(pfrm, *this, *state_);

        state_ = std::move(next_state_);
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

    pfrm.speaker().set_position(player_.get_position());

    next_state_ = state_->update(pfrm, *this, delta);

    if (next_state_) {
        state_->exit(pfrm, *this, *next_state_);
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

    if (-view_size.y / 2 <= slope * (view_size.x / 2) and
        slope * (view_size.x / 2) <= view_size.y / 2) {
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
    } else if (-view_size.x / 2 <= (view_size.y / 2) / slope and
               (view_size.y / 2) / slope <= view_size.x / 2) {
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
    effects_.transform(show_sprites);
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

    std::sort(display_buffer.begin(),
              display_buffer.end(),
              [](const auto& l, const auto& r) {
                  return l->get_position().y > r->get_position().y;
              });

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
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, },
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, },
    {0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    {0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, },
    {0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, },
    {0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, },
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, },
    {0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, },
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, },
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, },
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, },
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, },
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, },
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
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
        static constexpr const BossLevel ret{&boss_level_2,
            "spritesheet_boss2",
            &boss_level_2_gr
        };
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
    "hiraeth",
    Microseconds{0},
    ColorConstant::electric_blue,
    ColorConstant::picton_blue,
    ColorConstant::aerospace_orange,
    [](Platform& pfrm, Game&) {
        draw_starfield(pfrm);

        const int x = 16;
        const int y = 16;

        draw_image(pfrm, 61, x, y, 3, 3, Layer::background);
    },
    [](int x, int y, const TileMap&) { return 0; }};


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
    "september",
    Microseconds{0},
    ColorConstant::cerulean_blue,
    ColorConstant::picton_blue,
    ColorConstant::aerospace_orange,
    [](Platform& pfrm, Game&) {
        draw_starfield(pfrm);

        const int x = 12;
        const int y = 12;

        draw_image(pfrm, 120, x, y, 9, 9, Layer::background);
    },
    [](int x, int y, const TileMap&) {
        if (rng::choice<13>(rng::critical_state) == 0) {
            switch (rng::choice<2>(rng::critical_state)) {
            case 0:
                return 18;
            case 1:
                return 19;
            }
        }
        return 0;
    }};


const ZoneInfo& zone_info(Level level)
{
    if (UNLIKELY(level > boss_2_level)) {
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
            [](Platform&, Game&) {},
            [](int, int, const TileMap&) { return 0; }};
        return null_zone;
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


void Game::init_script(Platform& pfrm)
{
    lisp::init(pfrm);

    lisp::set_var("*game*", lisp::make_userdata(this));
    lisp::set_var("*pfrm*", lisp::make_userdata(&pfrm));

    lisp::set_var("level", lisp::make_function([](int argc) {
                      if (auto game = interp_get_game()) {
                          if (argc == 1) {
                              L_EXPECT_OP(0, integer);

                              game->persistent_data().level_ =
                                  lisp::get_op(0)->integer_.value_;

                          } else {
                              return lisp::make_integer(
                                  game->persistent_data().level_);
                          }
                      }
                      return L_NIL;
                  }));

    lisp::set_var("zone", lisp::make_function([](int argc) {
                      if (auto game = interp_get_game()) {
                          auto& zone =
                              zone_info(game->persistent_data().level_);
                          if (zone == zone_1) {
                              return lisp::make_integer(1);
                          } else if (zone == zone_2) {
                              return lisp::make_integer(2);
                          } else if (zone == zone_3) {
                              return lisp::make_integer(3);
                          } else {
                              // Error?
                              return lisp::make_integer(3);
                          }
                      }
                      return L_NIL;
                  }));

    lisp::set_var("add-items", lisp::make_function([](int argc) {
                      if (auto game = interp_get_game()) {
                          for (int i = 0; i < argc; ++i) {
                              const auto item = static_cast<Item::Type>(
                                  lisp::get_op(i)->integer_.value_);

                              if (auto pfrm = interp_get_pfrm()) {
                                  game->inventory().push_item(
                                      *pfrm, *game, item);
                              }
                          }
                      }

                      return L_NIL;
                  }));

    lisp::set_var("set-hp", lisp::make_function([](int argc) {
                      L_EXPECT_ARGC(argc, 2);
                      L_EXPECT_OP(0, integer);
                      L_EXPECT_OP(1, integer);

                      auto game = interp_get_game();
                      if (not game) {
                          return L_NIL;
                      }

                      auto entity = get_entity_by_id(
                          *game, lisp::get_op(1)->integer_.value_);
                      if (entity) {
                          entity->set_health(lisp::get_op(0)->integer_.value_);
                      }

                      return L_NIL;
                  }));

    lisp::set_var(
        "kill", lisp::make_function([](int argc) {
            auto game = interp_get_game();
            if (not game) {
                return L_NIL;
            }

            for (int i = 0; i < argc; ++i) {

                if (lisp::get_op(i)->type_ not_eq lisp::Value::Type::integer) {
                    const auto err = lisp::Error::Code::invalid_argument_type;
                    return lisp::make_error(err);
                }

                auto entity =
                    get_entity_by_id(*game, lisp::get_op(i)->integer_.value_);

                if (entity) {
                    entity->set_health(0);
                }
            }

            return L_NIL;
        }));

    lisp::set_var("get-pos", lisp::make_function([](int argc) {
                      L_EXPECT_ARGC(argc, 1);
                      L_EXPECT_OP(0, integer);

                      auto game = interp_get_game();
                      if (not game) {
                          return L_NIL;
                      }

                      auto entity = get_entity_by_id(
                          *game, lisp::get_op(0)->integer_.value_);
                      if (not entity) {
                          return L_NIL;
                      }

                      return lisp::make_cons(
                          lisp::make_integer(entity->get_position().x),
                          lisp::make_integer(entity->get_position().y));
                  }));

    lisp::set_var(
        "enemies", lisp::make_function([](int argc) {
            auto game = interp_get_game();
            if (not game) {
                return L_NIL;
            }

            lisp::Value* lat = L_NIL;

            game->enemies().transform([&](auto& buf) {
                for (auto& enemy : buf) {
                    lisp::push_op(lat); // To keep it from being collected
                    lat = lisp::make_cons(L_NIL, lat);
                    lisp::pop_op(); // lat

                    lisp::push_op(lat);
                    lat->cons_.set_car(lisp::make_integer(enemy->id()));
                    lisp::pop_op(); // lat
                }
            });

            return lat;
        }));

    lisp::set_var("log-severity", lisp::make_function([](int argc) {
                      auto game = interp_get_game();
                      if (not game) {
                          return L_NIL;
                      }

                      if (argc == 0) {
                          const auto sv =
                              game->persistent_data().settings_.log_severity_;
                          return lisp::make_integer(static_cast<int>(sv));
                      } else {
                          L_EXPECT_ARGC(argc, 1);
                          L_EXPECT_OP(0, integer);

                          auto val = static_cast<Severity>(
                              lisp::get_op(0)->integer_.value_);
                          game->persistent_data().settings_.log_severity_ = val;
                      }

                      return L_NIL;
                  }));

    lisp::set_var("register-controller", lisp::make_function([](int argc) {
                      L_EXPECT_ARGC(argc, 7);

                      for (int i = 0; i < 7; ++i) {
                          L_EXPECT_OP(i, integer);
                      }

                      auto pfrm = interp_get_pfrm();
                      if (not pfrm) {
                          return L_NIL;
                      }

                      pfrm->keyboard().register_controller(
                          {lisp::get_op(6)->integer_.value_,
                           lisp::get_op(5)->integer_.value_,
                           lisp::get_op(4)->integer_.value_,
                           lisp::get_op(3)->integer_.value_,
                           lisp::get_op(2)->integer_.value_,
                           lisp::get_op(1)->integer_.value_,
                           lisp::get_op(0)->integer_.value_});

                      return L_NIL;
                  }));

    lisp::set_var("platform", lisp::make_function([](int argc) {
                      auto pfrm = interp_get_pfrm();
                      if (not pfrm) {
                          return L_NIL;
                      }
                      return lisp::make_symbol(pfrm->device_name().c_str());
                  }));

    lisp::set_var("set-tile", lisp::make_function([](int argc) {
                      L_EXPECT_ARGC(argc, 4);
                      for (int i = 0; i < 4; ++i) {
                          L_EXPECT_OP(i, integer);
                      }

                      auto pfrm = interp_get_pfrm();
                      if (not pfrm) {
                          return L_NIL;
                      }

                      pfrm->set_tile((Layer)lisp::get_op(3)->integer_.value_,
                                     lisp::get_op(2)->integer_.value_,
                                     lisp::get_op(1)->integer_.value_,
                                     lisp::get_op(0)->integer_.value_);

                      return L_NIL;
                  }));

    lisp::set_var("get-tile", lisp::make_function([](int argc) {
                      L_EXPECT_ARGC(argc, 3);
                      for (int i = 0; i < 3; ++i) {
                          L_EXPECT_OP(i, integer);
                      }

                      auto pfrm = interp_get_pfrm();
                      if (not pfrm) {
                          return L_NIL;
                      }

                      auto tile = pfrm->get_tile(
                          (Layer)lisp::get_op(2)->integer_.value_,
                          lisp::get_op(1)->integer_.value_,
                          lisp::get_op(0)->integer_.value_);

                      return lisp::make_integer(tile);
                  }));

    lisp::set_var(
        "pattern-replace-tile", lisp::make_function([](int argc) {
            L_EXPECT_ARGC(argc, 2);
            L_EXPECT_OP(1, integer);
            L_EXPECT_OP(0, cons);
            lisp::Value* filter[3][3];

            auto row = lisp::get_op(0);
            for (int y = 0; y < 3; ++y) {
                if (row->type_ not_eq lisp::Value::Type::cons) {
                    while (true)
                        ;
                }

                auto cell = row->cons_.car();
                for (int x = 0; x < 3; ++x) {
                    if (cell->type_ not_eq lisp::Value::Type::cons) {
                        while (true)
                            ;
                    }

                    filter[x][y] = cell->cons_.car();

                    cell = cell->cons_.cdr();
                }

                row = row->cons_.cdr();
            }

            auto pfrm = interp_get_pfrm();
            if (not pfrm) {
                return L_NIL;
            }

            for (int x = 0; x < TileMap::width; ++x) {
                for (int y = 0; y < TileMap::height; ++y) {
                    bool match = true;
                    for (int xx = -1; xx < 2; ++xx) {
                        for (int yy = -1; yy < 2; ++yy) {
                            int filter_x = xx + 1;
                            int filter_y = yy + 1;

                            auto fval = filter[filter_x][filter_y];
                            if (fval->type_ == lisp::Value::Type::integer) {
                                if (fval->integer_.value_ not_eq
                                    (int) pfrm->get_tile(
                                        Layer::map_0, x + xx, y + yy)) {
                                    match = false;
                                    goto DONE;
                                }
                            } else if (fval->type_ == lisp::Value::Type::cons) {
                                bool submatch = false;
                                while (fval not_eq L_NIL) {
                                    if (fval->type_ not_eq
                                        lisp::Value::Type::cons) {
                                        return L_NIL;
                                    }

                                    if (fval->cons_.car()->type_ not_eq
                                        lisp::Value::Type::integer) {
                                        return L_NIL;
                                    }

                                    if (fval->cons_.car()->integer_.value_ ==
                                        (int)pfrm->get_tile(
                                            Layer::map_0, x + xx, y + yy)) {
                                        submatch = true;
                                    }

                                    fval = fval->cons_.cdr();
                                }
                                if (not submatch) {
                                    match = false;
                                    goto DONE;
                                }
                            }
                        }
                    }
                DONE:
                    if (match) {
                        pfrm->set_tile(Layer::map_0,
                                       x,
                                       y,
                                       lisp::get_op(1)->integer_.value_);
                    }
                }
            }

            return L_NIL;
        }));

    if (auto eval_opt = pfrm.get_opt('e')) {
        lisp::dostring(eval_opt);
    }

    lisp::dostring(pfrm.load_script("init.lisp"));
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
    if (set_level) {
        persistent_data_.level_ = *set_level;
    } else {
        persistent_data_.level_ += 1;
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

    persistent_data_.score_ = score_;
    persistent_data_.player_health_ = player_.get_health();
    persistent_data_.seed_ = rng::critical_state;
    persistent_data_.inventory_ = inventory_;
    persistent_data_.store_powerups(powerups_);

    pfrm.load_tile0_texture(current_zone(*this).tileset0_name_);
    pfrm.load_tile1_texture(current_zone(*this).tileset1_name_);

    lisp::dostring(pfrm.load_script("pre_levelgen.lisp"));

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

    lisp::dostring(pfrm.load_script("post_levelgen.lisp"));

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
        error(pfrm, "fatal error in floodfill");
        pfrm.fatal();
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
    } else {
        tiles_.for_each([&](auto& t, int, int) {
            t = rng::choice<int(Tile::sand)>(rng::critical_state);
        });

        const auto cell_iters =
            lisp::get_var("cell-iters")->expect<lisp::Integer>().value_;

        for (int i = 0; i < cell_iters; ++i) {
            cell_automata_advance(tiles_, workspace);
        }
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


static void debug_log_tilemap(Platform& pfrm, TileMap& map)
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


COLD void Game::regenerate_map(Platform& pfrm)
{
    ScratchBufferBulkAllocator mem(pfrm);

    auto temporary = mem.alloc<TileMap>();
    if (not temporary) {
        error(pfrm, "falled to create temporary tilemap");
        pfrm.fatal();
    }

    seed_map(pfrm, *temporary);
    debug_log_tilemap(pfrm, tiles_);

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

    if (level() not_eq 0 and not is_boss_level(level())) {
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

    auto grass_overlay = mem.alloc<TileMap>([&](u8& t, int, int) {
        if (rng::choice<3>(rng::critical_state)) {
            t = Tile::none;
        } else {
            t = Tile::plate;
        }
    });

    if (not grass_overlay) {
        error(pfrm, "failed to alloc map1 workspace");
        pfrm.fatal();
    }

    const auto cell_iters = lisp::loadv<lisp::Integer>("cell-iters").value_;

    for (int i = 0; i < cell_iters; ++i) {
        cell_automata_advance(*grass_overlay, *temporary);
    }

    debug_log_tilemap(pfrm, *grass_overlay);

    if (auto info = get_boss_level(level())) {
        if (info->grass_pattern_) {
            for (int x = 0; x < TileMap::width; ++x) {
                for (int y = 0; y < TileMap::height; ++y) {
                    grass_overlay
                        ->set_tile(x, y, info->grass_pattern_->get(x, y));
                }
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
            if (tile == Tile::plate and
                grass_overlay->get_tile(x, y) == Tile::none) {
                const auto up = tiles_.get_tile(x, y + 1);
                const auto down = tiles_.get_tile(x, y - 1);
                const auto left = tiles_.get_tile(x - 1, y);
                const auto right = tiles_.get_tile(x + 1, y);

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
    }

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


using MapCoord = Vec2<TIdx>;
using MapCoordBuf = Buffer<MapCoord, TileMap::tile_count>;


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
        if (not(tile == Tile::sand or tile == Tile::sand_sprouted
                // FIXME-TILES
                //  or
                // (u8(tile) >= u8(Tile::grass_sand) and
                //  u8(tile) < u8(Tile::grass_plate))
                )) {
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
spawn_enemies(Platform& pfrm, Game& game, MapCoordBuf& free_spots)
{
    // We want to spawn enemies based on both the difficulty of level, and the
    // number of free spots that are actually available on the map. Some
    // enemies, like snakes, take up a lot of resources on constrained systems,
    // so we can only display one or _maybe_ two.
    //
    // Some other enemies require a lot of map space to fight effectively, so
    // they are banned from tiny maps.

    const auto max_density = 160;

    const auto min_density = 70;

    const auto density_incr = 4;


    const auto density = std::min(
        Float(max_density) / 1000,
        Float(min_density) / 1000 + game.level() * Float(density_incr) / 1000);

    const auto max_enemies = 6;

    const int spawn_count =
        std::max(std::min(max_enemies, int(free_spots.size() * density)), 1);

    struct EnemyInfo {
        int min_level_;
        Function<64, void()> spawn_;
        int max_level_ = std::numeric_limits<Level>::max();
        int max_allowed_ = 1000;
    } info[] = {
        {0,
         [&]() {
             spawn_entity<Drone>(pfrm, free_spots, game.enemies());
             if (game.level() > 6) {
                 spawn_entity<Drone>(pfrm, free_spots, game.enemies());
             }
         },
         10},
        {5,
         [&]() { spawn_entity<Dasher>(pfrm, free_spots, game.enemies()); },
         boss_1_level},
        {7,
         [&]() {
             spawn_entity<SnakeHead>(pfrm, free_spots, game.enemies(), game);
         },
         boss_0_level,
         1},
        {1, [&]() { spawn_entity<Turret>(pfrm, free_spots, game.enemies()); }},
        {boss_0_level,
         [&]() { spawn_entity<Scarecrow>(pfrm, free_spots, game.enemies()); },
         boss_1_level}};


    Buffer<EnemyInfo*, 100> distribution;

    while (not distribution.full()) {
        auto selected = &info[rng::choice<sizeof info / sizeof(EnemyInfo)>(
            rng::critical_state)];

        if (selected->min_level_ > std::max(Level(0), game.level()) or
            selected->max_level_ < game.level()) {
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

        ++i;
        choice->max_allowed_--;
        choice->spawn_();
    }
}


using LevelRange = std::array<Level, 2>;


static LevelRange level_range(Item::Type item)
{
    static const auto max = std::numeric_limits<Level>::max();
    static const auto min = std::numeric_limits<Level>::min();

    switch (item) {
    case Item::Type::old_poster_1:
    case Item::Type::surveyor_logbook:
        return {min, boss_0_level};

    case Item::Type::engineer_notebook:
        return {boss_0_level, max};

    default:
        return {min, max};
    }
}


using ItemRarity = int;


// Higher rarity value makes an item more common
ItemRarity rarity(Item::Type item)
{
    switch (item) {
    case Item::Type::heart:
    case Item::Type::coin:
    case Item::Type::blaster:
    case Item::Type::count:
    case Item::Type::orange_seeds:
        return 0;

    case Item::Type::null:
        return 1;

    case Item::Type::orange:
        return 2;

    case Item::Type::engineer_notebook:
        return 3;

    case Item::Type::postal_advert:
        return 0;

    case Item::Type::surveyor_logbook:
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


static bool level_in_range(Level level, LevelRange range)
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
                (item == Item::Type::lethargy or
                 item == Item::Type::signal_jammer)) {
                // Lethargy changes the timestep, and the signal jammer changes
                // the enemy's target to something other than one of the
                // players. Due to how these items complicate keeping
                // multiplayer state synchronized, do not spawn them when remote
                // players have connected.
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

    if (level() == 0) {
        details().spawn<Lander>(Vec2<Float>{409.f, 112.f});
        details().spawn<ItemChest>(Vec2<Float>{348, 154},
                                   Item::Type::explosive_rounds_2);
        enemies().spawn<Drone>(Vec2<Float>{159, 275});
        if (static_cast<int>(difficulty()) <
            static_cast<int>(Settings::Difficulty::hard)) {
            details().spawn<Item>(
                Vec2<Float>{80, 332}, pfrm, Item::Type::heart);
        }
        tiles_.set_tile(12, 4, Tile::none);
        player_.move({409.1f, 167.2f});
        transporter_.set_position({110, 306});
        return true;
    }

    auto free_spots = get_free_map_slots(tiles_);

    const size_t initial_free_spaces = free_spots.size();

    if (free_spots.size() < 6) {
        // The randomly generated map is unacceptably tiny! Try again...
        return false;
    }

    auto player_coord = select_coord(free_spots);
    if (player_coord) {

        const auto wc = world_coord(*player_coord);

        player_.move({wc.x + 0.1f, wc.y + 0.1f});

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
