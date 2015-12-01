/* This test contains switch statement parser tests.
 * default and case are valid member names, and should be
 * interpreted as switch label elements only in proper positions.
 */

switch (x) {
  case a.default:
  default: a ? a.default : a.case
}
