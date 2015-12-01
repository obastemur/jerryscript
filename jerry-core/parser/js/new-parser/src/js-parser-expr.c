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

#include "js-parser-internal.h"

static const uint8_t parser_binary_precedence_table[36] =
{
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  4, 5, 6, 7, 8, 9, 10, 10, 10, 10,
  11, 11, 11, 11, 11, 11, 12, 12, 12,
  13, 13, 14, 14, 14
};

/**
 * Generate byte code for operators with lvalue.
 */
static PARSER_INLINE void
parser_push_result (parser_context_t *context_p) /**< context */
{
  if (CBC_NO_RESULT_BINARY_OPERATION (context_p->last_cbc_opcode))
  {
    context_p->last_cbc_opcode = (uint16_t) PARSER_TO_BINARY_OPERATION_WITH_RESULT (context_p->last_cbc_opcode);
    parser_flush_cbc (context_p);
  }
  else if (CBC_NO_RESULT_OPERATION (context_p->last_cbc_opcode))
  {
    PARSER_ASSERT (context_p->last_cbc_opcode == PARSER_TO_EXT_OPCODE (CBC_EXT_CALL_EVAL)
                   || CBC_SAME_ARGS (context_p->last_cbc_opcode, context_p->last_cbc_opcode + 1));
    context_p->last_cbc_opcode++;
    parser_flush_cbc (context_p);
  }
} /* parser_push_result */

/**
 * Generate byte code for operators with lvalue.
 */
static void
parser_emit_unary_lvalue_opcode (parser_context_t *context_p, /**< context */
                                 cbc_opcode_t opcode) /**< opcode */
{
  if (context_p->last_cbc_opcode == CBC_PUSH_IDENT)
  {
    PARSER_ASSERT (CBC_SAME_ARGS (CBC_PUSH_IDENT, opcode + CBC_UNARY_LVALUE_WITH_IDENT));

    if ((context_p->status_flags & PARSER_IS_STRICT)
        && context_p->last_cbc.u.literal_type[1] != lexer_literal_object_any)
    {
      parser_error_t error;

      if (context_p->last_cbc.u.literal_type[1] == lexer_literal_object_eval)
      {
        error = PARSER_ERR_EVAL_CANNOT_ASSIGNED;
      }
      else
      {
        PARSER_ASSERT (context_p->last_cbc.u.literal_type[1] == lexer_literal_object_arguments);
        error = PARSER_ERR_ARGUMENTS_CANNOT_ASSIGNED;
      }
      parser_raise_error (context_p, error);
    }

    context_p->last_cbc_opcode = (uint16_t) (opcode + CBC_UNARY_LVALUE_WITH_IDENT);
  }
  else if (context_p->last_cbc_opcode == CBC_PROP_GET)
  {
    PARSER_ASSERT (CBC_SAME_ARGS (CBC_PROP_GET, opcode));
    context_p->last_cbc_opcode = (uint16_t) opcode;
  }
  else if (context_p->last_cbc_opcode == CBC_PROP_STRING_GET)
  {
    PARSER_ASSERT (CBC_SAME_ARGS (CBC_PROP_STRING_GET, opcode + CBC_UNARY_LVALUE_WITH_PROP_STRING));
    context_p->last_cbc_opcode = (uint16_t) (opcode + CBC_UNARY_LVALUE_WITH_PROP_STRING);
  }
  else
  {
    /* A runtime error will happen. */
    parser_emit_cbc_ext (context_p, CBC_EXT_PUSH_UNDEFINED_BASE);
    parser_emit_cbc (context_p, opcode);
  }
} /* parser_emit_unary_lvalue_opcode */

/**
 * Parse array literal.
 */
