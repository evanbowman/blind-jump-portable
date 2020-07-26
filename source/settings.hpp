#include "graphics/contrast.hpp"
#include "localization.hpp"


enum class Difficulty { normal, hard, count };


struct Settings {
    LocaleLanguage language_ = LocaleLanguage::null;
    bool show_fps_ = false;
    bool dynamic_camera_ = true;
    Contrast contrast_ = 0;
    Difficulty difficulty_ = Difficulty::normal;
};
