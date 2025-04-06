#include "LexicalAnalyzer.h"
#include <string>
#include <fstream>
using namespace std;
InputBuffer::InputBuffer(const string path) :index(0)
{
	ifstream readFile(path, ios::in);
	if (!readFile.is_open())
	{
		cerr << "无法打开源文件: " << path << endl;
		throw runtime_error("无法打开源文件: " + path);
	}
	source.assign(istreambuf_iterator<char>(readFile), istreambuf_iterator<char>());
	readFile.close();
	//cout << source << endl;
}

void InputBuffer::filter_comments()
{
	char lastch = '\0';
	char currch = '\0';
	int sourceSize = source.size();
	enum state { normal, single_slash, in_line_comment, in_block_comment, in_block_comment_halfend, in_block_comment_slash };
	enum state curr = normal;
	int in_block_nested = 0;//注释嵌套次数
	for (int i = 0; i < sourceSize; i++) {
		currch = source[i];
		switch (curr) {
		case single_slash:
			if (currch == '/')
				curr = in_line_comment;
			else if (currch == '*')
				curr = in_block_comment;
			else {
				clean_code += '/';
				curr = normal;
				break;
			}
			continue;

		case in_line_comment:
			if (currch == '\r' || currch == '\n') {
				curr = normal;
				break;
			}
			else
				continue;

		case in_block_comment:
			if (currch == '\r' || currch == '\n')
				break;
			else if (currch == '*')
				curr = in_block_comment_halfend;
			else if (currch == '/')
				curr = in_block_comment_slash;
			continue;

		case in_block_comment_halfend:
			if (currch == '/')
				if (in_block_nested) {
					in_block_nested--;
					curr = in_block_comment;
				}
				else
					curr = normal;
			else if (currch != '*') {
				curr = in_block_comment;
				if (currch == '\r' || currch == '\n')
					break;
			}
			continue;

		case in_block_comment_slash:
			if (currch == '*') {
				curr = in_block_comment;
				in_block_nested++;
			}
			else if (currch != '/') {
				curr = in_block_comment;
				if (currch == '\r' || currch == '\n')
					break;
			}
			continue;

		default:
			break;
		}

		if (currch == '/')
			curr = single_slash;
		else {
			clean_code += currch;
			if (currch == '\r' && i < sourceSize - 1 && source[i + 1] == '\n') {
				line_breaks.push_back(clean_code.size() - 1);
				clean_code += source[++i];
			}
			else if (currch == '\r' || currch == '\n')
				line_breaks.push_back(clean_code.size() - 1);
		}
	}
	cout << clean_code << endl;
}

char InputBuffer::GetChar()
{
	if (!isEnd())
		index++;
	return clean_code[index];
}

char InputBuffer::Retract()
{
	if (index != 0)
		index--;
	return clean_code[index];
}

bool InputBuffer::isEnd()
{
	if (index >= clean_code.size())
		return true;
	else
		return false;
}

void InputBuffer::FindOriPos(unsigned int& line, unsigned int& column)//找到当前index在源程序中的位置
{
	for (line = 0; line < line_breaks.size(); line++)
		if (line_breaks[line] >= index)
			break;
	//line此时为行号-1，出错行换行对应数组下标
	line++;
}

Token::Token(TokenType type, TokenValue value) :type(type), value(value)
{
}

const string Scanner::ReservedWordsTable[] = { "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "i128", "u128",
												"isize", "usize", "f32", "f64", "bool", "char","unit", "array",
												"let", "if", "else","while", "return", "mut", "fn", "for", "in", "loop", "break", "continue" };//与TokenType第一行内容完全匹配，应同步修改

Scanner::Scanner(InputBuffer& inputbuffer) :input(inputbuffer), ch('\0')
{
}

void Scanner::GetChar()
{
	ch = input.GetChar();
	return;
}

