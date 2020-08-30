#pragma once

#include <algorithm>

#include "graphics/sprite.hpp"


class Platform;
class Game;

class Entity {
public:
    using Health = s32;
    using Id = u32;


    Entity();


    Entity(Health health);


    // Entity(Entity&) = delete;


    void update(Platform&, Game&, Microseconds);


    Health get_health() const;


    bool alive() const;


    const Sprite& get_sprite() const;
    Sprite& get_sprite();


    void add_health(Health amount);


    const Vec2<Float>& get_position() const;


    static constexpr bool multiface_sprite = false;
    static constexpr bool has_shadow = false;
    static constexpr bool multiface_shadow = false;


    void set_health(Health health)
    {
        health_ = health;
    }


    bool visible() const
    {
        return visible_;
    }


    // IMPORTANT! The automatically assigned Entity ids are guaranteed to be
    // synchronized between multiplayer games at the start of each level. But
    // after the level has started, there is no guarantee that newly spawned
    // entities will have the same ID. If you need to synchronize entities
    // created after level generation, you should exercise caution. See, for
    // example, how the game allows players to trade items. One player drops an
    // item chest, and sends an event over the network, with the item, and the
    // entity id of the item chest. Then, the receiver must check that no
    // existing entities within the same group have a matching id. In the event
    // of an id collision, the receiver should not spawn the new entity, and
    // send its own event back to the sender, if appropriate.
    static void reset_ids();

    static Id max_id();
    // Be careful with this function! Before calling, you should make sure that
    // no similar extant entities share the same id.
    void override_id(Id id);


    Id id() const
    {
        return id_;
    }


    // This is VERY BAD CODE. Basically, the rendering loop already determines
    // which objects are visible within the window when drawing sprites. To save
    // CPU cycles, we are marking an object visible during a rendering pass, so
    // that the game logic doesn't need to repeat the visibility calculation.
    void mark_visible(bool visible)
    {
        visible_ = visible;
    }

    void on_death(Platform&, Game&)
    {
    }

    void set_position(const Vec2<Float>& position);

protected:
    void debit_health(Health amount = 1)
    {
        health_ = std::max(Health(0), health_ - amount);
    }

    void kill()
    {
        health_ = 0;
    }

    Sprite sprite_;
    Vec2<Float> position_;

private:
    Health health_;
    Id id_;
    bool visible_ = false;
};


#include <memory>


template <typename T> using EntityRef = std::unique_ptr<T, void (*)(T*)>;
