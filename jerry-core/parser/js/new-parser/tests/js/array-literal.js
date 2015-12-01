/* This test contains array literal parser tests. */

next_statement;

[ //
]

next_statement;

[, /* */ ]

next_statement;

["xx" + "yy",]

next_statement;

[,a + b,] * [function () { return [] }, , a().b <<= magic]

next_statement;

f([ [], [a],, a[x()] ])

next_statement;
