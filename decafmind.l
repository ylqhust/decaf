%option noyywrap
%option nodefault
%{
    #include "decafmind.tab.hpp"
    #include "std.hpp"
    #define COM_LAB_1_DEBUG 0
    void poutput(std::string s);
    void ppoutput(std::string,std::string);
    void adjustInfo();
    void pperror(Info i);
    unsigned int switchKeyWord(char *keyWordName);
    unsigned int switchSy(char *symb);
    unsigned int hash(char *id);

    Info info={0,0,0};
    Location loc(1,0);
    stack<Location> idStack;//每识别出一个标识符，生成一个Location压入栈中

    typedef union {
      char buf[1024];
      unsigned int intConst;
      unsigned char boolConst;
    } YYLVAL;
    #define YYSTYPE YYLVAL
%}
Notes     \/\/.*
Space     [ \t]*
NEWLINE   \n|\r\n|\r
DInt      [0-9]+
HInt      0[xX][0-9a-fA-F]+
Integer   {HInt}|{DInt}
Symb      [\~\`\!\@\#\$\%\^\&\*\(\)\-\_\+\=\|\{\}\[\]\:\;\'\<\>\,\.\?\/ ]
String    \"([a-zA-Z0-9]|(\\\")|(\\t)|(\\n)|(\\\\)|{Symb}|(\\[^tn]))*\"
BoolCst   true|false
KeyWords  bool|break|class|else|extends|for|if|int|instanceof|new|null|return|string|static|this|void|while|Print|ReadInteger|ReadLine
Sy        "+"|"-"|"*"|"/"|"%"|"<="|">="|"<"|">"|"=="|"!="|"="|"&&"|"||"|"!"|";"|","|"."|"["|"]"|"("|")"|"{"|"}"
Identifier        [a-zA-Z_][a-zA-Z0-9_]*
Error_ID  [0-9{Sy}]+{Identifier}
%%
{NEWLINE}         {info.lineNum++;info.currentLineChar=0;info.allChar++;loc.row++;loc.column=0;}
{Space}           {adjustInfo();}
{Notes}           {adjustInfo();}
{String}          {adjustInfo();
                    strcpy(yylval.buf,yytext+1);//从yytext+1处开始拷贝是为了将字符串的第一个字符 " 去掉
                    yylval.buf[strlen(yylval.buf)-1]=0; //将最后一个字符 " 去掉
                    ppoutput("字符串常量",yytext);return STRINGCONST;}
{Integer}         {adjustInfo();yylval.intConst=atoi(yytext);ppoutput("整数常量",yytext);return INTCONST;}
{BoolCst}         {adjustInfo();yylval.boolConst=!strcmp(yytext,"true");ppoutput("布尔值常量",yytext);return BOOLCONST;}
{KeyWords}        {adjustInfo();ppoutput("关键字",yytext);return switchKeyWord(yytext);}
{Identifier}      {adjustInfo();idStack.push(Location(loc));strcpy(yylval.buf,yytext);ppoutput("标识符",yytext);return IDENTIFIER;}
{Sy}              {adjustInfo();ppoutput("操作符或者分隔符号",yytext);return switchSy(yytext);}
{Error_ID}        {adjustInfo();pperror(info);}
.                 {adjustInfo();pperror(info);}
%%

void poutput(std::string s){
    if(COM_LAB_1_DEBUG)
    std::cout<<s<<std::endl;
}

void ppoutput(std::string s,std::string ytx){
    if(COM_LAB_1_DEBUG)
    std::cout<<info.lineNum+1<<" "<<s<<" "<<ytx<<std::endl;
}

void adjustInfo(){
  info.currentLineChar+=strlen(yytext);
  info.allChar+=strlen(yytext);
  loc.column+=strlen(yytext);
}


void pperror(Info i){
  if(COM_LAB_1_DEBUG){
    printf("\033[01;40;31m词法错误出错行 %d\n\033[0m",i.lineNum+1);
  }
}


unsigned int switchKeyWord(char *keyWordName){
  switch(keyWordName[0]){
    case 'b':
      if(keyWordName[1]=='o')
        return KEY_BOOL;
      return KEY_BREAK;
    case 'c':
      return KEY_CLASS;
    case 'e':
      if(keyWordName[1]=='l')
        return KEY_ELSE;
      return KEY_EXTENDS;
    case 'f':
      return KEY_FOR;
    case 'i':
      if(keyWordName[1]=='f')
        return KEY_IF;
      if(keyWordName[2]=='s')
        return KEY_INSTANCEOF;
      return KEY_INT;
    case 'n':
      if(keyWordName[1]=='e')
        return KEY_NEW;
      return KEY_NULL;
    case 'r':
      return KEY_RETURN;
    case 's':
      if(keyWordName[2]=='r')
        return KEY_STRING;
      return KEY_STATIC;
    case 't':
      return KEY_THIS;
    case 'v':
      return KEY_VOID;
    case 'w':
      return KEY_WHILE;
    case 'P':
      return KEY_PRINT;
    case 'R':
      if(keyWordName[4]=='I')
        return KEY_READINTEGER;
      return KEY_READLINE;
    default:
      pperror(info);
      ppoutput("无法识别的关键字",keyWordName);
      return -1;
  }
}

unsigned int switchSy(char *symb){
  switch(symb[0]){
    case '+':
      return SY_ADD;
    case '-':
      return SY_SUB;
    case '*':
      return SY_MUL;
    case '/':
      return SY_DIV;
    case '%':
      return SY_MOD;
    case '&':
      return SY_AND_AND;
    case '|':
      return SY_OR_OR;
    case ';':
      return SY_SEMICOLON;
    case ',':
      return SY_COMMA;
    case '.':
      return SY_DOT;
    case '[':
      return SY_L_MID;
    case ']':
      return SY_R_MID;
    case '(':
      return SY_L_SM;
    case ')':
      return SY_R_SM;
    case '{':
      return SY_L_BG;
    case '}':
      return SY_R_BG;
    case '>':
      if(symb[1]=='=')
        return SY_BG_EQ;
      return SY_BG;
    case '<':
      if(symb[1]=='=')
        return SY_SM_EQ;
      return SY_SM;
    case '=':
      if(symb[1]=='=')
        return SY_EQ_EQ;
      return SY_EQ;
    case '!':
      if(symb[1]=='=')
        return SY_NOT_EQ;
      return SY_NOT;
    default:
        pperror(info);
        ppoutput("无法识别的符号",symb);
        return -1;
  }

}

unsigned int hash(char *id){
  int length=strlen(id);
  unsigned int result=0;
  for(int i=0;i<length;++i)
    result += (id[i]*(1<<i));
  return result;
}
