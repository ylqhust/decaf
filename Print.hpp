#ifndef _PRINT_H_
#define _PRINT_H_
#include "std.hpp"
#include "SymbolTable.hpp"

class Print
{
public:
    static void printGST(shared_ptr<GlobalSymbolTable> gst)
    {
        for(auto pos=gst->getCST().begin();pos!=gst->getCST().end();++pos)
            printCST(*pos,0);
    }
private:
    static void Format(string const &s1,unsigned int row,string const &s2,unsigned int depth)
    {
        string space(depth*2,' ');
        cout<<space<<s1<<"["<<row<<"] : "<<s2<<endl;
    }
    static void printCST(shared_ptr<ClassSymbolTable> cst,unsigned int depth)
    {
        Format("类符号",cst->loc.row,cst->name,depth);
        for(auto vst=cst->vvst.begin();vst!=cst->vvst.end();++vst)
            printVST(*vst,depth+1);
        for(auto fst=cst->vfst.begin();fst!=cst->vfst.end();++fst)
            printFST(*fst,depth+1);
    }
    static void printVST(shared_ptr<VariableSymbolTable> vst,unsigned int depth)
    {
        Format("变量符号",vst->loc.row,vst->type->symbolTypeToString()+" "+vst->name,depth);
    }
    static void printFST(shared_ptr<FunctionSymbolTable> fst,unsigned int depth)
    {
        stringstream ss;
        ss<<(fst->isStatic?"static ":"")<<fst->returnType->symbolTypeToString()<<" "<<fst->name;
        Format("函数符号",fst->loc.row,ss.str(),depth);
        Format("形参符号",fst->loc.row,"",depth+1);
        for(auto formal=fst->vvst.begin();formal!=fst->vvst.end();++formal)
            printVST(*formal,depth+2);
        printLST(fst->localST,depth+2);
    }
    static void printLST(shared_ptr<LocalSymbolTable> lst,unsigned int depth)
    {
        Format("局部作用域",lst->loc.row,"",depth);
        for(auto pos=lst->vsst.begin();pos!=lst->vsst.end();++pos)
            if((*pos)->stType==ST_VAR)
                printVST(dynamic_pointer_cast<VariableSymbolTable>(*pos),depth+1);
            else
                printLST(dynamic_pointer_cast<LocalSymbolTable>(*pos), depth+1);
    }
};



#endif /* end of include guard: _PRINT_H_ */
