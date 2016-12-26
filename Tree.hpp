#ifndef _TREE_H_
#define _TREE_H_
#include "std.hpp"
#include "Visitor.hpp"
#include "SymbolTable.hpp"
#include "SymbolType.hpp"
#include "lab3_define.hpp"
enum Kind{
  PROGRAM,CLASSDEF,VARIABLEDEF,VARIABLE,
  TYPE_INT,TYPE_BOOL,TYPE_STRING,TYPE_CLASS,TYPE_ARRAY,TYPE_VOID,
  FUNCTIONDEF,STMTBLOCK,ASSIGNSTMT,IDLVALUE,ARRAYLVALUE,CALL,EMPTY,
  FORSTMT,WHILESTMT,IFSTMT,RETURNSTMT,BREAK,PRINTSTMT,
  CONST_INT,CONST_STRING,CONST_BOOL,CONST_NULL,
  EXPR_THIS,READINTEGER,READLINE,NEWOBJECT,NEWARRAY,CLASSTEST,CLASSCAST,

  ONE_EXPR_BRACK,ONE_EXPR_UMINUS,ONE_EXPR_NOT,
  TWO_EXPR_ADD,TWO_EXPR_SUB,TWO_EXPR_MUL,TWO_EXPR_DIV,TWO_EXPR_MOD,TWO_EXPR_SM,TWO_EXPR_SM_EQ,TWO_EXPR_BG,TWO_EXPR_BG_EQ,TWO_EXPR_EQ_EQ,TWO_EXPR_NOT_EQ,
  TWO_EXPR_AND_AND,TWO_EXPR_OR_OR
};

typedef SymbolType ExprType;
typedef BuildInSymbolType BuildInExprType;
typedef ClassSymbolType ClassExprType;
typedef ArraySymbolType ArrayExprType;
struct ExprAttribute
{
    shared_ptr<ExprType> et;
    bool isLValue=false;//是否是左值
    bool justClassName=false;//类名调用静态函数的时候使用
    string exprTypeToString()
    {
        if(et.get()==nullptr)
        //将表达式的类型转换成 string 时，表达式的类型决不能是未知的，否则就意味着发生了错误
        SeriousErrorHandler::seriousError("Tree.h->ExprAttribute->exprTypeToString()");
        return et->symbolTypeToString();
    }
};
class BaseNode
{
public:
  Location loc;
  enum Kind kind;
  ExprAttribute ea;
  lab3::ExprAttributeLab3 ea3;
  BaseNode(Location const &loc,enum Kind k):loc(loc),kind(k){}
  virtual void display(unsigned int depth)=0;
  virtual void accept(Visitor *visitor)=0;

  void Format(string const &s1,unsigned int depth)
  {
    string s(depth,' ');
    cout<<s<<s1<<"("<<loc.row<<")"<<endl;
  }
};

class Type : public BaseNode
{
public:
  Type(Location const &loc,enum Kind k):BaseNode::BaseNode(loc,k){}
};

class BuildInType : public Type//内建类型
{
public:
  BuildInType(Location const &loc,enum Kind k):Type::Type(loc,k)
  {
      switch(kind)
      {
          case TYPE_INT:
          ea.et.reset(new BuildInExprType(BIST_INT));
          break;
          case TYPE_STRING:
          ea.et.reset(new BuildInExprType(BIST_STRING));
          break;
          case TYPE_VOID:
          ea.et.reset(new BuildInExprType(BIST_VOID));
          break;
          case TYPE_BOOL:
          ea.et.reset(new BuildInExprType(BIST_BOOL));
          break;
          default:
          SeriousErrorHandler::seriousError("Tree.h->BuildInType()");
      }
  }
  virtual void accept(Visitor *visitor)
  {
    visitor->visitBuildInType(this);
  }
  virtual void display(unsigned int depth)
  {
    switch(kind)
    {
      case TYPE_INT:
      Format("TYPE_INT",depth);
      break;
      case TYPE_BOOL:
      Format("TYPE_BOOL",depth);
      break;
      case TYPE_STRING:
      Format("TYPE_STRING",depth);
      break;
      case TYPE_VOID:
      Type::BaseNode::Format("TYPE_VOID",depth);
      break;
    }
  }
};

