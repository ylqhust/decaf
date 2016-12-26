#ifndef _ERROR_H_
#define _ERROR_H_
#include "std.hpp"

class DecafError
{
public:
  virtual string errorMsg()=0;
};
class JustLoc : public DecafError
{
    Location const &loc;
public:
    JustLoc(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行 "<<loc.column<<" 列";
        return ss.str();
    }
};

class LocAndString : public DecafError
{
    Location const &loc;
    string s;
public:
    LocAndString(Location const &loc,string const &s):loc(loc),s(s){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行 ："<<s;
        return ss.str();
    }
};

class ErrorContainer
{
  vector<shared_ptr<DecafError> > errors;
public:
  void addError(shared_ptr<DecafError> error)
  {
    errors.push_back(error);
  }
  bool noError() const
  {
    return errors.size()==0?true:false;
  }
  void printErrorAndExit()
  {
    for(auto pos=errors.begin();pos!=errors.end();++pos)
    {
        cout<<"\033[01;40;31mError: \033[0m"<<(*pos)->errorMsg()<<endl;
    }
    exit(0);
  }
  void addJustLoc(Location const &loc)
  {
      errors.push_back(shared_ptr<JustLoc>(new JustLoc(loc)));
  }
  void addLocAndString(Location const &loc,string const &s)
  {
      errors.push_back(shared_ptr<LocAndString>(new LocAndString(loc,s)));
  }
};



class NoMainClassError : public DecafError
{
public:
  virtual string errorMsg()
  {
    return "没有一个名称是Main的类";
  }
};

class NoMainFuncitonError : public DecafError
{
public:
  virtual string errorMsg()
  {
    return "必须有一个main函数，并且这个main函数必须在类Main中";
  }
};

class MainFuncReturnTypeError : public DecafError
{
    Location const &loc;
public:
    MainFuncReturnTypeError(Location const &loc):loc(loc){}
  virtual string errorMsg()
  {
      stringstream ss;
      ss<<"第"<<loc.row<<"行，main函数的返回值类型必须是void";
      return ss.str();
  }
};

class MainFuncStaticError : public DecafError
{
    Location const &loc;
public:
    MainFuncStaticError(Location const &loc):loc(loc){}
  virtual string errorMsg()
  {
      stringstream ss;
      ss<<"第"<<loc.row<<"行，main函数必须是static的";
      return ss.str();
  }
};

class MainFuncFormalError : public DecafError
{
    Location const &loc;
public:
    MainFuncFormalError(Location const &loc):loc(loc){}
  virtual string errorMsg()
  {
      stringstream ss;
      ss<<"第"<<loc.row<<"行，main函数的参数列表必须为空";
      return ss.str();
  }
};

class ClassNameConflictError : public DecafError
{
    string conflictClassName;
    Location before;
    Location after;
public:
    ClassNameConflictError(string const &conflictClassName,Location const &before,Location const &after)
    :conflictClassName(conflictClassName),before(before),after(after){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"类"<<conflictClassName<<"多次定义，在第"<<before.row<<"行定义过一次，在第"<<after.row<<"行又定义过一次";
        return ss.str();
    }
};

class FunctionNameConflictError : public DecafError
{
    const Location &loc1;
    const Location &loc2;
    const string str;
public:
    FunctionNameConflictError(const Location &loc1,const Location &loc2,const string &str):loc1(loc1),loc2(loc2),str(str){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc1.row<<" 行，第 "<<loc2.row<<" 行，函数名称冲突("<<str<<")";
        return ss.str();
    }
};

class ParentClassNotFoundError: public DecafError
{
    string className;
    string parentName;
    Location loc;
public:
    ParentClassNotFoundError(string const &className,string const &parentName,Location const &loc)
    :className(className),parentName(parentName),loc(loc){}

    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"在第"<<loc.row<<"行，类 "<<className<<" 继承了类 "<<parentName<<",但是类 "<<parentName<<" 没有定义";
        return ss.str();
    }
};

