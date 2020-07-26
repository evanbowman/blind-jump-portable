#include "game.hpp"
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

            const auto delta = DeltaClock::instance().reset();

            game.update(*pf_, delta);
        });
    } else {
        Task::completed();
    }
}


void start(Platform& pf)
{
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
