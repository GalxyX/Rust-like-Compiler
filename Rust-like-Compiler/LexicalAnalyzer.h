#pragma once
#include <iostream>
#include <variant>
#include <vector>
class InputBuffer
{
private:
	// const static int RUST_MAX_IDENTIFIER_LENGTH// δ�ҵ�rust����ʶ������������Ϣ
	std::string source;				//����Դ����
	std::string clean_code;			//ɾ��ע�ͺ����
	std::vector<size_t>line_breaks;	//ɾ��ע�ͺ�����л����±�
	unsigned int index;				//ɨ������ǰָ�룬ָ����һ����Ҫ��ȡλ��
public:
	InputBuffer(const std::string path);
	void filter_comments();
	char GetChar();
	char Retract();
	bool isEnd();
	void FindOriPos(unsigned int& line, unsigned int& column);
};
/********************************************************************************
* Rust ÿ��ֵ������ȷ�е��������ͣ��ܵ���˵���Է�Ϊ���ࣺ�������ͺ͸������͡� ����������ζ������������һ����С��ԭ�����ͣ��޷��⹹Ϊ�������ͣ�һ����������˵������������ɣ�
* ��ֵ���ͣ��з������� (i8, i16, i32, i64, isize)�� �޷������� (u8, u16, u32, u64, usize) �������� (f32, f64)���Լ�������������
* �ַ������ַ������������ַ�����Ƭ &str
* �������ͣ�true �� false
* �ַ����ͣ���ʾ���� Unicode �ַ����洢Ϊ 4 ���ֽ�
* ��Ԫ���ͣ��� () ����Ψһ��ֵҲ�� ()
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
!	ident!(...), ident!{...}, ident![...]	��չ��
!	!expr	��λ�ǻ��߼���	Not
!=	var != expr	���ȱȽ�	PartialEq
%	expr % expr	����ȡģ	Rem
%=	var %= expr	����ȡģ�븳ֵ	RemAssign
&	&expr, &mut expr	����
&	&type, &mut type, &'a type, &'a mut type	����ָ������
&	expr & expr	��λ��	BitAnd
&=	var &= expr	��λ�뼰��ֵ	BitAndAssign
&&	expr && expr	�߼���
*	expr * expr	�����˷�	Mul
*=	var *= expr	�����˷��븳ֵ	MulAssign
*	*expr	������
*	*const type, *mut type	��ָ��
+	trait + trait, 'a + trait	������������
+	expr + expr	�����ӷ�	Add
+=	var += expr	�����ӷ��븳ֵ	AddAssign
,	expr, expr	�����Լ�Ԫ�طָ���
-	- expr	����ȡ��	Neg
-	expr - expr	��������	Sub
-=	var -= expr	���������븳ֵ	SubAssign
->	fn(...) -> type, |...| -> type	������հ�����������
.	expr.ident	��Ա����
..	.., expr.., ..expr, expr..expr	���ų���Χ
..	..expr	�ṹ������﷨
..	variant(x, ..), struct_type { x, .. }	����ʣ�ಿ�֡���ģʽ��
...	expr...expr	ģʽ: ��Χ����ģʽ
/	expr / expr	��������	Div
/=	var /= expr	���������븳ֵ	DivAssign
:	pat: type, ident: type	Լ��
:	ident: expr	�ṹ���ֶγ�ʼ��
:	'a: loop {...}	ѭ����־
;	expr;	������������
;	[...; len]	�̶���С�����﷨�Ĳ���
<<	expr << expr	����	Shl
<<=	var <<= expr	�����븳ֵ	ShlAssign
<	expr < expr	С�ڱȽ�	PartialOrd
<=	expr <= expr	С�ڵ��ڱȽ�	PartialOrd
=	var = expr, ident = type	��ֵ/��ֵ
==	expr == expr	���ڱȽ�	PartialEq
=>	pat => expr	ƥ��׼���﷨�Ĳ���
>	expr > expr	���ڱȽ�	PartialOrd
>=	expr >= expr	���ڵ��ڱȽ�	PartialOrd
>>	expr >> expr	����	Shr
>>=	var >>= expr	�����븳ֵ	ShrAssign
@	ident @ pat	ģʽ��
^	expr ^ expr	��λ���	BitXor
^=	var ^= expr	��λ����븳ֵ	BitXorAssign
|	pat | pat	ģʽѡ��
|	expr | expr	��λ��	BitOr
|=	var |= expr	��λ���븳ֵ	BitOrAssign
||	expr || expr	�߼���
?	expr?	���󴫲�
********************************************************************************/
//enum TokenType {
//	None,//����ƥ�䷵��ֵ
//	I8, U8, I16, U16, I32, U32, I64, U64, I128, U128, ISIZE, USIZE, F32, F64, BOOL, CHAR, UNIT, ARRAY, LET, IF, ELSE, WHILE, RETURN, MUT, FN, FOR, IN, LOOP, BREAK, CONTINUE,//�ؼ���reservedWord
//	Identifier,//��ʶ��Identifier
//	/*i8, u8, i16, u16, */i32, /*u32, i64, u64, i128, u128, isize, usize, f32, f64, bool_, char_, unit_, array_,*/ //��������Constant
//	//���������Operator, Delimiter
//	Assignment,//��ֵ�ţ� =
//	Addition, Subtraction, Multiplication, Division, Equality, GreaterThan, GreaterOrEqual, LessThan, LessOrEqual, Inequality,//����� + | -| *| / | == | > | >= | < | <= | !=
//	ParenthesisL, ParenthesisR, CurlyBraceL, CurlyBraceR, SquareBracketL, SquareBracketR,//�����( | ) | { | } | [ | ]
//	Semicolon, Colon, Comma,//�ָ�����; | : | ,
//	ArrowOperator, DotOperator, RangeOperator,//������ţ� -> | . | ..
//	End,//������#��'\0'��
//	DoubleQuote, SingleQuote,//�������" '
//	Not, Modulo, ModuloAssign, BitAnd, BitAndAssign, LogicalAnd, BitOr, BitOrAssign, LogicalOr, ErrorPropagation,//�������! % %= & &= && | |= || ?
//	MultiplicationAssign, AdditionAssign, SubtractionAssign, DivisionAssign, LeftShift, LeftShiftAssign, Arrowmatch, RightShift, RightShiftAssign, PatternBinding, BitXor, BitXorAssign//�������*= += -= /= << <<= => >> >>= @ ^ ^=
//};
// ��������ö�ٳ�Ա�ĺ��б�
#define TOKEN_TYPES \
    X(None) \
    /* �ؼ���reservedWord */ \
    X(I8) X(U8) X(I16) X(U16) X(I32) X(U32) X(I64) X(U64) X(I128) X(U128) X(ISIZE) X(USIZE) X(F32) X(F64) X(BOOL) X(CHAR) \
    X(UNIT) X(ARRAY) X(LET) X(IF) X(ELSE) X(WHILE) X(RETURN) X(MUT) X(FN) X(FOR) X(IN) X(LOOP) X(BREAK) X(CONTINUE) \
    /* ��ʶ��Identifier */ \
    X(Identifier) \
    /* ��������Constant */ \
	/*X(i8) X(u8) X(i16) X(u16) */X(i32) /*X(u32) X(i64) X(u64) X(i128) X(u128) X(isize) X(usize) X(f32) X(f64) X(bool_) X(char_) X(unit_) X(array_)*/ \
    /* ���������Operator, Delimiter */ \
    X(Assignment)																																	/* ��ֵ�ţ� = */ \
    X(Addition) X(Subtraction) X(Multiplication) X(Division) X(Equality) X(GreaterThan) X(GreaterOrEqual) X(LessThan) X(LessOrEqual) X(Inequality)	/* ����� + | -| *| / | == | > | >= | < | <= | != */ \
    X(ParenthesisL) X(ParenthesisR) X(CurlyBraceL) X(CurlyBraceR) X(SquareBracketL) X(SquareBracketR)												/* �����( | ) | { | } | [ | ] */ \
    X(Semicolon) X(Colon) X(Comma)																													/* �ָ�����; | : | , */ \
    X(ArrowOperator) X(DotOperator) X(RangeOperator)																								/* ������ţ� -> | . | .. */ \
    X(End)																																			/* ������#��'\0'�� */ \
    /* ������� */ \
    X(DoubleQuote) X(SingleQuote)																													/* �������" ' */ \
    X(Not) X(Modulo) X(ModuloAssign) X(BitAnd) X(BitAndAssign) X(LogicalAnd) X(BitOr) X(BitOrAssign) X(LogicalOr) X(ErrorPropagation)				/* �������! % %= & &= && | |= || ? */ \
    X(MultiplicationAssign) X(AdditionAssign) X(SubtractionAssign) X(DivisionAssign) X(LeftShift) X(LeftShiftAssign) X(Arrowmatch) X(RightShift) X(RightShiftAssign) X(PatternBinding) X(BitXor) X(BitXorAssign) /* �������*= += -= /= << <<= => >> >>= @ ^ ^= */

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
	static const std::string ReservedWordsTable[];	//��TokenType��һ��������ȫƥ�䣬Ӧͬ���޸�
	InputBuffer& input;								//���뻺����
	char ch;										//��ŵ�ǰ�����ַ�
	std::string strToken;							//��ŵ��ʵ��ַ���
	std::vector<SymbolTableEntry> SymbolTable;		//��ʶ����
	std::vector<Token> tokens;						//���е��ʷ���
	void GetChar();									//ȡ�ַ����̡�ȡ��һ�ַ���ch ������ָ�� + 1
	void GetBC();									//�˳����ַ����̡����ܣ���ch = �� ? ���ǣ������GetChar
	void Retract();									//�ӳ�����̡����ܣ�����ָ�����һ�ַ�
	void Concat();									//�ӳ�����̡����ܣ���ch�е��ַ�ƴ��strToken
	void Clear();									//���strToken��Ϊ��һ��ʶ����׼��
	bool IsLetter() const;							//�������������ܣ� ch��Ϊ��ĸʱ����.T.
	bool IsDigit() const;							//�������������ܣ� ch��Ϊ����ʱ����.T.
	enum TokenType Reserve() const;					//���ͺ��������ܣ���strToken���ַ����鱣���ֱ��鵽���ر����ֱ���; ���򷵻�0
	size_t InsertId(/*string token, */enum SymbolType type = _i32);//���������ܣ�����ʶ��������ű����ط��ű�ָ��
	Token tokenize();								//�ҵ���һ�����ʷ���
	void ProcError();								//�����޷�ʶ���ַ���ʱ������ʾ
public:
	Scanner(InputBuffer& inputbuffer);
	void LexicalAnalysis();							//�����ʷ�����
	const std::vector<Token>& GetTokens() const;
	const std::vector<SymbolTableEntry>& GetSymbolTable() const;
};