static void
parser_parse_array_literal (parser_context_t *context_p) /**< context */
{
  uint32_t pushed_items = 0;

  PARSER_ASSERT (context_p->token.type == LEXER_LEFT_SQUARE);

  parser_emit_cbc (context_p, CBC_CREATE_ARRAY);
  lexer_next_token (context_p);

  while (PARSER_TRUE)
  {
    if (context_p->token.type == LEXER_RIGHT_SQUARE)
    {
      if (pushed_items > 0)
      {
        parser_emit_cbc_call (context_p, CBC_ARRAY_APPEND, pushed_items);
      }
      return;
    }

    pushed_items++;

    if (context_p->token.type == LEXER_COMMA)
    {
      parser_emit_cbc (context_p, CBC_PUSH_ELISION);
      lexer_next_token (context_p);
    }
    else
    {
      parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);

      if (context_p->token.type == LEXER_COMMA)
      {
        lexer_next_token (context_p);
      }
      else if (context_p->token.type != LEXER_RIGHT_SQUARE)
      {
        parser_raise_error (context_p, PARSER_ERR_ARRAY_ITEM_SEPARATOR_EXPECTED);
      }
    }

    if (pushed_items >= 64)
    {
      parser_emit_cbc_call (context_p, CBC_ARRAY_APPEND, pushed_items);
      pushed_items = 0;
    }
  }
} /* parser_parse_array_literal */

/**
 * Parse object literal.
 */
static void
parser_parse_object_literal (parser_context_t *context_p) /**< context */
{
  PARSER_ASSERT (context_p->token.type == LEXER_LEFT_BRACE);

  parser_emit_cbc (context_p, CBC_CREATE_OBJECT);

  while (PARSER_TRUE)
  {
    lexer_expect_object_literal_id (context_p, PARSER_FALSE);

    if (context_p->token.type == LEXER_RIGHT_BRACE)
    {
      return;
    }

    if (context_p->token.type == LEXER_PROPERTY_GETTER
        || context_p->token.type == LEXER_PROPERTY_SETTER)
    {
      uint32_t status_flags;
      cbc_ext_opcode_t opcode;
      uint16_t literal_index;

      if (context_p->token.type == LEXER_PROPERTY_GETTER)
      {
        status_flags = PARSER_IS_FUNCTION | PARSER_IS_CLOSURE | PARSER_IS_PROPERTY_GETTER;
        opcode = CBC_EXT_SET_GETTER;
      }
      else
      {
        status_flags = PARSER_IS_FUNCTION | PARSER_IS_CLOSURE | PARSER_IS_PROPERTY_SETTER;
        opcode = CBC_EXT_SET_SETTER;
      }

      lexer_expect_object_literal_id (context_p, PARSER_TRUE);
      literal_index = context_p->lit_object.index;

      parser_flush_cbc (context_p);
      lexer_construct_function_object (context_p, PARSER_ANONYMOUS_FUNCTION, status_flags);

      parser_emit_cbc_literal (context_p,
                               CBC_PUSH_LITERAL,
                               (uint16_t) (context_p->literal_count - 1));

      parser_emit_cbc_ext_literal (context_p,
                                   opcode,
                                   literal_index);

      lexer_next_token (context_p);
    }
    else
    {
      uint16_t literal_index = context_p->lit_object.index;

      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_COLON)
      {
        parser_raise_error (context_p, PARSER_ERR_COLON_EXPECTED);
      }

      lexer_next_token (context_p);
      parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);

      parser_emit_cbc_literal (context_p, CBC_SET_PROPERTY, literal_index);
    }

    if (context_p->token.type == LEXER_RIGHT_BRACE)
    {
      return;
    }
    else if (context_p->token.type != LEXER_COMMA)
    {
      parser_raise_error (context_p, PARSER_ERR_OBJECT_ITEM_SEPARATOR_EXPECTED);
    }
  }
} /* parser_parse_object_literal */

/**
 * Parse and record unary operators, and parse the primary literal.
 */
