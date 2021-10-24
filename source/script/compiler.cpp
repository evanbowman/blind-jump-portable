#include "bytecode.hpp"
#include "lisp.hpp"
#include "number/endian.hpp"


namespace lisp {


Value* make_bytecode_function(Value* buffer);


u16 symbol_offset(const char* symbol);


int compile_impl(ScratchBuffer& buffer,
                 int write_pos,
                 Value* code,
                 int jump_offset,
                 bool tail_expr);


template <typename Instruction>
static Instruction* append(ScratchBuffer& buffer, int& write_pos)
{
    if (write_pos + sizeof(Instruction) >= SCRATCH_BUFFER_SIZE) {
        // FIXME: propagate error! We should return nullptr, but the caller does
        // not check the return value yet. Now, a lambda that takes up 2kb of
        // bytecode seems highly unlikely in the first place, but you never know
        // I guess...
        while (true)
            ;
    }

    auto result = (Instruction*)(buffer.data_ + write_pos);
    result->header_.op_ = Instruction::op();
    write_pos += sizeof(Instruction);
    return result;
}


int compile_lambda(ScratchBuffer& buffer,
                   int write_pos,
                   Value* code,
                   int jump_offset)
{
    auto lat = code;
    while (lat not_eq get_nil()) {
        if (lat->type_ not_eq Value::Type::cons) {
            // error...
            break;
        }

        if (write_pos not_eq 0) {
            append<instruction::Pop>(buffer, write_pos);
        }

        bool tail_expr = lat->cons_.cdr() == get_nil();

        write_pos = compile_impl(buffer,
                                 write_pos,
                                 lat->cons_.car(),
                                 jump_offset,
                                 tail_expr);

        lat = lat->cons_.cdr();
    }

    append<instruction::Ret>(buffer, write_pos);

    return write_pos;
}


int compile_quoted(ScratchBuffer& buffer, int write_pos, Value* code, bool tail_expr)
{
    if (code->type_ == Value::Type::integer) {
        write_pos = compile_impl(buffer, write_pos, code, 0, tail_expr);
    } else if (code->type_ == Value::Type::symbol) {
        auto inst = append<instruction::PushSymbol>(buffer, write_pos);
        inst->name_offset_.set(symbol_offset(code->symbol_.name_));
    } else if (code->type_ == Value::Type::cons) {

        int list_len = 0;

        while (code not_eq get_nil()) {
            if (code->type_ not_eq Value::Type::cons) {
                // ...
                break;
            }
            write_pos = compile_quoted(buffer, write_pos, code->cons_.car(), tail_expr);

            code = code->cons_.cdr();

            list_len++;

            if (list_len == 255) {
                // FIXME: raise error!
                while (true)
                    ;
            }
        }

        auto inst = append<instruction::PushList>(buffer, write_pos);
        inst->element_count_ = list_len;
    }

    return write_pos;
}


int compile_impl(ScratchBuffer& buffer,
                 int write_pos,
                 Value* code,
                 int jump_offset,
                 bool tail_expr)
{
    if (code->type_ == Value::Type::nil) {

        append<instruction::PushNil>(buffer, write_pos);

    } else if (code->type_ == Value::Type::integer) {

        if (code->integer_.value_ == 0) {
            append<instruction::Push0>(buffer, write_pos);
        } else if (code->integer_.value_ == 1) {
            append<instruction::Push1>(buffer, write_pos);
        } else if (code->integer_.value_ == 2) {
            append<instruction::Push2>(buffer, write_pos);
        } else if (code->integer_.value_ < 127 and
                   code->integer_.value_ > -127) {

            append<instruction::PushSmallInteger>(buffer, write_pos)->value_ =
                code->integer_.value_;

        } else {
            append<instruction::PushInteger>(buffer, write_pos)
                ->value_.set(code->integer_.value_);
        }
    } else if (code->type_ == Value::Type::symbol) {

        append<instruction::LoadVar>(buffer, write_pos)
            ->name_offset_.set(symbol_offset(code->symbol_.name_));

    } else if (code->type_ == Value::Type::cons) {

        auto lat = code;

        auto fn = lat->cons_.car();

        if (fn->type_ == Value::Type::symbol and
            str_cmp(fn->symbol_.name_, "let") == 0) {

            while (true) ; // TODO: implement compilation for let
        } else if (fn->type_ == Value::Type::symbol and
            str_cmp(fn->symbol_.name_, "if") == 0) {

            lat = lat->cons_.cdr();
            if (lat->type_ not_eq Value::Type::cons) {
                while (true)
                    ; // TODO: raise error!
            }

            write_pos =
                compile_impl(buffer, write_pos, lat->cons_.car(), jump_offset, false);

            auto jne = append<instruction::JumpIfFalse>(buffer, write_pos);

            auto true_branch = get_nil();
            auto false_branch = get_nil();

            if (lat->cons_.cdr()->type_ == Value::Type::cons) {
                true_branch = lat->cons_.cdr()->cons_.car();

                if (lat->cons_.cdr()->cons_.cdr()->type_ == Value::Type::cons) {
                    false_branch = lat->cons_.cdr()->cons_.cdr()->cons_.car();
                }
            }

            write_pos =
                compile_impl(buffer, write_pos, true_branch, jump_offset, tail_expr);

            auto jmp = append<instruction::Jump>(buffer, write_pos);

            jne->offset_.set(write_pos - jump_offset);

            write_pos =
                compile_impl(buffer, write_pos, false_branch, jump_offset, tail_expr);

            jmp->offset_.set(write_pos - jump_offset);

        } else if (fn->type_ == Value::Type::symbol and
                   str_cmp(fn->symbol_.name_, "lambda") == 0) {

            lat = lat->cons_.cdr();

            if (lat->type_ not_eq Value::Type::cons) {
                while (true)
                    ; // TODO: raise error!
            }

            auto lambda = append<instruction::PushLambda>(buffer, write_pos);

            // TODO: compile multiple nested expressions! FIXME... pretty broken.
            write_pos = compile_impl(buffer, write_pos, lat->cons_.car(), jump_offset + write_pos, false);

            append<instruction::Ret>(buffer, write_pos);

            lambda->lambda_end_.set(write_pos - jump_offset);

        } else if (fn->type_ == Value::Type::symbol and
                   str_cmp(fn->symbol_.name_, "'") == 0) {

            write_pos = compile_quoted(buffer, write_pos, lat->cons_.cdr(), tail_expr);
        } else {
            u8 argc = 0;

            lat = lat->cons_.cdr();

            // Compile each function arument
            while (lat not_eq get_nil()) {
                if (lat->type_ not_eq Value::Type::cons) {
                    // ...
                    break;
                }

                write_pos = compile_impl(
                    buffer, write_pos, lat->cons_.car(), jump_offset, false);

                lat = lat->cons_.cdr();

                argc++;

                if (argc == 255) {
                    // FIXME: raise error!
                    while (true)
                        ;
                }
            }

            if (fn->type_ == Value::Type::symbol and
                str_cmp(fn->symbol_.name_, "cons") == 0 and argc == 2) {

                append<instruction::MakePair>(buffer, write_pos);

            } else if (fn->type_ == Value::Type::symbol and
                       str_cmp(fn->symbol_.name_, "car") == 0 and argc == 1) {

                append<instruction::First>(buffer, write_pos);

            } else if (fn->type_ == Value::Type::symbol and
                       str_cmp(fn->symbol_.name_, "cdr") == 0 and argc == 1) {

                append<instruction::Rest>(buffer, write_pos);

            } else if (fn->type_ == Value::Type::symbol and
                       str_cmp(fn->symbol_.name_, "arg") == 0 and argc == 1) {

                append<instruction::Arg>(buffer, write_pos);

            } else if (fn->type_ == Value::Type::symbol and
                       str_cmp(fn->symbol_.name_, "this") == 0 and argc == 0) {
                append<instruction::PushThis>(buffer, write_pos);

            } else {

                write_pos = compile_impl(buffer, write_pos, fn, jump_offset, false);

                if (tail_expr) {
                    switch (argc) {
                    case 1:
                        append<instruction::TailCall1>(buffer, write_pos);
                        break;

                    case 2:
                        append<instruction::TailCall2>(buffer, write_pos);
                        break;

                    case 3:
                        append<instruction::TailCall3>(buffer, write_pos);
                        break;

                    default:
                        append<instruction::TailCall>(buffer, write_pos)->argc_
                            = argc;
                        break;
                    }
                } else {
                    switch (argc) {
                    case 1:
                        append<instruction::Funcall1>(buffer, write_pos);
                        break;

                    case 2:
                        append<instruction::Funcall2>(buffer, write_pos);
                        break;

                    case 3:
                        append<instruction::Funcall3>(buffer, write_pos);
                        break;

                    default:
                        append<instruction::Funcall>(buffer, write_pos)->argc_ =
                            argc;
                        break;
                    }
                }
            }
        }

    } else {
        append<instruction::PushNil>(buffer, write_pos);
    }
    return write_pos;
}


void live_values(::Function<24, void(Value&)> callback);


class PeepholeOptimizer {
public:
    template <typename U, typename T>
    void replace(ScratchBuffer& code_buffer, U& dest, T& source, u32& code_size)
    {
        static_assert(sizeof dest > sizeof source);
        memcpy(&dest, &source, sizeof source);


        auto diff = (sizeof dest) - (sizeof source);
        const int start = ((u8*)&dest - (u8*)code_buffer.data_) + sizeof source;

        for (u32 i = start; i < code_size - diff; ++i) {
            code_buffer.data_[i] = code_buffer.data_[i + diff];
        }

        code_size -= diff;

        fixup_jumps(code_buffer, start, -diff);
    }


