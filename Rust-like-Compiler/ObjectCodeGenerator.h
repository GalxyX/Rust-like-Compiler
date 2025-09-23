#pragma once
#include "SemanticAnalyzer.h"
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <fstream>

//待用活跃信息
/*****************************************************************************************
 * 取值仅含三种情况：
 * (^,^)：该变量当前具有的取值在后续均不再使用，无需保存至内存。
 * (^,y)：该变量当前具有的取值在当前基本块内不再使用，在后继基本块内会被使用。
 * (i,y)：该变量当前具有的取值至少在当前基本块内会被使用。
*****************************************************************************************/
struct UseLiveInfo
{
	int use;			//待用，取-1时表示不待用（^），取正数时表示本函数第几句（从0开始）
	bool live;			//活跃，取true时表示活跃（y），false表示不活跃（^）
	UseLiveInfo();		//默认构造函数，初始化为（^, ^）
	UseLiveInfo(int use, bool live);
};

struct BasicBlock
{
	size_t startAddr;											//基本块起始地址（本函数第几句，从0开始）
	size_t endAddr;												//基本块结束（endAddr对应语句属于该基本块）地址（本函数第几句，从0开始）
	std::vector<Quadruple> quads;								//基本块内所有中间代码
	//std::vector<int> predecessors;							//转移至基本块的基本块，在所属FlowGraph中的下标
	std::unordered_set<int> successors;							//基本块转移至的基本块，在所属FlowGraph中的下标
	//待用/活跃信息相关
	std::vector<std::array<UseLiveInfo, 3>> quadsUseLives;		//每一条中间代码的待用活跃信息，下标与quads对应。0：rst，1：op1，2：op2
	std::unordered_set<std::string> liveIn;						//基本块入口处的活跃变量
	std::unordered_set<std::string> liveOut;					//基本块出口处的活跃变量
};

//单个函数的流图
class FlowGraph
{
private:
	bool calBlockUseLive(BasicBlock& block);					//求解指定的基本块的待用活跃信息表（BasicBlock.quadsUseLives、liveIn、liveOut）
	std::unordered_set<std::string> getBlockLiveOut(BasicBlock& block);	//求解指定的基本块出口之后的活跃变量（BasicBlock.liveOut）
public:
	size_t startAddr;											//起始四元式地址（整个程序中四元式地址，START_STMT_ADDR开始）
	std::vector<BasicBlock> blocks;								//本函数中所有基本块
	FlowGraph();												//默认构造函数，初始化为0地址，空基本块
	FlowGraph(size_t startAddr, std::vector<Quadruple> qList);	//根据函数的全部四元式列表，划分基本块，求解基本块之间的转移关系（BasicBlock.successors），构建流图
	void calFunctionUseLive();									//根据函数的流图，求解函数的待用活跃信息表
};

//符号表中每一变量信息
struct symbolInfo
{
	std::string ID;				//变量名
	size_t addr;				//变量在语法分析中分配的地址（此处未使用，均按i32分配）
	enum ValueType type;		//变量类型
	bool isNormal;				//是否为形参
};

//函数及符号表
struct SymProTable
{
	std::string ID;							//本函数名称
	size_t addr;							//本函数起始四元式地址（START_STMT_ADDR开始）
	SymProTable* prev;						//指向定义本函数的符号函数表指针
	std::vector<symbolInfo> symbols;		//本函数直接定义的变量
	std::vector<SymProTable*> procedures;	//本函数直接定义的函数
	SymProTable(std::string ID, size_t addr, SymProTable* prev, std::vector<symbolInfo> symbols, std::vector<SymProTable*> procedures);
};


