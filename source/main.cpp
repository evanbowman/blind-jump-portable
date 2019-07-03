#include "game.hpp"
#include "transformGroup.hpp"


int main()
{
    Platform pfrm;

    Screen& screen = pfrm.screen();
    Keyboard& keyboard = pfrm.keyboard();

    DeltaClock clock;

    Game game;

    View view;
    view.set_center({-42, -42});
    screen.set_view(view);

    while (true) {
        keyboard.poll();
        game.update(pfrm, clock.reset());

        // NOTE: On some platforms, e.g. GBA and other consoles, Screen::clear()
        // performs the vsync, so Game::update() should be called before the
        // clear, and Game::render() should be called after clear(), to prevent
        // tearing.
        screen.clear();

        game.render(pfrm);

        screen.display();
    }
}
