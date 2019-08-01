#pragma once

#define COLD [[gnu::cold]]
#define HOT [[gnu::hot]]

#define UNLIKELY(COND) __builtin_expect((COND), false)
