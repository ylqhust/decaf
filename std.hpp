#ifndef _STD_H_
#define _STD_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <set>
#include <sstream>
#include <memory>

#define _YLQ_DEBUG_
using std::vector;
using std::stack;
using std::map;
using std::pair;
using std::set;
using std::shared_ptr;
using std::dynamic_pointer_cast;
using std::make_shared;
using std::string;
using std::cout;
using std::endl;
using std::stringstream;

struct Location
{
  unsigned int row;//行号
  unsigned int column;//列号
  Location(unsigned int r,unsigned c):row(r),column(c){}
  Location(Location const &loc):row(loc.row),column(loc.column){}
  Location &operator=(Location const &loc)
  {
    row=loc.row;
    column=loc.column;
}

  bool operator<(Location const &loc)const
  {
      if(row<loc.row)
      return true;
      if(row==loc.row)
      return column<loc.column;
      return false;
  }
};

typedef struct _Info{
  int lineNum;
  int allChar;
  int currentLineChar;
}Info,*LInfo;

class SeriousErrorHandler
{
public:
    static void seriousError(string s)
    {
        //发生了严重错误，输出错误后退出
        cout<<"\033[01:40:31mSeriousError:"<<s<<"\033[0m"<<endl;
        exit(0);
    }
    static void seriousError2(string file,int line)
    {
        cout<<"\033[01:40:31mSeriousError: ["<<line<<"] "<<file<<"\033[0m"<<endl;
        exit(0);
    }
};
#define SEE SeriousErrorHandler::seriousError2(__FILE__,__LINE__)

class Trace
{
    int const line;
    string const file;
    string const extr;
public:
    static bool openTrace;
    Trace(int const line,string const &file,string const &extr=""):line(line),file(file),extr(extr)
    {
        if(openTrace)
            cout<<"进入 ["<<line<<"] "<<file<<" "<<extr<<" ***"<<endl;
    }
    ~Trace()
    {
        if(openTrace)
            cout<<"离开 ["<<line<<"] "<<file<<" "<<extr<<" ***"<<endl;
    }
};
#endif /* end of include guard: _STD_H_ */
