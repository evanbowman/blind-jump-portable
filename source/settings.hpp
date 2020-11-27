#pragma once

#include "graphics/contrast.hpp"
#include "number/endian.hpp"
#include "platform/key.hpp"
#include "severity.hpp"


struct Settings {
    enum Difficulty : u8 { easy, normal, hard, survival, count };

    Settings()
    {
        language_.set(1);
    }

    HostInteger<s32> language_;
    bool show_stats_ = false;
    bool night_mode_ = false;
    Contrast contrast_ = 0;
    Difficulty difficulty_ = Difficulty::normal;
    Severity log_severity_ = Severity::error;

    static constexpr const auto default_action1_key = Key::action_1;
    static constexpr const auto default_action2_key = Key::action_2;
    Key action1_key_ = default_action1_key;
    Key action2_key_ = default_action2_key;

    bool show_speedrun_clock_ = true;
};
