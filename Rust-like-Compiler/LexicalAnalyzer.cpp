#include "LexicalAnalyzer.h"
#include <string>
#include <fstream>
using namespace std;
InputBuffer::InputBuffer(const filesystem::path& path) :index(0)
{
	ifstream readFile(path, ios::in);
	if (!readFile.is_open())
	{
		cerr << "�޷���Դ�ļ�: " << path << endl;
		throw runtime_error("�޷���Դ�ļ�: " + path.string());
	}
	source.assign(istreambuf_iterator<char>(readFile), istreambuf_iterator<char>());
	readFile.close();
	cout << source << endl;
}

InputBuffer::InputBuffer(const string& src) :index(0), source(src)
{
}

void InputBuffer::filter_comments()
{
	char lastch = '\0';
	char currch = '\0';
	int sourceSize = source.size();
	enum state { normal, single_slash, in_line_comment, in_block_comment, in_block_comment_halfend, in_block_comment_slash };
	enum state curr = normal;
	int in_block_nested = 0;//ע��Ƕ�״���
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
	//����δ��ȫ��ʼ��δ��ɵ�ע��
	switch (curr) {
	case normal:
	case in_line_comment:
		break;
	case single_slash:
		clean_code += '/';
		break;
	case in_block_comment:
	case in_block_comment_halfend:
	case in_block_comment_slash:
		cerr << "�ʷ����������ļ�ĩβ��δ�����Ŀ�ע��" << endl;
		break;
	default:
		break;
	}

	//cout << clean_code << endl;
}

char InputBuffer::GetChar()
{
	if (!isEnd())
		return clean_code[index++];
	else
		return clean_code[index];
}

char InputBuffer::Retract()
{
	if (index == 0)
		throw runtime_error("��Ӧ��ָ��Ϊ0ʱ����Retract��������δ��ȡ�ַ���ʱ����Retract����");
	if (--index == 0)
		return clean_code[index];
	else
		return clean_code[index - 1];
}

bool InputBuffer::isEnd()
{
	if (index >= clean_code.size())
		return true;
	else
		return false;
}

void InputBuffer::FindOriPos(int& line, int& column) const//�ҵ���ǰindex��Դ�����е�λ��
{
	if (clean_code.size() <= 0) {
		line = column = -1;
		return;
	}
	//�ҵ�index�ڵڼ���line
	for (line = 0; line < int(line_breaks.size()); line++)
		if (line_breaks[line] >= index - 1)
			break;
	if (line == line_breaks.size()) {
		line = column = -1;
		return;
	}
	//line��ʱΪ�к�-1�������л��ж�Ӧ�����±�

	//�ҵ���line��������Դ�����е�λ��ori_index
	int line_remain = line;//Ҫ���ҵڼ������У�0��Ϊ�����ң���һ�У�
	int ori_index = -1;//��line��������ԭʼsource�е�����
	const char* start = source.data();
	const char* end = start + source.size();
	const char* ptr = start;
	while (ptr < end && line_remain > 0) {
		if (*ptr == '\r' || *ptr == '\n') {
			if (*ptr == '\r' && ptr < end - 1 && *(ptr + 1) == '\n')
				++ptr;
			--line_remain;
			if (line_remain == 0) {
				ori_index = ptr - start;
				break;
			}
		}
		++ptr;
	}
	//�ҵ�index��Ӧ�ַ��ڱ��е������column
	int temp_index = line == 0 ? -1 : line_breaks[line - 1];//ȥ��ע��clean_code�Ŀ��ƶ�ָ��
	int temp_ori_index = ori_index;//ԭʼsource�Ŀ��ƶ�ָ��
	++temp_index;//���ҵ�һ��ʱ����Ϊ-1������-1
	++temp_ori_index;
	if (temp_ori_index < int(source.size()) && source[temp_ori_index] == '\n' && temp_index < int(clean_code.size()) && source[temp_index] == '\n') {//����\r\n
		++temp_ori_index;
		++temp_index;
	}
	bool find = false;//��ʼ����
	while (temp_index <= int(index - 1) && temp_ori_index < int(source.size()) && source[temp_ori_index] != '\r' && source[temp_ori_index] != '\n')
		if (source[temp_ori_index] == clean_code[temp_index]) {
			if (temp_index == index - 1) {
				find = true;
				break;
			}
			++temp_ori_index;
			++temp_index;
		}
		else
			++temp_ori_index;

	if (find) {
		++line;
		column = temp_ori_index - ori_index;
	}
	else
		line = column = -1;
}

Token::Token(TokenType type, TokenValue value) :type(type), value(value)
{
}

const string Scanner::ReservedWordsTable[] = { "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "i128", "u128",
												"isize", "usize", "f32", "f64", "bool", "char","unit", "array",
												"let", "if", "else","while", "return", "mut", "fn", "for", "in", "loop", "break", "continue" };//��TokenType��һ��������ȫƥ�䣬Ӧͬ���޸�

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
	return ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch == '_';//_����ĸ��Ч
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
	ch = input.Retract();//ע��Ҫ���µ�ǰ�����ַ�������ch�Ա�������һ�ַ���
	return;
}

void Scanner::Clear()
{
	strToken.clear();
}

size_t Scanner::InsertId(/*string token*/)//�ַ���Ϊ��Ҳ���˽�ȥ��δ�����Ƿ�Ϊ��
{
	size_t IdNum = 0;
	for (; IdNum < SymbolTable.size(); IdNum++) {
		if (SymbolTable[IdNum].ID == strToken)
			return IdNum;
	}
	SymbolTable.push_back({ IdNum, strToken, NULL, undefined });//addr��type���﷨����ģ�����
	return IdNum;
}

Token Scanner::tokenize()
{
	Clear();
	enum TokenType code;
	size_t value;
	GetChar();
	GetBC();
	//���ַ������
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
	//˫�ַ������
	if (ch == '=') {			//== =>
		GetChar();
		if (ch == '=')				//���ۺ����Ƿ����������ݣ�ֻҪ����==��ȡ��==�������������ݷ���
			return Token(Equality);
		else if (ch == '>')
			return Token(Arrowmatch);
		else
			Retract();
	}
	else if (ch == '>') {			//>= >>
		GetChar();
		if (ch == '=')				//���ۺ����Ƿ����������ݣ�ֻҪ����>=��ȡ��>=�������������ݷ���
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
	//�ؼ��֡���ʶ��
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
	//������ֵ
	else if (IsDigit()) {
		while (IsDigit()) {
			Concat();
			GetChar();
		}
		Retract();
		return Token(i32, stoi(strToken));//Ŀǰ��ʶ��i32�����������������������ֵ��ʶ��
	}
	//���ַ������
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
	int line, column;
	input.FindOriPos(line, column);
	if (strToken.empty())
		cerr << "�ʷ����������޷�ʶ��ı�ǣ�" << line << "��" << column << "��ASCII" << int(ch) << endl;
	else
		cerr << "�ʷ���������Ӧ����������" << line << "��" << column - strToken.size() << "��" << strToken << endl;
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
