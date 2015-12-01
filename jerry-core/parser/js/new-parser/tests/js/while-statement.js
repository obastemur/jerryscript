/* This test contains while statement parser tests. */

next_statement;

while (a)
 ;

next_statement;

while (!(a && b))
  while (
     /**/
     true
     /**/
     )
   {{{
      print("text");
   }}}

next_statement;

while (false)
  /* comment \uXX */
  // comment \uX
  while (a) while (b ? true : /**/ true) x; { y } //

next_statement;

