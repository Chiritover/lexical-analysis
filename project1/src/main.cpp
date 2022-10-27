#ifndef yyFlexLexerOne
#include <FlexLexer.h>
#endif

#include <iostream>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <iomanip>
#include <filesystem>

#include "lexer.h"
namespace fs = std::filesystem;

static size_t token_count = 0;
static size_t error_count = 0;

void PrintHeadings(std::ostream &output)
{
  output << std::left;
  output << std::setw(6) << "ROW";
  output << std::setw(6) << "COW";
  output << std::setw(20) << "TYPE";
  output << "TOKEN / ERROR MESSAGE\n";
}

void PrintEndline(std::ostream &output)
{
  output << "Total:";
  output << token_count << " tokens,";
  output << error_count << "errors\n";
}

std::pair<int, int> UpdatePosition(const yyFlexLexer &lexer)
{
  static int cur_row = 1;
  static int cur_column = 1;

  auto start_row = cur_row;
  auto start_column = cur_column;
  std::string token = lexer.YYText();
  auto size = lexer.YYLeng();

  if (lexer.lineno() == start_row)
  {
    cur_column += size;
  }
  else
  {
    cur_row = lexer.lineno();
    auto end = token.find_last_of('\n');
    cur_column = end == std::string::npos ? size : size - end;
  }
  return std::make_pair(start_row, start_column);
}

std::pair<std::string, std::string> GetNormalResult(const std::string &token, const int t)
{
  switch (t)
  {
  case T_INTEGER:
    if (token.size() > 10 || std::stoull(token) > INT32_MAX)
    {
      return std::make_pair("error", "RangeError: out of range");
    }
    else
    {
      return std::make_pair("integer", "");
    }
  case T_REAL:
    return std::make_pair("real", "");
  case T_STRING:
  {
    if (token.size() > 257)
    {
      return std::make_pair("error", "ValueError: string literal is too long");
    }
    else if (token.find('\t') != std::string::npos)
    {
      return std::make_pair("error", "ValueError: invalid character found in string");
    }
    return std::make_pair("string", "");
  }
  case T_RESERVED:
    return std::make_pair("reserved keyword", "");
  case T_IDENTIFIER:
  {
    if (token.size() > 255)
    {
      return std::make_pair("error", "CompileError: identifier is too long");
    }
    return std::make_pair("identifier", "");
  }
  case T_OPERATOR:
    return std::make_pair("operator", "");
  case T_DELIMITER:
    return std::make_pair("delimiter", "");
  case T_COMMENTS_END:
    return std::make_pair("comments", "");

  case E_UNTERM_STRING:
    return std::make_pair("error", "SyntaxError: unterminated string literal");
  case E_UNTERM_COMMENTS:
    return std::make_pair("error", "SyntaxError: unterminated comments");
  case E_UNKNOWN_CHAR:
    return std::make_pair("error", "Unknown character");
  default:
    return std::make_pair("error", "UnknwonError");
  }
}

int ReadToken(yyFlexLexer &lexer, std::ostream &output)
{
  static std::string comment;
  static int comment_row_start = 1;
  static int comment_col_start = 1;

  auto t = lexer.yylex();
  std::pair<int, int> start_position = UpdatePosition(lexer);
  std::string token = lexer.YYText();
  std::pair<std::string, std::string> message;
  switch (t)
  {
  case T_EOF:
  case T_WS:
  case T_NEWLINE:
    break;

  case T_COMMENTS_BEGIN:
  {
    comment_row_start = start_position.first;
    comment_col_start = start_position.second;
  }
  case T_COMMENTS:
  {
    start_position.first = comment_row_start;
    start_position.second = comment_col_start;
    comment += token;
    break;
  }
  case E_UNTERM_COMMENTS:
  case T_COMMENTS_END:
  {
    start_position.first = comment_row_start;
    start_position.second = comment_col_start;
    token = comment + token;
    comment.clear();
  }
  default:
    message = GetNormalResult(token, t);
  }
  if (!message.first.empty())
  {
    output << std::setw(6) << start_position.first;
    output << std::setw(6) << start_position.second;
    output << std::setw(20) << message.first;
    if (!message.second.empty())
    {
      output << message.second << ": ";
      ++error_count;
    }
    else
    {
      ++token_count;
    }
    output << token << std::endl;
  }

  return t;
}

int main(int argc, char **argv)
{
  std::string output_dir("output/");
  std::istream *yyin = &std::cin;
  std::ostream *yyout = &std::cout;
  if (argc > 1)
  {
    auto input = fs::path{argv[1]};
    yyin = new std::ifstream{input.c_str()};
    std::cout << input.c_str() << std::endl;
    std::string output_path = output_dir.append(input.filename().replace_extension(".txt"));
    std::cout << output_path << std::endl;
    yyout = new std::ofstream{output_path};
  }
  auto &in = *yyin;
  auto &out = *yyout;
  auto lexer = std::make_unique<yyFlexLexer>(in, out);
  PrintHeadings(*yyout);
  while (true)
  {
    auto t = ReadToken(*lexer, *yyout);
    if (t == T_EOF || t == E_UNTERM_COMMENTS)
    {
      PrintEndline(*yyout);
      break;
    }
  }
  if (yyin && yyin != &std::cin)
  {
    reinterpret_cast<std::ifstream *>(yyin)->close();
    delete yyin;
  }
  if (yyout && yyout != &std::cout)
  {
    reinterpret_cast<std::ofstream *>(yyout)->close();
    delete yyout;
    return 0;
  }
  return 0;
}
