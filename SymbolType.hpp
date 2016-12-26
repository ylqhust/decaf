#ifndef _SYMBOL_TYPE_H_
#define _SYMBOL_TYPE_H_
#include "std.hpp"

enum BIST
{
  BIST_INT,BIST_STRING,BIST_BOOL,BIST_VOID,BIST_ERROR
};

class SymbolType
{
public:
  virtual bool isBuildInType()const{return false;}
  virtual bool isClassType()const{return false;}
  virtual bool isArrayType()const{return false;}
  virtual bool isVoidType()const{return false;}
  virtual bool isIntType()const{return false;}
  virtual bool isStringType()const{return false;}
  virtual bool isBoolType()const{return false;}
  virtual bool isErrorType()const{return false;}
  virtual bool compatible(shared_ptr<SymbolType> st)const=0;//类型是否兼容
  virtual string symbolTypeToString()const=0;
};


class BuildInSymbolType : public SymbolType
{
public:
  enum BIST bist;
  BuildInSymbolType(enum BIST bist):bist(bist){}
  virtual bool isBuildInType(){return true;}

  virtual string symbolTypeToString()const
  {
      switch(bist)
      {
          case BIST_INT:
          return "int";
          case BIST_STRING:
          return "string";
          case BIST_VOID:
          return "void";
          case BIST_BOOL:
          return "bool";
          case BIST_ERROR:
          return "error";
      }
      SeriousErrorHandler::seriousError("SymbolType.h->BuildInSymbolType->symbolTypeToString()");
  }
  virtual bool isVoidType()const{return bist==BIST_VOID;}
  virtual bool isIntType()const{return bist==BIST_INT;}
  virtual bool isStringType()const{return bist==BIST_STRING;}
  virtual bool isBoolType()const{return bist==BIST_BOOL;}
  virtual bool isErrorType()const{return bist==BIST_ERROR;}
  virtual bool isBuildInType()const{return true;}

  virtual bool compatible(shared_ptr<SymbolType> st)const
  {
      if(!st->isBuildInType())
      return false;
      shared_ptr<BuildInSymbolType> tmp=dynamic_pointer_cast<BuildInSymbolType>(st);
      if(bist==BIST_ERROR || tmp->bist==BIST_ERROR)
      return true;
      return bist==tmp->bist;
  }
};

class ClassSymbolType : public SymbolType
{
public:
  string const name;
  ClassSymbolType(string const &name):name(name){}
  virtual bool isClassType()const
  {
      return true;
  }
  virtual string symbolTypeToString()const
  {
      return "class@"+name;
  }
  virtual bool compatible(shared_ptr<SymbolType> st)const
  {
      if(st->isBuildInType() && dynamic_pointer_cast<BuildInSymbolType>(st)->bist==BIST_ERROR)
      return true;
      if(st->isClassType())
      {
         shared_ptr<ClassSymbolType> cst=dynamic_pointer_cast<ClassSymbolType>(st);
         if(cst->name=="null")
         return true;

      }
      return false;
  }
};

class ArraySymbolType : public SymbolType
{
public:
  shared_ptr<SymbolType> type;
  virtual bool isArrayType()const
  {
      return true;
  }
  virtual string symbolTypeToString()const
  {
      return type->symbolTypeToString()+"[]";
  }
  virtual bool compatible(shared_ptr<SymbolType> st)const
  {
      //不要使用这个函数，是错的，要改
      return false;
  }

};

#endif /* end of include guard: _SYMBOL_TYPE__ */
