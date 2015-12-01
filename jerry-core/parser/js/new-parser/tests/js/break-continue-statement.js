/* This test contains break and continue statement parser tests. */

next_statement;

for (;;) {
  x++;
  break;
  y++;
  continue;
  z++;
}

next_statement;

while (true)
  { { {
  x++;
  break; y++;
  continue; z++;
  } } }

next_statement;

while (true)
  do {
    x++
    break
    y
    continue
    z
  } while (true)

next_statement;

a: { for( ; ; ) {
  x++;
  b: {
    while (x) {
      break
      a; break a
      break b; b
    }
  }
  y++;
} }

next_statement;

{
outer_label
  :
     label: for (var a,b,c; a++, b < c; b(c), c)

       whil : while
       (a+b)
       { inner_label: { do {
         x();
         continue
         outer_label ; continue outer_label
         continue outer_label ; outer_label
         continue whil
         x();
       } while (true) } }
}

next_statement;
