#include "LexicalAnalyzer.h"
using namespace std;

// ���������ַ�������
const char* TokenTypeNames[] = {
#define X(name) #name,
	TOKEN_TYPES
#undef X
};

// ͨ��ö��ֵ��ȡ���ƣ����鷶Χ��
static const char* TokenTypeToString(TokenType type) {
	int index = static_cast<int>(type);
	if (index >= 0 && index < sizeof(TokenTypeNames) / sizeof(char*))
		return TokenTypeNames[index];
	return "Unknown";
}

int main(int argc, char** argv)
{
	InputBuffer* inputb;
	filesystem::path path = "rust/test.rs";

	if (argc == 3 && !strcmp(argv[1], "-f"))
		path = argv[2];
	else if (argc == 2 && !strcmp(argv[1], "-s"))
		while (true) {
			string s;
			cin >> s;
			InputBuffer newinput(s);
			newinput.filter_comments();

			Scanner newscanner(newinput);
			newscanner.LexicalAnalysis();
			vector<SymbolTableEntry> SymbolTable = newscanner.GetSymbolTable();
			vector<Token> tokens = newscanner.GetTokens();
			for (int i = 0; i < tokens.size(); ++i)
				if (tokens[i].type == Identifier)
					cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<unsigned int>(tokens[i].value) << ")��" << SymbolTable[get<unsigned int>(tokens[i].value)].ID << endl;
				else
					cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<int>(tokens[i].value) << ')' << endl;
		}

	inputb = new(nothrow)InputBuffer(path);
	inputb->filter_comments();

	Scanner newscanner(*inputb);
	newscanner.LexicalAnalysis();
	vector<SymbolTableEntry> SymbolTable = newscanner.GetSymbolTable();
	vector<Token> tokens = newscanner.GetTokens();
	for (int i = 0; i < tokens.size(); ++i)
		if (tokens[i].type == Identifier)
			cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<unsigned int>(tokens[i].value) << ")��" << SymbolTable[get<unsigned int>(tokens[i].value)].ID << endl;
		else
			cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<int>(tokens[i].value) << ')' << endl;
	delete inputb;
	return 0;
}