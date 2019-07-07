#include "game.hpp"
#include "transformGroup.hpp"


int main()
{
    Platform pfrm;

    Screen& screen = pfrm.screen();
    Keyboard& keyboard = pfrm.keyboard();

    Game game(pfrm);

    while (true) {
        keyboard.poll();
        game.update(pfrm, DeltaClock::instance().reset());

        // NOTE: On some platforms, e.g. GBA and other consoles, Screen::clear()
        // performs the vsync, so Game::update() should be called before the
        // clear, and Game::render() should be called after clear(), to prevent
        // tearing.
        screen.clear();

        game.render(pfrm);

        screen.display();
    }
}
