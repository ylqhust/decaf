#ifndef _SYMBOL_TABLE_H_
#define _SYMBOL_TABLE_H_
#include "std.hpp"
#include "SymbolType.hpp"
static Location l(0,0);//never used

enum ST_TYPE{
    ST_CLASS,ST_FUNCTION,ST_VAR,ST_LOCAL,ST_GLOBAL
};

class SymbolTable
{
public:
    Location const &loc;
    shared_ptr<SymbolTable> parent;
    enum ST_TYPE stType;
public:
    SymbolTable(enum ST_TYPE st):loc(l),stType(st){}
    SymbolTable(Location const &loc,enum ST_TYPE st):loc(loc),stType(st){}
    virtual void setParent(shared_ptr<SymbolTable> par)
    {
        parent=par;
    }
    virtual const shared_ptr<SymbolTable> &getCSTFromChild() const
    {
        if(parent->stType==ST_CLASS)
            return parent;
        return parent->getCSTFromChild();
    }
    virtual const shared_ptr<SymbolTable> &getFSTFromChild() const
    {
        if(parent->stType==ST_FUNCTION)
            return parent;
        return parent->getFSTFromChild();
    }
    virtual shared_ptr<SymbolTable> findVSTByNameAndLoc(string const &name,Location const &loc) const
    {
        shared_ptr<shared_ptr<SymbolTable> > sst(new shared_ptr<SymbolTable>);
        return *sst;
    }
};
class VariableSymbolTable : public SymbolTable
{
public:
  shared_ptr<SymbolType> type; //变量类型
  string const name;//变量名
public:
  VariableSymbolTable(string const &name,Location const &loc)
  :SymbolTable::SymbolTable(loc,ST_VAR),name(name){}
  void setType(shared_ptr<SymbolType> &t)
  {
    type.swap(t);
  }
};

class LocalSymbolTable : public SymbolTable
{
public:
  vector<shared_ptr<VariableSymbolTable> > vvst;//局部中定义的变量
  vector<shared_ptr<LocalSymbolTable> > vlst;//局部中嵌入的局部
  vector<shared_ptr<SymbolTable> > vsst;//按照先后顺序排列的符号表，上面两者的结合
  bool returnStmtMustCanBeExec=false;//当前局部作用域中的 return 语句是否一定能被执行，如果没有 return 语句，那就是 false，如果有
  //return 语句，还要看 return 语句的出现形式
public:
    LocalSymbolTable():SymbolTable::SymbolTable(ST_LOCAL){}
  void addVariable(shared_ptr<VariableSymbolTable> &vst)
  {
      vvst.push_back(vst);
      vsst.push_back(vst);
  }

  void addLocalST(shared_ptr<LocalSymbolTable> &lst)
  {
      vlst.push_back(lst);
      vsst.push_back(lst);
  }

  //通过变量的名字和出现的位置在符号表中查找该变量，如果在当前符号表中没有找到的话，就在父类中继续查找
  virtual shared_ptr<SymbolTable> findVSTByNameAndLoc(string const &name,Location const &loc) const
  {
      for(auto pos=vvst.begin();pos!=vvst.end();++pos)
          if((*pos)->name==name && ((*pos)->loc) < loc)
              return (*pos);
      return parent->findVSTByNameAndLoc(name,loc);
  }

};

class FunctionSymbolTable : public SymbolTable
{
public:
  bool const isStatic;
  shared_ptr<SymbolType> returnType;
  string const name;
  bool const isMain;
  vector<shared_ptr<VariableSymbolTable> > vvst;//形参符号表数组
  shared_ptr<LocalSymbolTable> localST;//函数体
public:
  FunctionSymbolTable(bool isStatic,string const &name,bool isMain,Location const &loc)
  :SymbolTable::SymbolTable(loc,ST_FUNCTION),isStatic(isStatic),name(name),isMain(isMain){}

