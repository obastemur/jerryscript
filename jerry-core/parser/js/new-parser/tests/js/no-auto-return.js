/* This test contains no auto return parser tests. */

function f()
{
  a + b
}

function g()
{
  return x + y * 4;
}

function h()
{
  if (x) return 8
}

function i()
{
  while (true) { return }
}

function k()
{
  if (x) return 1
  else return 2
}

