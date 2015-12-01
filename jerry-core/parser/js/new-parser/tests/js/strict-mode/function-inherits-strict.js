/* This test contains strict mode test, whether function inherits strict mode. */

"use strict";

function inherits_strict() {
  with ( obj ) next_statement;
}
