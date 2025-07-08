#pragma once
#include <string>
#include <vector>
#include <stack>
#include "LexicalAnalyzer.h"
struct ParseError;
struct Production;
////////////////////////////////////////////////////符号表////////////////////////////////////////////////////
/********************************************************************************
* Rust 每个值都有其确切的数据类型，总的来说可以分为两类：基本类型和复合类型。 基本类型意味着它们往往是一个最小化原子类型，无法解构为其它类型（一般意义上来说），由以下组成：
* 数值类型：有符号整数 (i8, i16, i32, i64, isize)、 无符号整数 (u8, u16, u32, u64, usize) 、浮点数 (f32, f64)、以及有理数、复数
* 字符串：字符串字面量和字符串切片 &str
* 布尔类型：true 和 false
* 字符类型：表示单个 Unicode 字符，存储为 4 个字节
* 单元类型：即 () ，其唯一的值也是 ()
* https://rustwiki.org/zh-CN/book/ch03-02-data-types.html
********************************************************************************/
//符号（变量）类型
enum ValueType {
	undefined, none, _i8, _u8, _i16, _u16, _i32, _u32, _i64, _u64, _i128, _u128, _isize, _usize, _f32, _f64, _bool, _char, _unit, _array, _procedure
};
//常量/变量
enum SymbolKind {
	Constant, Variable
};
//符号（变量）表表项
struct SymbolTableEntry {
	std::string ID;									//符号（变量）名称
	enum SymbolKind kind;							//符号（变量）为常量/变量
	enum ValueType type;							//符号（变量）类型
	bool isNormal;									//是否为形参
	bool isAssigned;								//是否已赋值
	//TokenValue value;								//符号（变量）的值
	size_t addr;									//符号（变量）存储地址
};

//符号（变量）表
class SymbolTable {
public:
	std::vector<SymbolTableEntry> table;			//本层符号表，存储所有符号列表
	size_t width = 0;								//所有名字占用的总宽度
	SymbolTable* prev;								//上一层符号表
	SymbolTable(SymbolTable* prevTable);			//建立本层符号表并链接上一层符号表
	bool isExist(std::string symbolName);			//查找（仅本层）：给出名字，确定它是否在表中
	void put(const SymbolTableEntry& newSymbol);	//填入（仅本层）：填入新名字、信息（未阻止重复添加）传入的addr属性无效，自动更新
	SymbolTableEntry get(std::string symbolName);	//访问（所有层）：根据名字，访问符号表项（返回符号表中后添加的符号）
	bool update(const SymbolTableEntry& newSymbol);	//更新（所有层）：返回符号是否存在。传入的addr属性无效，移至表最后新建
	void erase(std::string symbolName);				//删除（仅本层）：删除符号表中后添加的符号（未删除全部）
	//void addwidth(const size_t wid);				//符号表表头中记录下该表中所有名字占用的总宽度
};

////////////////////////////////////////////////////函数表////////////////////////////////////////////////////
class ProcedureTableEntry;							//循环定义预先声明
//函数表
typedef std::vector<ProcedureTableEntry> ProcedureTable;
//函数表项
class ProcedureTableEntry {
public:
	std::string ID;									//函数名称
	enum ValueType returntype;						//返回类型
	ProcedureTable subProcedureTable;				//子函数函数表指针
	std::vector<ValueType> paratype;				//形参类型
	size_t addr;									//函数起始四元式地址
	//bool isRturned;
	ProcedureTableEntry(std::string ID, ValueType returntype, const std::vector<ValueType>& paratype, size_t addr);
};
//函数表

////////////////////////////////////////////////////四元式////////////////////////////////////////////////////
struct Quadruple {
	std::string op;
	std::string arg1;
	std::string arg2;
	std::string result;
};

////////////////////////////////////////////////////语义分析及中间代码生成////////////////////////////////////////////////////
//节点基类
class ASTNode {
public:
	size_t line;									//符号起始字符的行号
	size_t column;									//符号起始字符的列号
	ASTNode(size_t line, size_t column);
};

//表达式结点<表达式><加法表达式><项><因子><元素>
class ExprNode : public ASTNode {
public:
	ValueType type;									//表达式结果数据类型
	//size_t width;
	enum SymbolKind kind;							//此处不表示是常量还是变量，表示是变（常）量名还是数值
	std::variant<std::string, TokenValue> name_value;
	ExprNode(size_t line, size_t column, ValueType type/*, size_t width*/, std::string name);
	ExprNode(size_t line, size_t column, ValueType type/*, size_t width*/, TokenValue value);
};

