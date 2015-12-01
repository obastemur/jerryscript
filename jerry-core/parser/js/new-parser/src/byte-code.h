/* Copyright 2015 University of Szeged.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BYTE_CODE_H
#define BYTE_CODE_H

/**
 * Compact byte code (CBC) is a byte code representation
 * of EcmaScript which is designed for low memory
 * environments. Most opcodes are only one or sometimes
 * two byte long so the CBC provides a small binary size.
 *
 * The execution engine of CBC is a stack machine, where
 * the maximum stack size is known in advance for each
 * function.
 */

/**
 * Byte code flags. Only the lower 5 bit can be used
 * since the stack change is encoded in the upper
 * three bits for each instruction between -3 and 4
 * (except for call / construct opcodes).
 */
#define CBC_STACK_ADJUST_BASE          3
#define CBC_STACK_ADJUST_SHIFT         5
#define CBC_STACK_ADJUST_VALUE(value)  \
  (((value) >> CBC_STACK_ADJUST_SHIFT) - CBC_STACK_ADJUST_BASE)

#define CBC_NO_FLAG                    0x00u
#define CBC_HAS_BYTE_ARG               0x01u
#define CBC_HAS_LITERAL_ARG            0x02u
#define CBC_HAS_LITERAL_ARG2           0x04u
#define CBC_HAS_BRANCH_ARG             0x08u

/* These flags are shared */
#define CBC_FORWARD_BRANCH_ARG         0x10u
#define CBC_POP_STACK_BYTE_ARG         0x10u

#define CBC_ARG_TYPES (CBC_HAS_BYTE_ARG | CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2 | CBC_HAS_BRANCH_ARG)

#define CBC_HAS_POP_STACK_BYTE_ARG (CBC_HAS_BYTE_ARG | CBC_POP_STACK_BYTE_ARG)

/* Debug macro. */
#define CBC_ARGS_EQ(op, types) \
  ((cbc_flags[op] & CBC_ARG_TYPES) == (types))

/* Debug macro. */
#define CBC_SAME_ARGS(op1, op2) \
  ((cbc_flags[op1] & CBC_ARG_TYPES) == (cbc_flags[op2] & CBC_ARG_TYPES))

