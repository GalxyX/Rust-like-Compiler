#pragma once
#include <iostream>
#include <variant>
#include <vector>
#include <filesystem>
// 定义此宏以启用调试输出，注释掉此行以禁用调试输出
//#define ENABLE_NOTING_OUTPUT

#ifdef ENABLE_NOTING_OUTPUT
#define DEBUG_CERR cerr
#else
#define DEBUG_CERR if(0) cerr
#endif // ENABLE_NOTING_OUTPUT

class InputBuffer
{
private:
	// const static int RUST_MAX_IDENTIFIER_LENGTH// 未找到rust最大标识符长度限制信息
	std::string source;				//完整源代码
	std::string clean_code;			//删除注释后代码
	std::vector<size_t>line_breaks;	//删除注释后代码中换行下标
	unsigned int index;				//扫描器当前指针，指向下一个将要读取位置
public:
	InputBuffer(const std::filesystem::path& path);
	InputBuffer(const std::string& src);
	void filter_comments();
	char GetChar();
	char Retract();
	bool isEnd();
	void FindOriPos(int& line, int& column) const;
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
//	/*i8, u8, i16, u16, */i32, /*u32, i64, u64, i128, u128, isize, usize, f32, f64, bool_, char_, string_, unit_, array_,*/ //常数类型Constant
//	//运算符与界符Operator, Delimiter
//	Assignment,//赋值号： =
//	Addition, Subtraction, Multiplication, Division, Equality, GreaterThan, GreaterOrEqual, LessThan, LessOrEqual, Inequality,//算符： + | -| *| / | == | > | >= | < | <= | !=
//	ParenthesisL, ParenthesisR, CurlyBraceL, CurlyBraceR, SquareBracketL, SquareBracketR,//界符：( | ) | { | } | [ | ]
//	Semicolon, Colon, Comma,//分隔符：; | : | ,
//	ArrowOperator, DotOperator, RangeOperator,//特殊符号： -> | . | ..
//	DoubleQuote, SingleQuote,//补充符号" '
//	Not, Modulo, ModuloAssign, BitAnd, BitAndAssign, LogicalAnd, BitOr, BitOrAssign, LogicalOr, ErrorPropagation,//补充符号! % %= & &= && | |= || ?
//	MultiplicationAssign, AdditionAssign, SubtractionAssign, DivisionAssign, LeftShift, LeftShiftAssign, Arrowmatch, RightShift, RightShiftAssign, PatternBinding, BitXor, BitXorAssign,//补充符号*= += -= /= << <<= => >> >>= @ ^ ^=
//	End//结束符#（'\0'）
//};
// 定义所有枚举成员的宏列表
#define TOKEN_TYPES \
    X(None) \
    /* 关键字reservedWord */ \
    X(I8) X(U8) X(I16) X(U16) X(I32) X(U32) X(I64) X(U64) X(I128) X(U128) X(ISIZE) X(USIZE) X(F32) X(F64) X(BOOL) X(CHAR) \
    X(UNIT) X(ARRAY) X(LET) X(IF) X(ELSE) X(WHILE) X(RETURN) X(MUT) X(FN) X(FOR) X(IN) X(LOOP) X(BREAK) X(CONTINUE) \
	X(TRUE) X(FALSE) \
    /* 标识符Identifier */ \
    X(Identifier) \
    /* 常数类型Constant */ \
	/*X(i8_) X(u8_) X(i16_) X(u16_) */X(i32_) /*X(u32_) X(i64_) X(u64_) X(i128_) X(u128_) X(isize_) X(usize_) X(f32_) X(f64_) X(bool_) */X(char_) X(string_)/* X(unit_) X(array_)*/ \
    /* 运算符与界符Operator, Delimiter */ \
    X(Assignment)																																	/* 赋值号： = */ \
    X(Addition) X(Subtraction) X(Multiplication) X(Division) X(Equality) X(GreaterThan) X(GreaterOrEqual) X(LessThan) X(LessOrEqual) X(Inequality)	/* 算符： + | -| *| / | == | > | >= | < | <= | != */ \
    X(ParenthesisL) X(ParenthesisR) X(CurlyBraceL) X(CurlyBraceR) X(SquareBracketL) X(SquareBracketR)												/* 界符：( | ) | { | } | [ | ] */ \
    X(Semicolon) X(Colon) X(Comma)																													/* 分隔符：; | : | , */ \
    X(ArrowOperator) X(DotOperator) X(RangeOperator)																								/* 特殊符号： -> | . | .. */ \
    /* 补充符号 */ \
    X(DoubleQuote) X(SingleQuote)																													/* 补充符号" ' */ \
    X(Not) X(Modulo) X(ModuloAssign) X(BitAnd) X(BitAndAssign) X(LogicalAnd) X(BitOr) X(BitOrAssign) X(LogicalOr) X(ErrorPropagation)				/* 补充符号! % %= & &= && | |= || ? */ \
    X(MultiplicationAssign) X(AdditionAssign) X(SubtractionAssign) X(DivisionAssign) X(LeftShift) X(LeftShiftAssign) X(Arrowmatch) X(RightShift) X(RightShiftAssign) X(PatternBinding) X(BitXor) X(BitXorAssign) /* 补充符号*= += -= /= << <<= => >> >>= @ ^ ^= */ \
    X(End) X(EPSILON)																																/* 结束符#（'\0'） */

enum TokenType {
#define X(name) name,
	TOKEN_TYPES
#undef X
};
using TokenValue = std::variant<std::monostate, int, unsigned int, long, unsigned long, char, float, double, std::string>;
struct Token {
	enum TokenType type;
	TokenValue value;
	int line;    // 行号
	int column;  // 列号
	int length;  // 词法单元长度
	Token(TokenType type, int line = -1, int column = -1, int length = 0, TokenValue value = 0);
	Token& operator=(const Token& other);
};

const char* TokenTypeToString(enum TokenType type);// 通过枚举值获取名称
const char OutOfRangeTokenType[] = "UnknownTokenType";//超过定义的TokenType传入TokenTypeToString时返回值

class Scanner
{
private:
	static const std::string ReservedWordsTable[];	//与TokenType第一行内容完全匹配，应同步修改
	InputBuffer& input;								//输入缓冲区
	char ch;										//存放当前读入字符
	std::string strToken;							//存放单词的字符串
	std::vector<Token> tokens;						//所有单词符号
	void GetChar();									//取字符过程。取下一字符到ch ；搜索指针 + 1
	void GetBC();									//滤除空字符过程。功能：判ch = 空 ? 若是，则调用GetChar
	void Retract();									//子程序过程。功能：搜索指针回退一字符
	void Concat();									//子程序过程。功能：把ch中的字符拼入strToken
	void Clear();									//清空strToken，为下一轮识别做准备
	bool IsLetter() const;							//布尔函数。功能： ch中为字母时返回.T.
	bool IsDigit() const;							//布尔函数。功能： ch中为数字时返回.T.
	enum TokenType Reserve() const;					//整型函数。功能：按strToken中字符串查保留字表；查到返回保留字编码; 否则返回0
public:
	Scanner(InputBuffer& inputbuffer);
	void LexicalAnalysis();							//启动词法分析
	Token scan();									//找到下一个单词符号
	void ProcError(const std::string errorMessage, int line = -1, int column = -1) const;		//遇到无法识别字符串时错误提示
	const std::vector<Token>& GetTokens() const;
};
