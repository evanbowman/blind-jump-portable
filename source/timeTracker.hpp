#pragma once

#include "number/numeric.hpp"


class TimeTracker {
public:
    TimeTracker(u32 seconds) : whole_seconds_(seconds), fractional_time_(0)
    {
    }

    int whole_seconds() const
    {
        return whole_seconds_;
    }

    void count_up(Microseconds delta)
    {
        fractional_time_ += delta;

        if (fractional_time_ > seconds(1)) {
            fractional_time_ -= seconds(1);
            whole_seconds_ += 1;
        }
    }

    void count_down(Microseconds delta)
    {
        fractional_time_ += delta;

        if (fractional_time_ > seconds(1)) {
            fractional_time_ -= seconds(1);
            if (whole_seconds_ > 0) {
                whole_seconds_ -= 1;
            }
        }
    }

    void reset(u32 seconds)
    {
        whole_seconds_ = seconds;
        fractional_time_ = 0;
    }

private:
    u32 whole_seconds_;
    Microseconds fractional_time_;
};