//标识符ID
class Id : public ASTNode {
public:
	std::string name;								//标识符名字
	static int count;								//临时变量命名序号
	static std::string newtemp();					//返回一个新的临时变量名Tx，更新x（count）++
	Id(size_t line, size_t column, std::string name/*, ValueType type, size_t width*/);
};

//参数列表
class ParaListNode : public ASTNode {
public:
	std::vector<ValueType> paratype;				//所有参数类型
	ParaListNode(size_t line, size_t column, const std::vector<ValueType>& paratype);
};

//数值
class I32num : public ASTNode {
public:
	ValueType type;									//数据类型i32
	int value;										//数据值
	I32num(size_t line, size_t column, int value, ValueType type = ValueType::_i32);
};

//非终结符中间结点<变量声明内部><类型><可赋值元素>
class BridgeNode : public ASTNode {
public:
	enum SymbolKind kind;							//常量/变量
	std::string name;								//名字
	ValueType type;									//类型
	size_t width;									//字节数
	//BridgeNode(size_t line, size_t column, SymbolKind kind, std::string name, ValueType type, size_t width);
	BridgeNode(size_t line, size_t column, SymbolKind kind, std::string name);//<变量声明内部>
	BridgeNode(size_t line, size_t column, ValueType type, size_t width);//<类型>
	BridgeNode(size_t line, size_t column, std::string name);//<可赋值元素>
};

////breaklist, continuelist的表项，因错误提示的定位需求，添加行列存储
//struct BrkConListEntry {
//	size_t line;									//break/continue语句起始行号
//	size_t column;									//break/continue语句起始列号
//	size_t quad;									//break/continue四元式序号
//};
//语句<语句><语句串><语句块><if语句> //listtype：0breaklist，1continuelist
class Stmt : public ASTNode {
public:
	bool returned;									//语句内部所有出口是否均返回
	std::vector<size_t> nextlist;					//nextlist
	std::vector<size_t> breaklist;			//类似nextlist、truelist、falselist的用于回填break跳转地址的breaklist记录
	std::vector<size_t> continuelist;		//类似nextlist、truelist、falselist的用于回填break跳转地址的continuelist记录
	Stmt(size_t line, size_t column, bool returned);//语句不涉及nextlist、breaklist、continuelist时，使用该构造函数，三个list默认为空
	Stmt(size_t line, size_t column, bool returned, const std::vector<size_t>& nextlist, int listtype = 0);//listtype为0，list拷贝为nextlist；listtype为1，list拷贝为breaklist；listtype为2，list拷贝为continuelist
	Stmt(size_t line, size_t column, bool returned, const std::vector<size_t>& nextlist, const std::vector<size_t>& breaklist, const std::vector<size_t>& continuelist);//所有list均需构造时使用该构造函数，构造nextlist；breaklist；continuelist
};

//运算符
class Op : public ASTNode {
public:
	enum TokenType op;								//运算符内容
	Op(size_t line, size_t column, TokenType op);
	std::string GetOperatorString();
};

//<Q>
class Q : public ASTNode {
public:
	size_t quad;									//下一条四元式地址
	Q(size_t line, size_t column, size_t quad);
};

//<B>
class B : public ASTNode {
public:
	std::vector<size_t> truelist;					//truelist
	std::vector<size_t> falselist;					//falselist
	B(size_t line, size_t column, const std::vector<size_t>& truelist, const std::vector<size_t>& falselist);
};

//<P>
class P : public ASTNode {
public:
	std::vector<size_t> nextlist;					//nextlist
	P(size_t line, size_t column, const std::vector<size_t>& nextlist);
};

//<else部分>
class ElseStmt : public Stmt {
public:
	int firstquad;									//<else部分>第一条语句地址，没有为-1
	ElseStmt(size_t line, size_t column, bool returned, int firstquad, const std::vector<size_t>& nextlist = std::vector<size_t>());
};

