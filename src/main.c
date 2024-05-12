#define CLEXER_IMPL
#include "./clexer.h"
#include <stdio.h>

#define FILEPATH "./input.txt"

int main(void) {
  char *keywords[] = {
    "int",
    "def",
    "return",
    "let"
  };

  size_t keywords_len = sizeof(keywords)/sizeof(*keywords);
  char *comment = "#";

  struct lexer l = lex_file(FILEPATH, keywords, keywords_len, comment);
  lexer_dump(&l);

  lexer_free(&l);

  return 0;
}
