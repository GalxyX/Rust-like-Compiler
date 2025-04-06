#pragma once
#include <iostream>
#include <variant>
#include <vector>
class InputBuffer
{
private:
	// const static int RUST_MAX_IDENTIFIER_LENGTH// 未找到rust最大标识符长度限制信息
	std::string source;				//完整源代码
	std::string clean_code;			//删除注释后代码
	std::vector<size_t>line_breaks;	//删除注释后代码中换行下标
	unsigned int index;				//扫描器当前指针，指向下一个将要读取位置
public:
	InputBuffer(const std::string path);
	void filter_comments();
	char GetChar();
	char Retract();
	bool isEnd();
	void FindOriPos(unsigned int& line, unsigned int& column);
};
/********************************************************************************
* Rust 每个值都有其确切的数据类型，总的来说可以分为两类：基本类型和复合类型。 基本类型意味着它们往往是一个最小化原子类型，无法解构为其它类型（一般意义上来说），由以下组成：
* 数值类型：有符号整数 (i8, i16, i32, i64, isize)、 无符号整数 (u8, u16, u32, u64, usize) 、浮点数 (f32, f64)、以及有理数、复数
* 字符串：字符串字面量和字符串切片 &str
* 布尔类型：true 和 false
* 字符类型：表示单个 Unicode 字符，存储为 4 个字节
* 单元类型：即 () ，其唯一的值也是 ()
* https://rustwiki.org/zh-CN/book/ch03-02-data-types.html
********************************************************************************/
enum SymbolType { _i8, _u8, _i16, _u16, _i32, _u32, _i64, _u64, _i128, _u128, _isize, _usize, _f32, _f64, _bool, _char, _unit, _array };
struct SymbolTableEntry {
	size_t No;
	std::string ID;
	void* Addr;
	enum SymbolType type;
};

/********************************************************************************
!	ident!(...), ident!{...}, ident![...]	宏展开
!	!expr	按位非或逻辑非	Not
!=	var != expr	不等比较	PartialEq
%	expr % expr	算术取模	Rem
%=	var %= expr	算术取模与赋值	RemAssign
&	&expr, &mut expr	借用
&	&type, &mut type, &'a type, &'a mut type	借用指针类型
&	expr & expr	按位与	BitAnd
&=	var &= expr	按位与及赋值	BitAndAssign
&&	expr && expr	逻辑与
*	expr * expr	算术乘法	Mul
*=	var *= expr	算术乘法与赋值	MulAssign
*	*expr	解引用
*	*const type, *mut type	裸指针
+	trait + trait, 'a + trait	复合类型限制
+	expr + expr	算术加法	Add
+=	var += expr	算术加法与赋值	AddAssign
,	expr, expr	参数以及元素分隔符
-	- expr	算术取负	Neg
-	expr - expr	算术减法	Sub
-=	var -= expr	算术减法与赋值	SubAssign
->	fn(...) -> type, |...| -> type	函数与闭包，返回类型
.	expr.ident	成员访问
..	.., expr.., ..expr, expr..expr	右排除范围
..	..expr	结构体更新语法
..	variant(x, ..), struct_type { x, .. }	“与剩余部分”的模式绑定
...	expr...expr	模式: 范围包含模式
/	expr / expr	算术除法	Div
/=	var /= expr	算术除法与赋值	DivAssign
:	pat: type, ident: type	约束
:	ident: expr	结构体字段初始化
:	'a: loop {...}	循环标志
;	expr;	语句和语句结束符
;	[...; len]	固定大小数组语法的部分
<<	expr << expr	左移	Shl
<<=	var <<= expr	左移与赋值	ShlAssign
<	expr < expr	小于比较	PartialOrd
<=	expr <= expr	小于等于比较	PartialOrd
=	var = expr, ident = type	赋值/等值
==	expr == expr	等于比较	PartialEq
=>	pat => expr	匹配准备语法的部分
>	expr > expr	大于比较	PartialOrd
>=	expr >= expr	大于等于比较	PartialOrd
>>	expr >> expr	右移	Shr
>>=	var >>= expr	右移与赋值	ShrAssign
@	ident @ pat	模式绑定
^	expr ^ expr	按位异或	BitXor
^=	var ^= expr	按位异或与赋值	BitXorAssign
|	pat | pat	模式选择
|	expr | expr	按位或	BitOr
|=	var |= expr	按位或与赋值	BitOrAssign
||	expr || expr	逻辑或
?	expr?	错误传播
********************************************************************************/
//enum TokenType {
//	None,//均不匹配返回值
//	I8, U8, I16, U16, I32, U32, I64, U64, I128, U128, ISIZE, USIZE, F32, F64, BOOL, CHAR, UNIT, ARRAY, LET, IF, ELSE, WHILE, RETURN, MUT, FN, FOR, IN, LOOP, BREAK, CONTINUE,//关键字reservedWord
//	Identifier,//标识符Identifier
//	/*i8, u8, i16, u16, */i32, /*u32, i64, u64, i128, u128, isize, usize, f32, f64, bool_, char_, unit_, array_,*/ //常数类型Constant
//	//运算符与界符Operator, Delimiter
//	Assignment,//赋值号： =
//	Addition, Subtraction, Multiplication, Division, Equality, GreaterThan, GreaterOrEqual, LessThan, LessOrEqual, Inequality,//算符： + | -| *| / | == | > | >= | < | <= | !=
//	ParenthesisL, ParenthesisR, CurlyBraceL, CurlyBraceR, SquareBracketL, SquareBracketR,//界符：( | ) | { | } | [ | ]
//	Semicolon, Colon, Comma,//分隔符：; | : | ,
//	ArrowOperator, DotOperator, RangeOperator,//特殊符号： -> | . | ..
//	End,//结束符#（'\0'）
//	DoubleQuote, SingleQuote,//补充符号" '
//	Not, Modulo, ModuloAssign, BitAnd, BitAndAssign, LogicalAnd, BitOr, BitOrAssign, LogicalOr, ErrorPropagation,//补充符号! % %= & &= && | |= || ?
//	MultiplicationAssign, AdditionAssign, SubtractionAssign, DivisionAssign, LeftShift, LeftShiftAssign, Arrowmatch, RightShift, RightShiftAssign, PatternBinding, BitXor, BitXorAssign//补充符号*= += -= /= << <<= => >> >>= @ ^ ^=
//};
// 定义所有枚举成员的宏列表
#define TOKEN_TYPES \
    X(None) \
    /* 关键字reservedWord */ \
    X(I8) X(U8) X(I16) X(U16) X(I32) X(U32) X(I64) X(U64) X(I128) X(U128) X(ISIZE) X(USIZE) X(F32) X(F64) X(BOOL) X(CHAR) \
    X(UNIT) X(ARRAY) X(LET) X(IF) X(ELSE) X(WHILE) X(RETURN) X(MUT) X(FN) X(FOR) X(IN) X(LOOP) X(BREAK) X(CONTINUE) \
    /* 标识符Identifier */ \
    X(Identifier) \
    /* 常数类型Constant */ \
	/*X(i8) X(u8) X(i16) X(u16) */X(i32) /*X(u32) X(i64) X(u64) X(i128) X(u128) X(isize) X(usize) X(f32) X(f64) X(bool_) X(char_) X(unit_) X(array_)*/ \
    /* 运算符与界符Operator, Delimiter */ \
    X(Assignment)																																	/* 赋值号： = */ \
    X(Addition) X(Subtraction) X(Multiplication) X(Division) X(Equality) X(GreaterThan) X(GreaterOrEqual) X(LessThan) X(LessOrEqual) X(Inequality)	/* 算符： + | -| *| / | == | > | >= | < | <= | != */ \
    X(ParenthesisL) X(ParenthesisR) X(CurlyBraceL) X(CurlyBraceR) X(SquareBracketL) X(SquareBracketR)												/* 界符：( | ) | { | } | [ | ] */ \
    X(Semicolon) X(Colon) X(Comma)																													/* 分隔符：; | : | , */ \
    X(ArrowOperator) X(DotOperator) X(RangeOperator)																								/* 特殊符号： -> | . | .. */ \
    X(End)																																			/* 结束符#（'\0'） */ \
    /* 补充符号 */ \
    X(DoubleQuote) X(SingleQuote)																													/* 补充符号" ' */ \
    X(Not) X(Modulo) X(ModuloAssign) X(BitAnd) X(BitAndAssign) X(LogicalAnd) X(BitOr) X(BitOrAssign) X(LogicalOr) X(ErrorPropagation)				/* 补充符号! % %= & &= && | |= || ? */ \
    X(MultiplicationAssign) X(AdditionAssign) X(SubtractionAssign) X(DivisionAssign) X(LeftShift) X(LeftShiftAssign) X(Arrowmatch) X(RightShift) X(RightShiftAssign) X(PatternBinding) X(BitXor) X(BitXorAssign) /* 补充符号*= += -= /= << <<= => >> >>= @ ^ ^= */

