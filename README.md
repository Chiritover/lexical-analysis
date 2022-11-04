<h3 align='center'> 编译原理PJ1：词法分析  </h3>
<p align='center'> 20300240006 吴骁  </p>
<p align='center'> 20307130001 蔡哲飚  </p>



## 一、实验目的

1、学习flex这一在文本上做正则匹配的工具；

2、使用flex工具对于给定的PCAT语言样例做词法分析；

3、通过实验，对程序的编译过程有一个更全面立体的认识





## 二、具体实现

以下将从flex的用法、识别不同token所使用的正则表达式及其原理、判断token行列号及类型的方法、报错功能实现以及实验结果来展示词法分析的具体实现。



### 2.1 Flex的使用方法

Flex是一种可以使用正则表达式完成文本词法分析的工具，将正则表达式描述（.lex）转化为C++解析程序（.cpp）。



总体上而言，Flex词法分析由以下三部分组成：

```c++
// token定义与初始状态声明
%%
// 正则表达式与匹配规则 
%%
// 代码
```

第一部分定义了所有需要使用的的token，于lexer.h中；并对Flex做设置，声明了初始状态COMMENT。

第二部分将读到的token匹配分类。

第三部分用户代码用来调用Flex接口。



在这之后，执行make构建并运行此词法分析，其中将编译器设置为c++17。

词法分析的项目共有PCAT语言测试样例14个，从文件中读入执行 `make INPUT=<path>`，例如对第一个样例：

```Do
make INPUT="tests/case_1.pcat"
```



### 2.2 识别不同token所使用的正则表达式及其原理

声明了一些名称，用来对一些正则表达式片段以 `${name}` 的形式进行复用

```c++
WS                 ([ \t]+)
NEWLINE            (\r?\n)

DIGIT              [0-9]
INTEGER            ({DIGIT}+)
REAL               ({DIGIT}+"."{DIGIT}*)
LETTER             [A-Za-z]
STRING             (\"[^\n"]*\")
UNTERM_STRING      (\"[^\n"]*)

RESERVED           (AND|ARRAY|BEGIN|BY|DIV|DO|ELSE|ELSIF|END|EXIT|FOR|IF|IN|IS|LOOP|MOD|NOT|OF|OR|OUT|PROCEDURE|PROGRAM|READ|RECORD|RETURN|THEN|TO|TYPE|VAR|WHILE|WRITE)
IDENTIFIER         ({LETTER}({LETTER}|{DIGIT})*)
OPERATOR           (":="|"+"|"-"|"*"|"/"|"<"|"<="|">"|">="|"+"|"<>")
DELIMITER          (":"|";"|","|"."|"("|")"|"["|"]"|"{"|"}"|"[<"|">]"|"\\")

COMMENTS_BEGIN     "(*"
COMMENTS_END       "*)"
```

其中为这次所要分析的PCAT语言具有的标识符，其对应的匹配规则依次如下：

- `WS`：一个或多个空白字符用`[ \t]+` 匹配
- `NEWLINE`：换行符用`\r?\n` 匹配（对应LF (`\n`) 与 CRLF (`\r\n`)） 
- `DIGIT`：数字用`[0-9]` 匹配
- `INTEGER`：整数用`{DIGIT}+` 。由于 `INTEGER` 一定非负，因此不需要检查开头是否有负号，是否越界的判断在用户代码部分进行
- `REAL`：浮点数（实数）用`{DIGIT}+"."{DIGIT}*` 
- `LETTER`：字母用`[A-Za-z]` 匹配
- `STRING`：字符串用`\"[^\n"]*\")` 匹配，其开头和结尾均为双引号 `"` 
- `UNTERM_STRING`：未闭合的字符串，这个规则为后续错误处理服务
- `RESERVED`：保留字，原表达式表示匹配其中一个关键字。
- `IDENTIFIER`：标识符用`{LETTER}({LETTER}|{DIGIT})*` 匹配（第一个字符必须是字母）
- `OPERATOR`：运算符，原表达式表示匹配其中一个运算符。
- `DELIMITER`：分隔符，原表达式表示匹配其中一个分隔符。
- `COMMENTS_BEGIN`：（多行）注释开始符 `(*`
- `COMMENTS_END`：（多行）注释结束符 `*)`



之后利用已定义的token类型，将扫描过程中读取到的 token 进行分类

```c++
<INITIAL><<EOF>>         return T_EOF;
<INITIAL>{WS}            return T_WS;
<INITIAL>{NEWLINE}       return T_NEWLINE;

<INITIAL>{INTEGER}       return T_INTEGER;
<INITIAL>{REAL}          return T_REAL;
<INITIAL>{STRING}        return T_STRING;
<INITIAL>{UNTERM_STRING} return E_UNTERM_STRING;

<INITIAL>{RESERVED}      return T_RESERVED;
<INITIAL>{IDENTIFIER}    return T_IDENTIFIER;
<INITIAL>{OPERATOR}      return T_OPERATOR;
<INITIAL>{DELIMITER}     return T_DELIMITER;

<INITIAL>{COMMENTS_BEGIN} BEGIN(COMMENT); return T_COMMENTS_BEGIN;
<COMMENT>{COMMENTS_END}   BEGIN(INITIAL); return T_COMMENTS_END;
<COMMENT>(.|{NEWLINE})    return T_COMMENTS;
<COMMENT><<EOF>>          return E_UNTERM_COMMENTS;

<INITIAL>.                return E_UNKNOWN_CHAR;
```

