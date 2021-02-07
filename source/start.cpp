#include "blind_jump/game.hpp"
#include "globals.hpp"
#include "transformGroup.hpp"


class UpdateTask : public Platform::Task {
public:
    UpdateTask(Synchronized<Game>* game, Platform* pf);

    void run() override;

private:
    Synchronized<Game>* game_;
    Platform* pf_;
};


UpdateTask::UpdateTask(Synchronized<Game>* game, Platform* pf)
    : game_(game), pf_(pf)
{
}


void UpdateTask::run()
{
    if (pf_->is_running()) {
        game_->acquire([this](Game& game) {
            pf_->keyboard().poll();

            const auto delta = pf_->delta_clock().reset();

            game.update(*pf_, delta);

            pf_->network_peer().update();
        });
    } else {
        Task::completed();
    }
}


void blind_jump_main_loop(Platform& pf)
{
    pf.remote_console().printline("BlindJump LISP Console");

    globals().emplace<BlindJumpGlobalData>();

    Synchronized<Game> game(pf, pf);

    UpdateTask update(&game, &pf);
    pf.push_task(&update);

    while (pf.is_running()) {

        pf.feed_watchdog();

        pf.screen().clear();
        game.acquire([&](Game& gm) { gm.render(pf); });
        pf.screen().display();
    }
}


void start(Platform& pf)
{
    return blind_jump_main_loop(pf);
}
