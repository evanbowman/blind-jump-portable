#pragma once

#include "blind_jump/network_event.hpp"
#include "entity.hpp"
#include "platform/platform.hpp"


////////////////////////////////////////////////////////////////////////////////
//
// Intended for multiplayer. Represents a player character from another game
// instance, connected via a NetworkPeer. This class tries to smooth out the
// player's movement as much as possible, but in practice, this is always going
// to be a little janky, given the connection speeds that we're dealing with on
// some platforms, for example, the gameboy's game link cable manages about
// 1Kb/second.
//
////////////////////////////////////////////////////////////////////////////////


class PeerPlayer : public Entity {
public:
    PeerPlayer(Platform& pfrm);

    void sync(Platform& pfrm, Game& game, const net_event::PlayerInfo& info);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    auto get_sprites() const
    {
        std::array<const Sprite*, 2> ret;
        ret[0] = &sprite_;
        ret[1] = &head_;
        return ret;
    }

    const Sprite& get_shadow()
    {
        return shadow_;
    }

    const Sprite& get_blaster_sprite()
    {
        return blaster_;
    }

    bool& warping()
    {
        return warping_;
    }

private:
    void update_sprite_position();

    std::optional<Platform::DynamicTexturePtr> dynamic_texture_;
    Vec2<Float> speed_;
    Sprite shadow_;
    Sprite head_;
    Vec2<Float> interp_offset_;
    Sprite blaster_;
    bool warping_ = false;
};
