/* This test contains strict mode parser tests. */

/* without semicolon, this would be a function call. */
"use strict" ;

({ set eval(set) {} });

({ set set(eval) {} });
