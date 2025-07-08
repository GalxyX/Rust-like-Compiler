#pragma once
#include <iostream>
#include <variant>
#include <vector>
#include <filesystem>
// ����˺������õ��������ע�͵������Խ��õ������
//#define ENABLE_NOTING_OUTPUT

#ifdef ENABLE_NOTING_OUTPUT
#define DEBUG_CERR cerr
#else
#define DEBUG_CERR if(0) cerr
#endif // ENABLE_NOTING_OUTPUT

class InputBuffer
{
private:
	// const static int RUST_MAX_IDENTIFIER_LENGTH// δ�ҵ�rust����ʶ������������Ϣ
	std::string source;				//����Դ����
	std::string clean_code;			//ɾ��ע�ͺ����
	std::vector<size_t>line_breaks;	//ɾ��ע�ͺ�����л����±�
	unsigned int index;				//ɨ������ǰָ�룬ָ����һ����Ҫ��ȡλ��
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
//	/*i8, u8, i16, u16, */i32, /*u32, i64, u64, i128, u128, isize, usize, f32, f64, bool_, char_, string_, unit_, array_,*/ //��������Constant
//	//���������Operator, Delimiter
//	Assignment,//��ֵ�ţ� =
//	Addition, Subtraction, Multiplication, Division, Equality, GreaterThan, GreaterOrEqual, LessThan, LessOrEqual, Inequality,//����� + | -| *| / | == | > | >= | < | <= | !=
//	ParenthesisL, ParenthesisR, CurlyBraceL, CurlyBraceR, SquareBracketL, SquareBracketR,//�����( | ) | { | } | [ | ]
//	Semicolon, Colon, Comma,//�ָ�����; | : | ,
//	ArrowOperator, DotOperator, RangeOperator,//������ţ� -> | . | ..
//	DoubleQuote, SingleQuote,//�������" '
//	Not, Modulo, ModuloAssign, BitAnd, BitAndAssign, LogicalAnd, BitOr, BitOrAssign, LogicalOr, ErrorPropagation,//�������! % %= & &= && | |= || ?
//	MultiplicationAssign, AdditionAssign, SubtractionAssign, DivisionAssign, LeftShift, LeftShiftAssign, Arrowmatch, RightShift, RightShiftAssign, PatternBinding, BitXor, BitXorAssign,//�������*= += -= /= << <<= => >> >>= @ ^ ^=
//	End//������#��'\0'��
//};
// ��������ö�ٳ�Ա�ĺ��б�
#define TOKEN_TYPES \
    X(None) \
    /* �ؼ���reservedWord */ \
    X(I8) X(U8) X(I16) X(U16) X(I32) X(U32) X(I64) X(U64) X(I128) X(U128) X(ISIZE) X(USIZE) X(F32) X(F64) X(BOOL) X(CHAR) \
    X(UNIT) X(ARRAY) X(LET) X(IF) X(ELSE) X(WHILE) X(RETURN) X(MUT) X(FN) X(FOR) X(IN) X(LOOP) X(BREAK) X(CONTINUE) \
	X(TRUE) X(FALSE) \
    /* ��ʶ��Identifier */ \
    X(Identifier) \
    /* ��������Constant */ \
	/*X(i8_) X(u8_) X(i16_) X(u16_) */X(i32_) /*X(u32_) X(i64_) X(u64_) X(i128_) X(u128_) X(isize_) X(usize_) X(f32_) X(f64_) X(bool_) */X(char_) X(string_)/* X(unit_) X(array_)*/ \
    /* ���������Operator, Delimiter */ \
    X(Assignment)																																	/* ��ֵ�ţ� = */ \
    X(Addition) X(Subtraction) X(Multiplication) X(Division) X(Equality) X(GreaterThan) X(GreaterOrEqual) X(LessThan) X(LessOrEqual) X(Inequality)	/* ����� + | -| *| / | == | > | >= | < | <= | != */ \
    X(ParenthesisL) X(ParenthesisR) X(CurlyBraceL) X(CurlyBraceR) X(SquareBracketL) X(SquareBracketR)												/* �����( | ) | { | } | [ | ] */ \
    X(Semicolon) X(Colon) X(Comma)																													/* �ָ�����; | : | , */ \
    X(ArrowOperator) X(DotOperator) X(RangeOperator)																								/* ������ţ� -> | . | .. */ \
    /* ������� */ \
    X(DoubleQuote) X(SingleQuote)																													/* �������" ' */ \
    X(Not) X(Modulo) X(ModuloAssign) X(BitAnd) X(BitAndAssign) X(LogicalAnd) X(BitOr) X(BitOrAssign) X(LogicalOr) X(ErrorPropagation)				/* �������! % %= & &= && | |= || ? */ \
    X(MultiplicationAssign) X(AdditionAssign) X(SubtractionAssign) X(DivisionAssign) X(LeftShift) X(LeftShiftAssign) X(Arrowmatch) X(RightShift) X(RightShiftAssign) X(PatternBinding) X(BitXor) X(BitXorAssign) /* �������*= += -= /= << <<= => >> >>= @ ^ ^= */ \
    X(End) X(EPSILON)																																/* ������#��'\0'�� */

enum TokenType {
#define X(name) name,
	TOKEN_TYPES
#undef X
};
using TokenValue = std::variant<std::monostate, int, unsigned int, long, unsigned long, char, float, double, std::string>;
struct Token {
	enum TokenType type;
	TokenValue value;
	int line;    // �к�
	int column;  // �к�
	int length;  // �ʷ���Ԫ����
	Token(TokenType type, int line = -1, int column = -1, int length = 0, TokenValue value = 0);
	Token& operator=(const Token& other);
};

const char* TokenTypeToString(enum TokenType type);// ͨ��ö��ֵ��ȡ����
const char OutOfRangeTokenType[] = "UnknownTokenType";//���������TokenType����TokenTypeToStringʱ����ֵ

class Scanner
{
private:
	static const std::string ReservedWordsTable[];	//��TokenType��һ��������ȫƥ�䣬Ӧͬ���޸�
	InputBuffer& input;								//���뻺����
	char ch;										//��ŵ�ǰ�����ַ�
	std::string strToken;							//��ŵ��ʵ��ַ���
	std::vector<Token> tokens;						//���е��ʷ���
	void GetChar();									//ȡ�ַ����̡�ȡ��һ�ַ���ch ������ָ�� + 1
	void GetBC();									//�˳����ַ����̡����ܣ���ch = �� ? ���ǣ������GetChar
	void Retract();									//�ӳ�����̡����ܣ�����ָ�����һ�ַ�
	void Concat();									//�ӳ�����̡����ܣ���ch�е��ַ�ƴ��strToken
	void Clear();									//���strToken��Ϊ��һ��ʶ����׼��
	bool IsLetter() const;							//�������������ܣ� ch��Ϊ��ĸʱ����.T.
	bool IsDigit() const;							//�������������ܣ� ch��Ϊ����ʱ����.T.
	enum TokenType Reserve() const;					//���ͺ��������ܣ���strToken���ַ����鱣���ֱ��鵽���ر����ֱ���; ���򷵻�0
public:
	Scanner(InputBuffer& inputbuffer);
	void LexicalAnalysis();							//�����ʷ�����
	Token scan();									//�ҵ���һ�����ʷ���
	void ProcError(const std::string errorMessage, int line = -1, int column = -1) const;		//�����޷�ʶ���ַ���ʱ������ʾ
	const std::vector<Token>& GetTokens() const;
};