static void
parser_parse_unary_expression (parser_context_t *context_p, /**< context */
                               size_t *grouping_level_p) /**< grouping level */
{
  int new_was_seen = 0;

  /* Collect unary operators. */
  while (1)
  {
    /* Convert plus and minus binary operators to unary operators. */
    if (context_p->token.type == LEXER_ADD)
    {
      context_p->token.type = LEXER_PLUS;
    }
    else if (context_p->token.type == LEXER_SUBTRACT)
    {
      context_p->token.type = LEXER_NEGATE;
    }

    /* Bracketed expressions are primary expressions. At this
     * point their left paren is pushed onto the stack and
     * they are processed when their closing paren is reached. */
    if (context_p->token.type == LEXER_LEFT_PAREN)
    {
      (*grouping_level_p)++;
      new_was_seen = 0;
    }
    else if (context_p->token.type == LEXER_KEYW_NEW)
    {
      /* After 'new' unary operators are not allowed. */
      new_was_seen = 1;
    }
    else if (new_was_seen || !LEXER_IS_UNARY_OP_TOKEN (context_p->token.type))
    {
      break;
    }

    parser_stack_push_uint8 (context_p, context_p->token.type);
    lexer_next_token (context_p);
  }

  /* Parse primary expression. */
  switch (context_p->token.type)
  {
    case LEXER_LITERAL:
    {
      cbc_opcode_t opcode = CBC_PUSH_LITERAL;

      if (context_p->token.lit_location.type == LEXER_IDENT_LITERAL
          || context_p->token.lit_location.type == LEXER_STRING_LITERAL)
      {
        lexer_construct_literal_object (context_p,
                                        &context_p->token.lit_location,
                                        context_p->token.lit_location.type);
      }
      else if (context_p->token.lit_location.type == LEXER_NUMBER_LITERAL)
      {
        lexer_construct_number_object (context_p);
      }

      if (PARSER_OPCODE_IS_PUSH_LITERAL (context_p->last_cbc_opcode)
          && context_p->lit_object.type != lexer_literal_object_eval)
      {
        context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
        context_p->last_cbc.u.value = context_p->lit_object.index;
      }
      else
      {
        if (context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
        {
          opcode = CBC_PUSH_IDENT;
        }

        parser_emit_cbc_literal_from_token (context_p, opcode);
      }
      break;
    }
    case LEXER_KEYW_FUNCTION:
    {
      uint16_t prev_literal = PARSER_ANONYMOUS_FUNCTION;

      if (PARSER_OPCODE_IS_PUSH_LITERAL (context_p->last_cbc_opcode))
      {
        prev_literal = context_p->last_cbc.literal_index;
        context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
      }
      else
      {
        parser_flush_cbc (context_p);
      }

      lexer_construct_function_object (context_p,
                                       PARSER_ANONYMOUS_FUNCTION,
                                       PARSER_IS_FUNCTION | PARSER_IS_FUNC_EXPRESSION | PARSER_IS_CLOSURE);

      PARSER_ASSERT (context_p->last_cbc_opcode == PARSER_CBC_UNAVAILABLE);

      if (prev_literal != PARSER_ANONYMOUS_FUNCTION)
      {
        context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
        context_p->last_cbc.literal_index = prev_literal;
        context_p->last_cbc.u.value = (uint16_t) (context_p->literal_count - 1);
      }
      else
      {
        parser_emit_cbc_literal (context_p,
                                 CBC_PUSH_LITERAL,
                                 (uint16_t) (context_p->literal_count - 1));
      }
      break;
    }
    case LEXER_LEFT_BRACE:
    {
      parser_parse_object_literal (context_p);
      break;
    }
    case LEXER_LEFT_SQUARE:
    {
      parser_parse_array_literal (context_p);
      break;
    }
    case LEXER_DIVIDE:
    case LEXER_ASSIGN_DIVIDE:
    {
      lexer_construct_regexp_object (context_p, PARSER_FALSE);

      if (PARSER_OPCODE_IS_PUSH_LITERAL (context_p->last_cbc_opcode))
      {
        context_p->last_cbc_opcode = CBC_PUSH_TWO_LITERALS;
        context_p->last_cbc.u.value = (uint16_t) (context_p->literal_count - 1);
      }
      else
      {
        parser_emit_cbc_literal (context_p,
                                 CBC_PUSH_LITERAL,
                                 (uint16_t) (context_p->literal_count - 1));
      }
      break;
    }
    case LEXER_KEYW_THIS:
    {
      parser_emit_cbc (context_p, CBC_PUSH_THIS);
      break;
    }
    case LEXER_LIT_TRUE:
    {
      parser_emit_cbc (context_p, CBC_PUSH_TRUE);
      break;
    }
    case LEXER_LIT_FALSE:
    {
      parser_emit_cbc (context_p, CBC_PUSH_FALSE);
      break;
    }
    case LEXER_LIT_NULL:
    {
      parser_emit_cbc (context_p, CBC_PUSH_NULL);
      break;
    }
    default:
    {
      parser_raise_error (context_p, PARSER_ERR_PRIMARY_EXP_EXPECTED);
      break;
    }
  }
  lexer_next_token (context_p);
} /* parser_parse_unary_expression */

/**
 * Parse the postfix part of unary operators, and
 * generate byte code for the whole expression.
 */
static void
parser_process_unary_expression (parser_context_t *context_p) /**< context */
{
  /* Parse postfix part of a primary expression. */
  while (PARSER_TRUE)
  {
    /* Since break would only break the switch, we use
     * continue to continue this loop. Without continue,
     * the code abandons the loop. */
    switch (context_p->token.type)
    {
      case LEXER_DOT:
      {
        parser_push_result (context_p);

        lexer_expect_identifier (context_p, LEXER_STRING_LITERAL);
        PARSER_ASSERT (context_p->token.type == LEXER_LITERAL
                       && context_p->token.lit_location.type == LEXER_STRING_LITERAL);
        parser_emit_cbc_literal_from_token (context_p, CBC_PROP_STRING_GET);
        lexer_next_token (context_p);
        continue;
        /* FALLTHRU */
      }

      case LEXER_LEFT_SQUARE:
      {
        parser_push_result (context_p);

        lexer_next_token (context_p);
        parser_parse_expression (context_p, PARSE_EXPR);
        if (context_p->token.type != LEXER_RIGHT_SQUARE)
        {
          parser_raise_error (context_p, PARSER_ERR_RIGHT_SQUARE_EXPECTED);
        }
        lexer_next_token (context_p);

        if (context_p->last_cbc_opcode == CBC_PUSH_LITERAL
            && context_p->last_cbc.u.literal_type[0] == LEXER_STRING_LITERAL)
        {
          context_p->last_cbc_opcode = CBC_PROP_STRING_GET;
        }
        else
        {
          parser_emit_cbc (context_p, CBC_PROP_GET);
        }
        continue;
        /* FALLTHRU */
      }

      case LEXER_LEFT_PAREN:
      {
        size_t call_arguments = 0;
        cbc_argument_t argument = context_p->last_cbc;
        uint16_t opcode;

        parser_push_result (context_p);

        if (context_p->stack_top_uint8 == LEXER_KEYW_NEW)
        {
          parser_stack_pop_uint8 (context_p);
          if (context_p->last_cbc_opcode == CBC_PUSH_IDENT)
          {
            context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
            opcode = CBC_NEW_IDENT;
          }
          else
          {
            opcode = CBC_NEW;
          }
        }
        else
        {
          if (context_p->last_cbc_opcode == CBC_PUSH_IDENT)
          {
            context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
            opcode = CBC_CALL_IDENT;

            if (context_p->last_cbc.u.literal_type[1] == lexer_literal_object_eval)
            {
              context_p->status_flags |= PARSER_NO_REG_STORE;
              opcode = PARSER_TO_EXT_OPCODE (CBC_EXT_CALL_EVAL);
            }
          }
          else if (context_p->last_cbc_opcode == CBC_PROP_GET)
          {
            context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
            opcode = CBC_CALL_PROP;
          }
          else if (context_p->last_cbc_opcode == CBC_PROP_STRING_GET)
          {
            context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
            opcode = CBC_CALL_PROP_STRING;
          }
          else
          {
            opcode = CBC_CALL;
          }
        }

        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_RIGHT_PAREN)
        {
          while (PARSER_TRUE)
          {
            if (++call_arguments > CBC_MAXIMUM_BYTE_VALUE)
            {
              parser_raise_error (context_p, PARSER_ERR_ARGUMENT_LIMIT_REACHED);
            }

            parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);

            if (context_p->token.type != LEXER_COMMA)
            {
              break;
            }
            lexer_next_token (context_p);
          }

          if (context_p->token.type != LEXER_RIGHT_PAREN)
          {
            parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
          }
        }

        lexer_next_token (context_p);
        parser_flush_cbc (context_p);

        /* Pushing the next instruction manually. */
        argument.u.value = (uint16_t) call_arguments;
        context_p->last_cbc = argument;
        context_p->last_cbc_opcode = opcode;
        continue;
        /* FALLTHRU */
      }

      default:
      {
        if (context_p->stack_top_uint8 == LEXER_KEYW_NEW)
        {
          /* Push result is unnecessary since new has higher precedence than call. */
          if (context_p->last_cbc_opcode == CBC_PUSH_IDENT)
          {
            context_p->last_cbc_opcode = CBC_NEW_IDENT;
            context_p->last_cbc.u.value = 0;
          }
          else
          {
            parser_emit_cbc_call (context_p, CBC_NEW, 0);
          }
          parser_stack_pop_uint8 (context_p);
          continue;
        }

        if (!context_p->token.was_newline
            && (context_p->token.type == LEXER_INCREASE || context_p->token.type == LEXER_DECREASE))
        {
          cbc_opcode_t opcode = (context_p->token.type == LEXER_INCREASE) ? CBC_POST_INCR : CBC_POST_DECR;
          parser_push_result (context_p);
          parser_emit_unary_lvalue_opcode (context_p, opcode);
          lexer_next_token (context_p);
        }
        break;
      }
    }
    break;
  }

  /* Generate byte code for the unary operators. */
  while (PARSER_TRUE)
  {
    uint8_t token = context_p->stack_top_uint8;
    if (!LEXER_IS_UNARY_OP_TOKEN (token))
    {
      break;
    }

    parser_push_result (context_p);
    parser_stack_pop_uint8 (context_p);

    if (LEXER_IS_UNARY_LVALUE_OP_TOKEN (token))
    {
      token = (uint8_t) (LEXER_UNARY_LVALUE_OP_TOKEN_TO_OPCODE (token));
      parser_emit_unary_lvalue_opcode (context_p, token);
    }
    else
    {
      token = (uint8_t) (LEXER_UNARY_OP_TOKEN_TO_OPCODE (token));

      if (PARSER_OPCODE_IS_PUSH_LITERAL (context_p->last_cbc_opcode))
      {
        PARSER_ASSERT (CBC_SAME_ARGS (context_p->last_cbc_opcode, token + 1));
        context_p->last_cbc_opcode = (uint16_t) (token + 1);
      }
      else
      {
        parser_emit_cbc (context_p, token);
      }
    }
  }
} /* parser_process_unary_expression */

