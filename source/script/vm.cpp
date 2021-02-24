#include "bytecode.hpp"
#include "lisp.hpp"
#include "number/endian.hpp"


namespace lisp {


bool is_boolean_true(Value* val);


const char* symbol_from_offset(u16 offset);


Value* make_bytecode_function(Value* buffer);


void vm_execute(Value* code_buffer, int start_offset)
{
    int pc = start_offset;

    auto& code = *code_buffer->data_buffer_.value();

#define READ_U16 ((HostInteger<u16>*)(code.data_ + pc))->get()
#define READ_S16 ((HostInteger<s16>*)(code.data_ + pc))->get()
#define READ_S32 ((HostInteger<s32>*)(code.data_ + pc))->get()
#define READ_U8 *(code.data_ + pc)
#define READ_S8 *(s8*)(code.data_ + pc)

    while (true) {

        switch ((Opcode)code.data_[pc]) {
        case Opcode::jump_if_false:
            ++pc;
            if (not is_boolean_true(get_op(0))) {
                pc = start_offset + READ_U16;
            } else {
                pc += 2;
            }
            pop_op();
            break;

        case Opcode::jump:
            ++pc;
            pc = start_offset + READ_U16;
            break;

        case Opcode::load_var:
            ++pc;
            push_op(get_var(symbol_from_offset(READ_S16)));
            pc += 2;
            break;

        case Opcode::dup:
            ++pc;
            push_op(get_op(0));
            ;
            break;

        case Opcode::push_nil:
            ++pc;
            push_op(get_nil());
            break;

        case Opcode::push_integer:
            ++pc;
            push_op(make_integer(READ_S32));
            pc += 4;
            break;

        case Opcode::push_0:
            push_op(make_integer(0));
            ++pc;
            break;

        case Opcode::push_1:
            push_op(make_integer(1));
            ++pc;
            break;

        case Opcode::push_2:
            push_op(make_integer(2));
            ++pc;
            break;

        case Opcode::push_small_integer:
            ++pc;
            push_op(make_integer(READ_S8));
            ++pc;
            break;

        case Opcode::push_symbol:
            while (true)
                ; // TODO...
            break;

        case Opcode::funcall: {
            ++pc;
            Protected fn(get_op(0));
            auto argc = READ_U8;
            ++pc;
            pop_op();
            funcall(fn, argc);
            break;
        }

        case Opcode::funcall_1: {
            ++pc;
            Protected fn(get_op(0));
            pop_op();
            funcall(fn, 1);
            break;
        }

        case Opcode::funcall_2: {
            ++pc;
            Protected fn(get_op(0));
            pop_op();
            funcall(fn, 2);
            break;
        }

        case Opcode::funcall_3: {
            ++pc;
            Protected fn(get_op(0));
            pop_op();
            funcall(fn, 3);
            break;
        }

        case Opcode::pop:
            pop_op();
            ++pc;
            break;

        case Opcode::ret:
            return;

        case Opcode::push_lambda: {
            ++pc;
            auto fn = make_bytecode_function(code_buffer);
            if (fn->type_ == lisp::Value::Type::function) {
                fn->function_.bytecode_impl_.bc_offset_ = pc + 2;
            }
            push_op(fn);
            pc = start_offset + READ_U16;
            break;
        }

        default:
        case Opcode::fatal:
            while (true)
                ;
            break;
        }
    }
}


} // namespace lisp
