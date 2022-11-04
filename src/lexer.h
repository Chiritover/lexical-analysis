#ifndef _LEXER_H_
#define _LEXER_H_

#define  T_EOF  0
#define  T_WS 1
#define  T_NEWLINE 2
#define  T_INTEGER 3
#define  T_REAL 4
#define  T_STRING 5
#define  T_RESERVED 6
#define  T_IDENTIFIER 7
#define  T_OPERATOR 8
#define  T_DELIMITER 9
#define  T_COMMENTS_BEGIN 10
#define  T_COMMENTS 11
#define  T_COMMENTS_END 12

#define  E_UNTERM_STRING 10000
#define  E_UNTERM_COMMENTS 10001
#define  E_UNKNOWN_CHAR 10002

#endif
