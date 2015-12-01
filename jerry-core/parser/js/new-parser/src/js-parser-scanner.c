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

/**
 * Scan mode types types.
 */
typedef enum
{
  SCAN_MODE_PRIMARY_EXPRESSION,
  SCAN_MODE_PRIMARY_EXPRESSION_AFTER_NEW,
  SCAN_MODE_POST_PRIMARY_EXPRESSION,
  SCAN_MODE_PRIMARY_EXPRESSION_END,
  SCAN_MODE_STATEMENT,
  SCAN_MODE_FUNCTION_ARGUMENTS,
  SCAN_MODE_PROPERTY_NAME,
} scan_modes_t;

/**
 * Scan stack mode types types.
 */
typedef enum
{
  SCAN_STACK_HEAD,
  SCAN_STACK_PAREN_EXPRESSION,
  SCAN_STACK_PAREN_STATEMENT,
  SCAN_STACK_COLON_EXPRESSION,
  SCAN_STACK_COLON_STATEMENT,
  SCAN_STACK_SQUARE_BRACKETED_EXPRESSION,
  SCAN_STACK_OBJECT_LITERAL,
  SCAN_STACK_BLOCK_STATEMENT,
  SCAN_STACK_BLOCK_EXPRESSION,
  SCAN_STACK_BLOCK_PROPERTY,
} scan_stack_modes_t;

/**
 * Scan primary expression.
 *
 * @return PARSER_TRUE for continue, PARSER_FALSE for break
 */
static int
parser_scan_primary_expression (parser_context_t *context_p, /**< context */
                                lexer_token_type_t type, /**< current token type */
                                scan_stack_modes_t stack_top, /**< current stack top */
                                scan_modes_t *mode) /**< scan mode */
{
  switch (type)
  {
    case LEXER_KEYW_NEW:
    {
      *mode = SCAN_MODE_PRIMARY_EXPRESSION_AFTER_NEW;
      break;
    }
    case LEXER_DIVIDE:
    case LEXER_ASSIGN_DIVIDE:
    {
      lexer_construct_regexp_object (context_p, PARSER_TRUE);
      *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_KEYW_FUNCTION:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_EXPRESSION);
      *mode = SCAN_MODE_FUNCTION_ARGUMENTS;
      break;
    }
    case LEXER_LEFT_PAREN:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_PAREN_EXPRESSION);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_LEFT_SQUARE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_SQUARE_BRACKETED_EXPRESSION);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_LEFT_BRACE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_OBJECT_LITERAL);
      *mode = SCAN_MODE_PROPERTY_NAME;
      return PARSER_TRUE;
    }
    case LEXER_LITERAL:
    case LEXER_KEYW_THIS:
    case LEXER_LIT_TRUE:
    case LEXER_LIT_FALSE:
    case LEXER_LIT_NULL:
    {
      *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_RIGHT_SQUARE:
    {
      if (stack_top != SCAN_STACK_SQUARE_BRACKETED_EXPRESSION)
      {
        parser_raise_error (context_p, PARSER_ERR_PRIMARY_EXP_EXPECTED);
      }
      parser_stack_pop_uint8 (context_p);
      *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_COMMA:
    {
      if (stack_top != SCAN_STACK_SQUARE_BRACKETED_EXPRESSION)
      {
        parser_raise_error (context_p, PARSER_ERR_PRIMARY_EXP_EXPECTED);
      }
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      break;
    }
    case LEXER_RIGHT_PAREN:
    {
      *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
      if (stack_top == SCAN_STACK_PAREN_STATEMENT)
      {
        *mode = SCAN_MODE_STATEMENT;
      }
      else if (stack_top != SCAN_STACK_PAREN_EXPRESSION)
      {
        parser_raise_error (context_p, PARSER_ERR_PRIMARY_EXP_EXPECTED);
      }

      parser_stack_pop_uint8 (context_p);
      break;
    }
    case LEXER_SEMICOLON:
    {
      /* Needed by for (;;) statements. */
      if (stack_top != SCAN_STACK_PAREN_STATEMENT)
      {
        parser_raise_error (context_p, PARSER_ERR_PRIMARY_EXP_EXPECTED);
      }
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      break;
    }
    default:
    {
      parser_raise_error (context_p, PARSER_ERR_PRIMARY_EXP_EXPECTED);
    }
  }
  return PARSER_FALSE;
} /* parser_scan_primary_expression */

