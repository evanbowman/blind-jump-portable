#include "localization.hpp"
#include "graphics/contrast.hpp"


struct Settings {
    LocaleLanguage language_ = LocaleLanguage::null;
    bool show_fps_ = false;
    bool dynamic_camera_ = true;
    Contrast contrast_ = 0;
};
