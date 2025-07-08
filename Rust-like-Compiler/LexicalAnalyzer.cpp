#include "LexicalAnalyzer.h"
#include <string>
#include <fstream>
using namespace std;
InputBuffer::InputBuffer(const filesystem::path& path) :index(0)
{
	ifstream readFile(path, ios::in);
	if (!readFile.is_open())
	{
		DEBUG_CERR << "�޷���Դ�ļ�: " << path << endl;
		throw runtime_error("�޷���Դ�ļ�: " + path.string());
	}
	source.assign(istreambuf_iterator<char>(readFile), istreambuf_iterator<char>());
	readFile.close();
	//cout << source << endl;
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
			bool notEmpty = !clean_code.empty();
			if (notEmpty && (currch == ' ' && clean_code[clean_code.size() - 1] == ' ' || currch == '\t' && clean_code[clean_code.size() - 1] == '\t'))//ɾ������ո��Ʊ��
				continue;
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
		DEBUG_CERR << "�ʷ����������ļ�ĩβ��δ�����Ŀ�ע��" << endl;
		break;
	default:
		break;
	}

	//cout << clean_code << endl;
}

char InputBuffer::GetChar()
{//ָ��ǰ��1������ָ��ǰ��ǰָ���ַ�
	if (!isEnd())
		return clean_code[index++];
	else
		return '\0';//indexָ��'\0'֮��ʱ��ʼ�շ���'\0'
}

char InputBuffer::Retract()
{//ָ�����1�����ػ��˺�ָ��ǰһ���ַ�
	if (index == 0)
		throw runtime_error("��Ӧ��ָ��Ϊ0ʱ����Retract��������δ��ȡ�ַ���ʱ����Retract����");
	if (--index == 0)
		return -2;//��ʱ����-2
	else
		return clean_code[index - 1];
}

bool InputBuffer::isEnd()
{
	if (index > clean_code.size())//����'\0'�Դ������һ���ַ�
		return true;
	else
		return false;
}

void InputBuffer::FindOriPos(int& line, int& column) const//�ҵ���ǰindex��Դ�����е�λ�ã�columnΪȥ��ע�ͺ���У�
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
	++line;
	column = index - 1 - temp_index;
}

Token::Token(TokenType type, int line, int column, int length, TokenValue value) :type(type), value(value), line(line), column(column), length(length)
{
}

Token& Token::operator=(const Token& other)
{
	if (this != &other) {
		this->type = other.type;
		this->value = other.value;
		this->line = other.line;
		this->column = other.column;
		this->length = other.length;
	}
	return *this;
}

const char* TokenTypeToString(enum TokenType type) {
	// ���������ַ�������
	const char* TokenTypeNames[] = {
	#define X(name) #name,
		TOKEN_TYPES
	#undef X
	};
	int index = static_cast<int>(type);
	if (index >= 0 && index <= sizeof(TokenTypeNames) / sizeof(char*))
		return TokenTypeNames[index];
	return OutOfRangeTokenType;
}

const string Scanner::ReservedWordsTable[] = { "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "i128", "u128",
												"isize", "usize", "f32", "f64", "bool", "char","unit", "array",
												"let", "if", "else","while", "return", "mut", "fn", "for", "in", "loop", "break", "continue",
												"true", "false" };//��TokenType��һ��������ȫƥ�䣬Ӧͬ���޸�

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