/**
 * Scan the tokens after the primary expression.
 *
 * @return PARSER_TRUE for break, PARSER_FALSE for fall through
 */
static int
parser_scan_post_primary_expression (parser_context_t *context_p, /**< context */
                                     lexer_token_type_t type, /**< current token type */
                                     scan_modes_t *mode) /**< scan mode */
{
  switch (type)
  {
    case LEXER_DOT:
    {
      lexer_scan_identifier (context_p, PARSER_FALSE);
      return PARSER_TRUE;
    }
    case LEXER_LEFT_PAREN:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_PAREN_EXPRESSION);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return PARSER_TRUE;
    }
    case LEXER_LEFT_SQUARE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_SQUARE_BRACKETED_EXPRESSION);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return PARSER_TRUE;
    }
    case LEXER_INCREASE:
    case LEXER_DECREASE:
    {
      if (!context_p->token.was_newline)
      {
        *mode = SCAN_MODE_PRIMARY_EXPRESSION_END;
        return PARSER_TRUE;
      }
      /* FALLTHRU */
    }
    default:
    {
      break;
    }
  }

  return PARSER_FALSE;
} /* parser_scan_post_primary_expression */

/**
 * Scan the tokens after the primary expression.
 *
 * @return PARSER_TRUE for continue, PARSER_FALSE for break
 */
static int
parser_scan_primary_expression_end (parser_context_t *context_p, /**< context */
                                    lexer_token_type_t type, /**< current token type */
                                    scan_stack_modes_t stack_top, /**< current stack top */
                                    lexer_token_type_t end_type, /**< terminator token type */
                                    scan_modes_t *mode) /**< scan mode */
{
  switch (type)
  {
    case LEXER_QUESTION_MARK:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_COLON_EXPRESSION);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return PARSER_FALSE;
    }
    case LEXER_COMMA:
    {
      if (stack_top == SCAN_STACK_OBJECT_LITERAL)
      {
        *mode = SCAN_MODE_PROPERTY_NAME;
        return PARSER_TRUE;
      }
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return PARSER_FALSE;
    }
    case LEXER_COLON:
    {
      if (stack_top == SCAN_STACK_COLON_EXPRESSION
          || stack_top == SCAN_STACK_COLON_STATEMENT)
      {
        if (stack_top == SCAN_STACK_COLON_EXPRESSION)
        {
          *mode = SCAN_MODE_PRIMARY_EXPRESSION;
        }
        else
        {
          *mode = SCAN_MODE_STATEMENT;
        }
        parser_stack_pop_uint8 (context_p);
        return PARSER_FALSE;
      }
      /* FALLTHRU */
    }
    default:
    {
      break;
    }
  }

  if (LEXER_IS_BINARY_OP_TOKEN (type)
      || (type == LEXER_SEMICOLON && stack_top == SCAN_STACK_PAREN_STATEMENT))
  {
    *mode = SCAN_MODE_PRIMARY_EXPRESSION;
    return PARSER_FALSE;
  }

  if ((type == LEXER_RIGHT_SQUARE && stack_top == SCAN_STACK_SQUARE_BRACKETED_EXPRESSION)
      || (type == LEXER_RIGHT_PAREN && stack_top == SCAN_STACK_PAREN_EXPRESSION)
      || (type == LEXER_RIGHT_BRACE && stack_top == SCAN_STACK_OBJECT_LITERAL))
  {
    parser_stack_pop_uint8 (context_p);
    *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
    return PARSER_FALSE;
  }

  *mode = SCAN_MODE_STATEMENT;
  if (type == LEXER_RIGHT_PAREN && stack_top == SCAN_STACK_PAREN_STATEMENT)
  {
    parser_stack_pop_uint8 (context_p);
    return PARSER_FALSE;
  }

  /* Check whether we can enter to statement mode. */
  if (stack_top != SCAN_STACK_BLOCK_STATEMENT
      && stack_top != SCAN_STACK_BLOCK_EXPRESSION
      && !(stack_top == SCAN_STACK_HEAD && end_type == LEXER_SCAN_SWITCH))
  {
    parser_raise_error (context_p, PARSER_ERR_INVALID_EXPRESSION);
  }

  if (type == LEXER_RIGHT_BRACE
      || context_p->token.was_newline)
  {
    return PARSER_TRUE;
  }

  if (type != LEXER_SEMICOLON)
  {
    parser_raise_error (context_p, PARSER_ERR_INVALID_EXPRESSION);
  }

  return PARSER_FALSE;
} /* parser_scan_primary_expression_end */

