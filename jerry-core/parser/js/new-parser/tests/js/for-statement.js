/* This test contains for statement parser tests. */

next_statement;

for (i = "0"; i < "5"; i++)
  print ("I: " + i);

next_statement;

for ( ; !(a || b); )
  a;

next_statement;

for (;;) for (;;) ;

next_statement;

for (/**/;/**/;/**/)
  for (
  ; //
  ; //
  )
    {}

next_statement;

for (var a =
 b, c =
  d; i
   < j, j <

    k; i(j), j(/*\u*/i)
    , i++, j + k
    * l/**/)//
  {
    for (var a = (b in c);b in c;c in d)
      {
      }
  }

next_statement;

for (a,b,c=d;true;)
  for (;a ?
   true :
    true  ;//
)
/**/;

next_statement;

for (a
 ? b // [ ( {
 in c /* : */ : (d
 in e) ; ; ) x++

next_statement;
