#include "context.hpp"


int main()
{
    Context ct;
    bind_context(&ct);

    Screen& screen = ct.platform_.screen();
    Keyboard& keyboard = ct.platform_.keyboard();


    Sprite test;
    test.initialize();
    test.set_position({50.f, 80.f});


    while (true) {
        keyboard.poll();
        screen.clear();

        if (keyboard.pressed<Keyboard::Key::left>()) {
            auto pos2 = test.get_position();
            test.set_position({pos2.x - 1.f, pos2.y});
         }

        screen.display();
    }
}