/**
 * Scan statements.
 *
 * @return PARSER_TRUE for continue, PARSER_FALSE for break
 */
static int
parser_scan_statement (parser_context_t *context_p, /**< context */
                       lexer_token_type_t type, /**< current token type */
                       scan_stack_modes_t stack_top, /**< current stack top */
                       scan_modes_t *mode) /**< scan mode */
{
  switch (type)
  {
    case LEXER_SEMICOLON:
    case LEXER_KEYW_ELSE:
    case LEXER_KEYW_DO:
    case LEXER_KEYW_RETURN:
    case LEXER_KEYW_TRY:
    case LEXER_KEYW_FINALLY:
    case LEXER_KEYW_DEBUGGER:
    {
      return PARSER_FALSE;
    }
    case LEXER_KEYW_IF:
    case LEXER_KEYW_WHILE:
    case LEXER_KEYW_WITH:
    case LEXER_KEYW_SWITCH:
    case LEXER_KEYW_CATCH:
    {
      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_LEFT_PAREN)
      {
        parser_raise_error (context_p, PARSER_ERR_LEFT_PAREN_EXPECTED);
      }

      parser_stack_push_uint8 (context_p, SCAN_STACK_PAREN_STATEMENT);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return PARSER_FALSE;
    }
    case LEXER_KEYW_FOR:
    {
      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_LEFT_PAREN)
      {
        parser_raise_error (context_p, PARSER_ERR_LEFT_PAREN_EXPECTED);
      }

      lexer_next_token (context_p);
      parser_stack_push_uint8 (context_p, SCAN_STACK_PAREN_STATEMENT);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;

      if (context_p->token.type == LEXER_KEYW_VAR)
      {
        return PARSER_FALSE;
      }
      return PARSER_TRUE;
    }
    case LEXER_KEYW_VAR:
    case LEXER_KEYW_THROW:
    {
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return PARSER_FALSE;
    }
    case LEXER_KEYW_BREAK:
    case LEXER_KEYW_CONTINUE:
    {
      lexer_next_token (context_p);
      if (!context_p->token.was_newline
          && context_p->token.type == LEXER_LITERAL
          && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
      {
        return PARSER_FALSE;
      }
      return PARSER_TRUE;
    }
    case LEXER_KEYW_DEFAULT:
    {
      lexer_next_token (context_p);
      if (context_p->token.type != LEXER_COLON)
      {
        parser_raise_error (context_p, PARSER_ERR_COLON_EXPECTED);
      }
      return PARSER_FALSE;
    }
    case LEXER_KEYW_CASE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_COLON_STATEMENT);
      *mode = SCAN_MODE_PRIMARY_EXPRESSION;
      return PARSER_FALSE;
    }
    case LEXER_RIGHT_BRACE:
    {
      if (stack_top == SCAN_STACK_BLOCK_STATEMENT
          || stack_top == SCAN_STACK_BLOCK_EXPRESSION
          || stack_top == SCAN_STACK_BLOCK_PROPERTY)
      {
        parser_stack_pop_uint8 (context_p);

        if (stack_top == SCAN_STACK_BLOCK_EXPRESSION)
        {
          *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
        }
        else if (stack_top == SCAN_STACK_BLOCK_PROPERTY)
        {
          *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
          lexer_next_token (context_p);
          if (context_p->token.type != LEXER_COMMA
              && context_p->token.type != LEXER_RIGHT_BRACE)
          {
            parser_raise_error (context_p, PARSER_ERR_OBJECT_ITEM_SEPARATOR_EXPECTED);
          }
          return PARSER_TRUE;
        }
        return PARSER_FALSE;
      }
      break;
    }
    case LEXER_LEFT_BRACE:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_STATEMENT);
      return PARSER_FALSE;
    }
    case LEXER_KEYW_FUNCTION:
    {
      parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_STATEMENT);
      *mode = SCAN_MODE_FUNCTION_ARGUMENTS;
      return PARSER_FALSE;
    }
    default:
    {
      break;
    }
  }

  *mode = SCAN_MODE_PRIMARY_EXPRESSION;

  if (type == LEXER_LITERAL
      && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
  {
    lexer_next_token (context_p);
    if (context_p->token.type == LEXER_COLON)
    {
      *mode = SCAN_MODE_STATEMENT;
      return PARSER_FALSE;
    }
    *mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
  }

  return PARSER_TRUE;
} /* parser_scan_statement */