    u32 run(ScratchBuffer& code_buffer, u32 code_size)
    {
        int index = 0;
        while (true) {
            using namespace instruction;
            auto inst = load_instruction(code_buffer, index);

            switch (inst->op_) {
            case PushLambda::op():
                // FIXME! We do not support optimization yet when there are
                // nested lambda definitions, we need to carefully adjust the
                // jump instruction coordinates in these cases, as jumps are
                // relative to the beginning of the lambda.
                return code_size;

            case Ret::op():
                goto TOP;

            default:
                ++index;
                break;
            }
        }

    TOP:
        index = 0;

        while (true) {
            using namespace instruction;

            auto inst = load_instruction(code_buffer, index);
            switch (inst->op_) {
            case Ret::op():
                return code_size;

            case PushLambda::op():
                return code_size;

            case PushSmallInteger::op(): {
                if (index > 0) {
                    auto prev = load_instruction(code_buffer, index - 1);
                    if (prev->op_ == PushSmallInteger::op()) {
                        if (((PushSmallInteger*)prev)->value_ ==
                            ((PushSmallInteger*)inst)->value_) {

                            Dup d;
                            d.header_.op_ = Dup::op();
                            replace(code_buffer,
                                    *(PushSmallInteger*)inst,
                                    d,
                                    code_size);
                            goto TOP;
                        }
                    } else if (prev->op_ == Dup::op()) {
                        int backtrack = index - 2;
                        while (backtrack > 0) {
                            auto it = load_instruction(code_buffer, backtrack);
                            if (it->op_ == Dup::op()) {
                                --backtrack;
                            } else if (it->op_ == PushSmallInteger::op()) {
                                if (((PushSmallInteger*)it)->value_ ==
                                    ((PushSmallInteger*)inst)->value_) {

                                    Dup d;
                                    d.header_.op_ = Dup::op();
                                    replace(code_buffer,
                                            *(PushSmallInteger*)inst,
                                            d,
                                            code_size);
                                    // code_size -= sizeof(PushSmallInteger) - sizeof(Dup);
                                    goto TOP;
                                }
                            } else {
                                break;
                            }
                        }
                    }
                }
                ++index;
                break;
            }

            case PushInteger::op(): {
                if (index > 0) {
                    auto prev = load_instruction(code_buffer, index - 1);
                    if (prev->op_ == PushInteger::op()) {
                        if (((PushInteger*)prev)->value_.get() ==
                            ((PushInteger*)inst)->value_.get()) {

                            Dup d;
                            d.header_.op_ = Dup::op();
                            replace(
                                code_buffer, *(PushInteger*)inst, d, code_size);
                            goto TOP;
                        }
                    } else if (prev->op_ == Dup::op()) {
                        int backtrack = index - 2;
                        while (backtrack > 0) {
                            auto it = load_instruction(code_buffer, backtrack);
                            if (it->op_ == Dup::op()) {
                                --backtrack;
                            } else if (it->op_ == PushInteger::op()) {
                                if (((PushInteger*)it)->value_.get() ==
                                    ((PushInteger*)inst)->value_.get()) {

                                    Dup d;
                                    d.header_.op_ = Dup::op();
                                    replace(code_buffer,
                                            *(PushInteger*)inst,
                                            d,
                                            code_size);
                                    goto TOP;
                                }
                            } else {
                                break;
                            }
                        }
                    }
                }
                ++index;
                break;
            }

            case Jump::op():
                if (((Jump*)inst)->offset_.get() < 255) {
                    SmallJump j;
                    j.header_.op_ = SmallJump::op();
                    j.offset_ = ((Jump*)inst)->offset_.get();
                    replace(code_buffer, *(Jump*)inst, j, code_size);
                    goto TOP;
                }
                ++index;
                break;

            case JumpIfFalse::op():
                if (((JumpIfFalse*)inst)->offset_.get() < 255) {
                    SmallJumpIfFalse j;
                    j.header_.op_ = SmallJumpIfFalse::op();
                    j.offset_ = ((JumpIfFalse*)inst)->offset_.get();
                    replace(code_buffer, *(JumpIfFalse*)inst, j, code_size);
                    goto TOP;
                }
                ++index;
                break;

            default:
                index++;
                break;
            }
        }
    }


