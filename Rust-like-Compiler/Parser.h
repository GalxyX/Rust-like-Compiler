#pragma once
#include "LexicalAnalyzer.h"
#include <unordered_map>
#include <set>

//产生式中的符号
struct Symbol
{
	bool isTerminal;
	union {
		enum TokenType terminalId;//ε存储为EPSILON
		unsigned int nonterminalId;
	};
	std::string name;
	Symbol(const enum TokenType tokenType);//符号为终结符
	Symbol(const unsigned int nonterminalId, const std::string symbolName);//符号为非终结符
	bool operator==(const Symbol& other) const;
	bool operator!=(const Symbol& other) const;
};

//产生式
struct Production
{
	Symbol left;
	std::vector<Symbol> right;//不会出现ε，为ε则容器为空
	Production(const Symbol& left, const std::vector<Symbol>& right);
};

//LR1项目
struct LR1Item
{
	int productionIndex;
	int dotPosition;
	enum TokenType lookahead;//ε存储为EPSILON

	LR1Item(int productionIndex, int dotPosition, enum TokenType lookahead);
	bool operator<(const LR1Item& other) const;
	bool operator==(const LR1Item& other) const;
};

//LR1项目集
struct LR1ItemSet
{
	std::set<LR1Item> items;
	bool operator==(const LR1ItemSet& other) const;
};

//LR1Action表的动作
enum Action { shift, reduce, accept, error };
//LR1分析表的表项
struct ActionTableEntry
{
	enum Action act = Action::error;
	int num;
	bool operator==(const ActionTableEntry& other) const;
};

//语法分析错误提示
struct ParseError {
	int line;
	int column;
	int length;
	std::string message;
};

class Parser
{
	//static const enum TokenType EPSILON = static_cast<enum TokenType>(999);
	Scanner& lexer;
	Token look;																		//当前的词法单元
	std::vector<Production> productions;											//所有产生式
	std::unordered_map<std::string, unsigned int> nonTerminals;						//产生式中出现的所有非终结符<名称,序号>
	std::vector<std::set<enum TokenType>>firsts;									//所有非终结符的FIRST集合，下标为在nonTerminals中的序号
	std::vector<LR1ItemSet> Itemsets;												//所有LR1项目集
	std::vector<std::vector<ActionTableEntry>> actionTable;							//横坐标Itemsets下标，纵坐标TokenType值
	std::vector<std::vector<int>> gotoTable;										//横坐标Itemsets下标，纵坐标nonTerminals下标
	std::vector<Production> reduceProductionLists;									//规约过程的产生式
	std::vector<ParseError> parseErrors;											//归约过程中遇到的错误
	void GetToken();																//调用词法分析器读取下一个token并存储到look
	void LoadGrammar(const std::string filepath);									//读入文件中所有产生式至productions
	void augmentProduction();														//拓广文法，将productions的第一个产生式左部作为拓广文法右部
	const Symbol GetNonTerminal(const std::string name);							//根据非终结符名称生成非终结符symbol（没有则添加至nonTerminals）并返回
	const enum TokenType GetTerminalType(const std::string name);					//根据终结符名称生成终结符TokenType（没有则返回TokenType::None）并返回
	void ComputeFirsts();															//计算{所有非终结符的FIRST集合}firsts
	const std::set<enum TokenType> First(const std::vector<Symbol>& symbols);		//根据句型（symbols）返回FIRST(symbols)
	const LR1ItemSet Closure(LR1ItemSet itemset);									//根据LR1项目集（itemset）返回CLOSURE(symbols)
	const LR1ItemSet Goto(const LR1ItemSet& itemset, const Symbol& sym);			//根据LR1项目集和符号返回GOTO(itemset,sym)
	void addReduceEntry(int ItemsetsIndex);											//为LR1项目在actionTable添加reduce表项
	void writeActionTable(int itemset, int terminal, const ActionTableEntry& entry);//写actionTable，处理冲突，主要内容为冲突时输出提示
	void Items();																	//根据文法productions生成所有LR1项目集
	void AddParseError(int line, int column, int length, const std::string& message);//将归约过程中遇到的错误存入parseErrors
public:
	Parser(Scanner& lexer, const std::string filepath);
	void SyntaxAnalysis();															//语法分析，按照归约顺序将使用的产生式存储在reduceProductionLists
	const std::vector<Production>& GetProductions() const;							//返回{所有产生式}productions
	const std::unordered_map<std::string, unsigned int>& GetNonTerminals() const;	//返回{所有非终结符的名：序号映射表}nonTerminals
	const std::vector<std::set<enum TokenType>>& GetFirsts() const;					//返回{所有非终结符的FIRST集合}firsts
	const std::vector<LR1ItemSet>& GetItemsets() const;								//返回{所有LR1项目集}Itemsets
	const std::vector<std::vector<ActionTableEntry>>& GetActionTable() const;		//返回{Action表}actionTable
	const std::vector<std::vector<int>>& GetGotoTable() const;						//返回{Goto表}gotoTable
	void printParsingTables() const;												//打印展示LR1分析表：{Action表}actionTable和{Goto表}gotoTable
	const std::vector<Production>& getReduceProductionLists() const;				//返回{规约过程的产生式}reduceProductionLists
	const std::vector<ParseError>& GetParseErrors() const;							//返回{归约过程的错误}parseErrors
	void printSyntaxTree() const;													//打印语法树
	void saveToFile(const std::string& filepath) const;
	Parser(Scanner& lexer, const std::string& filepath, bool fromFile);
};
