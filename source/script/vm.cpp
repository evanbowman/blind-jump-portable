#include "bytecode.hpp"
#include "lisp.hpp"
#include "number/endian.hpp"


namespace lisp {


bool is_boolean_true(Value* val);


const char* symbol_from_offset(u16 offset);


Value* make_bytecode_function(Value* buffer);


Value* get_var_stable(const char* intern_str);


template <typename Instruction>
Instruction* read(ScratchBuffer& buffer, int& pc)
{
    auto result = (Instruction*)(buffer.data_ + pc);
    pc += sizeof(Instruction);
    return result;
}


void vm_execute(Value* code_buffer, const int start_offset)
{
    int pc = start_offset;

    auto& code = *code_buffer->data_buffer_.value();

    using namespace instruction;

 TOP:
    while (true) {

        switch ((Opcode)code.data_[pc]) {
        case JumpIfFalse::op(): {
            auto inst = read<JumpIfFalse>(code, pc);
            if (not is_boolean_true(get_op(0))) {
                pc = start_offset + inst->offset_.get();
            }
            pop_op();
            break;
        }

        case Jump::op(): {
            auto inst = read<Jump>(code, pc);
            pc = start_offset + inst->offset_.get();
            break;
        }

        case SmallJumpIfFalse::op(): {
            auto inst = read<SmallJumpIfFalse>(code, pc);
            if (not is_boolean_true(get_op(0))) {
                pc = start_offset + inst->offset_;
            }
            pop_op();
            break;
        }

        case SmallJump::op(): {
            auto inst = read<SmallJump>(code, pc);
            pc = start_offset + inst->offset_;
            break;
        }

        case LoadVar::op(): {
            auto inst = read<LoadVar>(code, pc);
            push_op(
                get_var_stable(symbol_from_offset(inst->name_offset_.get())));
            break;
        }

        case Dup::op(): {
            read<Dup>(code, pc);
            push_op(get_op(0));
            break;
        }

        case PushNil::op():
            read<PushNil>(code, pc);
            push_op(get_nil());
            break;

        case PushInteger::op(): {
            auto inst = read<PushInteger>(code, pc);
            push_op(make_integer(inst->value_.get()));
            break;
        }

        case Push0::op():
            read<Push0>(code, pc);
            push_op(make_integer(0));
            break;

        case Push1::op():
            read<Push1>(code, pc);
            push_op(make_integer(1));
            break;

        case Push2::op():
            read<Push2>(code, pc);
            push_op(make_integer(2));
            break;

        case PushSmallInteger::op(): {
            auto inst = read<PushSmallInteger>(code, pc);
            push_op(make_integer(inst->value_));
            break;
        }

        case PushSymbol::op(): {
            auto inst = read<PushSymbol>(code, pc);
            push_op(make_symbol(symbol_from_offset(inst->name_offset_.get()),
                                Symbol::ModeBits::stable_pointer));
            break;
        }

        case TailCall::op(): {

            Protected fn(get_op(0));

            auto argc = read<TailCall>(code, pc)->argc_;


            if (fn == get_this()) {
                pop_op(); // function on stack

                if (get_argc() not_eq argc) {
                    // TODO: raise error: attempted recursive call with
                    // different number of args than current function.
                    // Actually...
                    // The isn't really anything preventing a variadic function
                    // from being executed recursively with a different number
                    // of args, right? So maybe shouldn't be isn't an error...
                    while (true) ;
                }

                if (argc == 0) {
                    pc = start_offset;
                    goto TOP;
                } else {
                    // TODO: perform TCO for N-arg function
                    while (true) {
                        // ...
                    }
                }

            } else {

                pop_op();
                funcall(fn, argc);
            }

            break;
        }

        case TailCall1::op(): {
            read<TailCall1>(code, pc);
            Protected fn(get_op(0));

            if (fn == get_this()) {
                auto arg = get_op(1);

                if (get_argc() not_eq 1) {
                    // TODO: raise error: attempted recursive call with
                    // different number of args than current function.
                    while (true) ;
                }

                pop_op(); // function on stack
                pop_op(); // argument
                pop_op(); // previous arg

                push_op(arg);

                pc = start_offset;
                goto TOP;

            } else {
                pop_op();
                funcall(fn, 1);
            }
            break;
        }

        case TailCall2::op(): {
            read<TailCall2>(code, pc);
            Protected fn(get_op(0));

            if (fn == get_this()) {
                auto arg0 = get_op(1);
                auto arg1 = get_op(2);

                if (get_argc() not_eq 2) {
                    // TODO: raise error: attempted recursive call with
                    // different number of args than current function.
                    while (true) ;
                }

                pop_op(); // function on stack
                pop_op(); // arg
                pop_op(); // arg
                pop_op(); // prev arg
                pop_op(); // prev arg

                push_op(arg1);
                push_op(arg0);

                pc = start_offset;
                goto TOP;

            } else {
                pop_op();
                funcall(fn, 2);
            }
            break;
        }

        case TailCall3::op(): {
            read<TailCall3>(code, pc);
            Protected fn(get_op(0));

            if (fn == get_this()) {
                auto arg0 = get_op(1);
                auto arg1 = get_op(2);
                auto arg2 = get_op(3);

                if (get_argc() not_eq 3) {
                    while (true) ;
                }

                pop_op(); // function on stack
                pop_op(); // arg
                pop_op(); // arg
                pop_op(); // arg
                pop_op(); // prev arg
                pop_op(); // prev arg
                pop_op(); // prev arg

                push_op(arg2);
                push_op(arg1);
                push_op(arg0);

                pc = start_offset;
                goto TOP;

            } else {
                pop_op();
                funcall(fn, 3);
            }
            break;
        }

        case Funcall::op(): {
            Protected fn(get_op(0));
            auto argc = read<Funcall>(code, pc)->argc_;
            pop_op();
            funcall(fn, argc);
            break;
        }

        case Funcall1::op(): {
            read<Funcall1>(code, pc);
            Protected fn(get_op(0));
            pop_op();
            funcall(fn, 1);
            break;
        }

        case Funcall2::op(): {
            read<Funcall2>(code, pc);
            Protected fn(get_op(0));
            pop_op();
            funcall(fn, 2);
            break;
        }

        case Funcall3::op(): {
            read<Funcall3>(code, pc);
            Protected fn(get_op(0));
            pop_op();
            funcall(fn, 3);
            break;
        }

        case Arg::op(): {
            read<Arg>(code, pc);
            auto arg_num = get_op(0);
            auto arg = get_arg(arg_num->integer_.value_);
            pop_op();
            push_op(arg);
            break;
        }

        case Arg0::op(): {
            read<Arg0>(code, pc);
            push_op(get_arg(0));
            break;
        }

        case Arg1::op(): {
            read<Arg1>(code, pc);
            push_op(get_arg(1));
            break;
        }

        case Arg2::op(): {
            read<Arg2>(code, pc);
            push_op(get_arg(2));
            break;
        }

        case MakePair::op(): {
            read<MakePair>(code, pc);
            auto car = get_op(1);
            auto cdr = get_op(0);
            auto cons = make_cons(car, cdr);
            pop_op();
            pop_op();
            push_op(cons);
            break;
        }

        case First::op(): {
            read<First>(code, pc);
            auto arg = get_op(0);
            pop_op();
            if (arg->type_ == Value::Type::cons) {
                push_op(arg->cons_.car());
            } else {
                push_op(make_error(Error::Code::invalid_argument_type, L_NIL));
            }
            break;
        }

        case Rest::op(): {
            read<Rest>(code, pc);
            auto arg = get_op(0);
            pop_op();
            if (arg->type_ == Value::Type::cons) {
                push_op(arg->cons_.cdr());
            } else {
                push_op(make_error(Error::Code::invalid_argument_type, L_NIL));
            }
            break;
        }

        case Pop::op():
            read<Pop>(code, pc);
            pop_op();
            break;

        case Ret::op():
            return;

        case PushLambda::op(): {
            auto inst = read<PushLambda>(code, pc);
            auto fn = make_bytecode_function(code_buffer);
            if (fn->type_ == lisp::Value::Type::function) {
                fn->function_.bytecode_impl_.bc_offset_ = pc;
            }
            push_op(fn);
            pc = start_offset + inst->lambda_end_.get();
            break;
        }

        case PushList::op(): {
            auto list_size = read<PushList>(code, pc)->element_count_;
            Protected lat(make_list(list_size));
            for (int i = 0; i < list_size; ++i) {
                set_list(lat, i, get_op((list_size - 1) - i));
            }
            for (int i = 0; i < list_size; ++i) {
                pop_op();
            }
            push_op(lat);
            break;
        }

        case PushThis::op(): {
            push_op(get_this());
            read<PushThis>(code, pc);
            break;
        }

        default:
        case Fatal::op():
            while (true)
                ;
            break;
        }
    }
}


} // namespace lisp
