/* This test contains strict mode test, whether function strict mode
 * applies only to the function.
 */

function strict_mode() {
  "use strict";
  return;
}

function non_strict_mode(eval) {
  with ( obj ) next_statement;
}

var arguments;

next_statement;