void Scanner::GetBC()
{
	while (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t' || ch == EOF)
		GetChar();
	return;
}

void Scanner::Concat()
{
	strToken += ch;
	return;
}

bool Scanner::IsLetter() const
{
	return ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch == '_';//_与字母等效
}

bool Scanner::IsDigit() const
{
	return ch >= '0' && ch <= '9';
}

TokenType Scanner::Reserve() const
{
	//const vector<string>::const_iterator it = find(ReservedWordsTable.begin(), ReservedWordsTable.end(), strToken);
	//unsigned int reserveNum = distance(ReservedWordsTable.begin(), it);
	//if (reserveNum < ReservedWordsTable.size())
	//	return TokenType(reserveNum + 1);
	//else
	//	return None;
	unsigned int reserveNum;
	unsigned int size = sizeof(ReservedWordsTable) / sizeof(string);
	for (reserveNum = 0; reserveNum < size; reserveNum++)
		if (ReservedWordsTable[reserveNum] == strToken)
			return TokenType(reserveNum + 1);
	return None;
}

void Scanner::Retract()
{
	ch = input.Retract();//注意要更新当前读入字符，否则ch仍保留在下一字符处
	return;
}

void Scanner::Clear()
{
	strToken.clear();
}

size_t Scanner::InsertId(/*string token, */enum SymbolType type)//字符串为空也存了进去，未检验是否为空
{
	size_t IdNum = 0;
	for (; IdNum < SymbolTable.size(); IdNum++) {
		if (SymbolTable[IdNum].ID == strToken)
			return IdNum;
	}
	SymbolTable.push_back({ IdNum, strToken, NULL, type });//////待解决：词法分析怎么知道addr、type是什么？
	return IdNum;
}

Token Scanner::tokenize()
{
	Clear();
	enum TokenType code;
	size_t value;
	GetChar();
	GetBC();
	//三字符运算符
	if (ch == '<') {				//<<=
		GetChar();
		if (ch == '<') {
			GetChar();
			if (ch == '=')
				return Token(LeftShiftAssign);
			else
				Retract();
		}
		Retract();
	}
	else if (ch == '>') {				//>>=
		GetChar();
		if (ch == '>') {
			GetChar();
			if (ch == '=')
				return Token(RightShiftAssign);
			else
				Retract();
		}
		Retract();
	}
	//双字符运算符
	if (ch == '=') {			//== =>
		GetChar();
		if (ch == '=')				//无论后续是否有其它内容，只要读到==就取该==，后续按新内容分析
			return Token(Equality);
		else if (ch == '>')
			return Token(Arrowmatch);
		else
			Retract();
	}
	else if (ch == '>') {			//>= >>
		GetChar();
		if (ch == '=')				//无论后续是否有其它内容，只要读到>=就取该>=，后续按新内容分析
			return Token(GreaterOrEqual);
		else if (ch == '>')
			return Token(RightShift);
		else
			Retract();
	}
	else if (ch == '<') {			//<= <<
		GetChar();
		if (ch == '=')
			return Token(LessOrEqual);
		else if (ch == '<')
			return Token(LeftShift);
		else
			Retract();
	}
	else if (ch == '!') {			//!=
		GetChar();
		if (ch == '=')
			return Token(Inequality);
		else
			Retract();
	}
	else if (ch == '-') {			//->
		GetChar();
		if (ch == '>')
			return Token(ArrowOperator);
		else
			Retract();
	}
	else if (ch == '.') {			//..
		GetChar();
		if (ch == '.')
			return Token(RangeOperator);
		else
			Retract();
	}
	else if (ch == '%') {			//%=
		GetChar();
		if (ch == '=')
			return Token(ModuloAssign);
		else
			Retract();
	}
	else if (ch == '&') {			//&= &&
		GetChar();
		if (ch == '=')
			return Token(BitAndAssign);
		else if (ch == '&')
			return Token(LogicalAnd);
		else
			Retract();
	}
	//else if (ch == '&') {			//&&
	//	GetChar();
	//	if (ch == '&')
	//		return Token(LogicalAnd);
	//	else
	//		Retract();
	//}
	else if (ch == '|') {			//|= ||
		GetChar();
		if (ch == '=')
			return Token(BitOrAssign);
		else if (ch == '|')
			return Token(LogicalOr);
		else
			Retract();
	}
	//else if (ch == '|') {			//||
	//	GetChar();
	//	if (ch == '|')
	//		return Token(LogicalOr);
	//	else
	//		Retract();
	//}
	else if (ch == '*') {			//*=
		GetChar();
		if (ch == '=')
			return Token(MultiplicationAssign);
		else
			Retract();
	}
	else if (ch == '+') {			//+=
		GetChar();
		if (ch == '=')
			return Token(AdditionAssign);
		else
			Retract();
	}
	else if (ch == '-') {			//-=
		GetChar();
		if (ch == '=')
			return Token(SubtractionAssign);
		else
			Retract();
	}
	else if (ch == '/') {			///=
		GetChar();
		if (ch == '=')
			return Token(DivisionAssign);
		else
			Retract();
	}
	//else if (ch == '<') {			//<<
	//	GetChar();
	//	if (ch == '<')
	//		return Token(LeftShift);
	//	else
	//		Retract();
	//}
	//else if (ch == '=') {			//=>
	//	GetChar();
	//	if (ch == '>')
	//		return Token(Arrowmatch);
	//	else
	//		Retract();
	//}
	//else if (ch == '>') {			//>>
	//	GetChar();
	//	if (ch == '>')
	//		return Token(RightShift);
	//	else
	//		Retract();
	//}
	else if (ch == '^') {			//^=
		GetChar();
		if (ch == '=')
			return Token(BitXorAssign);
		else
			Retract();
	}
	//关键字、标识符
	if (IsLetter()) {
		while (IsLetter() || IsDigit()) {
			Concat();
			GetChar();
		}
		Retract();
		code = Reserve();
		if (code == None) {
			value = InsertId();
			return Token(Identifier, value);
		}
		else
			return Token(code);
	}
	//整型数值
	else if (IsDigit()) {
		while (IsDigit()) {
			Concat();
			GetChar();
		}
		Retract();
		return Token(i32, stoi(strToken));//目前仅识别i32常数，仍需添加其它类型数值的识别
	}
	//单字符运算符
	else if (ch == '=')
		return Token(Assignment);
	else if (ch == '+')
		return Token(Addition);
	else if (ch == '-')
		return Token(Subtraction);
	else if (ch == '*')
		return Token(Multiplication);
	else if (ch == '/')
		return Token(Division);
	else if (ch == '>')
		return Token(GreaterThan);
	else if (ch == '<')
		return Token(LessThan);
	else if (ch == '(')
		return Token(ParenthesisL);
	else if (ch == ')')
		return Token(ParenthesisR);
	else if (ch == '{')
		return Token(CurlyBraceL);
	else if (ch == '}')
		return Token(CurlyBraceR);
	else if (ch == '[')
		return Token(SquareBracketL);
	else if (ch == ']')
		return Token(SquareBracketR);
	else if (ch == ';')
		return Token(Semicolon);
	else if (ch == ':')
		return Token(Colon);
	else if (ch == ',')
		return Token(Comma);
	else if (ch == '.')
		return Token(DotOperator);
	else if (ch == '!')
		return Token(Not);
	else if (ch == '%')
		return Token(Modulo);
	else if (ch == '&')
		return Token(BitAnd);
	else if (ch == '|')
		return Token(BitOr);
	else if (ch == '?')
		return Token(ErrorPropagation);
	else if (ch == '@')
		return Token(PatternBinding);
	else if (ch == '^')
		return Token(BitXor);
	else if (ch == '\"')
		return Token(DoubleQuote);
	else if (ch == '\'')
		return Token(SingleQuote);
	else if (ch == '\0')
		return Token(End);
	else
		ProcError();
	return Token(None);
}

void Scanner::ProcError()
{
	unsigned int line, column;
	input.FindOriPos(line, column);
	if (strToken.empty())
		cerr << "词法分析错误：无法识别的标记：第" << line << "行：ASCII" << int(ch);
	else
		cerr << "词法分析错误：应输入声明：第" << line << "行：" << strToken;
	return;
}


void Scanner::LexicalAnalysis()
{
	while (!input.isEnd())
		tokens.push_back(tokenize());
	return;
}

const std::vector<Token>& Scanner::GetTokens() const
{
	return tokens;
}

const std::vector<SymbolTableEntry>& Scanner::GetSymbolTable() const
{
	return SymbolTable;
}
