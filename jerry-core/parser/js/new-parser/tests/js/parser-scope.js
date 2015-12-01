/* This test contains parser scope tests. */

var a = 2, b = "str";

function f(arg)
{
  var val = arg;

  function g(x) {
    var v;
    return x + v + val;
  }

  val = g();
  return val;
}

function f(arg)
{
  var val = arg;

  val = function(arg,arg) {
    var local1 = 2, local_2 = 5;
    return arg + local1 * local_2 + val;
  } ();
  return val;
}