class ClassType : public Type
{
public:
  string idName;
public:
  ClassType(Location const &loc,string const &s):Type::Type(loc,TYPE_CLASS),idName(s){}
  virtual void accept(Visitor *visitor)
  {
    visitor->visitClassType(this);
  }
  virtual void display(unsigned int depth)
  {
    Format("TYPE_CLASS:"+idName,depth);
  }
};

class ArrayType : public Type
{
public:
  shared_ptr<BaseNode> type;
public:
  ArrayType(Location const &loc,shared_ptr<BaseNode> t):Type::Type(loc,TYPE_ARRAY),type(t)
  {}
  virtual void accept(Visitor *visitor)
  {
    visitor->visitArrayType(this);
  }
  virtual void display(unsigned int depth)
  {
    type->display(depth+1);
    Type::BaseNode::Format("TYPE_ARRAY",depth);
  }
};


class Program : public BaseNode
{
public:
    shared_ptr<GlobalSymbolTable> gst;
    shared_ptr<vector<shared_ptr<lab3::VirtualTable> > > vts;
    shared_ptr<vector<shared_ptr<lab3::Function> > > staticFuncs;

public:
    shared_ptr<vector<shared_ptr<BaseNode> > > clist;
    Program(Location const &loc,shared_ptr<vector<shared_ptr<BaseNode> > >clist):
    BaseNode::BaseNode(loc,PROGRAM),clist(clist){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitProgram(this);
    }
    virtual void display(unsigned int depth)
	{
        for(vector<shared_ptr<class BaseNode> >::iterator pos = clist->begin();pos!=clist->end();++pos)
        (*pos).get()->display(depth+1);
    }
    void setGST(shared_ptr<GlobalSymbolTable> g)
    {
        gst=g;
    }
};


class Variable : public BaseNode
{
public:
    shared_ptr<VariableSymbolTable> vst;
public:
    shared_ptr<BaseNode> type;
    string idName;
public:
    Variable(Location const &loc,shared_ptr<BaseNode> t,string const &name)
    :BaseNode::BaseNode(loc,VARIABLE),type(t),idName(name){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitVariable(this);
    }
    virtual void display(unsigned int depth)
	{
        Format("VarDef",depth);
        type->display(depth+1);
        BaseNode::Format("VarId:"+idName,depth+1);
    }
    void setVST(shared_ptr<VariableSymbolTable> v)
    {
        vst=v;
    }
};

class FunctionDef : public BaseNode
{
public:
    shared_ptr<FunctionSymbolTable> fst;
public:
    bool isStatic;
    shared_ptr<BaseNode> type;
    string idName;
    shared_ptr<vector<shared_ptr<BaseNode> > > formals;
    shared_ptr<BaseNode> stmtBlock;
public:
    FunctionDef(Location const &loc,bool isStatic,shared_ptr<BaseNode> type,string const &name,shared_ptr<vector<shared_ptr<BaseNode> > > formals,shared_ptr<BaseNode> stmtBlock)
    :BaseNode::BaseNode(loc,FUNCTIONDEF),isStatic(isStatic),type(type),idName(name),formals(formals),stmtBlock(stmtBlock){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitFunctionDef(this);
    }
    virtual void display(unsigned int depth)
	{
        Format("FuncDef",depth);
        if(isStatic)
        {
            BaseNode::Format("STATIC",depth+1);
        }
        type->display(depth+1);
        BaseNode::Format("FuncId:"+idName,depth+1);
        for(auto pos=formals->begin();pos!=formals->end();++pos)
            (*pos)->display(depth+2);
        stmtBlock->display(depth+3);
    }
    void setFST(shared_ptr<FunctionSymbolTable> f)
    {
        fst=f;
    }
};

class ClassDef : public BaseNode
{
public:
    shared_ptr<ClassSymbolTable> cst;
public:
    string idName;
    bool haveParent;
    string parentName;
    shared_ptr<vector<shared_ptr<BaseNode> > > fields;

