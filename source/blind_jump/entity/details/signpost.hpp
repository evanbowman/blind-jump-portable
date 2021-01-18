#pragma once

#include "blind_jump/entity/entity.hpp"
#include "blind_jump/localeString.hpp"
#include "memory/buffer.hpp"
#include "state.hpp"


class Signpost : public Entity {
public:
    enum class Type { lander, memorial, knocked_out_peer };

    Signpost(const Vec2<Float>& position, Type type);

    static constexpr bool has_shadow = true;
    static constexpr bool multiface_shadow = true;
    static constexpr bool multiface_sprite = true;

    Type get_type() const
    {
        return type_;
    }

    void update(Platform&, Game&, Microseconds dt);

    auto get_sprites() const
    {
        Buffer<const Sprite*, 2> result;

        switch (type_) {
        case Type::memorial:
            result.push_back(&sprite_);
            result.push_back(&extra_);
            break;

        case Type::knocked_out_peer:
        default:
            result.push_back(&sprite_);
        }

        return result;
    }

    auto get_shadow() const
    {
        Buffer<const Sprite*, 1> result;

        switch (type_) {
        case Type::lander:
        case Type::memorial:
            break;

        case Type::knocked_out_peer:
            break;
        }

        return result;
    }

    const LocaleString* get_dialog() const;

    DeferredState resume_state(Platform& pfrm, Game& game) const;

private:
    Sprite extra_;
    Type type_;
};
