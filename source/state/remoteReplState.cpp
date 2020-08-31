#include "script/lisp.hpp"
#include "state_impl.hpp"


void RemoteReplState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    pfrm.screen().fade(1.f);

    pfrm.remote_console().print("BlindJump LISP v01\n");
}


StatePtr RemoteReplState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    pfrm.remote_console().print("lisp> ");

    if (not pfrm.remote_console().readline([](Platform& pfrm, const char* line) {
        lisp::eval(line);

        pfrm.remote_console().print(format(lisp::get_op(0)).c_str());

        lisp::pop_op();

        pfrm.remote_console().print("\n");

    })) {
        return state_pool().create<PauseScreenState>(false);
    } else {
        return null_state();
    }
}