/**
 * Pre-scan for token(s).
 */
void
parser_scan_until (parser_context_t *context_p, /**< context */
                   lexer_range_t *range_p, /**< destination range */
                   lexer_token_type_t end_type) /**< terminator token type */
{
  scan_modes_t mode;
  lexer_token_type_t end_type_b = end_type;

  range_p->source_p = context_p->source_p;
  range_p->source_end_p = context_p->source_p;
  range_p->line = context_p->line;
  range_p->column = context_p->column;

  mode = SCAN_MODE_PRIMARY_EXPRESSION;

  if (end_type == LEXER_KEYW_CASE)
  {
    end_type = LEXER_SCAN_SWITCH;
    end_type_b = LEXER_SCAN_SWITCH;
    mode = SCAN_MODE_STATEMENT;
  }
  else
  {
    lexer_next_token (context_p);

    if (end_type == LEXER_KEYW_IN)
    {
      end_type_b = LEXER_SEMICOLON;
      if (context_p->token.type == LEXER_KEYW_VAR)
      {
        lexer_next_token (context_p);
      }
    }
  }

  parser_stack_push_uint8 (context_p, SCAN_STACK_HEAD);

  while (PARSER_TRUE)
  {
    lexer_token_type_t type = context_p->token.type;
    scan_stack_modes_t stack_top = context_p->stack_top_uint8;

    if (type == LEXER_EOS)
    {
      parser_raise_error (context_p, PARSER_ERR_EXPRESSION_EXPECTED);
    }

    if (stack_top == SCAN_STACK_HEAD
        && (type == end_type || type == end_type_b))
    {
      parser_stack_pop_uint8 (context_p);
      return;
    }

    switch (mode)
    {
      case SCAN_MODE_PRIMARY_EXPRESSION:
      {
        if (type == LEXER_ADD
            || type == LEXER_SUBTRACT
            || LEXER_IS_UNARY_OP_TOKEN (type))
        {
          break;
        }
        /* FALLTHRU */
      }
      case SCAN_MODE_PRIMARY_EXPRESSION_AFTER_NEW:
      {
        if (parser_scan_primary_expression (context_p, type, stack_top, &mode))
        {
          continue;
        }
        break;
      }
      case SCAN_MODE_POST_PRIMARY_EXPRESSION:
      {
        if (parser_scan_post_primary_expression (context_p, type, &mode))
        {
          break;
        }
        /* FALLTHRU */
      }
      case SCAN_MODE_PRIMARY_EXPRESSION_END:
      {
        if (parser_scan_primary_expression_end (context_p, type, stack_top, end_type, &mode))
        {
          continue;
        }
        break;
      }
      case SCAN_MODE_STATEMENT:
      {
        if (end_type == LEXER_SCAN_SWITCH
            && stack_top == SCAN_STACK_HEAD
            && (type == LEXER_KEYW_DEFAULT || type == LEXER_KEYW_CASE || type == LEXER_RIGHT_BRACE))
        {
          parser_stack_pop_uint8 (context_p);
          return;
        }

        if (parser_scan_statement (context_p, type, stack_top, &mode))
        {
          continue;
        }
        break;
      }
      case SCAN_MODE_FUNCTION_ARGUMENTS:
      {
        PARSER_ASSERT (stack_top == SCAN_STACK_BLOCK_STATEMENT
                       || stack_top == SCAN_STACK_BLOCK_EXPRESSION
                       || stack_top == SCAN_STACK_BLOCK_PROPERTY);

        if (context_p->token.type == LEXER_LITERAL
            && context_p->token.lit_location.type == LEXER_IDENT_LITERAL)
        {
          lexer_next_token (context_p);
        }

        if (context_p->token.type != LEXER_LEFT_PAREN)
        {
          parser_raise_error (context_p, PARSER_ERR_ARGUMENT_LIST_EXPECTED);
        }
        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_RIGHT_PAREN)
        {
          while (PARSER_TRUE)
          {
            if (context_p->token.type != LEXER_LITERAL
                || context_p->token.lit_location.type != LEXER_IDENT_LITERAL)
            {
              parser_raise_error (context_p, PARSER_ERR_IDENTIFIER_EXPECTED);
            }
            lexer_next_token (context_p);

            if (context_p->token.type != LEXER_COMMA)
            {
              break;
            }
            lexer_next_token (context_p);
          }
        }

        if (context_p->token.type != LEXER_RIGHT_PAREN)
        {
          parser_raise_error (context_p, PARSER_ERR_RIGHT_PAREN_EXPECTED);
        }

        lexer_next_token (context_p);

        if (context_p->token.type != LEXER_LEFT_BRACE)
        {
          parser_raise_error (context_p, PARSER_ERR_LEFT_BRACE_EXPECTED);
        }
        mode = SCAN_MODE_STATEMENT;
        break;
      }
      case SCAN_MODE_PROPERTY_NAME:
      {
        PARSER_ASSERT (stack_top == SCAN_STACK_OBJECT_LITERAL);

        lexer_scan_identifier (context_p, PARSER_TRUE);

        if (context_p->token.type == LEXER_RIGHT_BRACE)
        {
          parser_stack_pop_uint8 (context_p);
          mode = SCAN_MODE_POST_PRIMARY_EXPRESSION;
          break;
        }

        if (context_p->token.type == LEXER_PROPERTY_GETTER
            || context_p->token.type == LEXER_PROPERTY_SETTER)
        {
          parser_stack_push_uint8 (context_p, SCAN_STACK_BLOCK_PROPERTY);
          mode = SCAN_MODE_FUNCTION_ARGUMENTS;
          break;
        }

        lexer_next_token (context_p);
        if (context_p->token.type != LEXER_COLON)
        {
          parser_raise_error (context_p, PARSER_ERR_COLON_EXPECTED);
        }

        mode = SCAN_MODE_PRIMARY_EXPRESSION;
        break;
      }
    }

    range_p->source_end_p = context_p->source_p;
    lexer_next_token (context_p);
  }
} /* parser_scan_until */
