/* This test contains try statement parser tests. */

next_statement;

try {
  x();
} finally {
  x();
}

next_statement;

try {
  x();
} catch (exc) {
  x();
} finally {
  x();
}

next_statement;

try {
  x();
} catch (exc) {
  x();
}

next_statement;

label:
for (;false;) {
  try {
    continue
    continue label
    break
    break label
  } catch (evall) {
    continue
    continue label
    break
    break label
  } finally {
    continue
    continue label
    break
    break label
  }
}

next_statement;
