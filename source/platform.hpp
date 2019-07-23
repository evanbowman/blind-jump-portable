#pragma once

#include "numeric.hpp"
#include "save.hpp"
#include "tileMap.hpp"
#include <array>
#include <optional>
#include "sprite.hpp"
#include "view.hpp"


// Anything platform specific should be defined here.


////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


class DeltaClock {
public:
    static DeltaClock& instance()
    {
        static DeltaClock inst;
        return inst;
    }

    ~DeltaClock();

    Microseconds reset();

private:
    DeltaClock();

    void* impl_;
};


enum class Key {
    action_1,
    action_2,
    start,
    left,
    right,
    up,
    down,
    alt_1,
    alt_2,
    count
};


////////////////////////////////////////////////////////////////////////////////
// Platform
////////////////////////////////////////////////////////////////////////////////


class Platform {
public:
    Platform();

    class Screen;
    class Keyboard;
    class Logger;


    inline Screen& screen()
    {
        return screen_;
    }

    inline Keyboard& keyboard()
    {
        return keyboard_;
    }

    inline Logger& logger()
    {
        return logger_;
    }

    void push_map(const TileMap& map);


    // Sleep halts the game for an amount of time equal to some number
    // of game updates. Given that the game should be running at
    // 60fps, one update equals 1/60 of a second.
    void sleep(u32 frames);


    bool is_running() const;


    bool write_save(const SaveData& data);
    std::optional<SaveData> read_save();


    ////////////////////////////////////////////////////////////////////////////
    // Screen
    ////////////////////////////////////////////////////////////////////////////


    class Screen {
    public:
        static constexpr u32 sprite_limit = 128;

        void draw(const Sprite& spr);

        void clear();

        void display();

        Vec2<u32> size() const;

        void set_view(const View& view)
        {
            view_ = view;
        }

        const View& get_view() const
        {
            return view_;
        }

        void fade(float amount, ColorConstant = ColorConstant::rich_black);

    private:
        Screen();

        friend class Platform;

        View view_;
        void* userdata_;
    };


    ////////////////////////////////////////////////////////////////////////////
    // Keyboard
    ////////////////////////////////////////////////////////////////////////////


    class Keyboard {
    public:

        void poll();

        template <Key k> bool pressed() const
        {
            return states_[int(k)];
        }

        template <Key k> bool down_transition() const
        {
            return states_[int(k)] and not prev_[int(k)];
        }

        template <Key k> bool up_transition() const
        {
            return not states_[int(k)] and prev_[int(k)];
        }

    private:
        Keyboard()
        {
            for (int i = 0; i < int(Key::count); ++i) {
                states_[i] = false;
                prev_[i] = false;
            }
        }

        friend class Platform;

        std::array<bool, int(Key::count)> prev_;
        std::array<bool, int(Key::count)> states_;
    };


    ////////////////////////////////////////////////////////////////////////////
    // Logger
    ////////////////////////////////////////////////////////////////////////////


    class Logger {
    public:
        void log(const char* msg);

    private:

        friend class Platform;
    };


private:
    Screen screen_;
    Keyboard keyboard_;
    Logger logger_;
};





#ifdef __BLINDJUMP_ENABLE_LOGS
#ifdef __GBA__
// #pragma message "Warning: logging can wear down Flash memory, be careful using this on physical hardware!"
#endif
inline void log(Platform& pf, const char* msg)
{
    pf.logger().log(msg);
}
#else
inline void log(Platform&, const char*)
{
}
#endif // __BLINDJUMP_ENABLE_LOGS
