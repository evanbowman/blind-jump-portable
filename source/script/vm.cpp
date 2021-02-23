#include "lisp.hpp"
#include "bytecode.hpp"
#include "number/endian.hpp"


namespace lisp {


const char* symbol_from_offset(u16 offset);


void vm_execute(ScratchBuffer& code, int start_offset)
{
    int pc = start_offset;

#define READ_S16 ((HostInteger<s16>*)(code.data_ + pc))->get()
#define READ_S32 ((HostInteger<s32>*)(code.data_ + pc))->get()
#define READ_U8 *(code.data_ + pc)

    while (true) {

        switch ((Opcode)code.data_[pc]) {
        case Opcode::load_var:
            ++pc;
            push_op(get_var(symbol_from_offset(READ_S16)));
            pc += 2;
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

        case Opcode::push_symbol:
            while (true) ; // TODO...
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

        case Opcode::pop:
            pop_op();
            ++pc;
            break;

        case Opcode::ret:
            return;

        default:
        case Opcode::fatal:
            while (true) ;
            break;
        }

    }
}


}
