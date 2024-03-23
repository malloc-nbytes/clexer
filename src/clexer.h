#ifndef CLEXER_H
#define CLEXER_H

#ifdef CLEXER_IMPL

#include <assert.h>
#include <ctype.h>
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

size_t consume_until(char *s, int (*predicate)(char))
{
  size_t i;
  for (i = 0; s[i] && !predicate(s[i]); ++i);
  return i;
}

int is_quote(char c)
{
  return c == '"';
}

int nisdigit(char c)
{
  return !isdigit(c);
}

// Code from:
//   chux - Reinstate Monica
//   https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
char *file_to_str(char *filepath) {
  FILE *f = fopen(filepath, "rb");

  if (f == NULL || fseek(f, 0, SEEK_END)) {
    return NULL;
  }

  long length = ftell(f);
  rewind(f);
  if (length == -1 || (unsigned long) length >= SIZE_MAX) {
    return NULL;
  }

  size_t ulength = (size_t)length;
  char *buffer = malloc(ulength + 1);

  if (buffer == NULL || fread(buffer, 1, ulength, f) != ulength) {
    free(buffer);
    return NULL;
  }
  buffer[ulength] = '\0';

  return buffer;
}

typedef enum TokenType {
  TOKENTYPE_LPAREN,
  TOKENTYPE_RPAREN,
  TOKENTYPE_LBRACKET,
  TOKENTYPE_RBRACKET,
  TOKENTYPE_LBRACE,
  TOKENTYPE_RBRACE,
  TOKENTYPE_SYM_LEN,

  TOKENTYPE_EOF,
  TOKENTYPE_INTLIT,
  TOKENTYPE_STRLIT,
  TOKENTYPE_IDENT,
} TokenType;

typedef struct Token {
  char *lexeme; // alloc'd
  TokenType type;
  size_t row;
  size_t col;
  char *fp;
  struct Token *next;
} Token;

typedef struct Lexer {
  Token *hd;
  Token *tl;
  size_t len;
  Arena *arena;
} Lexer;

char *tokentype_to_str(TokenType type)
{
  switch (type) {
  case TOKENTYPE_EOF:
    return "EOF";
  case TOKENTYPE_INTLIT:
    return "INTLIT";
  case TOKENTYPE_STRLIT:
    return "STRLIT";
  case TOKENTYPE_IDENT:
    return "IDENT";
  case TOKENTYPE_LPAREN:
    return "LPAREN";
  case TOKENTYPE_RPAREN:
    return "RPAREN";
  default:
    assert(0 && "unknown tokentype");
  }
  return NULL;
}

Token *token_alloc(Lexer *lexer,
                   char *start, size_t len,
                   TokenType type,
                   size_t row, size_t col, char *fp)
{
  Token *tok = (Token *)arena_alloc(lexer->arena, sizeof(Token));

  char *lexeme = (char *)arena_alloc(lexer->arena, len+1);
  (void)strncpy(lexeme, start, len);
  lexeme[len] = '\0';

  tok->lexeme = lexeme;
  tok->type = type;
  tok->row = row;
  tok->col = col;
  tok->fp = fp;
  tok->next = NULL;

  return tok;
}

void lexer_append(Lexer *lexer, Token *tok)
{
  if (!lexer->hd) {
    lexer->hd = tok;
    lexer->tl = tok;
  } else {
    lexer->tl->next = tok;
    lexer->tl = tok;
  }
  ++lexer->len;
}

Token *lexer_next(Lexer *lexer)
{
  Token *tok = lexer->hd;
  if (tok) {
    lexer->hd = tok->next;
    lexer->len--;
  }
  return tok;
}

void lexer_dump(Lexer *lexer)
{
  Token *tok;
  while ((tok = lexer_next(lexer))) {
    printf("lexeme: %s, type: %s, row: %zu, col: %zu, fp: %s\n",
           tok->lexeme, tokentype_to_str(tok->type), tok->row, tok->col, tok->fp);
  }
}