Token Scanner::scan()
{
	Clear();
	enum TokenType code;
	size_t value;
	GetChar();
	GetBC();
	int line, column;//���ŵ���ʼλ�ã���ע�ʹ����У�
	input.FindOriPos(line, column);
	//���ַ������
	if (ch == '<') {				//<<=
		GetChar();
		if (ch == '<') {
			GetChar();
			if (ch == '=')
				return Token(LeftShiftAssign, line, column, 3);
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
				return Token(RightShiftAssign, line, column, 3);
			else
				Retract();
		}
		Retract();
	}
	//˫�ַ������
	if (ch == '=') {			//== =>
		GetChar();
		if (ch == '=')				//���ۺ����Ƿ����������ݣ�ֻҪ����==��ȡ��==�������������ݷ���
			return Token(Equality, line, column, 2);
		else if (ch == '>')
			return Token(Arrowmatch, line, column, 2);
		else
			Retract();
	}
	else if (ch == '>') {			//>= >>
		GetChar();
		if (ch == '=')				//���ۺ����Ƿ����������ݣ�ֻҪ����>=��ȡ��>=�������������ݷ���
			return Token(GreaterOrEqual, line, column, 2);
		else if (ch == '>')
			return Token(RightShift, line, column, 2);
		else
			Retract();
	}
	else if (ch == '<') {			//<= <<
		GetChar();
		if (ch == '=')
			return Token(LessOrEqual, line, column, 2);
		else if (ch == '<')
			return Token(LeftShift, line, column, 2);
		else
			Retract();
	}
	else if (ch == '!') {			//!=
		GetChar();
		if (ch == '=')
			return Token(Inequality, line, column, 2);
		else
			Retract();
	}
	else if (ch == '-') {			//->
		GetChar();
		if (ch == '>')
			return Token(ArrowOperator, line, column, 2);
		else
			Retract();
	}
	else if (ch == '.') {			//..
		GetChar();
		if (ch == '.')
			return Token(RangeOperator, line, column, 2);
		else
			Retract();
	}
	else if (ch == '%') {			//%=
		GetChar();
		if (ch == '=')
			return Token(ModuloAssign, line, column, 2);
		else
			Retract();
	}
	else if (ch == '&') {			//&= &&
		GetChar();
		if (ch == '=')
			return Token(BitAndAssign, line, column, 2);
		else if (ch == '&')
			return Token(LogicalAnd, line, column, 2);
		else
			Retract();
	}
	else if (ch == '|') {			//|= ||
		GetChar();
		if (ch == '=')
			return Token(BitOrAssign, line, column, 2);
		else if (ch == '|')
			return Token(LogicalOr, line, column, 2);
		else
			Retract();
	}
	else if (ch == '*') {			//*=
		GetChar();
		if (ch == '=')
			return Token(MultiplicationAssign, line, column, 2);
		else
			Retract();
	}
	else if (ch == '+') {			//+=
		GetChar();
		if (ch == '=')
			return Token(AdditionAssign, line, column, 2);
		else
			Retract();
	}
	else if (ch == '-') {			//-=
		GetChar();
		if (ch == '=')
			return Token(SubtractionAssign, line, column, 2);
		else
			Retract();
	}
	else if (ch == '/') {			///=
		GetChar();
		if (ch == '=')
			return Token(DivisionAssign, line, column, 2);
		else
			Retract();
	}
	else if (ch == '^') {			//^=
		GetChar();
		if (ch == '=')
			return Token(BitXorAssign, line, column, 2);
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
			//value = InsertId();
			return Token(Identifier, line, column, strToken.length(), strToken);
		}
		else
			return Token(code, line, column, strToken.length());
	}
	//������ֵ
	else if (IsDigit()) {
		while (IsDigit()) {
			Concat();
			GetChar();
		}
		Retract();
		return Token(i32_, line, column, strToken.length(), stoi(strToken));//Ŀǰ��ʶ��i32�����������������������ֵ��ʶ��
	}
	//���ַ������
	else if (ch == '=')
		return Token(Assignment, line, column, 1);
	else if (ch == '+')
		return Token(Addition, line, column, 1);
	else if (ch == '-')
		return Token(Subtraction, line, column, 1);
	else if (ch == '*')
		return Token(Multiplication, line, column, 1);
	else if (ch == '/')
		return Token(Division, line, column, 1);
	else if (ch == '>')
		return Token(GreaterThan, line, column, 1);
	else if (ch == '<')
		return Token(LessThan, line, column, 1);
	else if (ch == '(')
		return Token(ParenthesisL, line, column, 1);
	else if (ch == ')')
		return Token(ParenthesisR, line, column, 1);
	else if (ch == '{')
		return Token(CurlyBraceL, line, column, 1);
	else if (ch == '}')
		return Token(CurlyBraceR, line, column, 1);
	else if (ch == '[')
		return Token(SquareBracketL, line, column, 1);
	else if (ch == ']')
		return Token(SquareBracketR, line, column, 1);
	else if (ch == ';')
		return Token(Semicolon, line, column, 1);
	else if (ch == ':')
		return Token(Colon, line, column, 1);
	else if (ch == ',')
		return Token(Comma, line, column, 1);
	else if (ch == '.')
		return Token(DotOperator, line, column, 1);
	else if (ch == '!')
		return Token(Not, line, column, 1);
	else if (ch == '%')
		return Token(Modulo, line, column, 1);
	else if (ch == '&')
		return Token(BitAnd, line, column, 1);
	else if (ch == '|')
		return Token(BitOr, line, column, 1);
	else if (ch == '?')
		return Token(ErrorPropagation, line, column, 1);
	else if (ch == '@')
		return Token(PatternBinding, line, column, 1);
	else if (ch == '^')
		return Token(BitXor, line, column, 1);
	else if (ch == '\"') {
		GetChar();
		while (ch != '\"') {
			if (ch == '\0') {
				ProcError("δ��ֹ���ַ���");
				return Token(DoubleQuote, line, column, 1);
			}
			else if (ch == '\\') {
				GetChar();
				if (ch == 'n' || ch == 't' || ch == 'r' || ch == '\\' || ch == '\"' || ch == '\'') {//ת���
					if (ch == 'n')
						ch = '\n';
					else if (ch == 't')
						ch = '\t';
					else if (ch == 'r')
						ch = '\r';
					else if (ch == '\\')
						ch = '\\';
					else if (ch == '\"')
						ch = '\"';
					else if (ch == '\'')
						ch = '\'';
					Concat();
					GetChar();
				}
				else if (ch == '\r' || ch == '\n') {//\��ӻ���
					if (ch == '\r') {
						GetChar();
						if (ch == '\n')
							GetChar();
					}
					else if (ch == '\n')
						GetChar();
					while (ch == ' ')
						GetChar();
				}
				//\��ӷǻ���
				else {
					Retract();
					Concat();
					GetChar();
				}
			}
			else if (ch == '\r' || ch == '\n') {//ֱ�ӻ���
				Concat();
				GetChar();
				if (ch == ' ')
					Concat();
				while (ch == ' ')
					GetChar();
			}
			else {
				Concat();
				GetChar();
			}
		}
		return Token(string_, line, column, strToken.length(), strToken);
	}
	else if (ch == '\'') {
		GetChar();
		while (ch != '\'') {
			if (ch == '\\') {
				GetChar();
				if (ch == 'n')
					ch = '\n';
				else if (ch == 't')
					ch = '\t';
				else if (ch == 'r')
					ch = '\r';
				else if (ch == '\\')
					ch = '\\';
				else if (ch == '\"')
					ch = '\"';
				else if (ch == '\'')
					ch = '\'';
				else {
					ProcError("�Ƿ���ת���", line, column);
					return Token(SingleQuote, line, column, 1);
				}
				GetChar();
			}
			if (ch == '\r' || ch == '\n') {
				ProcError("δ��ֹ���ַ�����", line, column);
				return Token(SingleQuote, line, column, 1);
			}
			else {
				Concat();
				GetChar();
			}
		}
		return Token(char_, line, column, strToken.length(), ch);
	}
	else if (ch == '\0')
		return Token(End, line, column, 1);
	else
		ProcError(strToken.empty() ? "�ʷ����������޷�ʶ��ı��" : "�ʷ���������Ӧ��������", line, column);
	return Token(None, line, column, strToken.length());
}

void Scanner::ProcError(const string errorMessage, int line, int column) const
{
	if (line == -1 || column == -1)
		input.FindOriPos(line, column);
	if (strToken.empty())
		DEBUG_CERR << errorMessage << "��" << line << "��" << column << "��ASCII" << int(ch) << endl;
	else
		DEBUG_CERR << errorMessage << line << "��" << column - strToken.size() << "��" << strToken << endl;
	return;
}


void Scanner::LexicalAnalysis()
{
	while (!input.isEnd())
		tokens.push_back(scan());
	return;
}

const std::vector<Token>& Scanner::GetTokens() const
{
	return tokens;
}
