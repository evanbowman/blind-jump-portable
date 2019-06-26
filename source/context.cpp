#include "context.hpp"


static Context* bound = nullptr;


void bind_context(Context* ct)
{
    bound = ct;
}


Context* bound_context()
{
    return bound;
}