class InheritError : public DecafError
{
    string className;
    string inheritLink;
public:
    InheritError(string const &className,string const &inheritLink)
    :className(className),inheritLink(inheritLink){}
    virtual string errorMsg()
    {
        return "类 "+className+" 通过继承链\n\t"+inheritLink+"\n继承了自己";
    }
};

class VariableTypeIsVoidError : public DecafError
{
    string const &varName;
    Location const &loc;
public:
    VariableTypeIsVoidError(string const &varName,Location const &loc):varName(varName),loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"在第"<<loc.row<<"行，变量"<<varName<<"的类型是void，这是不允许的";
        return ss.str();
    }
};

class NameConflictError : public DecafError
{
    string const &name;
    Location const &before;
    Location const &after;
public:
    NameConflictError(string const &name,Location const &b,Location const &after)
    :name(name),before(b),after(after){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<name<<" 重定义，在第"<<before.row<<"行处定义过一次，在第"<<after.row<<"行处又定义过一次";
        return ss.str();
    }
};

class NotLValueError : public DecafError
{
    Location const &loc;
public:
    NotLValueError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<loc.row<<"行，"<<loc.column<<" 列的表达式不是一个左值";
        return ss.str();
    }
};

class AssignTypeError : public DecafError
{
    Location const &loc1;
    Location const &loc2;
public:
    AssignTypeError(Location const &loc1,Location const &loc2)
    :loc1(loc1),loc2(loc2){}
    virtual string errorMsg()
    {
        stringstream ss;
        if(loc1.row==loc2.row)
        ss<<"第 "<<loc1.row<<" 行处，等号两边的表达式类型不同，无法赋值";
        else
        ss<<"第 "<<loc1.row<<" 行和第 "<<loc2.row<<" 行处，等号两边的表达式类型不同，无法赋值";
        return ss.str();
    }
};

class ForStmtNotBoolExprError : public DecafError
{
    Location const &loc;
public:
    ForStmtNotBoolExprError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，for 语句要求bool表达式的类型必须是bool 型的";
        return ss.str();
    }
};

class WhileStmtNotBoolExprError : public DecafError
{
    Location const &loc;
public:
    WhileStmtNotBoolExprError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，while 语句要求bool表达式的类型必须是bool 型的";
        return ss.str();
    }
};

class IfStmtNotBoolExprError : public DecafError
{
    Location const &loc;
public:
    IfStmtNotBoolExprError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，if 语句要求bool表达式的类型必须是bool 型的";
        return ss.str();
    }
};

class FunctionReturnTypeNotSameWithReturnStmtExprTypeError : public DecafError
{
    string const &funcName;
    Location const &funcLoc;
    Location const &returnStmtLoc;
    string type1,type2;
public:
    FunctionReturnTypeNotSameWithReturnStmtExprTypeError
    (string const &funcName,Location const &funcLoc,Location const &returnStmtLoc,string type1,string type2)
    :funcName(funcName),funcLoc(funcLoc),returnStmtLoc(returnStmtLoc),type1(type1),type2(type2){}

    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<funcLoc.row<<" 行的函数 "<<funcName<<" 的返回类型是 "<<type1<<"，第 "<<returnStmtLoc.row<<" 行的返回语句的返回类型是 "<<type2<<"，两者不相同";
        return ss.str();
    }
};

class BreakStmtAppearError : public DecafError
{
    Location const &loc;
public:
    BreakStmtAppearError(Location const &loc):loc(loc) {}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行处不应该出现 break 语句";
        return ss.str();
    }
};

class PrintStmtExprError : public DecafError
{
    Location const &loc;
    string s;
public:
    PrintStmtExprError(Location const &loc,string s):loc(loc),s(s){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，Print 函数的 "<<s<<"参数的类型不是 int，string，bool，这三者之一";
        return ss.str();
    }
};

class NotClassTypeExprError : public DecafError
{
    Location const &loc;
public:
    NotClassTypeExprError(Location const &loc):loc(loc) {}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，"<<loc.column<<" 列处表达式的类型不是类类型的，不能够使用";
        return ss.str();
    }
};

