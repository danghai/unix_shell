CC=/usr/bin/cc
HEADERDIR=.
AWKLIB=libAWK.a
MATHLIB=-lm

$CC -I$HEADERDIR $FLAGS -o uniparse unipen.c $AWKLIB $MATHLIB
