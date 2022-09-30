#include<tuple>
#include<iostream>
#include<cstdio>
#include"lexer.h"

std::tuple<int, int> UpdatePosition(const yyFlexLexer& lexer) {
  static int cur_row = 1;     // self-maintained current row number
  static int cur_column = 1;  // self-maintained current column number

  auto start_row = cur_row;
  auto start_column = cur_column;
  std::string token = lexer.YYText();
  auto size = lexer.YYLeng();

  if (lexer.lineno() == start_row) {
    cur_column += size;
  } else {
    cur_row = lexer.lineno();
    auto endl_pos = token.find_last_of('\n');
    cur_column = endl_pos == std::string::npos ? size : size - endl_pos;
  }

  return {start_row, start_column};
}