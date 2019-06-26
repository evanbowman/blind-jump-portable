#include "context.hpp"


int main()
{
    Context ct;
    bind_context(&ct);

    Screen& screen = ct.platform_.screen();
    Keyboard& keyboard = ct.platform_.keyboard();


    Sprite test;
    test.initialize(20);

    Sprite test2;
    test2.initialize(60);

    while (true) {
        keyboard.poll();
        screen.clear();

        if (keyboard.pressed<Keyboard::Key::left>()) {
            screen.draw(test);
        }

        screen.display();
    }
}
