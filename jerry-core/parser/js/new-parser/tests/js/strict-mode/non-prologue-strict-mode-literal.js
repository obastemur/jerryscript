/* This test contains strict mode negative parser tests. */

next_statement;

"use strict"

next_statement;

{
  "use strict";
  var eval;
}

function non_strict_mode(arguments) {
  return;
}