    ClassDef(Location const &loc,string const &name,shared_ptr<vector<shared_ptr<BaseNode> > > fields)
    :BaseNode::BaseNode(loc,CLASSDEF),idName(name),haveParent(false),fields(fields){}
    ClassDef(Location const &loc,string const &name,string const &pname,shared_ptr<vector<shared_ptr<BaseNode> > > fields)
    :BaseNode::BaseNode(loc,CLASSDEF),idName(name),haveParent(true),parentName(pname),fields(fields){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitClassDef(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("ClassDef",depth);
        Format("ClassId:"+idName,depth+1);
        if(haveParent)
        {
            Format("ParentClassId:"+parentName,depth+2);
        }
        for(auto pos=fields->begin();pos!=fields->end();++pos)
            (*pos)->display(depth+2);
    }
    void setCST(shared_ptr<ClassSymbolTable> c)
    {
        cst=c;
    }
};

class StmtBlock : public BaseNode
{
public:
    shared_ptr<LocalSymbolTable> lst;
public:
    shared_ptr<vector<shared_ptr<BaseNode> > > stmts;
public:
    StmtBlock(Location const &loc,shared_ptr<vector<shared_ptr<BaseNode> > > stmts)
    :BaseNode::BaseNode(loc,STMTBLOCK),stmts(stmts){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitStmtBlock(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("StmtBlock",depth);
        for(auto pos=stmts->begin();pos!=stmts->end();++pos)
            (*pos)->display(depth+1);
    }
    void setLST(shared_ptr<LocalSymbolTable> l)
    {
        lst=l;
    }
};

class AssignStmt : public BaseNode
{
public:
    shared_ptr<BaseNode> lvalue;
    shared_ptr<BaseNode> expr;
public:
    AssignStmt(Location const &loc,shared_ptr<BaseNode> lvalue,shared_ptr<BaseNode> expr)
    :BaseNode::BaseNode(loc,ASSIGNSTMT),lvalue(lvalue),expr(expr){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitAssignStmt(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("AssignStmt",depth);
        lvalue->display(depth+1);
        this->BaseNode::Format("=",depth+1);
        expr->display(depth+1);
    }
};

class Empty : public BaseNode
{
public:
    Empty(Location const &loc):BaseNode(loc,EMPTY){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitEmpty(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("Empty",depth);
    }
};



class ForStmt : public BaseNode
{
public:
    shared_ptr<BaseNode> simpleStmt1;
    shared_ptr<BaseNode> boolExpr;
    shared_ptr<BaseNode> simpleStmt2;
    shared_ptr<BaseNode> stmt;
public:
    ForStmt(Location const &loc,shared_ptr<BaseNode> simpleStmt1,
    shared_ptr<BaseNode> boolExpr,
    shared_ptr<BaseNode> simpleStmt2,
    shared_ptr<BaseNode> stmt)
    :BaseNode::BaseNode(loc,FORSTMT),simpleStmt1(simpleStmt1),boolExpr(boolExpr),simpleStmt2(simpleStmt2),stmt(stmt)
    {
        if(this->stmt->kind!=STMTBLOCK)
        {
            //这里为了后面的分析方便，人为的将一条语句变成只包含这条语句的语句块
            shared_ptr<vector<shared_ptr<BaseNode> > > stmts(new vector<shared_ptr<BaseNode> >(1,stmt));
            this->stmt.reset(new StmtBlock(stmt->loc,stmts));
        }
    }
    virtual void accept(Visitor *visitor)
    {
        visitor->visitForStmt(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("ForStmt",depth);
        simpleStmt1->display(depth+1);
        boolExpr->display(depth+1);
        simpleStmt2->display(depth+1);
        stmt->display(depth+1);
  }
};

class WhileStmt : public BaseNode
{
public:
    shared_ptr<BaseNode> boolExpr;
    shared_ptr<BaseNode> stmt;
public:
    WhileStmt(Location const &loc,shared_ptr<BaseNode> boolExpr,
    shared_ptr<BaseNode> stmt):BaseNode(loc,WHILESTMT),boolExpr(boolExpr),stmt(stmt)
    {
        if(this->stmt->kind!=STMTBLOCK)
        {
            //这里为了后面的分析方便，人为的将一条语句变成只包含这条语句的语句块
            shared_ptr<vector<shared_ptr<BaseNode> > > stmts(new vector<shared_ptr<BaseNode> >(1,stmt));
            this->stmt.reset(new StmtBlock(stmt->loc,stmts));
        }
    }
    virtual void accept(Visitor *visitor)
    {
        visitor->visitWhileStmt(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("WhileStmt",depth);
        boolExpr->display(depth+1);
        stmt->display(depth+1);
    }
};

class IfStmt : public BaseNode
{
public:
    shared_ptr<BaseNode> boolExpr;
    shared_ptr<BaseNode> stmt1;
    bool haveElse;
    shared_ptr<BaseNode> stmt2;
public:
    IfStmt(Location const &loc,shared_ptr<BaseNode> boolExpr,shared_ptr<BaseNode> stmt1):
    BaseNode(loc,IFSTMT),boolExpr(boolExpr),stmt1(stmt1),haveElse(false)
    {
        if(this->stmt1->kind!=STMTBLOCK)
        {
            //这里为了后面的分析方便，人为的将一条语句变成只包含这条语句的语句块
            shared_ptr<vector<shared_ptr<BaseNode> > > stmts(new vector<shared_ptr<BaseNode> >(1,stmt1));
            this->stmt1.reset(new StmtBlock(stmt1->loc,stmts));
        }
    }
    IfStmt(Location const &loc,shared_ptr<BaseNode> boolExpr,shared_ptr<BaseNode> stmt1,shared_ptr<BaseNode> stmt2):
    BaseNode(loc,IFSTMT),boolExpr(boolExpr),stmt1(stmt1),haveElse(true),stmt2(stmt2)
    {
        if(stmt1->kind!=STMTBLOCK)
        {
            shared_ptr<vector<shared_ptr<BaseNode> > > stmts(new vector<shared_ptr<BaseNode> >(1,stmt1));
            this->stmt1.reset(new StmtBlock(stmt1->loc,stmts));
        }
        if(stmt2->kind!=STMTBLOCK)
        {
            shared_ptr<vector<shared_ptr<BaseNode> > > stmts(new vector<shared_ptr<BaseNode> >(1,stmt2));
            this->stmt2.reset(new StmtBlock(stmt2->loc,stmts));
        }

    }
    virtual void accept(Visitor *visitor)
    {
        visitor->visitIfStmt(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("IfStmt",depth);
        boolExpr->display(depth+1);
        stmt1->display(depth+1);
        if(haveElse)
        {
            Format("ElseStmt",depth);
            stmt2->display(depth+1);
        }
    }
};

class ReturnStmt : public BaseNode
{
public:
    bool haveExpr;
    shared_ptr<BaseNode> expr;
public:
    ReturnStmt(Location const &loc):
    BaseNode::BaseNode(loc,RETURNSTMT),haveExpr(false){}
    ReturnStmt(Location const &loc,shared_ptr<BaseNode> expr):
    BaseNode::BaseNode(loc,RETURNSTMT),haveExpr(true),expr(expr){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitReturnStmt(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("ReturnStmt",depth);
        if(haveExpr)
            expr->display(depth+1);
    }
};

class BreakStmt : public BaseNode
{
public:
    BreakStmt(Location const &loc):BaseNode::BaseNode(loc,BREAK){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitBreakStmt(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("BreakStmt",depth);
    }
};

class PrintStmt : public BaseNode
{
public:
    shared_ptr<vector<shared_ptr<BaseNode> > > exprs;
public:
    PrintStmt(Location const &loc,shared_ptr<vector<shared_ptr<BaseNode> > > exprs):
    BaseNode::BaseNode(loc,PRINTSTMT),exprs(exprs){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitPrintStmt(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("PrintStmt",depth);
        for(auto pos=exprs->begin();pos!=exprs->end();++pos)
            (*pos)->display(depth+1);
    }
};

class LValue : public BaseNode
{
public:
    LValue(Location const &loc,enum Kind k)
    :BaseNode::BaseNode(loc,k){}
};

class IDLValue : public LValue
{
public:
    bool haveExprDot;
    shared_ptr<BaseNode> exprDot;
    string idName;
public:
    IDLValue(Location const &loc,string const &name)
    :LValue::LValue(loc,IDLVALUE),haveExprDot(false),idName(name){}
    IDLValue(Location const &loc,shared_ptr<BaseNode> exprDot,string const &name)
    :LValue::LValue(loc,IDLVALUE),haveExprDot(true),exprDot(exprDot),idName(name){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitIDLValue(this);
    }
    void setError()
    {
        ea.et.reset(new BuildInExprType(BIST_ERROR));
        ea.isLValue=false;
    }
    virtual void display(unsigned int depth)
    {
        Format("LValue.",depth);
        if(haveExprDot)
        {
            exprDot->display(depth+1);
        }
        Format("Id:"+idName,depth+1);
    }
};

class ArrayLValue : public LValue
{
public:
    shared_ptr<BaseNode> expr1;
    shared_ptr<BaseNode> expr2;
public:
    ArrayLValue(Location const &loc,shared_ptr<BaseNode> expr1,shared_ptr<BaseNode> expr2)
    :LValue::LValue(loc,ARRAYLVALUE),expr1(expr1),expr2(expr2){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitArrayLValue(this);
    }
    void setError()
    {
        ea.et.reset(new BuildInExprType(BIST_ERROR));
        ea.isLValue=false;
    }
    virtual void display(unsigned int depth)
    {
        Format("LValue[]",depth);
        expr1->display(depth+1);
        expr2->display(depth+1);
    }
};



class Call : public BaseNode
{
public:
    bool haveExprDot;
    shared_ptr<BaseNode> exprDot;
    string idName;
    shared_ptr<vector<shared_ptr<BaseNode> > > actuals;
public:
    Call(Location const &loc,string const &name,shared_ptr<vector<shared_ptr<BaseNode> > > actuals)
    :BaseNode::BaseNode(loc,CALL),haveExprDot(false),idName(name),actuals(actuals){}
    Call(Location const &loc,shared_ptr<BaseNode> exprDot,string const &name,shared_ptr<vector<shared_ptr<BaseNode> > > actuals)
    :BaseNode::BaseNode(loc,CALL),haveExprDot(true),exprDot(exprDot),idName(name),actuals(actuals){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitCall(this);
    }
    void setError()
    {
        ea.et.reset(new BuildInExprType(BIST_ERROR));
        ea.isLValue=false;
    }
    virtual void display(unsigned int depth)
    {
        Format("Call",depth);
        if(haveExprDot)
            exprDot->display(depth+1);
        Format("Id:"+idName,depth+1);
        for(auto pos=actuals->begin();pos!=actuals->end();++pos)
            (*pos)->display(depth+2);
  }
};


class Constant : public BaseNode
{
public:
    Constant(Location const &loc,enum Kind k):
    BaseNode(loc,k){}
};

class IntConstant : public Constant
{
public:
    unsigned int value;
public:
    IntConstant(Location const &loc,unsigned int value):
    Constant(loc,CONST_INT),value(value)
    {
        ea.et.reset(new BuildInExprType(BIST_INT));
        ea.isLValue=false;
    }
    virtual void accept(Visitor *visitor)
    {
        visitor->visitIntConstant(this);
    }
    virtual void display(unsigned int depth)
    {
        stringstream ss;
        ss<<value;
        BaseNode::Format("IntConst:"+ss.str(),depth);
    }
};

class BoolConstant : public Constant
{
public:
    bool value;
public:
    BoolConstant(Location const &loc,bool value):
    Constant(loc,CONST_BOOL),value(value)
    {
        ea.et.reset(new BuildInExprType(BIST_BOOL));
        ea.isLValue=false;
    }
    virtual void accept(Visitor *visitor)
    {
        visitor->visitBoolConstant(this);
    }
    virtual void display(unsigned int depth)
    {
        Format(string("BoolConst:")+(value?"true":"false"),depth);
    }
};

class StringConstant : public Constant
{
public:
    string value;
public:
    StringConstant(Location const &loc,string value):
    Constant(loc,CONST_STRING),value(value)
    {
        ea.et.reset(new BuildInExprType(BIST_STRING));
        ea.isLValue=false;
    }
    virtual void accept(Visitor *visitor)
    {
        visitor->visitStringConstant(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("StrConst:"+value,depth);
    }
};

class NullConstant : public Constant
{
public:
    NullConstant(Location const &loc):
    Constant(loc,CONST_NULL)
    {
        ea.et.reset(new ClassExprType("null"));
        ea.isLValue=false;
    }
    virtual void accept(Visitor *visitor)
    {
        visitor->visitNullConstant(this);
    }
    virtual void display(unsigned int depth)
    {
        BaseNode::Format("NullConst:null",depth);
    }
};

class ThisExpr : public BaseNode
{
public:
    ThisExpr(Location const &loc):
    BaseNode(loc,EXPR_THIS){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitThisExpr(this);
    }

