#pragma once


#include "number/int.h"


struct Date {
    s32 year_;
    s32 month_;
    s32 day_;
};


struct DateTime {
    Date date_;
    s32 hour_;
    s32 minute_;
    s32 second_;

    u64 as_seconds() const
    {
        auto minute = [](u64 n) { return n * 60; };
        auto hour = [&minute](u64 n) { return n * minute(60); };
        auto day = [&hour](u64 n) { return n * hour(24); };
        auto month = [&day](u64 n) { return n * day(31); }; // FIXME...
        auto year = [&day](u64 n) { return n * day(365); }; // FIXME...

        return second_ + minute(minute_) + hour(hour_) + day(date_.day_) +
               month(date_.month_) + year(date_.year_);
    }
};


inline u64 time_diff(const DateTime& before, const DateTime& after)
{
    return after.as_seconds() - before.as_seconds();
}
