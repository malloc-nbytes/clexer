#define CLEXER_IMPL
#include "./clexer.h"
#include <stdio.h>

#define FILEPATH "./input.txt"

int main(void) {
  char *keywords[] = {
    "div",
    "h1",
    "h2",
    "h3",
    "HTML",
  };

  char *comments[] = {
    "//",
    "/*",
    "*/"
  };

  struct lexer l = lex_file(FILEPATH, keywords, comments);
  lexer_dump(&l);

  lexer_free(&l);

  return 0;
}