  void setReturnType(shared_ptr<SymbolType> &rt){returnType.swap(rt);}
  void addFormal(shared_ptr<VariableSymbolTable> &vst){vvst.push_back(vst);}
  void setLocalSymbolTable(shared_ptr<LocalSymbolTable> &lst){localST.swap(lst);}

  virtual shared_ptr<SymbolTable> findVSTByNameAndLoc(string const &name,Location const &loc)const
  {
      for(auto pos=vvst.begin();pos!=vvst.end();++pos)
          if((*pos)->name==name && (*pos)->loc < loc)
              return (*pos);
      if(!isStatic)
      return parent->findVSTByNameAndLoc(name,loc);
      shared_ptr<shared_ptr<VariableSymbolTable> > ssv(new shared_ptr<VariableSymbolTable>());
      return  *ssv;
  }
};

class ClassSymbolTable : public SymbolTable
{
public:
  string const name;//类名
  bool const haveParent;//是否有父类
  bool const isMain;//是否是 类 Main
  string const parentName;//如果有父类，那么这个就是父类的名字
  shared_ptr<ClassSymbolTable> parentClass;//如果有父类，那么这个就是父类的指针
  vector<shared_ptr<VariableSymbolTable> > vvst;//变量符号表数组
  vector<shared_ptr<FunctionSymbolTable> > vfst;//函数符号表数组
public:
  ClassSymbolTable(string const &name,bool haveParent,bool isMain,string const &parentName,Location const &loc)
  :SymbolTable::SymbolTable(loc,ST_CLASS),name(name),haveParent(haveParent),isMain(isMain),parentName(parentName){}

  void addVST(shared_ptr<VariableSymbolTable> &vst){vvst.push_back(vst);}
  void addFST(shared_ptr<FunctionSymbolTable> &fst){vfst.push_back(fst);}
  shared_ptr<VariableSymbolTable> findVSTByName(string const &name) const
  {
      //首先在当前类中找
      for(auto pos=vvst.begin();pos!=vvst.end();++pos)
            if((*pos)->name==name)
                return (*pos);
    //当前类没有找到，就判断有没有父类，如果有父类，就向上在父类中找
      if(haveParent)
      return parentClass->findVSTByName(name);
    //没有父类，就返回一个空
      shared_ptr<shared_ptr<VariableSymbolTable> > ssv(new shared_ptr<VariableSymbolTable>());
      return  *ssv;
  }

  //通过函数名查找函数的符号表，如果在当前类没有找到，那么就在父类中找，如果都没有，那么就返回 一个空的指针
  shared_ptr<FunctionSymbolTable> findFSTByName(string const &name) const
  {
      for(auto pos=vfst.begin();pos!=vfst.end();++pos)
      {
          if((*pos)->name==name)
            return *pos;
      }
      if(haveParent)
      return parentClass->findFSTByName(name);
      shared_ptr<shared_ptr<FunctionSymbolTable> > sfv(new shared_ptr<FunctionSymbolTable>());
      return *sfv;
  }

  virtual shared_ptr<SymbolTable> findVSTByNameAndLoc(string const &name,Location const &loc)const
  {
      return findVSTByName(name);
  }
};

class GlobalSymbolTable : public SymbolTable
{
  vector<shared_ptr<ClassSymbolTable> > vct;//类符号表
public:
    GlobalSymbolTable():SymbolTable::SymbolTable(ST_GLOBAL){}
  void add(shared_ptr<ClassSymbolTable> &ct)
  {
      vct.push_back(ct);
  }
  const vector<shared_ptr<ClassSymbolTable> > &getCST() const
  {
      return vct;
  }

  shared_ptr<ClassSymbolTable> getCSTByName(string const &name) const
  {
      for(auto pos=vct.begin();pos!=vct.end();++pos)
        if((*pos)->name==name)
            return (*pos);
      shared_ptr<shared_ptr<ClassSymbolTable> > ssc(new shared_ptr<ClassSymbolTable>());
      return *ssc;
  }
};

#endif /* end of include guard: _SYMBOL_TABLE_H_ */
