#include "LexicalAnalyzer.h"
using namespace std;

// 生成名称字符串数组
const char* TokenTypeNames[] = {
#define X(name) #name,
	TOKEN_TYPES
#undef X
};

// 通过枚举值获取名称（需检查范围）
static const char* TokenTypeToString(TokenType type) {
	int index = static_cast<int>(type);
	if (index >= 0 && index < sizeof(TokenTypeNames) / sizeof(char*))
		return TokenTypeNames[index];
	return "Unknown";
}

int main()
{
	string path = "rust/test.rs";
	InputBuffer newinput(path);
	newinput.filter_comments();

	Scanner newscanner(newinput);
	newscanner.LexicalAnalysis();
	vector<SymbolTableEntry> SymbolTable = newscanner.GetSymbolTable();
	vector<Token> tokens = newscanner.GetTokens();
	for (int i = 0; i < tokens.size(); ++i)
		if (tokens[i].type == Identifier)
			cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<unsigned int>(tokens[i].value) << ")：" << SymbolTable[get<unsigned int>(tokens[i].value)].ID << endl;
		else
			cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<int>(tokens[i].value) << ')' << endl;
	return 0;
}