/**
 * Append a binary token.
 */
static void
parser_append_binary_token (parser_context_t *context_p) /**< context */
{
  PARSER_ASSERT (LEXER_IS_BINARY_OP_TOKEN (context_p->token.type));

  parser_push_result (context_p);

  if (LEXER_IS_BINARY_LVALUE_TOKEN (context_p->token.type))
  {
    /* Unlike other tokens, the whole byte code is saved for binary
     * lvalue operators, since they have multiple forms depending
     * on the previos instruction. */

    cbc_opcode_t opcode = LEXER_BINARY_LVALUE_OP_TOKEN_TO_OPCODE (context_p->token.type);

    if (context_p->last_cbc_opcode == CBC_PUSH_IDENT)
    {
      PARSER_ASSERT (CBC_SAME_ARGS (CBC_PUSH_IDENT, opcode + CBC_BINARY_LVALUE_WITH_IDENT));

      if ((context_p->status_flags & PARSER_IS_STRICT)
          && context_p->last_cbc.u.literal_type[1] != lexer_literal_object_any)
      {
        parser_error_t error;

        if (context_p->last_cbc.u.literal_type[1] == lexer_literal_object_eval)
        {
          error = PARSER_ERR_EVAL_CANNOT_ASSIGNED;
        }
        else
        {
          PARSER_ASSERT (context_p->last_cbc.u.literal_type[1] == lexer_literal_object_arguments);
          error = PARSER_ERR_ARGUMENTS_CANNOT_ASSIGNED;
        }
        parser_raise_error (context_p, error);
      }

      parser_stack_push_uint16 (context_p, context_p->last_cbc.literal_index);
      parser_stack_push_uint8 (context_p, (uint8_t) (opcode + CBC_BINARY_LVALUE_WITH_IDENT));
      context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
    }
    else if (context_p->last_cbc_opcode == CBC_PROP_GET)
    {
      PARSER_ASSERT (CBC_SAME_ARGS (CBC_PROP_GET, opcode));
      parser_stack_push_uint8 (context_p, opcode);
      context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
    }
    else if (context_p->last_cbc_opcode == CBC_PROP_STRING_GET)
    {
      PARSER_ASSERT (CBC_SAME_ARGS (CBC_PROP_STRING_GET, opcode + CBC_BINARY_LVALUE_WITH_PROP_STRING));
      parser_stack_push_uint16 (context_p, context_p->last_cbc.literal_index);
      parser_stack_push_uint8 (context_p, (uint8_t) (opcode + CBC_BINARY_LVALUE_WITH_PROP_STRING));
      context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
    }
    else
    {
      /* A runtime error will happen. */
      parser_emit_cbc_ext (context_p, CBC_EXT_PUSH_UNDEFINED_BASE);
      parser_stack_push_uint8 (context_p, opcode);
    }
  }
  else if (context_p->token.type == LEXER_LOGICAL_OR
           || context_p->token.type == LEXER_LOGICAL_AND)
  {
    parser_branch_t branch;
    uint16_t opcode = CBC_BRANCH_IF_LOGICAL_TRUE;

    if (context_p->token.type == LEXER_LOGICAL_AND)
    {
      opcode = CBC_BRANCH_IF_LOGICAL_FALSE;
    }

    parser_emit_cbc_forward_branch (context_p, opcode, &branch);
    parser_stack_push (context_p, &branch, sizeof (parser_branch_t));
  }

  parser_stack_push_uint8 (context_p, context_p->token.type);
} /* parser_append_binary_token */

