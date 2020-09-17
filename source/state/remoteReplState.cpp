#include "script/lisp.hpp"
#include "state_impl.hpp"


void RemoteReplState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    pfrm.screen().fade(1.f);

    pfrm.remote_console().print("BlindJump LISP v01\nEnter 'done to exit.\n");
}


namespace {
class Printer : public lisp::Printer {
public:
    Printer(Platform& pfrm) : pfrm_(pfrm)
    {
    }

    void put_str(const char* str) override
    {
        pfrm_.remote_console().print(str);
    }

    Platform& pfrm_;
};
} // namespace


StatePtr RemoteReplState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    pfrm.remote_console().print("lisp> ");

    if (not pfrm.remote_console().readline(
            [](Platform& pfrm, const char* line) {
                lisp::eval(line);

                Printer p(pfrm);
                format(lisp::get_op(0), p);

                if (lisp::get_op(0)->type_ == lisp::Value::Type::symbol) {
                    if (str_cmp(lisp::get_op(0)->symbol_.name_, "done") == 0) {
                        pfrm.remote_console().print("exiting...\n");
                        lisp::pop_op();
                        return false;
                    }
                }

                lisp::pop_op();

                pfrm.remote_console().print("\n");
                return true;
            })) {
        return state_pool().create<PauseScreenState>(false);
    } else {
        return null_state();
    }
}
