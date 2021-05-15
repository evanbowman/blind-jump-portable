

extern const unsigned char file_init[];
//;
extern const unsigned char file_pre_levelgen[];
//;
extern const unsigned char file_post_levelgen[];
//;
extern const unsigned char file_waypoint_clear[];
//;
extern const unsigned char file_english[];
//;
extern const unsigned char file_spanish[];
//;
extern const unsigned char file_chinese[];
//;
extern const unsigned char file_russian[];
//

static const struct {
    const char* root_;
    const char* name_;
    const unsigned char* data_;
} files[] = {

    {"scripts", "init.lisp", file_init},
//;
    {"scripts", "pre_levelgen.lisp", file_pre_levelgen},
//;
    {"scripts", "post_levelgen.lisp", file_post_levelgen},
//;
    {"scripts", "waypoint_clear.lisp", file_waypoint_clear},
//;
    {"strings", "english.txt", file_english},
//;
    {"strings", "spanish.txt", file_spanish},
//;
    {"strings", "chinese.txt", file_chinese},
//;
    {"strings", "russian.txt", file_russian},
//
};