class NullRefError : public DecafError
{
    Location const &loc;
public:
    NullRefError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，"<<loc.column<<" 列处表达式的类型是 null";
        return ss.str();
    }
};

class UnDefineVarError : public DecafError
{
    string const &name;
    Location const &loc;
public:
    UnDefineVarError(string const &name,Location const &loc):name(name),loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，"<<loc.column<<" 列，符号 "<<name<<" 未定义";
        return ss.str();
    }
};

class VisitOtherClassPrivateFieldError : public DecafError
{
    string const &className;
    Location const &loc;
public:
    VisitOtherClassPrivateFieldError(string const &className,Location const &loc):className(className),loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，不能在当前类访问 "<<className<<" 类中的私有成员变量";
        return ss.str();
    }
};

class NotArrayTypeError : public DecafError
{
    Location const &loc;
public:
    NotArrayTypeError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，"<<loc.column<<" 列，不能对非数组类型进行数组访问";
        return ss.str();
    }
};

class VisitArrayUseNotIntTypeError : public DecafError
{
    Location const &loc;
public:
    VisitArrayUseNotIntTypeError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，"<<loc.column<<" 列， 不能使用非整数类型作为数组下标访问数组";
        return ss.str();
    }
};

class UnDefineClassError : public DecafError
{
    string const &name;
    Location const &loc;
public:
    UnDefineClassError(string const &name,Location const &loc):name(name),loc(loc){}
    virtual string errorMsg()
    {
        stringstream  ss;
        ss<<"第 "<<loc.row<<" 行，"<<name<<"类没有定义，无法使用";
        return ss.str();
    }
};

class NewArrayUseNotIntTypeError : public DecafError
{
    Location const &loc;
public:
    NewArrayUseNotIntTypeError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，"<<loc.column<<" 列，数组空间大小必须是整数类型";
        return ss.str();
    }
};

class UseThisInStaticFuncError : public DecafError
{
    Location const &loc;
public:
    UseThisInStaticFuncError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，"<<loc.column<<" 列，不能在静态函数中使用 this";
        return ss.str();
    }
};

class FormalsActualsCountNotSameError : public DecafError
{
    string const &fucName;
    Location const &fucLoc;
    int formalsCount;
    Location const &callLoc;
    int actualsCount;
public:
    FormalsActualsCountNotSameError(string const &fucName,Location const &fucLoc,int formalsCount,
    Location const &callLoc,int actualsCount):fucName(fucName),fucLoc(fucLoc),formalsCount(formalsCount),
    callLoc(callLoc),actualsCount(actualsCount){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<fucLoc.row<<" 行，函数 "<<fucName<<" 的参数个数是 "<<formalsCount<<"，第 "<<callLoc.row<<" 行，实际参数个数是 "<<actualsCount<<"，个数不匹配";
        return ss.str();
    }
};

class FormalActualTypeNotSameError : public DecafError
{
    string const &fucName;
    Location const &fucLoc;
    Location const &callLoc;
    string s;
public:
    FormalActualTypeNotSameError(string const &fucName,Location const &fucLoc,
    Location const &callLoc,string s):fucName(fucName),fucLoc(fucLoc),callLoc(callLoc),s(s){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<fucLoc.row<<" 行，函数 "<<fucName<<" 的参数类型与第 "<<callLoc.row<<" 行处调用的处传递个参数 "<<s<<" 个参数类型不相同";
        return ss.str();
    }
};

class ArrayTypeJustSupportLengthFuncError : public DecafError
{
    Location const &loc;
    string const &fucName;
public:
    ArrayTypeJustSupportLengthFuncError(Location const &loc,string const &fucName):loc(loc),fucName(fucName){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，数组类型只不支持 "<<fucName<<" 方法";
        return ss.str();
    }
};

class ArrayLengthFuncError : public DecafError
{
    Location const &loc;
public:
    ArrayLengthFuncError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，数组类型的 length()函数必须是无参数的";
        return ss.str();
    }
};

class UnDefineFuncError : public DecafError
{
    string const &fucName;
    Location const &loc;
public:
    UnDefineFuncError(string const &fucName,Location const &loc):fucName(fucName),loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，函数 "<<fucName<<" 未定义";
        return ss.str();
    }
};

