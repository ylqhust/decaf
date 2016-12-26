#ifndef _VISITOR_H_
#define _VISITOR_H_
#include "std.hpp"
#include "InCompleteTree.hpp"

class Visitor
{
public:
  virtual void visitProgram(const Program * program){};
  virtual void visitVariable(const Variable * variable){};
  virtual void visitBuildInType(const BuildInType * bit){};
  virtual void visitClassType(const ClassType * ct){};
  virtual void visitArrayType(const ArrayType * at){};
  virtual void visitFunctionDef(const FunctionDef * functionDef){};
  virtual void visitClassDef(const ClassDef * classDef){};
  virtual void visitStmtBlock(const StmtBlock * stmtBlock){};
  virtual void visitAssignStmt(const AssignStmt * assignStmt){};
  virtual void visitEmpty(const Empty * empty){};
  virtual void visitIDLValue(const IDLValue * idLValue){};
  virtual void visitArrayLValue(const ArrayLValue * arrayLValue){};
  virtual void visitCall(const Call * call){};
  virtual void visitForStmt(const ForStmt * forStmt){};
  virtual void visitWhileStmt(const WhileStmt * whileStmt){};
  virtual void visitIfStmt(const IfStmt * ifStmt){};
  virtual void visitReturnStmt(const ReturnStmt * returnStmt){};
  virtual void visitBreakStmt(const BreakStmt * breakStmt){};
  virtual void visitPrintStmt(const PrintStmt * printStmt){};
  virtual void visitIntConstant(const IntConstant * ic){};
  virtual void visitBoolConstant(const BoolConstant * bc){};
  virtual void visitStringConstant(const StringConstant * sc){};
  virtual void visitNullConstant(const NullConstant * nc){};
  virtual void visitThisExpr(const ThisExpr * te){};
  virtual void visitOneExpr(const OneExpr * oe){};
  virtual void visitTwoExpr(const TwoExpr * te){};
  virtual void visitReadInteger(const ReadInteger * ri){};
  virtual void visitReadLine(const ReadLine * rl){};
  virtual void visitNewObject(const NewObject * no){};
  virtual void visitNewArray(const NewArray * na){};
  virtual void visitClassTest(const ClassTest * ct){};
  virtual void visitClassCast(const ClassCast * cc){};
};

#endif
