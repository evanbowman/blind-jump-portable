#pragma once


#include "graphics/sprite.hpp"
#include "graphics/view.hpp"
#include "memory/buffer.hpp"
#include "number/numeric.hpp"
#include "save.hpp"
#include "sound.hpp"
#include "tileMap.hpp"
#include <array>
#include <optional>


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
    select,
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
    class Screen;
    class Keyboard;
    class Logger;
    class Speaker;
    class Stopwatch;

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

    inline Speaker& speaker()
    {
        return speaker_;
    }

    inline Stopwatch& stopwatch()
    {
        return stopwatch_;
    }

    void fatal();


    void push_map(const TileMap& map);

    void set_overlay_tile(u16 x, u16 y, u16 val);
    u16 get_overlay_tile(u16 x, u16 y);

    void fill_overlay(u16 tile);


    void load_sprite_texture(const char* name);
    void load_tile_texture(const char* name);
    void load_overlay_texture(const char* name);

    // Sleep halts the game for an amount of time equal to some number
    // of game updates. Given that the game should be running at
    // 60fps, one update equals 1/60 of a second.
    using Frame = u32;
    void sleep(Frame frames);


    bool is_running() const;


    bool write_save(const PersistentData& data);
    std::optional<PersistentData> read_save();


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

        // Blend color into sprite existing screen colors, unless a base color
        // is specified, in which case, computes the resulting color from the
        // base color blended with the color parameter.
        void fade(float amount,
                  ColorConstant color = ColorConstant::rich_black,
                  std::optional<ColorConstant> base = {});

        void pixelate(u8 amount,
                      bool include_overlay = true,
                      bool include_background = true,
                      bool include_sprites = true);

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

        using KeyStates = std::array<bool, int(Key::count)>;

        template <Key... k> bool all_pressed() const
        {
            return (... and states_[int(k)]);
        }

        template <Key... k> bool any_pressed() const
        {
            return (... or states_[int(k)]);
        }

        template <Key k> bool pressed() const
        {
            return states_[int(k)];
        }

        template <Key... k> bool down_transition() const
        {
            return (... or down_transition_helper<k>());
        }

        template <Key k> bool up_transition() const
        {
            return not states_[int(k)] and prev_[int(k)];
        }

        KeyStates dump_state()
        {
            return states_;
        }

        void restore_state(const KeyStates& state)
        {
            // NOTE: we're assigning both the current and previous state to the
            // restored state. Otherwise, we could re-trigger a keypress that
            // already happened
            prev_ = state;
            states_ = state;
        }

    private:
        template <Key k> bool down_transition_helper() const
        {
            return states_[int(k)] and not prev_[int(k)];
        }

        Keyboard()
        {
            for (int i = 0; i < int(Key::count); ++i) {
                states_[i] = false;
                prev_[i] = false;
            }
        }

        friend class Platform;

        KeyStates prev_;
        KeyStates states_;
    };


    ////////////////////////////////////////////////////////////////////////////
    // Logger
    ////////////////////////////////////////////////////////////////////////////


    class Logger {
    public:
        enum class Severity { info, warning, error };

        void log(Severity severity, const char* msg);

    private:
        Logger();

        friend class Platform;
    };


    ////////////////////////////////////////////////////////////////////////////
    // Speaker
    ////////////////////////////////////////////////////////////////////////////

    class Speaker {
    public:
        using Channel = int;

        void play_note(Note n, Octave o, Channel c);

        void load_music(const char* name, bool loop);
        void stop_music();

        // A platform's speaker may only have the resources to handle a limited
        // number of overlapping sounds. For such platforms, currently running
        // sounds with a lower priority will be evicted, to make room for
        // higher-priority sounds.
        void play_sound(const char* name, int priority);
        bool is_sound_playing(const char* name);

    private:
        friend class Platform;

        Speaker();
    };


    ////////////////////////////////////////////////////////////////////////////
    // Stopwatch
    ////////////////////////////////////////////////////////////////////////////

    class Stopwatch {
    public:
        Stopwatch();

        void start();
        int stop();

    private:
        void* impl_;
    };


    ////////////////////////////////////////////////////////////////////////////
    // Task
    ////////////////////////////////////////////////////////////////////////////

    class Task {
    public:
        virtual void run() = 0;

        virtual ~Task()
        {
        }

        bool complete() const
        {
            return complete_;
        }

        bool running() const
        {
            return running_;
        }

    protected:
        void completed()
        {
            complete_ = true;
        }

        friend class Platform;

    private:
        Atomic<bool> running_ = false;
        Atomic<bool> complete_ = false;
    };

    // If only we had a heap, and shared pointers, we could enforce better
    // ownership than raw pointers for Tasks... ah well.
    void push_task(Task* task);


    class Data;

    Data* data()
    {
        return data_;
    }

private:
    Platform();

    friend int main();

    Screen screen_;
    Keyboard keyboard_;
    Speaker speaker_;
    Logger logger_;
    Stopwatch stopwatch_;
    Data* data_ = nullptr;
};


class SynchronizedBase {
protected:
    void init(Platform& pf);
    void lock();
    void unlock();

    ~SynchronizedBase();

private:
    friend class Platform;

    void* impl_;
};


template <typename T> class Synchronized : SynchronizedBase {
public:
    template <typename... Args>
    Synchronized(Platform& pf, Args&&... args)
        : data_(std::forward<Args>(args)...)
    {
        init(pf);
    }

    template <typename F> void acquire(F&& handler)
    {
        lock();
        handler(data_);
        unlock();
    }

private:
    T data_;
};


#ifdef __BLINDJUMP_ENABLE_LOGS
#ifdef __GBA__
// #pragma message "Warning: logging can wear down Flash memory, be careful using this on physical hardware!"
#endif
inline void info(Platform& pf, const char* msg)
{
    pf.logger().log(Platform::Logger::Severity::info, msg);
}
inline void warning(Platform& pf, const char* msg)
{
    pf.logger().log(Platform::Logger::Severity::warning, msg);
}
inline void error(Platform& pf, const char* msg)
{
    pf.logger().log(Platform::Logger::Severity::error, msg);
}
#else
inline void info(Platform&, const char*)
{
}
inline void warning(Platform&, const char*)
{
}
inline void error(Platform&, const char*)
{
}
#endif // __BLINDJUMP_ENABLE_LOGS