class StaticFuncCantCallNotStaticFuncError : public DecafError
{
    string const &staticFuc;
    Location const &staticFucLoc;
    string const &notStaticFuc;
    Location const &notStaticFucLoc;
public:
    StaticFuncCantCallNotStaticFuncError(string const &s1,Location const &l1,string const &s2,Location const &l2):
    staticFuc(s1),staticFucLoc(l1),notStaticFuc(s2),notStaticFucLoc(l2){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<staticFucLoc.row<<" 行，函数 "<<staticFuc<<" 是静态的，第 "<<notStaticFucLoc.row<<" 行，函数 "<<notStaticFuc<<" 是非静态的，不允许在静态函数中调用非静态函数";
        return ss.str();
    }
};

class UminusUseToNotIntTypeError : public DecafError
{
    Location const &loc;
public:
    UminusUseToNotIntTypeError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行 '-' 号只能对整数类型表达式使用";
        return ss.str();
    }
};

class NotOpUseToNotBoolTypeError : public DecafError
{
    Location const &loc;
public:
    NotOpUseToNotBoolTypeError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，'!' 号只能对 bool 表达式使用";
        return ss.str();
    }
};

class CalcNotIntTypeError : public DecafError
{
    Location const &loc;
public:
    CalcNotIntTypeError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，只能对两个整型表达式使用+-*/%";
        return ss.str();
    }
};

class CompNotIntTypeError : public DecafError
{
    Location const &loc;
public:
    CompNotIntTypeError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，只能对两个整形表达式进行比较";
        return ss.str();
    }
};

class CompNotSameTypeError : public DecafError
{
    Location const &loc;
public:
    CompNotSameTypeError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，比较操作符两边的表达式类型不匹配";
        return ss.str();
    }
};

class CompVoidTypeError : public DecafError
{
    Location const &loc;
public:
    CompVoidTypeError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，不能对 void 类型进行比较";
        return ss.str();
    }
};

class RelaNotBoolTypeError : public DecafError
{
    Location const &loc;
public:
    RelaNotBoolTypeError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，只能对 bool 类型表达式进行关系运算";
        return ss.str();
    }
};

class ClassTestUseToNotClassTypeError : public DecafError
{
    Location const &loc;
    string type;
public:
    ClassTestUseToNotClassTypeError(Location const &loc,string type):loc(loc),type(type){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，instanceof 只能用于类类型表达式,但表达式的类型是 "<<type;
        return ss.str();
    }
};

class ClassTestUseToNullError : public DecafError
{
    Location const &loc;
public:
    ClassTestUseToNullError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，instanceof 不能用于 null";
        return ss.str();
    }
};

class ClassTestCantFindTheClassError : public DecafError
{
    Location const &loc;
    string const &className;
public:
    ClassTestCantFindTheClassError(Location const &loc,string const &className):loc(loc),className(className){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，用于 instanceof 的类 "<<className<<" 没有发现";
        return ss.str();
    }
};

class ClassCastUseToNotClassTypeError : public DecafError
{
    string type;
    Location const &loc;
public:
    ClassCastUseToNotClassTypeError(string type,Location const &loc):type(type),loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，类型转换只能用于类类型的表达式";
        return ss.str();
    }
};

class ClassCastUseToNullError : public DecafError
{
    Location const &loc;
public:
    ClassCastUseToNullError(Location const &loc):loc(loc){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，不能对 null 使用类型转换";
        return ss.str();
    }
};

class ClassCastError : public DecafError
{
    Location const &loc;
    string const &p;
    string const &c;
public:
    ClassCastError(Location const &loc,string const &p,string const &c):loc(loc),p(p),c(c){}
    virtual string errorMsg()
    {
        stringstream ss;
        ss<<"第 "<<loc.row<<" 行，不能从类型 "<<c<<" 转换成类型 "<<p;
        return ss.str();
    }
};
#endif /* end of include guard: _ERROR_H_
 */