    virtual void display(unsigned int depth)
	{
        Format("ThisExpr",depth);
	}
};

class OneExpr : public BaseNode
{
public:
    shared_ptr<BaseNode> expr;
public:
    OneExpr(Location const &loc,enum Kind k,shared_ptr<BaseNode> expr):BaseNode(loc,k),expr(expr){}
    virtual void accept(Visitor *visitor)
    {
        visitor->visitOneExpr(this);
    }

    virtual void display(unsigned int depth)
	{
        switch(kind)
        {
            case ONE_EXPR_BRACK:
            expr->display(depth+1);
            break;
            case ONE_EXPR_UMINUS:
            Format("Uminus",depth);
            expr->display(depth+1);
            break;
            case ONE_EXPR_NOT:
            BaseNode::Format("NOT",depth);
            expr->display(depth+1);
            break;
            default:
            SeriousErrorHandler::seriousError("Tree.h->OneExpr->display()");
        }
	 }
};

class TwoExpr : public BaseNode
{
public:
    shared_ptr<BaseNode> leftExpr;
    shared_ptr<BaseNode> rightExpr;
public:
    TwoExpr(Location const &loc,enum Kind k,shared_ptr<BaseNode> leftExpr,shared_ptr<BaseNode> rightExpr):BaseNode(loc,k),leftExpr(leftExpr)
    ,rightExpr(rightExpr)
    {
        switch(kind)
        {
            case TWO_EXPR_ADD:
            case TWO_EXPR_SUB:
            case TWO_EXPR_MUL:
            case TWO_EXPR_DIV:
            case TWO_EXPR_MOD:
            ea.et.reset(new BuildInExprType(BIST_INT));
            ea.isLValue=false;
            break;
            case TWO_EXPR_SM:
            case TWO_EXPR_SM_EQ:
            case TWO_EXPR_BG:
            case TWO_EXPR_BG_EQ:
            case TWO_EXPR_EQ_EQ:
            case TWO_EXPR_NOT_EQ:
            case TWO_EXPR_AND_AND:
            case TWO_EXPR_OR_OR:
            ea.et.reset(new BuildInExprType(BIST_BOOL));
            ea.isLValue=false;
            break;
            default:
            SeriousErrorHandler::seriousError("Tree.h->TwoExpr->TwoExpr()");
        }
    }
    virtual void accept(Visitor *visitor)
    {
        visitor->visitTwoExpr(this);
    }

