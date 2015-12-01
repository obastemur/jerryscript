/* This test contains if statement parser tests. */

next_statement;

  if (a) //
    if (b) /* c */ if (c) d();

next_statement;

  if (a) ;
    else if (b) { c() } else if (d) e()
    else f();

next_statement;

  if (a)

    if (b)

    if (c)
      d()

  else 
    { e() }

  else

  if (f)
    g();

  else
    h(); else i()

next_statement;

  if (a) ; x++;
  if (a) {} x++;

next_statement;

  if (a) ; else ; y++;
  if (a) {} else {} y++;

next_statement;
