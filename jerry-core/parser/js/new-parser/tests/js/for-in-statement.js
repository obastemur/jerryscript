/* This test contains for-in statement parser tests. */

next_statement;

for (a in b.c)
{
}

next_statement;

for
  (a.i in bb) for (
    b["i"] // comment
    in cc) x++

next_statement;

for ("x" in c[x])

  { d(); for (d[y] in bb) {} d(); }

next_statement;

label:
  for(a in"b"){ //
    for/**/(/**/c/**/in/**/d/**/)
    {
      x();
      continue;
      x();
      break;
      x();
      break label;
      x();
    }

    x();
    continue;
    x();
    break;
    x();
  }

next_statement;

for(var/**/var_a in"z")
{
  for(var/**/a=x*"y"in"z") x++;
}

next_statement;

for
(
var
var_a
in
"z")
{
for
(
var
/**/
a
=
x*"y"
in
"z")
x++;
}

