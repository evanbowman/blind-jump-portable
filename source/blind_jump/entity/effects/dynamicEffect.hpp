#pragma once

#include "blind_jump/entity/entity.hpp"
#include "function.hpp"
#include "platform/platform.hpp"


class DynamicEffect : public Entity {
public:
    using UpdateCallback =
        Function<16,
                 void(Platform&, Game&, const Microseconds&, DynamicEffect&)>;

    DynamicEffect(
        const Vec2<Float>& position,
        Platform::DynamicTexturePtr texture,
        Microseconds interval,
        u16 texture_start,
        u8 frame_count,
        UpdateCallback update_callback =
            [](Platform&, Game&, const Microseconds&, DynamicEffect&) {});

    void update(Platform&, Game&, Microseconds dt);

    Sprite& sprite()
    {
        return sprite_;
    }

    void set_backdrop(bool backdrop)
    {
        backdrop_ = backdrop;
    }

    bool is_backdrop() const
    {
        return backdrop_;
    }

private:
    Platform::DynamicTexturePtr texture_;
    UpdateCallback update_callback_;
    Microseconds timer_;
    const Microseconds interval_;
    u16 texture_start_;
    const u8 frame_count_;
    bool backdrop_ = false;
};
