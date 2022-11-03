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

  if (lexer.lineno() == start_row)//如果说还在一行内
  {
    cur_column += size;
  }
  else//token进行换行
  {
    cur_row = lexer.lineno();//当前行直接利用lexer的函数
    auto end = token.find_last_of('\n');
    //如果匹配对象中不含换行符，那么cur_column直接等于token的长度，如果含换行符，只有最后一行长度
    cur_column = end == std::string::npos ? size : size - end;
  }
  return std::make_pair(start_row, start_column);//返回token的开始位置
}

std::pair<std::string, std::string> GetNormalResult(const std::string &token, const int t)
{
  switch (t)
  {
  case T_INTEGER:
    if (token.size() > 10 || std::stoull(token) > INT32_MAX)
    {//如果token的size>10,必然超出int的范围，另外情况用INT32_MAX判断
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
    {//如果string长度>255报错，由于string前后有引号，这里需要为257
      return std::make_pair("error", "ValueError: string literal is too long");
    }
    else if (token.find('\t') != std::string::npos)
    {//如果string中间含有换行符
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
  static int comment_row_start = 1;//维护comment的起始行
  static int comment_col_start = 1;

  auto t = lexer.yylex();//得到lexer的匹配类型
  std::pair<int, int> start_position = UpdatePosition(lexer);//得到token的起始位置
  std::string token = lexer.YYText();//得到token的内容
  std::pair<std::string, std::string> message;//通过调用GetNormalResult得到token信息，空白字符时为空
  switch (t)
  {
  case T_EOF:
  case T_WS:
  case T_NEWLINE://这部分处理了空白字符
    break;

  case T_COMMENTS_BEGIN://开始进入comment状态
  {
    comment_row_start = start_position.first;//此时设置comment的起始位置
    comment_col_start = start_position.second;
  }
  case T_COMMENTS:
  {//在comment状态下，每匹配一个字符都会返回comment，这时要讲comment在这里进行拼凑，同时维护comment的开始
    start_position.first = comment_row_start;
    start_position.second = comment_col_start;
    comment += token;
    break;//在这里直接break，message不会在下面进行赋值，防止comment没结束就输出
  }
  case E_UNTERM_COMMENTS:
  case T_COMMENTS_END:
  {//如果comment结束了，要将start_position变成维护的comment的起始位置用来为下面输出
    start_position.first = comment_row_start;
    start_position.second = comment_col_start;
    token = comment + token;//将保存的comment加到token中，用来下面输出
    comment.clear();//将保存的comment清零
  }
  default:
    message = GetNormalResult(token, t);
  }
  if (!message.first.empty())//如果不是空白字符
  {
    output << std::setw(6) << start_position.first;
    output << std::setw(6) << start_position.second;
    output << std::setw(20) << message.first;
    if (!message.second.empty())//如果有错误信息
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
  std::istream *yyin = &std::cin;//默认使用cin，cout
  std::ostream *yyout = &std::cout;
  if (argc > 1)//如果输入是文件处理
  {
    auto input = fs::path{argv[1]};
    yyin = new std::ifstream{input.c_str()};
    std::cout << input.c_str() << std::endl;
    std::string output_path = output_dir.append(input.filename().replace_extension(".txt"));
    std::cout << output_path << std::endl;
    yyout = new std::ofstream{output_path};
  }
  // auto &in = *yyin;
  // auto &out = *yyout;
  auto lexer = std::make_unique<yyFlexLexer>(yyin,yyout);//创建lexer对象
  PrintHeadings(*yyout);
  while (true)
  {
    auto t = ReadToken(*lexer, *yyout);
    if (t == T_EOF || t == E_UNTERM_COMMENTS)//如果读到终结字符，结束
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
