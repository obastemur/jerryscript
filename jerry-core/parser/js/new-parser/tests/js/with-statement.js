/* This test contains with statement parser tests. */

function func(arg,val,c,arg,val)
{
  with (val) {
    v1 = arg;
    {
      try {
        with (ident)
          arg.other = v1.val;
      } finally {}
    }
    arg.other++
  }

  var v1, ident, other;
  other = val;
}
