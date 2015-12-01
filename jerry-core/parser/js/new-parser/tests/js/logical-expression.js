/* This test contains logical expression parser tests. */

next_statement;

a ^ //
b||/**/c & d
|| e/*&& g*/| f;

next_statement;

a/*\u
*/^b&&c&d&&e|f;

next_statement;

a |/*
*/b//&&
&&	c| d &&e |/*	*/f;

next_statement;

a||b&&c;

next_statement;

a  &&  b  ||  c;

next_statement;

a && b || c && d || e && f;

next_statement;

a || b && c || d && e || f;

next_statement;

(1 || 2) && 3;

next_statement;

a.x.y && a + [] - c++ || a(5,6) * "c" - 5 .x();

next_statement;
