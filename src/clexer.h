#ifndef CLEXER_H
#define CLEXER_H

#ifdef CLEXER_IMPL

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// ********** ARENA ********** //

typedef struct Arena {
  uint8_t *mem;
  size_t len;
  size_t cap;
} Arena;

uint8_t *arena_alloc(Arena *arena, size_t bytes)
{
  if (arena->len+bytes > arena->cap) {
    arena->cap *= 2;
    arena->mem = realloc(arena->mem, arena->cap);
  }
  uint8_t *mem = &arena->mem[arena->len];
  arena->len += bytes;
  return mem;
}

void arena_free(Arena *arena)
{
  free(arena->mem);
  free(arena);
}

Arena *arena_create(size_t cap)
{
  Arena *arena = malloc(sizeof(Arena));
  arena->mem = malloc(cap);
  arena->cap = cap;
  arena->len = 0;
  return arena;
}

// ********** HELPERS ********** //

char *file_to_str(const char *filepath)
{
  FILE *fp = fopen(filepath, "rb");

  char *buffer = 0;
  long len;

  if (fp) {
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buffer = malloc (len);

    if (buffer) {
      fread(buffer, 1, len, fp);
    }

    fclose(fp);
  }

  return buffer;
}

typedef enum TokenType {
  TOKENTYPE_EOF,
  TOKENTYPE_INTLIT,
  TOKENTYPE_STRLIT,
  TOKENTYPE_IDENT,
  TOKENTYPE_LPAREN,
  TOKENTYPE_RPAREN,
} TokenType;

typedef struct Token {
  char *lexeme; // alloc'd
  TokenType type;
  size_t row;
  size_t col;
  char *fp;
} Token;

typedef struct Lexer {
  Token *hd;
  Token *tl;
  size_t len;
  Arena *arena;
} Lexer;

Token *token_alloc(Lexer *lexer,
                   char *start, size_t len,
                   TokenType type,
                   size_t row, size_t col, char *fp)
{
  Token *tok = (Token *)arena_alloc(lexer->arena, sizeof(Token));

  char *lexeme = (char *)arena_alloc(lexer->arena, len+1);
  (void)strncpy(lexeme, start, len);
  lexeme[len+1] = '\0';

  tok->lexeme = lexeme;
  tok->type = type;
  tok->row = row;
  tok->col = col;
  tok->fp = fp;

  return tok;
}

Lexer lex_file(char *filepath)
{
  char *src = file_to_str(filepath);
  Lexer lexer = (Lexer) {
    .hd = NULL,
    .tl = NULL,
    .len = 0,
    .arena = arena_create(32768),
  };

  for (size_t i = 0, row = 1, col = 1; src[i]; ++i) {
    char c = src[i];
    char *lexeme = src+i;
    switch (c) {
    case '\r':
    case '\n':
      ++row;
      col = 0;
      break;
    case '\t':
    case ' ':
      ++col;
      break;
    case '(':
      break;
    case '"':
      assert(0 && "unimplemented");
      break;
    case '\'':
      assert(0 && "unimplemented");
      break;
    default:
      assert(0 && "unimplemented");
      break;
    }
  }

  free(src);
  return lexer;
}

#endif // CLEXER_IMPL

#endif // CLEXER_H
