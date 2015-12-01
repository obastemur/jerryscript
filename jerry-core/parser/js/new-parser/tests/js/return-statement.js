/* This test contains return statement parser tests. */

next_statement;

function return_undefined() {
  return; return
  without_exp
  { return }
}

next_statement;

function return_expression() {
  return a
    + b
  { return exp1 * (exp2) } return ++
  a
}

next_statement;
