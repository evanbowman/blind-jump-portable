#include "context.hpp"
#include "game.hpp"
#include "deltaClock.hpp"


int main()
{
    Context ct;
    bind_context(&ct);

    Screen& screen = ct.platform_.screen();
    Keyboard& keyboard = ct.platform_.keyboard();

    DeltaClock clock;

    Game game;

    while (true) {
        keyboard.poll();
        screen.clear();

        game.update(ct.platform_, clock.reset());

        screen.display();
    }
}
