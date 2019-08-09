#pragma once

#define COLD [[gnu::cold]]
#define HOT [[gnu::hot]]

#ifdef __GNUC__
#define UNLIKELY(COND) __builtin_expect((COND), false)
#else
#define UNLIKELY(COND) (COND)
#endif
