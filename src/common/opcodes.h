//
// Created by Mahamadou DOUMBIA [OML DSI] on 12/01/2022.
//

#ifndef CPMSC_OPCODES_H
#define CPMSC_OPCODES_H


typedef  enum  {
#define OPCODE(code, _) OP_##code,

#include "codes.h"

#undef OPCODE
} Opcode;


#endif