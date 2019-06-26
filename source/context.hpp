#pragma once

#include "platform.hpp"


struct Context {
    Platform platform_;
};


void bind_context(Context* ct);


Context* bound_context();