/**
 * Emit opcode for binary computations.
 */
static void
parser_process_binary_opcodes (parser_context_t *context_p, /**< context */
                               uint8_t min_prec_treshold) /**< minimal precedence of tokens */
{
  while (PARSER_TRUE)
  {
    uint8_t token = context_p->stack_top_uint8;
    cbc_opcode_t opcode;

    /* For left-to-right operators (all binary operators except assignment
     * and logical operators), the byte code is flushed if the precedence
     * of the next operator is less or equal than the current operator. For
     * assignment and logical operators, we add 1 to the min precendence to
     * force right-to-left evaluation order. */

    if (!LEXER_IS_BINARY_OP_TOKEN (token)
        || parser_binary_precedence_table[token - LEXER_FIRST_BINARY_OP] < min_prec_treshold)
    {
      return;
    }

    parser_push_result (context_p);
    parser_stack_pop_uint8 (context_p);

    if (LEXER_IS_BINARY_LVALUE_TOKEN (token))
    {
      opcode = context_p->stack_top_uint8;
      parser_stack_pop_uint8 (context_p);

      if (CBC_IS_BINARY_IDENT_LVALUE_OPERATION (opcode))
      {
        if (PARSER_OPCODE_IS_PUSH_LITERAL (context_p->last_cbc_opcode))
        {
          PARSER_ASSERT (CBC_ARGS_EQ (opcode + CBC_BINARY_LVALUE_WITH_IDENT_LITERAL,
                                      CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2));
          context_p->last_cbc.u.value = context_p->last_cbc.literal_index;
          context_p->last_cbc.literal_index = parser_stack_pop_uint16 (context_p);
          context_p->last_cbc_opcode = (uint16_t) (opcode + CBC_BINARY_LVALUE_WITH_IDENT_LITERAL);
          continue;
        }
      }

      if (cbc_flags[opcode] & CBC_HAS_LITERAL_ARG)
      {
        uint16_t index = parser_stack_pop_uint16 (context_p);
        parser_emit_cbc_literal (context_p, opcode, index);
        continue;
      }
    }
    else if (token == LEXER_LOGICAL_OR || token == LEXER_LOGICAL_AND)
    {
      parser_branch_t branch;
      parser_stack_pop (context_p, &branch, sizeof (parser_branch_t));
      parser_set_branch_to_current_position (context_p, &branch);
      continue;
    }
    else
    {
      opcode = LEXER_BINARY_OP_TOKEN_TO_OPCODE (token);

      if (PARSER_OPCODE_IS_PUSH_LITERAL (context_p->last_cbc_opcode))
      {
        PARSER_ASSERT (CBC_SAME_ARGS (context_p->last_cbc_opcode, opcode + CBC_BINARY_WITH_LITERAL));
        context_p->last_cbc_opcode = (uint16_t) (opcode + CBC_BINARY_WITH_LITERAL);
        continue;
      }
      else if (context_p->last_cbc_opcode == CBC_PUSH_TWO_LITERALS)
      {
        PARSER_ASSERT (CBC_ARGS_EQ (opcode + CBC_BINARY_WITH_TWO_LITERALS,
                                    CBC_HAS_LITERAL_ARG | CBC_HAS_LITERAL_ARG2));
        context_p->last_cbc_opcode = (uint16_t) (opcode + CBC_BINARY_WITH_TWO_LITERALS);
        continue;
      }
    }
    parser_emit_cbc (context_p, opcode);
  }
} /* parser_process_binary_opcodes */

