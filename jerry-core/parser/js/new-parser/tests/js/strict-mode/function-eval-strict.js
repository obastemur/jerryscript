/* This test contains strict mode parser tests. */

function eval(arguments) { return 1 }

function arguments(eval) { return 2 }

function arguments() { 'use strict' }
