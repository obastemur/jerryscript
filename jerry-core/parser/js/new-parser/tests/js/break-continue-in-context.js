/* This test contains break and continue statement parser tests. */

next_statement;

label: {
  break label
  x();
}

next_statement;

label:
  with (a) {
    x();
    break label;
  }

next_statement;

switch (x) {
default:
  with (a) {
    x();
    break
  }
}

next_statement;

for (a.m in b) {
  x();
  label: {
    break label; x();
  }
  break;
}

next_statement;

with (x) for ( ; false ; ) break

next_statement;

for (var var_a in b) { continue ; x() }

next_statement;

for ( ; x ; x++) with
  (x) continue

next_statement;

label:
  for ( ; false ; x++) for (a in b) continue label

next_statement;

label: label2: label3: for (a[x().m] in b() * c) {
continue label
x()
while ("5") continue label
label4: label5: label6
: for (var a = "x" >> y in c) continue label }

next_statement;
