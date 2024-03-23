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

int nisvalid_ident(char c) {
  return !(c == '_' || isalnum(c));
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
  TOKENTYPE_LPAREN = 0,
  TOKENTYPE_RPAREN,
  TOKENTYPE_LBRACKET,
  TOKENTYPE_RBRACKET,
  TOKENTYPE_LBRACE,
  TOKENTYPE_RBRACE,
  TOKENTYPE_SYM_LEN, // DO NOT USE! Used for the length of symbols.

  TOKENTYPE_EOF,
  TOKENTYPE_INTLIT,
  TOKENTYPE_STRLIT,
  TOKENTYPE_IDENT,
  TOKENTYPE_KEYWORD,
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
  case TOKENTYPE_LPAREN:
    return "LPAREN";
  case TOKENTYPE_RPAREN:
    return "RPAREN";
  case TOKENTYPE_LBRACKET:
    return "LBRACKET";
  case TOKENTYPE_RBRACKET:
    return "RBRACKET";
  case TOKENTYPE_LBRACE:
    return "LBRACE";
  case TOKENTYPE_RBRACE:
    return "RBRACE";
  case TOKENTYPE_SYM_LEN:
    assert(0 && "should not use TOKENTYPE_SYM_LEN");
    return NULL;
  case TOKENTYPE_EOF:
    return "EOF";
  case TOKENTYPE_INTLIT:
    return "INTLIT";
  case TOKENTYPE_STRLIT:
    return "STRLIT";
  case TOKENTYPE_IDENT:
    return "IDENT";
  case TOKENTYPE_KEYWORD:
    return "KEYWORD";
  default:
    assert(0 && "unknown type");
    return NULL;
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

int is_keyword(char *s, size_t len, char **keywords, size_t keywords_len)
{
  for (size_t i = 0; i < keywords_len; ++i) {
    if (strncmp(s, keywords[i], len) == 0) {
      return 1;
    }
  }
  return 0;
}

#define SYMTIDX(c)                              \
  ((c == '(') ? 0 :                             \
   (c == ')') ? 1 :                             \
   (c == '[') ? 2 :                             \
   (c == ']') ? 3 :                             \
   (c == '{') ? 4 :                             \
   (c == '}') ? 5 : -1)

Lexer lex_file(char *filepath, char **keywords)
{
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
    default: { // Idents, keywords
      size_t ident_len = consume_until(src+i, nisvalid_ident);
      TokenType type = is_keyword(src+i, ident_len, keywords, 1) ? TOKENTYPE_KEYWORD : TOKENTYPE_IDENT;
      tok = token_alloc(&lexer, src+i, ident_len, type, row, col, filepath);
      lexer_append(&lexer, tok);
      i += ident_len-1;
      col += ident_len;
    } break;
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
