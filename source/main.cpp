#include "game.hpp"
#include "transformGroup.hpp"


int main()
{
    Platform pf;


    Game game(pf);


    while (pf.is_running()) {

        pf.keyboard().poll();
        game.update(pf, DeltaClock::instance().reset());

        // NOTE: On some pfs, e.g. GBA and other consoles, Screen::clear()
        // performs the vsync, so Game::update() should be called before the
        // clear, and Game::render() should be called after clear(), to prevent
        // tearing.
        pf.screen().clear();

        game.render(pf);

        pf.screen().display();
    }
}
