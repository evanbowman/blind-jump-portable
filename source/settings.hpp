#pragma once

#include "graphics/contrast.hpp"
#include "localization.hpp"
#include "severity.hpp"


struct Settings {
    enum Difficulty { normal, hard, survival, count };

    LocaleLanguage language_ = LocaleLanguage::null;
    bool show_stats_ = false;
    bool dynamic_camera_ = true;
    Contrast contrast_ = 0;
    Difficulty difficulty_ = Difficulty::normal;
    Severity log_severity_ = Severity::error;
};
