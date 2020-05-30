#pragma once

#include "string.hpp"


StringBuffer<31> get_conf(const char* source,
                          const char* section,
                          const char* key);
