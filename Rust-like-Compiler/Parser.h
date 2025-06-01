#pragma once
#include "LexicalAnalyzer.h"
#include <unordered_map>
#include <set>

//����ʽ�еķ���
struct Symbol
{
	bool isTerminal;
	union {
		enum TokenType terminalId;//�Ŵ洢ΪEPSILON
		unsigned int nonterminalId;
	};
	std::string name;
	Symbol(const enum TokenType tokenType);//����Ϊ�ս��
	Symbol(const unsigned int nonterminalId, const std::string symbolName);//����Ϊ���ս��
	bool operator==(const Symbol& other) const;
	bool operator!=(const Symbol& other) const;
};

//����ʽ
struct Production
{
	Symbol left;
	std::vector<Symbol> right;//������֦ţ�Ϊ��������Ϊ��
	Production(const Symbol& left, const std::vector<Symbol>& right);
};

//LR1��Ŀ
struct LR1Item
{
	int productionIndex;
	int dotPosition;
	enum TokenType lookahead;//�Ŵ洢ΪEPSILON

	LR1Item(int productionIndex, int dotPosition, enum TokenType lookahead);
	bool operator<(const LR1Item& other) const;
	bool operator==(const LR1Item& other) const;
};

//LR1��Ŀ��
struct LR1ItemSet
{
	std::set<LR1Item> items;
	bool operator==(const LR1ItemSet& other) const;
};

//LR1Action��Ķ���
enum Action { shift, reduce, accept, error };
//LR1������ı���
struct ActionTableEntry
{
	enum Action act = Action::error;
	int num;
	bool operator==(const ActionTableEntry& other) const;
};

//�﷨����������ʾ
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
	Token look;																		//��ǰ�Ĵʷ���Ԫ
	std::vector<Production> productions;											//���в���ʽ
	std::unordered_map<std::string, unsigned int> nonTerminals;						//����ʽ�г��ֵ����з��ս��<����,���>
	std::vector<std::set<enum TokenType>>firsts;									//���з��ս����FIRST���ϣ��±�Ϊ��nonTerminals�е����
	std::vector<LR1ItemSet> Itemsets;												//����LR1��Ŀ��
	std::vector<std::vector<ActionTableEntry>> actionTable;							//������Itemsets�±꣬������TokenTypeֵ
	std::vector<std::vector<int>> gotoTable;										//������Itemsets�±꣬������nonTerminals�±�
	std::vector<Production> reduceProductionLists;									//��Լ���̵Ĳ���ʽ
	std::vector<ParseError> parseErrors;											//��Լ�����������Ĵ���
	void GetToken();																//���ôʷ���������ȡ��һ��token���洢��look
	void LoadGrammar(const std::string filepath);									//�����ļ������в���ʽ��productions
	void augmentProduction();														//�ع��ķ�����productions�ĵ�һ������ʽ����Ϊ�ع��ķ��Ҳ�
	const Symbol GetNonTerminal(const std::string name);							//���ݷ��ս���������ɷ��ս��symbol��û���������nonTerminals��������
	const enum TokenType GetTerminalType(const std::string name);					//�����ս�����������ս��TokenType��û���򷵻�TokenType::None��������
	void ComputeFirsts();															//����{���з��ս����FIRST����}firsts
	const std::set<enum TokenType> First(const std::vector<Symbol>& symbols);		//���ݾ��ͣ�symbols������FIRST(symbols)
	const LR1ItemSet Closure(LR1ItemSet itemset);									//����LR1��Ŀ����itemset������CLOSURE(symbols)
	const LR1ItemSet Goto(const LR1ItemSet& itemset, const Symbol& sym);			//����LR1��Ŀ���ͷ��ŷ���GOTO(itemset,sym)
	void addReduceEntry(int ItemsetsIndex);											//ΪLR1��Ŀ��actionTable���reduce����
	void writeActionTable(int itemset, int terminal, const ActionTableEntry& entry);//дactionTable�������ͻ����Ҫ����Ϊ��ͻʱ�����ʾ
	void Items();																	//�����ķ�productions��������LR1��Ŀ��
	void AddParseError(int line, int column, int length, const std::string& message);//����Լ�����������Ĵ������parseErrors
public:
	Parser(Scanner& lexer, const std::string filepath);
	void SyntaxAnalysis();															//�﷨���������չ�Լ˳��ʹ�õĲ���ʽ�洢��reduceProductionLists
	const std::vector<Production>& GetProductions() const;							//����{���в���ʽ}productions
	const std::unordered_map<std::string, unsigned int>& GetNonTerminals() const;	//����{���з��ս�����������ӳ���}nonTerminals
	const std::vector<std::set<enum TokenType>>& GetFirsts() const;					//����{���з��ս����FIRST����}firsts
	const std::vector<LR1ItemSet>& GetItemsets() const;								//����{����LR1��Ŀ��}Itemsets
	const std::vector<std::vector<ActionTableEntry>>& GetActionTable() const;		//����{Action��}actionTable
	const std::vector<std::vector<int>>& GetGotoTable() const;						//����{Goto��}gotoTable
	void printParsingTables() const;												//��ӡչʾLR1������{Action��}actionTable��{Goto��}gotoTable
	const std::vector<Production>& getReduceProductionLists() const;				//����{��Լ���̵Ĳ���ʽ}reduceProductionLists
	const std::vector<ParseError>& GetParseErrors() const;							//����{��Լ���̵Ĵ���}parseErrors
	void printSyntaxTree() const;													//��ӡ�﷨��
	void saveToFile(const std::string& filepath) const;
	Parser(Scanner& lexer, const std::string& filepath, bool fromFile);
};
