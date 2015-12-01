/* This test contains switch statement parser tests. */

next_statement;

  switch /**/
    (a * b
    -c) {
  }

next_statement;

  switch (x)
  {
  // comment
  default /**/ : // comment
    x()
    break
  }

next_statement;

label:
  switch (y()) {
  case a
    + b /* comment
*/ * c
  :
    y();
    switch (z())
    {
    default:
      z();
      break;
    case //
  a /**/:
      z(); break
    case b:
      switch (n) { case a: break label; a++ ; default: c++; break; case b: b++ }

      z();
      break;
      z();
    }
    y();
    break;
  case b:
  case c:
    y();
    break;
  case d + d():
  }

next_statement;

label:
while (n)
switch (y) {
  case a: default: case /**/ b: { { { a(); break } } }
  case c: { { continue } }
  case c: continue label; { break label; }
}

next_statement;

{ switch(false) { } }

next_statement;

switch (null)
{
  case
    a
    ?
    b
    ?
    (c[n ? true : false] ? false : true)
    :
    d
    :
    e
  /* end */ :
    f++;
}

next_statement;