enum TokenType {
#define X(name) name,
	TOKEN_TYPES
#undef X
};
using TokenValue = std::variant<std::monostate, int, unsigned int, long, unsigned long, char, float, double, std::string>;
struct Token {
	enum TokenType type;
	TokenValue value;
	Token(TokenType type, TokenValue value = 0);
};

class Scanner
{
private:
	static const std::string ReservedWordsTable[];	//与TokenType第一行内容完全匹配，应同步修改
	InputBuffer& input;								//输入缓冲区
	char ch;										//存放当前读入字符
	std::string strToken;							//存放单词的字符串
	std::vector<SymbolTableEntry> SymbolTable;		//标识符表
	std::vector<Token> tokens;						//所有单词符号
	void GetChar();									//取字符过程。取下一字符到ch ；搜索指针 + 1
	void GetBC();									//滤除空字符过程。功能：判ch = 空 ? 若是，则调用GetChar
	void Retract();									//子程序过程。功能：搜索指针回退一字符
	void Concat();									//子程序过程。功能：把ch中的字符拼入strToken
	void Clear();									//清空strToken，为下一轮识别做准备
	bool IsLetter() const;							//布尔函数。功能： ch中为字母时返回.T.
	bool IsDigit() const;							//布尔函数。功能： ch中为数字时返回.T.
	enum TokenType Reserve() const;					//整型函数。功能：按strToken中字符串查保留字表；查到返回保留字编码; 否则返回0
	size_t InsertId(/*string token, */enum SymbolType type = _i32);//函数。功能：将标识符插入符号表，返回符号表指针
	Token tokenize();								//找到下一个单词符号
	void ProcError();								//遇到无法识别字符串时错误提示
public:
	Scanner(InputBuffer& inputbuffer);
	void LexicalAnalysis();							//启动词法分析
	const std::vector<Token>& GetTokens() const;
	const std::vector<SymbolTableEntry>& GetSymbolTable() const;
};