左侧为正则表达式，右侧为匹配此式的操作。

左式的格式为 `<起始状态>正则表达式`。起始状态部分可以缺省，表示无需考虑状态，始终匹配。

其中`<INITIAL>` 是默认的起始状态，`<COMMENT>` 是声明的注释状态。正则表达式直接使用前面在定义区里定义的名称。

可以看到，当我们匹配操作时，通常我们会直接return一个 token 类型，完成分类。



### 2.3 判断token行列号及类型的方法

**判断token行列号**

需要关注的是遍历的行数和列数，用计数器进行记录。当然，如果注意到Flex自动维护行号，可重点关注列号。

我们一开始的设想是用两个计数器分别来统计当前遍历到的行数和列数；这时空白字符（空格，换行符和制表符）虽然出现，但是并无意 义，无需在程序末尾打印。

现在注意到Flex的特性，我们进一步采用如下的策略。用`cur_row`和`cur_column`维护当前的行列号，核心思想是根据如果没有换到新的一行，则将上次扫描的列号，加上匹配到的字符的长度`yyleng`，记为本次扫描的列号，如果换行了，则要考虑匹配的token中含不含有换行符，如果含有换行符，则要将cur_column设置为tokne换行符后的长度。

除了`UpdatePosition`以外，考虑到在`<COMMENT>` 状态下每个读到的字符都会返回，不能只用UpdatePosition来进行位置处理，`ReadToken`中对`<COMMENT>`状态有特别处理。具体而言，在`<COMMENT>`状态下，每匹配一个字符都会返回`<COMMENT>`，既将`<COMMENT>`进行拼凑，同时维护`<COMMENT>`的开始；如果`<COMMENT>`结束了，要将`start_position`变成维护的`<COMMENT>`的起始位置用以输出。

```c++
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
```

```c++
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
{//在comment状态下，每匹配一个字符都会返回comment，这时要将comment在这里进行拼凑，同时维护comment的开始
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
```



**判断token类型**

调用函数 `yylex()` 得到当前token的类型，再用一个 `switch` 语句得到返回token的信息

```c++
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
```

```c++
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
```



### 2.4 报错功能实现

最通俗的想法便是将各类错误分别检测出来（通过 switch），然后打印出来的思路。我认为，在这里预先做一些分类，或是将报错内容定义好，根据报错去按序访问，都是提升效率的方法。

当然最重要的是把错误类型和对应的报错实现落实，以下依次介绍。

其中，token相关的报错有：

- `SyntaxError: unterminated string literal`：要求开头和结尾均为双引号 `"` ，但实际结尾没有的报错
- `SyntaxError: unterminated comments`：要求注释闭合，但实际注释未封闭的报错
- `Unknown character`：啥都没匹配到的报错

还有，整数相关的报错有：

- `RangeError: out of range`：要求整数大小不超过$2^{31}-1$，当实际token为整数且位数超过10（或大于`INT32_MAX`）

以及，字符串相关的报错有：

- `ValueError: string literal is too long`：字符串超过255字符（带双引号超过257字符）
- `ValueError: invalid character found in string`：字符串中有`'\t'`的报错

和标识符相关的报错有：

- `CompileError: identifier is too long`：标识符超过255字符

其具体的实现可以见**2.3中已给出的`GetNormalResult`函数**



### 2.5 实验结果

分析出现的tokens并统计，得到的分析结果已存为case_1.txt至case_14.txt：

<img src="/Users/wuxiao/Library/Application Support/typora-user-images/Screen Shot 2022-11-04 at 08.19.21.png" alt="Screen Shot 2022-11-04 at 08.19.21" style="zoom:50%;" />

以case_1.txt为例，部分结果截图如下：

<img src="/Users/wuxiao/Library/Application Support/typora-user-images/Screen Shot 2022-11-04 at 08.22.01.png" alt="Screen Shot 2022-11-04 at 08.22.01" style="zoom:50%;" />

token的起始行号、列号与类型也存储在txt中，部分结果如上图



case 11中出现的各种无需语法分析的词法错误分析存储到了case_11.txt中，部分结果如下图：

<img src="/Users/wuxiao/Library/Application Support/typora-user-images/Screen Shot 2022-11-04 at 08.24.17.png" alt="Screen Shot 2022-11-04 at 08.24.17" style="zoom:50%;" />





## 三、项目完成情况

1、完成指标

（1）项目完成度 70分

- [x] 分析case1至10中出现的所有tokens并统计tokens的总数，将每个case的词法分析结果存储成txt格式

- [x] 输出每一个token的起始行号、列号与类型
- [x] 分析case11中出现的各种无需语法分析的词法错误，提供相应报错信息

（2）项目报告及展示 30分（待评判）

- [x] 撰写项目报告，并详细展现实现过程
- [ ] 与TA预约，在上机课时间向TA展示样例的词法分析结果（待周五完成）



2、成员分工

吴骁 55%：共同设计与讨论，并完成大部分代码

蔡哲飚 45%：共同设计与讨论，并完成大部分报告