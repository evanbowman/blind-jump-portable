#pragma once

#include "number/endian.hpp"
#include "number/numeric.hpp"


class TimeTracker {
public:
    TimeTracker(u32 seconds) : whole_seconds_(seconds), fractional_time_(0)
    {
    }

    int whole_seconds() const
    {
        return whole_seconds_.get();
    }

    void count_up(Microseconds delta)
    {
        fractional_time_.set(fractional_time_.get() + delta);

        if (fractional_time_.get() > seconds(1)) {
            fractional_time_.set(fractional_time_.get() - seconds(1));
            whole_seconds_.set(whole_seconds_.get() + 1);
        }
    }

    void count_down(Microseconds delta)
    {
        fractional_time_.set(fractional_time_.get() + delta);

        if (fractional_time_.get() > seconds(1)) {
            fractional_time_.set(fractional_time_.get() - seconds(1));
            if (whole_seconds_.get() > 0) {
                whole_seconds_.set(whole_seconds_.get() - 1);
            }
        }
    }

    void reset(u32 seconds)
    {
        whole_seconds_.set(seconds);
        fractional_time_.set(0);
    }

private:
    HostInteger<u32> whole_seconds_;
    HostInteger<Microseconds> fractional_time_;
};