    virtual void display(unsigned int depth)
	{
        leftExpr->display(depth+1);
        string op;
        switch(kind)
        {
            case TWO_EXPR_ADD:
            op="+";
            break;
            case TWO_EXPR_SUB:
            op="-";
            break;
            case TWO_EXPR_MUL:
            op="*";
            break;
            case TWO_EXPR_DIV:
            op="/";
            break;
            case TWO_EXPR_MOD:
            op="%";
            break;
            case TWO_EXPR_SM:
            op="<";
            break;
            case TWO_EXPR_SM_EQ:
            op="<=";
            break;
            case TWO_EXPR_BG:
            op=">";
            break;
            case TWO_EXPR_BG_EQ:
            op=">=";
            break;
            case TWO_EXPR_EQ_EQ:
            op="==";
            break;
            case TWO_EXPR_NOT_EQ:
            op="!=";
            break;
            case TWO_EXPR_AND_AND:
            op="&&";
            break;
            case TWO_EXPR_OR_OR:
            op="||";
            break;
            default:
            SeriousErrorHandler::seriousError("Tree.h->TwoExpr->display()");
        }
        Format("Operator:"+op,depth);
        rightExpr->display(depth+1);
	}
};

class ReadInteger : public BaseNode
{
public:
    ReadInteger(Location const &loc):BaseNode(loc,READINTEGER)
    {
        ea.et.reset(new BuildInExprType(BIST_INT));
        ea.isLValue=false;
    }
    virtual void accept(Visitor *visitor)
    {
        visitor->visitReadInteger(this);
    }
    virtual void display(unsigned int depth)
	{
        Format("ReadInteger",depth);
	}
};

class ReadLine : public BaseNode
{
public:
    ReadLine(Location const &loc):BaseNode(loc,READLINE)
    {
        ea.et.reset(new BuildInExprType(BIST_STRING));
        ea.isLValue=false;
    }
    virtual void accept(Visitor *visitor)
    {
        visitor->visitReadLine(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("ReadLine",depth);
    }
};

class NewObject : public BaseNode
{
public:
    string idName;
public:
    NewObject(Location const &loc,string name):BaseNode(loc,NEWOBJECT),idName(name)
    {
        ea.et.reset(new ClassExprType(idName));
        ea.isLValue=false;
    }
    virtual void accept(Visitor *visitor)
    {
        visitor->visitNewObject(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("NewObjectExpr",depth);
        Format("ClassId:"+idName,depth);
    }
};


class NewArray : public BaseNode
{
public:
    shared_ptr<BaseNode> type;
    shared_ptr<BaseNode> expr;
public:
    NewArray(Location const &loc,shared_ptr<BaseNode> type,shared_ptr<BaseNode> expr):BaseNode(loc,NEWARRAY),type(type),expr(expr){}

