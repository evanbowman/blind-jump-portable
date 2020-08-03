#pragma once

#include "entity.hpp"
#include "network_event.hpp"


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
    PeerPlayer();

    void sync(const net_event::PlayerInfo& info);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    const Sprite& get_shadow()
    {
        return shadow_;
    }

    const Sprite& get_blaster_sprite()
    {
        return blaster_;
    }

private:
    void update_sprite_position();

    Vec2<Float> speed_;
    Sprite shadow_;
    Vec2<Float> interp_offset_;
    Microseconds anim_timer_;
    Sprite blaster_;
};
