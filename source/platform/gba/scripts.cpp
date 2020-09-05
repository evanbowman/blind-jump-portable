

extern const unsigned char script_init[];
//;
extern const unsigned char script_pre_levelgen[];
//;
extern const unsigned char script_post_levelgen[];
//

static const struct {
    const char* name_;
    const unsigned char* data_;
} scripts[] = {

    {"init.lisp", script_init},
    //;
    {"pre_levelgen.lisp", script_pre_levelgen},
    //;
    {"post_levelgen.lisp", script_post_levelgen},
    //
};