/**
 * Parse expression.
 */
void
parser_parse_expression (parser_context_t *context_p, /**< context */
                         int options) /**< option flags */
{
  size_t grouping_level = 0;

  parser_stack_push_uint8 (context_p, LEXER_EXPRESSION_START);

  while (PARSER_TRUE)
  {
    if (options & PARSE_EXPR_HAS_LITERAL)
    {
      PARSER_ASSERT (PARSER_OPCODE_IS_PUSH_LITERAL (context_p->last_cbc_opcode));
      /* True only for the first expression. */
      options &= ~PARSE_EXPR_HAS_LITERAL;
    }
    else
    {
      parser_parse_unary_expression (context_p, &grouping_level);
    }

    while (PARSER_TRUE)
    {
      parser_process_unary_expression (context_p);

      /* The engine flush binary opcodes above this precedence. */
      uint8_t min_prec_treshold = CBC_MAXIMUM_BYTE_VALUE;

      if (LEXER_IS_BINARY_OP_TOKEN (context_p->token.type))
      {
        min_prec_treshold = parser_binary_precedence_table[context_p->token.type - LEXER_FIRST_BINARY_OP];
        if (LEXER_IS_BINARY_LVALUE_TOKEN (context_p->token.type)
            || context_p->token.type == LEXER_LOGICAL_OR
            || context_p->token.type == LEXER_LOGICAL_AND)
        {
          /* Right-to-left evaluation order. */
          min_prec_treshold++;
        }
      }
      else
      {
        min_prec_treshold = 0;
      }

      parser_process_binary_opcodes (context_p, min_prec_treshold);

      if (context_p->token.type == LEXER_RIGHT_PAREN
          && context_p->stack_top_uint8 == LEXER_LEFT_PAREN)
      {
        PARSER_ASSERT (grouping_level > 0);
        grouping_level --;
        parser_stack_pop_uint8 (context_p);
        lexer_next_token (context_p);
        continue;
      }
      else if (context_p->token.type == LEXER_QUESTION_MARK)
      {
        cbc_opcode_t opcode = CBC_BRANCH_IF_FALSE_FORWARD;
        parser_branch_t cond_branch;
        parser_branch_t uncond_branch;

        parser_push_result (context_p);

        if (context_p->last_cbc_opcode == CBC_LOGICAL_NOT)
        {
          context_p->last_cbc_opcode = PARSER_CBC_UNAVAILABLE;
          opcode = CBC_BRANCH_IF_TRUE_FORWARD;
        }

        parser_emit_cbc_forward_branch (context_p, opcode, &cond_branch);

        lexer_next_token (context_p);
        parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);
        parser_emit_cbc_forward_branch (context_p, CBC_JUMP_FORWARD, &uncond_branch);
        parser_set_branch_to_current_position (context_p, &cond_branch);

        /* Although byte code is constructed for two branches,
         * only one of them will be executed. To reflect this
         * the stack is manually adjusted. */
        PARSER_ASSERT (context_p->stack_depth > 0);
        context_p->stack_depth--;

        if (context_p->token.type != LEXER_COLON)
        {
          parser_raise_error (context_p, PARSER_ERR_COLON_FOR_CONDITIONAL_EXPECTED);
        }

        lexer_next_token (context_p);

        parser_parse_expression (context_p, PARSE_EXPR_NO_COMMA);
        parser_set_branch_to_current_position (context_p, &uncond_branch);

        /* Last opcode rewrite is not allowed because
         * the result may come from the first branch. */
        parser_flush_cbc (context_p);
        continue;
      }
      break;
    }

    if (context_p->token.type == LEXER_COMMA)
    {
      if (!(options & PARSE_EXPR_NO_COMMA) || grouping_level > 0)
      {
        if (!CBC_NO_RESULT_OPERATION (context_p->last_cbc_opcode))
        {
          parser_emit_cbc (context_p, CBC_POP);
        }
        lexer_next_token (context_p);
        continue;
      }
    }
    else if (LEXER_IS_BINARY_OP_TOKEN (context_p->token.type))
    {
      parser_append_binary_token (context_p);
      lexer_next_token (context_p);
      continue;
    }
    break;
  }

  if (grouping_level != 0)
  {
    parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
  }

  PARSER_ASSERT (context_p->stack_top_uint8 == LEXER_EXPRESSION_START);
  parser_stack_pop_uint8 (context_p);

  if (options & PARSE_EXPR_STATEMENT)
  {
    if (!CBC_NO_RESULT_OPERATION (context_p->last_cbc_opcode))
    {
      parser_emit_cbc (context_p, CBC_POP);
    }
  }
  else if (options & PARSE_EXPR_BLOCK)
  {
    if (CBC_NO_RESULT_BINARY_OPERATION (context_p->last_cbc_opcode))
    {
      context_p->last_cbc_opcode = PARSER_TO_BINARY_OPERATION_WITH_BLOCK (context_p->last_cbc_opcode);
      parser_flush_cbc (context_p);
    }
    else
    {
      if (CBC_NO_RESULT_OPERATION (context_p->last_cbc_opcode))
      {
        PARSER_ASSERT (context_p->last_cbc_opcode == PARSER_TO_EXT_OPCODE (CBC_EXT_CALL_EVAL)
                       || CBC_SAME_ARGS (context_p->last_cbc_opcode, context_p->last_cbc_opcode + 1));
        context_p->last_cbc_opcode++;
      }
      parser_emit_cbc (context_p, CBC_POP_BLOCK);
    }
  }
  else
  {
    parser_push_result (context_p);
  }
} /* parser_parse_expression */