#define SYMTIDX(c)                              \
  ((c == '(') ? 4 :                             \
   (c == ')') ? 5 :                             \
   (c == '[') ? 6 :                             \
   (c == ']') ? 7 :                             \
   (c == '{') ? 8 :                             \
   (c == '}') ? 9 : -1)

Lexer lex_file(char *filepath, char **keywords)
{
  (void)keywords;

  printf("%d\n", TOKENTYPE_SYM_LEN);

  int symtbl[TOKENTYPE_SYM_LEN] = {
    TOKENTYPE_LPAREN,
    TOKENTYPE_RPAREN,
    TOKENTYPE_LBRACKET,
    TOKENTYPE_RBRACKET,
    TOKENTYPE_LBRACE,
    TOKENTYPE_RBRACE,
  };

  char *src = file_to_str(filepath);
  Lexer lexer = (Lexer) {
    .hd = NULL,
    .tl = NULL,
    .len = 0,
    .arena = arena_create(32768),
  };

  size_t i, row, col;
  for (i = 0, row = 1, col = 1; src[i]; ++i) {
    char c = src[i];
    Token *tok = NULL;

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
    case ')':
    case '[':
    case ']':
    case '{':
    case '}': {
      tok = token_alloc(&lexer, src+i, 1, symtbl[SYMTIDX(c)], row, col, filepath);
      lexer_append(&lexer, tok);
      ++col;
    } break;
    /* case '(': { */
    /*   tok = token_alloc(&lexer, src+i, 1, TOKENTYPE_LPAREN, row, col, filepath); */
    /*   lexer_append(&lexer, tok); */
    /*   ++col; */
    /* } break; */
    /* case ')': { */
    /*   tok = token_alloc(&lexer, src+i, 1, TOKENTYPE_RPAREN, row, col, filepath); */
    /*   lexer_append(&lexer, tok); */
    /*   ++col; */
    /* } break; */
    /* case '[': { */
    /*   tok = token_alloc(&lexer, src+i, 1, TOKENTYPE_LBRACKET, row, col, filepath); */
    /*   lexer_append(&lexer, tok); */
    /*   ++col; */
    /* } break; */
    /* case ']': { */
    /*   tok = token_alloc(&lexer, src+i, 1, TOKENTYPE_RBRACKET, row, col, filepath); */
    /*   lexer_append(&lexer, tok); */
    /*   ++col; */
    /* } break; */
    /* case '{': { */
    /*   tok = token_alloc(&lexer, src+i, 1, TOKENTYPE_LBRACE, row, col, filepath); */
    /*   lexer_append(&lexer, tok); */
    /*   ++col; */
    /* } break; */
    /* case '}': { */
    /*   tok = token_alloc(&lexer, src+i, 1, TOKENTYPE_RBRACE, row, col, filepath); */
    /*   lexer_append(&lexer, tok); */
    /*   ++col; */
    /* } break; */
    case '"': {
      size_t strlit_len = consume_until(src+i+1, is_quote);
      tok = token_alloc(&lexer, src+i+1, strlit_len, TOKENTYPE_STRLIT, row, col, filepath);
      lexer_append(&lexer, tok);
      i += 1+strlit_len;
      col += 1+strlit_len+1;
    } break;
    case '\'':
      assert(0 && "unimplemented");
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      size_t intlit_len = consume_until(src+i, nisdigit);
      tok = token_alloc(&lexer, src+i, intlit_len, TOKENTYPE_INTLIT, row, col, filepath);
      lexer_append(&lexer, tok);
      i += intlit_len-1;
      col += intlit_len;
    } break;
    default:
      assert(0 && "unimplemented");
      break;
    }
  }

  lexer_append(&lexer, token_alloc(&lexer, "EOF", 3, TOKENTYPE_EOF, row, col, filepath));

  free(src);
  return lexer;
}

void lexer_free(Lexer *lexer)
{
  arena_free(lexer->arena);
  lexer->len = 0;
}

#endif // CLEXER_IMPL

#endif // CLEXER_H