#define CBC_UNARY_OPERATION(name) \
  CBC_OPCODE (name, CBC_NO_FLAG, 0) \
  CBC_OPCODE (name ## _LITERAL, CBC_HAS_LITERAL_ARG, 1)

#define CBC_BINARY_OPERATION(name) \
  CBC_OPCODE (name, CBC_NO_FLAG, -1) \
  CBC_OPCODE (name ## _RIGHT_LITERAL, CBC_HAS_LITERAL_ARG, 0) \
  CBC_OPCODE (name ## _TWO_LITERALS, CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2, 1)

#define CBC_UNARY_LVALUE_OPERATION(name) \
  CBC_OPCODE (name, CBC_NO_FLAG, -2) \
  CBC_OPCODE (name ## _PUSH_RESULT, CBC_NO_FLAG, -1) \
  CBC_OPCODE (name ## _IDENT, CBC_HAS_LITERAL_ARG, 0) \
  CBC_OPCODE (name ## _IDENT_PUSH_RESULT, CBC_HAS_LITERAL_ARG, 1) \
  CBC_OPCODE (name ## _PROP_STRING, CBC_HAS_LITERAL_ARG, -1) \
  CBC_OPCODE (name ## _PROP_STRING_PUSH_RESULT, CBC_HAS_LITERAL_ARG, 0)

#define CBC_BINARY_LVALUE_OPERATION(name) \
  CBC_OPCODE (name, CBC_NO_FLAG, -3) \
  CBC_OPCODE (name ## _IDENT, CBC_HAS_LITERAL_ARG, -1) \
  CBC_OPCODE (name ## _IDENT_LITERAL, CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2, 0) \
  CBC_OPCODE (name ## _PROP_STRING, CBC_HAS_LITERAL_ARG, -2) \

#define CBC_EXT_BINARY_LVALUE_OPERATION(name) \
  CBC_OPCODE (name ## _PUSH_RESULT, CBC_NO_FLAG, -2) \
  CBC_OPCODE (name ## _IDENT_PUSH_RESULT, CBC_HAS_LITERAL_ARG, 0) \
  CBC_OPCODE (name ## _IDENT_LITERAL_PUSH_RESULT, CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2, 1) \
  CBC_OPCODE (name ## _PROP_STRING_PUSH_RESULT, CBC_HAS_LITERAL_ARG, -1)

#define CBC_EXT_BINARY_LVALUE_BLOCK_OPERATION(name) \
  CBC_OPCODE (name ## _BLOCK, CBC_NO_FLAG, -3) \
  CBC_OPCODE (name ## _IDENT_BLOCK, CBC_HAS_LITERAL_ARG, -1) \
  CBC_OPCODE (name ## _IDENT_LITERAL_BLOCK, CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2, 0) \
  CBC_OPCODE (name ## _PROP_STRING_BLOCK, CBC_HAS_LITERAL_ARG, -2)

#define CBC_UNARY_LVALUE_WITH_IDENT 2
#define CBC_UNARY_LVALUE_WITH_PROP_STRING 4

#define CBC_BINARY_LVALUE_WITH_IDENT 1
#define CBC_BINARY_LVALUE_WITH_PROP_STRING 3

#define CBC_BINARY_WITH_LITERAL 1
#define CBC_BINARY_WITH_TWO_LITERALS 2
#define CBC_BINARY_LVALUE_WITH_IDENT_LITERAL 1

#define CBC_IS_BINARY_IDENT_LVALUE_OPERATION(opcode) \
  ((((opcode) - CBC_ASSIGN) & 0x3) == 1)

/**
 * Several opcodes (mostly call and assignment opcodes) have
 * two forms: one which does not push a return value onto
 * the stack, and another which does. The reasion is that
 * the return value of these opcodes are often not used
 * and the first form provides smaller byte code.
 *
 * The following rules must be kept by the code generator:
 *  - only the opcode without return value can be emitted
 *    by the code generator
 *  - the first form can be converted to the second form
 *    by adding 1 to the opcode
 *  - after the conversion the opcode must be immediately
 *    flushed, so no further changes are possible
 *
 * Hence CBC_NO_RESULT_OPERATION (context_p->last_cbc_opcode)
 * cannot be true for an opcode which has a result
 */

#define CBC_NO_RESULT_OPERATION(opcode) \
  (((opcode) >= CBC_DELETE && (opcode) < CBC_END) \
   || (opcode) == PARSER_TO_EXT_OPCODE (CBC_EXT_CALL_EVAL))

#define CBC_NO_RESULT_BINARY_OPERATION(opcode) \
  ((opcode) >= CBC_ASSIGN && (opcode) < CBC_END)

/**
 * Branch instructions are organized in group of 8 opcodes.
 *  - 1st opcode: unused, can be used for other purpose
 *  - 2nd opcode: forward branch with 1 byte offset
 *  - 3rd opcode: forward branch with 2 byte offset
 *  - 4th opcode: forward branch with 3 byte offset
 *  - 5th opcode: unused, can be used for other purpose
 *  - 6th opcode: backward branch with 1 byte offset
 *  - 7th opcode: backward branch with 2 byte offset
 *  - 8th opcode: backward branch with 3 byte offset
 *
 * Reasons:
 *  The branch_opcode & 0x3 tells the length in bytes of the offset
 *  If branch offset & 0x4 == 0, it is a forward branch. Otherwise
 *  it is backward.
 *
 * The offset bytes are encoded in higher to lower order.
 */

#define CBC_FORWARD_BRANCH(name, stack) \
  CBC_OPCODE (name, CBC_HAS_BRANCH_ARG | CBC_FORWARD_BRANCH_ARG, stack) \
  CBC_OPCODE (name ## _2, CBC_HAS_BRANCH_ARG | CBC_FORWARD_BRANCH_ARG, stack) \
  CBC_OPCODE (name ## _3, CBC_HAS_BRANCH_ARG | CBC_FORWARD_BRANCH_ARG, stack)

#define CBC_BACKWARD_BRANCH(name, stack) \
  CBC_OPCODE (name, CBC_HAS_BRANCH_ARG, stack) \
  CBC_OPCODE (name ## _2, CBC_HAS_BRANCH_ARG, stack) \
  CBC_OPCODE (name ## _3, CBC_HAS_BRANCH_ARG, stack)

#define CBC_BRANCH_OFFSET_LENGTH(opcode) \
  ((opcode) & 0x3)

#define CBC_BRANCH_IS_BACKWARD(opcode) \
  (!((flags) & CBC_FORWARD_BRANCH_ARG))

#define CBC_BRANCH_IS_FORWARD(flags) \
  ((flags) & CBC_FORWARD_BRANCH_ARG)

/* All EXT branches are statement block end
 * marks, so they are always forward branches. */

/**
 * Opcode definitions.
 */
#define CBC_OPCODE_LIST \
  /* Branch opcodes first. Some other opcodes are mixed. */ \
  CBC_OPCODE (CBC_EXT_OPCODE, CBC_NO_FLAG, 0) \
  CBC_FORWARD_BRANCH (CBC_JUMP_FORWARD, 0) \
  CBC_OPCODE (CBC_POP, CBC_NO_FLAG, -1) \
  CBC_BACKWARD_BRANCH (CBC_JUMP_BACKWARD, 0) \
  CBC_OPCODE (CBC_POP_BLOCK, CBC_NO_FLAG, -1) \
  CBC_FORWARD_BRANCH (CBC_BRANCH_IF_TRUE_FORWARD, -1) \
  CBC_OPCODE (CBC_RETURN, CBC_NO_FLAG, -1) \
  CBC_BACKWARD_BRANCH (CBC_BRANCH_IF_TRUE_BACKWARD, -1) \
  CBC_OPCODE (CBC_RETURN_WITH_UNDEFINED, CBC_NO_FLAG, 0) \
  CBC_FORWARD_BRANCH (CBC_BRANCH_IF_FALSE_FORWARD, -1) \
  CBC_OPCODE (CBC_CREATE_OBJECT, CBC_NO_FLAG, 1) \
  CBC_BACKWARD_BRANCH (CBC_BRANCH_IF_FALSE_BACKWARD, -1) \
  CBC_OPCODE (CBC_SET_PROPERTY, CBC_HAS_LITERAL_ARG, -1) \
  CBC_FORWARD_BRANCH (CBC_JUMP_FORWARD_EXIT_CONTEXT, 0) \
  CBC_OPCODE (CBC_CREATE_ARRAY, CBC_NO_FLAG, 1) \
  CBC_FORWARD_BRANCH (CBC_BRANCH_IF_LOGICAL_TRUE, -1) \
  CBC_OPCODE (CBC_ARRAY_APPEND, CBC_HAS_POP_STACK_BYTE_ARG, 0) \
  CBC_FORWARD_BRANCH (CBC_BRANCH_IF_LOGICAL_FALSE, -1) \
  CBC_OPCODE (CBC_PUSH_ELISION, CBC_NO_FLAG, 1) \
  CBC_FORWARD_BRANCH (CBC_BRANCH_IF_STRICT_EQUAL, -1) \
  \
  /* Basic opcodes. */ \
  CBC_OPCODE (CBC_PUSH_IDENT, CBC_HAS_LITERAL_ARG, 1) \
  CBC_OPCODE (CBC_PUSH_LITERAL, CBC_HAS_LITERAL_ARG, 1) \
  CBC_OPCODE (CBC_PUSH_TWO_LITERALS, CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2, 2) \
  CBC_OPCODE (CBC_PUSH_UNDEFINED, CBC_NO_FLAG, 1) \
  CBC_OPCODE (CBC_PUSH_TRUE, CBC_NO_FLAG, 1) \
  CBC_OPCODE (CBC_PUSH_FALSE, CBC_NO_FLAG, 1) \
  CBC_OPCODE (CBC_PUSH_NULL, CBC_NO_FLAG, 1) \
  CBC_OPCODE (CBC_PUSH_THIS, CBC_NO_FLAG, 1) \
  CBC_OPCODE (CBC_PROP_GET, CBC_NO_FLAG, -1) \
  CBC_OPCODE (CBC_PROP_STRING_GET, CBC_HAS_LITERAL_ARG, 0) \
  CBC_OPCODE (CBC_NEW, CBC_HAS_POP_STACK_BYTE_ARG, 0) \
  CBC_OPCODE (CBC_NEW_IDENT, CBC_HAS_LITERAL_ARG | CBC_HAS_POP_STACK_BYTE_ARG, 1) \
  CBC_OPCODE (CBC_DEFINE_VARS, CBC_HAS_LITERAL_ARG, 0) \
  CBC_OPCODE (CBC_INITIALIZE_VAR, CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2, 0) \
  CBC_OPCODE (CBC_CONTEXT_END, CBC_NO_FLAG, 0) \
  CBC_OPCODE (CBC_THROW, CBC_NO_FLAG, -1) \
  \
  /* Unary opcodes. */ \
  CBC_UNARY_OPERATION (CBC_PLUS) \
  CBC_UNARY_OPERATION (CBC_NEGATE) \
  CBC_UNARY_OPERATION (CBC_LOGICAL_NOT) \
  CBC_UNARY_OPERATION (CBC_BIT_NOT) \
  CBC_UNARY_OPERATION (CBC_VOID) \
  CBC_UNARY_OPERATION (CBC_TYPEOF) \
  \
  /* Binary opcodes. */ \
  CBC_BINARY_OPERATION (CBC_BIT_OR) \
  CBC_BINARY_OPERATION (CBC_BIT_XOR) \
  CBC_BINARY_OPERATION (CBC_BIT_AND) \
  CBC_BINARY_OPERATION (CBC_EQUAL) \
  CBC_BINARY_OPERATION (CBC_NOT_EQUAL) \
  CBC_BINARY_OPERATION (CBC_STRICT_EQUAL) \
  CBC_BINARY_OPERATION (CBC_STRICT_NOT_EQUAL) \
  CBC_BINARY_OPERATION (CBC_LESS) \
  CBC_BINARY_OPERATION (CBC_GREATER) \
  CBC_BINARY_OPERATION (CBC_LESS_EQUAL) \
  CBC_BINARY_OPERATION (CBC_GREATER_EQUAL) \
  CBC_BINARY_OPERATION (CBC_IN) \
  CBC_BINARY_OPERATION (CBC_INSTANCEOF) \
  CBC_BINARY_OPERATION (CBC_LEFT_SHIFT) \
  CBC_BINARY_OPERATION (CBC_RIGHT_SHIFT) \
  CBC_BINARY_OPERATION (CBC_UNS_RIGHT_SHIFT) \
  CBC_BINARY_OPERATION (CBC_ADD) \
  CBC_BINARY_OPERATION (CBC_SUBTRACT) \
  CBC_BINARY_OPERATION (CBC_MULTIPLY) \
  CBC_BINARY_OPERATION (CBC_DIVIDE) \
  CBC_BINARY_OPERATION (CBC_MODULO) \
  \
  /* Unary lvalue opcodes. */ \
  CBC_UNARY_LVALUE_OPERATION (CBC_DELETE) \
  CBC_UNARY_LVALUE_OPERATION (CBC_PRE_INCR) \
  CBC_UNARY_LVALUE_OPERATION (CBC_PRE_DECR) \
  CBC_UNARY_LVALUE_OPERATION (CBC_POST_INCR) \
  CBC_UNARY_LVALUE_OPERATION (CBC_POST_DECR) \
  \
  /* Call opcodes. */ \
  CBC_OPCODE (CBC_CALL, CBC_HAS_POP_STACK_BYTE_ARG, -1) \
  CBC_OPCODE (CBC_CALL_PUSH_RESULT, CBC_HAS_POP_STACK_BYTE_ARG, 0) \
  CBC_OPCODE (CBC_CALL_IDENT, CBC_HAS_LITERAL_ARG | CBC_HAS_POP_STACK_BYTE_ARG, 0) \
  CBC_OPCODE (CBC_CALL_IDENT_PUSH_RESULT, CBC_HAS_LITERAL_ARG | CBC_HAS_POP_STACK_BYTE_ARG, 1) \
  CBC_OPCODE (CBC_CALL_PROP, CBC_HAS_POP_STACK_BYTE_ARG, -2) \
  CBC_OPCODE (CBC_CALL_PROP_PUSH_RESULT, CBC_HAS_POP_STACK_BYTE_ARG, -1) \
  CBC_OPCODE (CBC_CALL_PROP_STRING, CBC_HAS_LITERAL_ARG | CBC_HAS_POP_STACK_BYTE_ARG, -1) \
  CBC_OPCODE (CBC_CALL_PROP_STRING_PUSH_RESULT, CBC_HAS_LITERAL_ARG | CBC_HAS_POP_STACK_BYTE_ARG, 0) \
  \
  /* Binary lvalue opcodes. */ \
  CBC_BINARY_LVALUE_OPERATION (CBC_ASSIGN) \
  CBC_BINARY_LVALUE_OPERATION (CBC_ASSIGN_ADD) \
  CBC_BINARY_LVALUE_OPERATION (CBC_ASSIGN_SUBTRACT) \
  CBC_BINARY_LVALUE_OPERATION (CBC_ASSIGN_MULTIPLY) \
  CBC_BINARY_LVALUE_OPERATION (CBC_ASSIGN_DIVIDE) \
  CBC_BINARY_LVALUE_OPERATION (CBC_ASSIGN_MODULO) \
  CBC_BINARY_LVALUE_OPERATION (CBC_ASSIGN_LEFT_SHIFT) \
  CBC_BINARY_LVALUE_OPERATION (CBC_ASSIGN_RIGHT_SHIFT) \
  CBC_BINARY_LVALUE_OPERATION (CBC_ASSIGN_UNS_RIGHT_SHIFT) \
  CBC_BINARY_LVALUE_OPERATION (CBC_ASSIGN_BIT_AND) \
  CBC_BINARY_LVALUE_OPERATION (CBC_ASSIGN_BIT_OR) \
  CBC_BINARY_LVALUE_OPERATION (CBC_ASSIGN_BIT_XOR) \
  \
  /* Last opcode (not a real opcode). */ \
  CBC_OPCODE (CBC_END, CBC_NO_FLAG, 0)

#define CBC_EXT_OPCODE_LIST \
  /* Branch opcodes first. Some other opcodes are mixed. */ \
  CBC_OPCODE (CBC_EXT_NOP, CBC_NO_FLAG, 0) \
  CBC_FORWARD_BRANCH (CBC_EXT_WITH_CREATE_CONTEXT, -1 + PARSER_WITH_CONTEXT_STACK_ALLOCATION) \
  CBC_OPCODE (CBC_EXT_FOR_IN_GET_NEXT, CBC_NO_FLAG, 1) \
  CBC_FORWARD_BRANCH (CBC_EXT_FOR_IN_CREATE_CONTEXT, -1 + PARSER_FOR_IN_CONTEXT_STACK_ALLOCATION) \
  CBC_OPCODE (CBC_EXT_SET_GETTER, CBC_HAS_LITERAL_ARG, -1) \
  CBC_BACKWARD_BRANCH (CBC_EXT_BRANCH_IF_FOR_IN_HAS_NEXT, 0) \
  CBC_OPCODE (CBC_EXT_SET_SETTER, CBC_HAS_LITERAL_ARG, -1) \
  CBC_FORWARD_BRANCH (CBC_EXT_TRY_CREATE_CONTEXT, PARSER_TRY_CONTEXT_STACK_ALLOCATION) \
  CBC_OPCODE (CBC_EXT_PUSH_UNDEFINED_BASE, CBC_NO_FLAG, 1) \
  CBC_FORWARD_BRANCH (CBC_EXT_CATCH, 1) \
  CBC_OPCODE (CBC_EXT_DEBUGGER, CBC_NO_FLAG, 0) \
  CBC_FORWARD_BRANCH (CBC_EXT_FINALLY, 0) \
  \
  /* Call opcodes. */ \
  CBC_OPCODE (CBC_EXT_CALL_EVAL, CBC_HAS_POP_STACK_BYTE_ARG, 0) \
  CBC_OPCODE (CBC_EXT_CALL_EVAL_PUSH_RESULT, CBC_HAS_POP_STACK_BYTE_ARG, 1) \
  \
  /* Binary lvalue opcodes. */ \
  CBC_EXT_BINARY_LVALUE_OPERATION (CBC_EXT_ASSIGN) \
  CBC_EXT_BINARY_LVALUE_OPERATION (CBC_EXT_ASSIGN_ADD) \
  CBC_EXT_BINARY_LVALUE_OPERATION (CBC_EXT_ASSIGN_SUBTRACT) \
  CBC_EXT_BINARY_LVALUE_OPERATION (CBC_EXT_ASSIGN_MULTIPLY) \
  CBC_EXT_BINARY_LVALUE_OPERATION (CBC_EXT_ASSIGN_DIVIDE) \
  CBC_EXT_BINARY_LVALUE_OPERATION (CBC_EXT_ASSIGN_MODULO) \
  CBC_EXT_BINARY_LVALUE_OPERATION (CBC_EXT_ASSIGN_LEFT_SHIFT) \
  CBC_EXT_BINARY_LVALUE_OPERATION (CBC_EXT_ASSIGN_RIGHT_SHIFT) \
  CBC_EXT_BINARY_LVALUE_OPERATION (CBC_EXT_ASSIGN_UNS_RIGHT_SHIFT) \
  CBC_EXT_BINARY_LVALUE_OPERATION (CBC_EXT_ASSIGN_BIT_AND) \
  CBC_EXT_BINARY_LVALUE_OPERATION (CBC_EXT_ASSIGN_BIT_OR) \
  CBC_EXT_BINARY_LVALUE_OPERATION (CBC_EXT_ASSIGN_BIT_XOR) \
  \
  /* Binary lvalue opcodes with saving the value. */ \
  CBC_EXT_BINARY_LVALUE_BLOCK_OPERATION (CBC_EXT_ASSIGN) \
  CBC_EXT_BINARY_LVALUE_BLOCK_OPERATION (CBC_EXT_ASSIGN_ADD) \
  CBC_EXT_BINARY_LVALUE_BLOCK_OPERATION (CBC_EXT_ASSIGN_SUBTRACT) \
  CBC_EXT_BINARY_LVALUE_BLOCK_OPERATION (CBC_EXT_ASSIGN_MULTIPLY) \
  CBC_EXT_BINARY_LVALUE_BLOCK_OPERATION (CBC_EXT_ASSIGN_DIVIDE) \
  CBC_EXT_BINARY_LVALUE_BLOCK_OPERATION (CBC_EXT_ASSIGN_MODULO) \
  CBC_EXT_BINARY_LVALUE_BLOCK_OPERATION (CBC_EXT_ASSIGN_LEFT_SHIFT) \
  CBC_EXT_BINARY_LVALUE_BLOCK_OPERATION (CBC_EXT_ASSIGN_RIGHT_SHIFT) \
  CBC_EXT_BINARY_LVALUE_BLOCK_OPERATION (CBC_EXT_ASSIGN_UNS_RIGHT_SHIFT) \
  CBC_EXT_BINARY_LVALUE_BLOCK_OPERATION (CBC_EXT_ASSIGN_BIT_AND) \
  CBC_EXT_BINARY_LVALUE_BLOCK_OPERATION (CBC_EXT_ASSIGN_BIT_OR) \
  CBC_EXT_BINARY_LVALUE_BLOCK_OPERATION (CBC_EXT_ASSIGN_BIT_XOR) \
  \
  /* Last opcode (not a real opcode). */ \
  CBC_OPCODE (CBC_EXT_END, CBC_NO_FLAG, 0)

/**
 * Literal encodings.
 */
typedef enum
{
  cbc_literal_encoding_small,    /**< one byte for literals <= 254, two byte for literals <= 510 */
  cbc_literal_encoding_full      /**< one byte for literals <= 127, two byte for literals <= 32767 */
} cbc_literal_encoding_t;

#define CBC_MAXIMUM_BYTE_VALUE 255
#define CBC_MAXIMUM_SMALL_VALUE 510
#define CBC_MAXIMUM_FULL_VALUE 32767

#define CBC_HIGHEST_BIT_MASK 0x80
#define CBC_LOWER_SEVEN_BIT_MASK 0x7f

/**
 * Literal indicies belong to one of the following groups:
 *
 * 0 <= index < argument_end                    : arguments
 * argument_end <= index < register_end         : registers
 * register_end <= index < ident_end            : identifiers
 * ident_end <= index < const_literal_end       : constant literals
 * const_literal_end <= index < literal_end     : template literals
 */

/**
 * Compiled byte code data.
 */
typedef struct
{
  uint16_t stack_limit;             /**< maximum number of values stored on the stack */
  uint16_t argument_end;            /**< number of arguments expected by the function */
  uint16_t register_end;            /**< end position of the register group */
  uint16_t ident_end;               /**< end position of the identifier group */
  uint16_t const_literal_end;       /**< end position of the const literal group */
  uint16_t literal_end;             /**< end position of the literal group */
  uint8_t literal_encoding;         /**< literal encoding mode */
} cbc_compiled_code_t;

#define CBC_OPCODE(arg1, arg2, arg3) arg1,

/**
 * Opcode list.
 */
typedef enum
{
  CBC_OPCODE_LIST
} cbc_opcode_t;

/**
 * Extended opcode list.
 */
typedef enum
{
  CBC_EXT_OPCODE_LIST
} cbc_ext_opcode_t;

#undef CBC_OPCODE

/**
 * Opcode flags.
 */
extern const uint8_t cbc_flags[];
extern const uint8_t cbc_ext_flags[];

#ifdef PARSER_DEBUG

/**
 * Opcode names for debugging.
 */
extern const char* cbc_names[];
extern const char* cbc_ext_names[];

#endif

#endif /* BYTE_CODE_H */
