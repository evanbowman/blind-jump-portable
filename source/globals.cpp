#include "globals.hpp"


static Globals __globals;


Globals& globals()
{
    return ::__globals;
}