const int AVAILABLE_REG_NUM = 20;			//可自由使用的寄存器数量
const std::string AVAILABLE_REG_NAMES[] = {	//寄存器编号与寄存器名映射，前AVAILABLE_REG_NUM个为可自由使用的寄存器，其余在目标代码生成中用于固定用途（如存储立即数）
	"$a0", "$a1", "$a2", "$a3",
	"$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
	"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
	"$t8", "$t9"
};
class ObjectCodeGenerator
{
	size_t START_STMT_ADDR;												//整个程序的四元式起始地址
	std::vector<Quadruple> qList;										//所有四元式
	SymProTable sympro;													//最外层符号表，其中定义了所有最外层函数与全局变量
	std::unordered_map<std::string, FlowGraph> flowgraphs;				//函数名与流图的映射
	std::ofstream outfile;												//目标代码输出文件句柄
	void divideBasicBlocks();											//根据各函数的起始四元式地址，划分每个函数的四元式，并根据函数的四元式生成流图，建立flowgraphs
	void calFunctionsUseLive();											//求解所有函数（流图）待用活跃信息表
	std::unordered_map<std::string, std::unordered_set<int>> Avalue;	//存储的内容为寄存器编号，即AVAILABLE_REG_NAMES的下标。集合中若含有-1，表示存储在内存中，寄存器下标从0开始
	std::array<std::unordered_set<std::string>, sizeof(AVAILABLE_REG_NAMES) / sizeof(std::string)> Rvalue;//存储的内容为变量名。//"$t8", "$t9"定义了但不使用，存储内容无效
	SymProTable* currtable;												//当前执行目标代码生成的函数符号表，供不同函数使用而无需传参
	BasicBlock* currentblock;											//当前执行目标代码生成的基本块，供不同函数使用而无需传参
	int currentquad;													//当前执行目标代码生成的基本块中语句的下标，供不同函数使用而无需传参
	inline void WriteObjectCode(const std::string& op, const std::string& data1);																					//向文件中写入一操作数的目标代码
	inline void WriteObjectCode(const std::string& op, const std::string& data1, const std::string& data2);															//向文件中写入二操作数的目标代码
	inline void WriteObjectCode(const std::string& op, const std::string& data1, const std::string& data2, const std::string& data3);								//向文件中写入三操作数的目标代码
	inline void WriteObjectCode(const std::string& op, const std::string& data1, const std::string& data2, const std::string& data3, const std::string& comment);	//向文件中写入带注释的三操作数的目标代码
	int getReg(const Quadruple& I, const std::array<UseLiveInfo, 3>& UseLives);//为目标操作数分配寄存器
	int getReadonlyReg(std::string var, int pos);						//为立即数或变量分配只读寄存器
	int findSymbolIndex(const std::string& name, const std::vector<symbolInfo>& currsymtable);//查找变量名在符号表中的下标
	void storeLiveOuts();												//在基本块结束时，存储基本块出口之后的活跃变量
	void loadVariable(const std::string& var, const std::string& regn); //从内存加载变量到寄存器
	void storeVariable(const std::string& var, const std::string& regn);//从寄存器存储变量到内存
	void ConvertStartupCode();											//生成初始代码，包括数据段与代码段定义、设置栈指针、跳转到主函数等
	void ConvertProcedureCode();										//生成当前的currtable函数的目标代码
public:
	ObjectCodeGenerator(std::string intermediateCodes, std::string symproTable);	//使用符号表与中间代码的路径初始化目标代码生成器，读取并存入qList、sympro
	void ConvertObjectCode(std::string objectFile = "rust/objectCode.txt");			//生成目标代码并存入指定路径
	~ObjectCodeGenerator();															//释放sympro指针
};

/***************************************MIPS程序结构**************************************
* https://www.cnblogs.com/thoupin/p/4018455.html
* 程序结构：数据声明+普通文本+程序编码
* （1）数据声明：数据段.data开始
*			name:		storage_type	value(s)
*			变量名:		数据类型		变量值
* （2）代码段：.text开始
* Load / Store：lw/lb/sw/sb/li	register_destination,	RAM_source
* 立即与间接寻址：
*	取地址：la $t0, var1
*	间接寻址：lw $t2, ($t0)；sw $t2, ($t0)
*	基址变址寻址：lw $t2, 4($t0)；sw $t2, -12($t0)
*	取立即数：li $t1, 5
* 算术指令集（操作数只寄存器）：
*	add $t0, $t1, $t2	# $t0 = $t1 + $t2;
*	sub、addi、addu、subu、mult、div、mfhi、mflo
*	move $t2, $t3		# $t2 = $t3
* 控制流：
*	Branches分支（if else系列）
*		b	target
*		beq、blt、ble、bgt、bge、bne	$t0,	$t1,	target
*	Jumps跳转（while, for, goto系列）
*		j	target
*		jr	$t3
*	Subroutine Calls子程序调用
*		jal	sub_label	# 函数调用："jump and link"，copy program counter (return address) to register $ra (return address register)，jump to program statement at sub_label
*		jr	$ra			# 函数返回："jump register"，jump to return address in $ra (stored by jal instruction)
*****************************************************************************************/
/****************************************MIPS寄存器***************************************
* REGISTER	NAME	USAGE
* $0		$zero	常量0(constant value 0)
* $1		$at		保留给汇编器(Reserved for assembler)
* $2-$3	$v0-$v1	函数调用返回值(values for results and expression evaluation)
* $4-$7	$a0-$a3	函数调用参数(arguments)
* $8-$15	$t0-$t7	暂时的(或随便用的)
* $16-$23	$s0-$s7	保存的(或如果用,需要SAVE/RESTORE的)(saved)
* $24-$25	$t8-$t9	暂时的(或随便用的)
* $28 	$gp		全局指针(Global Pointer)
* $29 	$sp		堆栈指针(Stack Pointer)
* $30 	$fp		帧指针(Frame Pointer)
* $31 	$ra		返回地址(return address)
*****************************************************************************************/

