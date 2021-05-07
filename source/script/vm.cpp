#include "bytecode.hpp"
#include "lisp.hpp"
#include "number/endian.hpp"


namespace lisp {


bool is_boolean_true(Value* val);


const char* symbol_from_offset(u16 offset);


Value* make_bytecode_function(Value* buffer);


template <typename Instruction>
Instruction* read(ScratchBuffer& buffer, int& pc)
{
    auto result = (Instruction*)(buffer.data_ + pc);
    pc += sizeof(Instruction);
    return result;
}


void vm_execute(Value* code_buffer, int start_offset)
{
    int pc = start_offset;

    auto& code = *code_buffer->data_buffer_.value();

    using namespace instruction;

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
            push_op(get_var(symbol_from_offset(inst->name_offset_.get())));
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
                push_op(make_error(Error::Code::invalid_argument_type));
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
                push_op(make_error(Error::Code::invalid_argument_type));
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

        default:
        case Fatal::op():
            while (true)
                ;
            break;
        }
    }
}


} // namespace lisp