    // We have just inserted/removed an instruction, and now need to scan
    // through the bytecode, and adjust the offsets of local jumps within the
    // lambda definition.
    void
    fixup_jumps(ScratchBuffer& code_buffer, int inflection_point, int size_diff)
    {
        int index = 0;
        int depth = 0;

        struct LambdaInfo {
            int start_ = 0;
        };

        Buffer<LambdaInfo, 15> function_stack;
        function_stack.push_back({0});

        while (true) {
            using namespace instruction;

            auto inst = load_instruction(code_buffer, index);
            switch (inst->op_) {
            case PushLambda::op():
                ++depth;
                if (((PushLambda*)inst)->lambda_end_.get() > inflection_point) {
                    auto end = ((PushLambda*)inst)->lambda_end_.get();
                    ((PushLambda*)inst)->lambda_end_.set(end + size_diff);
                }
                ++index;
                break;

            case Jump::op():
                if (((Jump*)inst)->offset_.get() > inflection_point) {
                    auto offset = ((Jump*)inst)->offset_.get();
                    ((Jump*)inst)->offset_.set(offset + size_diff);
                }
                ++index;
                break;

            case JumpIfFalse::op():
                if (((JumpIfFalse*)inst)->offset_.get() > inflection_point) {
                    auto offset = ((JumpIfFalse*)inst)->offset_.get();
                    ((JumpIfFalse*)inst)->offset_.set(offset + size_diff);
                }
                ++index;
                break;

            case SmallJump::op():
                if (((SmallJump*)inst)->offset_ > inflection_point) {
                    auto offset = ((SmallJump*)inst)->offset_;
                    ((SmallJump*)inst)->offset_ = offset + size_diff;
                }
                ++index;
                break;

            case SmallJumpIfFalse::op():
                if (((SmallJumpIfFalse*)inst)->offset_ > inflection_point) {
                    auto offset = ((SmallJumpIfFalse*)inst)->offset_;
                    ((SmallJumpIfFalse*)inst)->offset_ = offset + size_diff;
                }
                ++index;
                break;

            default:
                ++index;
                break;

            case Ret::op():
                return;
            }
        }
    }
};


void compile(Platform& pfrm, Value* code)
{
    // We will be rendering all of our compiled code into this buffer.
    push_op(make_databuffer(pfrm));

    if (get_op(0)->type_ not_eq Value::Type::data_buffer) {
        return;
    }

    auto fn = make_bytecode_function(get_op(0));
    if (fn->type_ not_eq Value::Type::function) {
        pop_op();
        auto err = fn;
        push_op(err);
        return;
    }

    fn->function_.bytecode_impl_.bc_offset_ = 0;

    auto buffer = get_op(0)->data_buffer_.value();
    pop_op();
    push_op(fn);

    __builtin_memset(buffer->data_, 0, sizeof buffer->data_);

    int write_pos = 0;

    write_pos = compile_lambda(*buffer, write_pos, code, 0);

    write_pos = PeepholeOptimizer().run(
        *dcompr(fn->function_.bytecode_impl_.data_buffer_)
             ->data_buffer_.value(),
        write_pos);

    // std::cout << "compilation finished, bytes used: " << write_pos << std::endl;

    // OK, so now, we've successfully compiled our function into the scratch
    // buffer. But, what about all the extra space in the buffer!? So we're
    // going to scan over all of the interpreter's allocated memory, and
    // collapse our own bytecode into a previously allocated slab of bytecode,
    // if possible.

    const int bytes_used = write_pos;
    bool done = false;

    live_values([fn, &done, bytes_used](Value& val) {
        if (done) {
            return;
        }

        if (fn not_eq &val and val.type_ == Value::Type::function and
            val.mode_bits_ == Function::ModeBits::lisp_bytecode_function) {

            auto buf = dcompr(val.function_.bytecode_impl_.data_buffer_);
            int used = SCRATCH_BUFFER_SIZE - 1;
            for (; used > 0; --used) {
                if ((Opcode)buf->data_buffer_.value()->data_[used] not_eq
                    instruction::Fatal::op()) {
                    ++used;
                    break;
                }
            }

            const int remaining = SCRATCH_BUFFER_SIZE - used;
            if (remaining >= bytes_used) {
                done = true;

                // std::cout << "found another buffer with remaining space: "
                //           << remaining
                //           << ", copying " << bytes_used
                //           << " bytes to dest buffer offset "
                //           << used;

                auto src_buffer =
                    dcompr(fn->function_.bytecode_impl_.data_buffer_);
                for (int i = 0; i < bytes_used; ++i) {
                    buf->data_buffer_.value()->data_[used + i] =
                        src_buffer->data_buffer_.value()->data_[i];
                }

                fn->function_.bytecode_impl_.bc_offset_ = used;
                fn->function_.bytecode_impl_.data_buffer_ = compr(buf);
            }
        }
    });
}


} // namespace lisp
