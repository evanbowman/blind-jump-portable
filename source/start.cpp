#include "game.hpp"
#include "transformGroup.hpp"


class UpdateTask : public Platform::Task {
public:
    UpdateTask(Synchronized<Game>* game, Platform* pf);

    void run() override;

private:
    Synchronized<Game>* game_;
    Platform* pf_;
    Microseconds fps_timer_;
    int fps_frame_count_;
    std::optional<Text> fps_text_;
};


UpdateTask::UpdateTask(Synchronized<Game>* game, Platform* pf)
    : game_(game), pf_(pf), fps_timer_(0), fps_frame_count_(0)
{
}


void UpdateTask::run()
{
    if (pf_->is_running()) {
        game_->acquire([this](Game& game) {
            pf_->keyboard().poll();

            const auto delta = DeltaClock::instance().reset();

            if (game.persistent_data().settings_.show_fps_) {

                fps_timer_ += delta;
                fps_frame_count_ += 1;

                if (fps_timer_ >= seconds(1)) {
                    fps_timer_ -= seconds(1);

                    fps_text_.emplace(*pf_, OverlayCoord{1, 2});
                    fps_text_->assign(fps_frame_count_);
                    fps_text_->append(" fps");
                    fps_frame_count_ = 0;
                }
            } else if (fps_text_) {
                fps_text_.reset();

                fps_frame_count_ = 0;
                fps_timer_ = 0;
            }

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
