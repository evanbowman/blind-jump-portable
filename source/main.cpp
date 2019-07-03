#include "game.hpp"
#include "transformGroup.hpp"


int main()
{
    Platform pfrm;

    Screen& screen = pfrm.screen();
    Keyboard& keyboard = pfrm.keyboard();

    DeltaClock clock;

    Game game;

    while (true) {
        keyboard.poll();
        screen.clear();

        game.update(pfrm, clock.reset());

        screen.display();
    }
}
