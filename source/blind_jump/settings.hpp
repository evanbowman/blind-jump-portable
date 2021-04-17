#pragma once

#include "graphics/contrast.hpp"
#include "number/endian.hpp"
#include "platform/key.hpp"
#include "severity.hpp"


struct Settings {
    enum class Difficulty : u8 { easy, normal, hard, survival, count };

    enum class ButtonMode : u8 { strafe_separate, strafe_combined, count };

    enum class CameraMode : u8 { fixed, tracking_weak, tracking_strong, count };

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

    static constexpr const auto default_action1_key = Key::action_2;
    static constexpr const auto default_action2_key = Key::action_1;
    Key action1_key_ = default_action1_key;
    Key action2_key_ = default_action2_key;

    bool show_speedrun_clock_ = true;
    bool rumble_enabled_ = true;

    ButtonMode button_mode_ = ButtonMode::strafe_combined;
    CameraMode camera_mode_ = CameraMode::tracking_weak;

    bool initial_lang_selected_ = false;
};