//<可迭代结构>
class IterableStructure : public ASTNode {
public:
	ExprNode left;									//当<可迭代结构>为<表达式> '..' <表达式>时有效，可迭代结构的第一个<表达式>（闭区间）
	ExprNode right;									//当<可迭代结构>为<表达式> '..' <表达式>时有效，可迭代结构的第二个<表达式>（开区间）
	std::string arrayName;							//当<可迭代结构>为<元素>时有效，可迭代结构的元素名
	IterableStructure(size_t line, size_t column, ExprNode left, ExprNode right);	//当<可迭代结构>为<表达式> '..' <表达式>时使用该构造函数
	IterableStructure(size_t line, size_t column, std::string arrayName);			//当<可迭代结构>为<元素>时使用该构造函数
};

//<for变量迭代结构>
class ForIri : public ASTNode {
public:
	int firstquad;									//<for变量迭代结构>第一条语句地址
	std::vector<size_t> truelist;					//truelist
	std::vector<size_t> falselist;					//falselist
	std::string name;								//<for变量迭代结构>的<变量声明内部>变量名
	IterableStructure iri;							//<for变量迭代结构>的<可迭代结构>
	ForIri(size_t line, size_t column, int firstquad, const std::vector<size_t>& truelist, const std::vector<size_t>& falselist, const std::string& name, const IterableStructure& iri);
};



class SemanticAnalyzer
{
private:
	const size_t START_STMT_ADDR = 100;													//四元式起始地址
	std::vector<Quadruple> qList;														//产生的所有四元式
	std::stack<SymbolTable*> tblptr;													//保存各外层过程的符号表指针
	//std::stack<size_t> offset;															//存放各嵌套过程的当前相对地址
	std::stack<ProcedureTable*> proptr;													//保存各外层过程的函数表指针，顶层为当前指针所在函数的函数表地址
	ProcedureTable procedureList;														//程序默认最外层函数表
	std::stack<std::unique_ptr<ASTNode>> nodeStack;										//随语法分析过程产生的结点
	std::vector<ParseError> semanticErrors;												//归约过程中遇到的错误
	size_t nextstat;																	//输出序列中下一条四地址语句的地址索引
	void emit(Quadruple s);																//将四地址代码送到qList中，每产生一条四地址语句后，过程emit便把nextstat加1
	void enterproc(ProcedureTable* table, ProcedureTableEntry newtable);				//在指针table指示的函数表中为newtable过程建立一个新项
	std::vector<size_t> makelist(size_t quad);											//将创建一个仅含quad的新链表，其中quad是四元式数组的一个下标(标号);函数返回指向这个链的指针
	std::vector<size_t> merge(std::vector<size_t> list1, std::vector<size_t> list2);	//把以list1和list2为链首的两条链合并为一作为函数值，回送合并后的链首
	void backpatch(std::vector<size_t> list, size_t quad);								//其功能是完成“回填”，把list所链接的每个四元式的第四区段都填为quad。
	void mkleaf(size_t line, size_t column, std::string id);							//建立一个标识符结点，标号为id，一个域entry指向标识符在符号表中的入口
	void mkleaf(size_t line, size_t column, int val);									//建立一个数结点，一个域val用于存放数的值
	void mkleaf(size_t line, size_t column, TokenType op);								//建立一个叶结点，一个域op用于存放运算符的值
	void mkleaf(size_t line, size_t column);											//建立一个叶结点，无属性值，仅用于占位
	void AddSemanticError(int line, int column, const std::string& message);			//将归约过程中遇到的错误存入parseErrors
public:
	SemanticAnalyzer();																	//初始化{当前四元式地址}nextstat、{函数表栈}proptr
	//符号表、函数表相关
	SymbolTable* mktable(SymbolTable* previous);										//创建一张新符号表，并返回指向新表的一个指针
	//void enter(SymbolTable* table, const SymbolTableEntry newSymbol);					//在table指示的符号表中为名字newSymbol.ID建立一个新项，并把符号的属性填入到该项中
	//void addwidth(SymbolTable* table, const size_t width);							//指针table指示的符号表表头中记录下该表中所有名字占用的总宽度
	void analyze(const Production& prod);												//根据当前归约的产生式进行语义分析
	void mkleaf(Token token);															//根据token类型建立终结符叶结点
	void addJumpToMain();																//在所有四元式之前添加跳转至函数名为main的函数
	const std::vector<Quadruple>& GetqList() const;										//返回语义分析结果，{所有四元式}qList
	const std::vector<ParseError>& GetSemanticErrors() const;							//返回{语义分析错误}semanticErrors
};

