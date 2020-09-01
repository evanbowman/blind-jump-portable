#include "memory/pool.hpp"
#include "memory/rc.hpp"
#include "platform/platform.hpp"


// This file should contain the minimal subset of platform code necessary for
// running the interpreter.


ObjectPool<RcBase<Platform::ScratchBuffer,
                  Platform::scratch_buffer_count>::ControlBlock,
           Platform::scratch_buffer_count>
    scratch_buffer_pool;


static int scratch_buffers_in_use = 0;
static int scratch_buffer_highwater = 0;


Platform::ScratchBufferPtr Platform::make_scratch_buffer()
{
    auto finalizer = [](RcBase<Platform::ScratchBuffer,
                               scratch_buffer_count>::ControlBlock* ctrl) {
        --scratch_buffers_in_use;
        ctrl->pool_->post(ctrl);
    };

    auto maybe_buffer = Rc<ScratchBuffer, scratch_buffer_count>::create(
        &scratch_buffer_pool, finalizer);
    if (maybe_buffer) {
        ++scratch_buffers_in_use;
        if (scratch_buffers_in_use > scratch_buffer_highwater) {
            scratch_buffer_highwater = scratch_buffers_in_use;
        }
        return *maybe_buffer;
    } else {
        while (true)
            ;
    }
}


void english__to_string(int num, char* buffer, int base)
{
    int i = 0;
    bool is_negative = false;

    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    // Based on the behavior of itoa()
    if (num < 0 && base == 10) {
        is_negative = true;
        num = -num;
    }

    while (num != 0) {
        int rem = num % base;
        buffer[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    str_reverse(buffer, i);

    return;
}


Platform::Platform()
{
}


Platform::~Platform()
{
}


Platform::SystemClock::SystemClock()
{
}


Platform::NetworkPeer::NetworkPeer()
{
}


Platform::DeltaClock::DeltaClock()
{
}


Platform::Screen::Screen()
{
}


Platform::Speaker::Speaker()
{
}


Platform::Logger::Logger()
{
}


Platform::DeltaClock::~DeltaClock()
{
}


Platform::NetworkPeer::~NetworkPeer()
{
}
