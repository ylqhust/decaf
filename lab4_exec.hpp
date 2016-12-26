#ifndef _LAB4_EXEC_H_
#define _LAB4_EXEC_H_
#include "std.hpp"
#include "Tree.hpp"
#include "lab3_define.hpp"
#include "lab4_define.hpp"
namespace lab4
{
    class GenCode
    {
        static void optFunc(shared_ptr<lab3::Function> &func)
        {
            //对函数的 tac 序列进行一些优化
          //  deleteAfterReturn(func);//删除 ret 语句后面的一些语句，直到遇到一个标号
        }

        static void genGloblCommData()
        {
            cout<<" .section .data"<<endl;
            int maxTempCount=lab3::Temp::getMaxCount();
            cout<<" .comm "<<lab3::Temp::getCommName()<<","<<4*(maxTempCount+1)<<",4"<<endl;
        }

        static void genRodataSegment()
        {
            cout<<" .section .rodata"<<endl;
        }

        static void genVirtualTable(shared_ptr<lab3::VirtualTable> vt)
        {
            cout<<" .globl\t"<<vt->className<<endl;
            cout<<" .align 4"<<endl;
            cout<<" .type\t"<<vt->className<<", @object"<<endl;
            vector<shared_ptr<lab3::Function> > funcs;
            stack<shared_ptr<lab3::VirtualTable> > vts;
            vts.push(vt);
            shared_ptr<lab3::VirtualTable> tmp=vt;
            while((tmp->parentVT).get()!=nullptr)
            {
                tmp=tmp->parentVT;
                vts.push(tmp);
            }

            while(!vts.empty())
            {
                vector<shared_ptr<lab3::Function> > &methods=vts.top()->funcs;
                for(auto method:methods)
                {
                    bool find=false;
                    for(int i=0;i<funcs.size();++i)
                    //如果在父类中找到了这个函数，那么就用子类的函数替换，和c++的虚函数机制一样
                        if(funcs[i]->funName==method->funName)
                        {
                            funcs[i]=method;
                            find=true;
                            break;
                        }
                    if(!find)//如果没有找到，那么就直接插入即可
                        funcs.push_back(method);
                }
                vts.pop();
            }
            cout<<" .size\t"<<vt->className<<","<<(funcs.size()+1)*4<<endl;//这里之所以加1，
            /**
            是因为如果一个类里面没有一个函数的话，那么这个类的虚函数表大小就是0，多个类的虚函数表大小是0的话，那么这些类在 rodata
            中的地址就有可能会是一样的，那么就无法使用 instanceof 来区分类，instanceof 的区分原理就是使用虚函数表的地址大小来区分的
            因为我这里是将父类的虚汗数表放到了子类的前面，所以父类的虚函数表的地址就比子类的虚函数表地址要小
            **/
            cout<<vt->className<<":"<<endl;
            for(auto func:funcs)
                cout<<" .long\t"<<func->getUniqueFuncId()<<endl;
            cout<<" .long\t0"<<endl;//这条语句保证每个类的虚函数表大小至少为4个字节
            cout<<endl;
        }
        static void genRodataStr(const string name,const string content)
        {
            cout<<name<<":"<<endl;
            cout<<" .string "<<"\""<<content<<"\""<<endl;
        }
        static void genCodeSegment()
        {
            cout<<" .section .text"<<endl;
        }
        static void genFuncAsmCode(shared_ptr<lab3::Function> &func)
        {
            cout<<" .globl "<<func->getUniqueFuncId()<<endl;
            cout<<" .type "<<func->getUniqueFuncId()<<",@function"<<endl;
            cout<<func->getUniqueFuncId()<<":"<<endl;
            cout<<AsmCode::beginFuncAsmCode();//函数开头的两个代码，push ebp  mov esp,ebp
            //下面就是根据函数的tac序列生成汇编代码了
            for(auto tac:func->tacList)
                cout<<AsmCode::asmCode(*tac);
            cout<<endl;
        }
    public:
        static void genCode(shared_ptr<Program> &program)
        {
            shared_ptr<vector<shared_ptr<lab3::VirtualTable> > > &vts=program->vts;
            shared_ptr<vector<shared_ptr<lab3::Function> > > &staticFuncs=program->staticFuncs;
            genGloblCommData();//将所有的中间变量用到的寄存器都放到数据段
            genRodataSegment();
            for(auto vt:(*vts))//将所有的虚函数表放到只读数据段
                genVirtualTable(vt);
            //下面将字符串常量写到rodata段中
            const map<const string,const string> &rodataStrs=lab3::Tac::getRodataStrs();
            for(auto pos=rodataStrs.begin();pos!=rodataStrs.end();++pos)
                genRodataStr(pos->second,pos->first);
            genCodeSegment();
            //生成函数的汇编代码
            for(auto vt:(*vts))
                for(auto func:vt->funcs)
                {
                    optFunc(func);
                    genFuncAsmCode(func);
                }
            for(auto func:(*staticFuncs))
            {
                optFunc(func);
                genFuncAsmCode(func);
            }
        }
    };
};

#endif /* end of include guard: _LAB4_EXEC_H_
 */
