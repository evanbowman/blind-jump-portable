#pragma once

#include "memory/rc.hpp"

#ifdef __GBA__
#define SCRATCH_BUFFER_SIZE 2000
#else
#define SCRATCH_BUFFER_SIZE 4000
#endif // __GBA__


struct ScratchBuffer {
    // NOTE: do not make any assumptions about the alignment of the data_
    // member.
    char data_[SCRATCH_BUFFER_SIZE];
};

static constexpr const int scratch_buffer_count = 100;
using ScratchBufferPtr = Rc<ScratchBuffer, scratch_buffer_count>;
