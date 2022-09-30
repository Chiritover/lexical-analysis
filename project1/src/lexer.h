#ifndef _LEXER_H_
#define _LEXER_H_

enum Tokens {
  T_EOF = 0,
  T_WS,
  T_NEWLINE,
  T_INTEGER,
  T_REAL,
  T_STRING,
  T_RESERVED,
  T_IDENTIFIER,
  T_OPERATOR,
  T_DELIMITER,
  T_COMMENTS_BEGIN,
  T_COMMENTS,
  T_COMMENTS_END,
};

enum Errors {
  E_UNTERM_STRING = 10000,
  E_UNTERM_COMMENTS,
  E_UNKNOWN_CHAR,
};

#endif