    virtual void accept(Visitor *visitor)
    {
        visitor->visitNewArray(this);
    }
    virtual void display(unsigned int depth)
	{
        Format("NewArrayExpr",depth);
        type->display(depth+1);
        expr->display(depth+1);
	}
};

class ClassTest : public BaseNode
{
public:
    shared_ptr<BaseNode> expr;
    string idName;
public:
    ClassTest(Location const &loc,shared_ptr<BaseNode> expr,string name):BaseNode(loc,CLASSTEST),expr(expr),idName(name)
    {
        ea.et.reset(new BuildInExprType(BIST_BOOL));
        ea.isLValue=false;
    }
    virtual void accept(Visitor *visitor)
    {
        visitor->visitClassTest(this);
    }
    virtual void display(unsigned int depth)
    {
        Format("InstanceofExpr",depth);
        expr->display(depth+1);
        Format("ClassID:"+idName,depth+1);
    }
};

class ClassCast : public BaseNode
{
public:
    string idName;
    shared_ptr<BaseNode> expr;
public:
    ClassCast(Location const &loc,string name,shared_ptr<BaseNode> expr):BaseNode(loc,CLASSCAST),expr(expr),idName(name)
    {
        ea.et.reset(new ClassExprType(idName));
        ea.isLValue=expr->ea.isLValue;
    }
    virtual void accept(Visitor *visitor)
    {
        visitor->visitClassCast(this);
    }
    virtual void display(unsigned int depth)
	{
        Format("ClassCast",depth);
        BaseNode::Format("ClassId:"+idName,depth+1);
        expr->display(depth+1);
	}
};

struct Wrap{
  shared_ptr<BaseNode> bn;
  shared_ptr<vector<shared_ptr<BaseNode> > > vec;
};

#endif
