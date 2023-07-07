#pragma once

#include "number/endian.hpp"



namespace lisp {


struct Module {
    struct Header {
        host_u16 symbol_count_;
        host_u16 bytecode_length_;
    } header_;

    // char symbol_data_[];
    // char bytecode_[];
};


} // namespace lisp
