#include "Parser.h"
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <array>
#include <stack>
using namespace std;

Symbol::Symbol(const enum TokenType tokenType) :isTerminal(true), terminalId(tokenType)
{
	string s = TokenTypeToString(tokenType);
	if (s == OutOfRangeTokenType)
		throw runtime_error("δ������ս��");
	else
		name = s;
}

Symbol::Symbol(const unsigned int nonterminalId, const string symbolName) :isTerminal(false), nonterminalId(nonterminalId), name(symbolName)
{
}

bool Symbol::operator==(const Symbol& other) const
{
	if (isTerminal != other.isTerminal)
		return false;
	if (isTerminal)
		return terminalId == other.terminalId;
	else
		return nonterminalId == other.nonterminalId;
}

bool Symbol::operator!=(const Symbol& other) const
{
	return !(*this == other);
}

Production::Production(const Symbol& left, const vector<Symbol>& right) :left(left), right(right)
{
}

LR1Item::LR1Item(int productionIndex, int dotPosition, enum TokenType lookahead) :productionIndex(productionIndex), dotPosition(dotPosition), lookahead(lookahead)
{
}

bool LR1Item::operator<(const LR1Item& other) const
{
	if (productionIndex != other.productionIndex)
		return productionIndex < other.productionIndex;
	if (dotPosition != other.dotPosition)
		return dotPosition < other.dotPosition;
	return int(lookahead) < int(other.lookahead);
}

bool LR1Item::operator==(const LR1Item& other) const
{
	return productionIndex == other.productionIndex && dotPosition == other.dotPosition && lookahead == other.lookahead;
}


bool LR1ItemSet::operator==(const LR1ItemSet& other) const
{
	return items == other.items;
}

bool ActionTableEntry::operator==(const ActionTableEntry& other) const
{
	return act == other.act && num == other.num;
}

void Parser::GetToken()
{
	look = lexer.scan();
	return;
}

void Parser::LoadGrammar(const string filepath)
{
	//���ļ�
	ifstream readFile(filepath, ios::in);
	if (!readFile.is_open())
	{
		DEBUG_CERR << "�޷����﷨����ʽ�ļ�: " << filepath << endl;
		throw runtime_error("�޷����﷨����ʽ�ļ�: " + filepath);
	}
	//��ȡ����ʽ
	string line;
	//��ȡÿһ��
	while (getline(readFile, line)) {
		//��������
		if (line.empty())
			continue;
		//�ҵ����ҷָ�����һ��->
		size_t arrowPos = line.find("->");
		if (arrowPos == string::npos) {
			DEBUG_CERR << "���Ը�ʽ����ȷ����" << line << endl;
			continue;
		}
		//��ȡ��
		string leftstr = line.substr(0, arrowPos);
		leftstr.erase(remove_if(leftstr.begin(), leftstr.end(), [](char ch) {return ch == ' '; }), leftstr.end());//ɾ��������λ�ÿո�
		Symbol leftSymbol = GetNonTerminal(leftstr);
		//��ȡ�Ҳ�����Ӳ���ʽ
		string rightstr = line.substr(arrowPos + 2);
		istringstream is(rightstr);//->�Ҳ������ַ�
		string singleSymbol;//->�Ҳ��ÿո�ָ���<����>
		vector<Symbol> rightSymbols;//ÿ������ʽ�Ҳ������<����>
		while (is >> singleSymbol) {
			if (singleSymbol == "|") {//��������ʽ���
				if (!rightSymbols.empty())
					productions.push_back(Production(leftSymbol, rightSymbols));
				rightSymbols.clear();
			}
			else {
				if (singleSymbol == "Epsilon") {
					rightSymbols.clear();
					productions.push_back(Production(leftSymbol, rightSymbols));
				}
				else {
					TokenType token = GetTerminalType(singleSymbol);
					if (token == TokenType::None)//Ϊ���ս��
						rightSymbols.push_back(GetNonTerminal(singleSymbol));
					else
						rightSymbols.push_back(Symbol(token));
				}
			}
		}
		if (!rightSymbols.empty())
			productions.push_back(Production(leftSymbol, rightSymbols));
	}
	readFile.close();
}

void Parser::augmentProduction()
{
	if (productions.empty())
		throw runtime_error("�޲���ʽ���޷������ع��ķ�");
	vector<Production>::iterator it = productions.begin();
	productions.insert(it, Production(GetNonTerminal(""), vector<Symbol>{ it->left }));
}

const Symbol Parser::GetNonTerminal(const string name)
{
	auto it = nonTerminals.find(name);
	if (it == nonTerminals.end()) {
		//��map������µķ��ս��
		unsigned int nonterminalId = nonTerminals.size();
		nonTerminals[name] = nonterminalId;
		return Symbol(nonterminalId, name);
	}
	else
		return Symbol(it->second, name);
}

const enum TokenType Parser::GetTerminalType(const string name)
{
	//Ԥ�����ս��ӳ���ϵ
	static const unordered_map<string, TokenType> terminalMap = {
		//�����ӳ��
		{"+", TokenType::Addition},
		{"-", TokenType::Subtraction},
		{"*", TokenType::Multiplication},
		{"/", TokenType::Division},
		{"=", TokenType::Assignment},
		{"==", TokenType::Equality},
		{">", TokenType::GreaterThan},
		{">=", TokenType::GreaterOrEqual},
		{"<", TokenType::LessThan},
		{"<=", TokenType::LessOrEqual},
		{"!=", TokenType::Inequality},
		{"(", TokenType::ParenthesisL},
		{")", TokenType::ParenthesisR},
		{"{", TokenType::CurlyBraceL},
		{"}", TokenType::CurlyBraceR},
		{"[", TokenType::SquareBracketL},
		{"]", TokenType::SquareBracketR},
		{";", TokenType::Semicolon},
		{":", TokenType::Colon},
		{",", TokenType::Comma},
		{"->", TokenType::ArrowOperator},
		{".", TokenType::DotOperator},
		{"..", TokenType::RangeOperator},
		{"!", TokenType::Not},
		{"%", TokenType::Modulo},
		{"%=", TokenType::ModuloAssign},
		{"&", TokenType::BitAnd},
		{"&=", TokenType::BitAndAssign},
		{"&&", TokenType::LogicalAnd},
		{"|", TokenType::BitOr},
		{"|=", TokenType::BitOrAssign},
		{"||", TokenType::LogicalOr},
		{"?", TokenType::ErrorPropagation},
		{"*=", TokenType::MultiplicationAssign},
		{"+=", TokenType::AdditionAssign},
		{"-=", TokenType::SubtractionAssign},
		{"/=", TokenType::DivisionAssign},
		{"<<", TokenType::LeftShift},
		{"<<=", TokenType::LeftShiftAssign},
		{"=>", TokenType::Arrowmatch},
		{">>", TokenType::RightShift},
		{">>=", TokenType::RightShiftAssign},
		{"@", TokenType::PatternBinding},
		{"^", TokenType::BitXor},
		{"^=", TokenType::BitXorAssign},
		{"\"", TokenType::DoubleQuote},
		{"'", TokenType::SingleQuote},
		//�ؼ���ӳ��
		{"i8", TokenType::I8},
		{"u8", TokenType::U8},
		{"i16", TokenType::I16},
		{"u16", TokenType::U16},
		{"i32", TokenType::I32},
		{"u32", TokenType::U32},
		{"i64", TokenType::I64},
		{"u64", TokenType::U64},
		{"i128", TokenType::I128},
		{"u128", TokenType::U128},
		{"isize", TokenType::ISIZE},
		{"usize", TokenType::USIZE},
		{"f32", TokenType::F32},
		{"f64", TokenType::F64},
		{"bool", TokenType::BOOL},
		{"char", TokenType::CHAR},
		{"unit", TokenType::UNIT},
		{"array", TokenType::ARRAY},
		{"let", TokenType::LET},
		{"if", TokenType::IF},
		{"else", TokenType::ELSE},
		{"while", TokenType::WHILE},
		{"return", TokenType::RETURN},
		{"mut", TokenType::MUT},
		{"fn", TokenType::FN},
		{"for", TokenType::FOR},
		{"in", TokenType::IN},
		{"loop", TokenType::LOOP},
		{"break", TokenType::BREAK},
		{"continue", TokenType::CONTINUE},
		//������
		{"ID", TokenType::Identifier},
		{"NUM", TokenType::i32_},
		{"CHAR", TokenType::char_},
		{"#", TokenType::End}
	};
	auto it = terminalMap.find(name);
	if (it != terminalMap.end())
		return TokenType(it->second);
	else
		return TokenType::None;
}

void Parser::ComputeFirsts()
{
	bool changed = true;//����ѭ�����в���ʽ�Ƿ���FIRST���ϸı�
	bool noepsilon = true;//������ʽ�Ҳ�ÿһ������ѭ����ǰ�����Ƿ���FIRST���а�����
	bool allepsilon = true;//������ʽ�Ҳ�ÿһ������ѭ�����з����Ƿ�FIRST���о�������
	firsts.resize(nonTerminals.size());
	while (changed) {
		changed = false;
		for (Production prod : productions) {
			const Symbol left = prod.left;
			if (left.isTerminal)
				continue;//����������Ϊ���ս����ִ�в���
			set<TokenType>& leftfirst = firsts[left.nonterminalId];//��ǰ����ʽ�����ս����first����
			//����ʽ�Ҳ�Ϊ��
			if (prod.right.empty()) {
				if (leftfirst.insert(EPSILON).second)//��Ϊ�����
					changed = true;
			}
			//����ʽ�Ҳ࿪ʼΪ���ǣ��ս����Ϊ�ս���ڵ�һ��ѭ����break
			else {
				allepsilon = true;
				for (Symbol sym : prod.right) {//����ʽ�Ҳ��ÿһ������
					if (sym.isTerminal) {//���ս�������FIRST(���ս��)�����
						if (leftfirst.insert(sym.terminalId).second)//�ս��Ϊ�����
							changed = true;
						allepsilon = false;
						break;//����
					}
					else {//�����ս�������FIRST(�÷��ս��)���������
						noepsilon = true;
						for (TokenType firstele : firsts[sym.nonterminalId])//���ս����ÿһ����֪FIRSTԪ��
							if (firstele != EPSILON) {//���FIRST(�÷��ս��)-{��}
								if (leftfirst.insert(firstele).second)//�ս��Ϊ�����
									changed = true;
							}
							else
								noepsilon = false;
						if (noepsilon) {//FIRST���������û�Ц�����ҵ��÷��ս�������
							allepsilon = false;
							break;
						}
					}
				}
				if (allepsilon && leftfirst.insert(EPSILON).second)//����ʽ�Ҳ����з���FIRST���������ţ���Ӧŵ�FIRST(������)
					changed = true;
			}
		}
	}
}

const set<enum TokenType> Parser::First(const vector<Symbol>& symbols)
{
	set<TokenType> firsteles;
	bool allepsilon = true;
	for (Symbol sym : symbols) {//���͵�ÿһ������
		bool noepsilon = true;
		if (sym.isTerminal) {//���ս�����ս�����뼯�Ϻ����
			firsteles.insert(sym.terminalId);
			allepsilon = false;
			break;//����
		}
		else {//�����ս��
			for (TokenType firstele : firsts[sym.nonterminalId])//���ս����ÿһ��FIRSTԪ��
				if (firstele != EPSILON)//���FIRST(�÷��ս��)-{��}
					firsteles.insert(firstele);
				else
					noepsilon = false;
			if (noepsilon) {//FIRST���������û�Ц�����ҵ��÷��ս�������
				allepsilon = false;
				break;
			}
		}
	}
	if (allepsilon)//symbolsΪ��/symbols���з���FIRST���������ţ���Ӧŵ�FIRST(symbols)
		firsteles.insert(EPSILON);
	return firsteles;
}

const LR1ItemSet Parser::Closure(LR1ItemSet itemset)
{
	bool added = true;
	while (added) {
		added = false;
		//I�е�ÿ����[A������B��,a]
		for (const LR1Item& item : itemset.items) {
			//�ҳ�ÿ��LR1��Ŀ�Ĳ���ʽ�Ҳ�
			vector<Symbol> prodRight = productions[item.productionIndex].right;//��B��
			int dotPos = item.dotPosition;
			if (dotPos < int(prodRight.size())) {//�ƽ��A������B��,a�����п���Ϊ�ţ�
				Symbol nextSym = prodRight[dotPos];//B
				//��Ϊ���ս��ʱ�������
				if (!nextSym.isTerminal) {
					//���a
					vector<Symbol> beta;//��a
					TokenType lookahead = item.lookahead;//a
					for (size_t i = dotPos + 1; i < prodRight.size(); ++i)//����£���=��ʱδ����ѭ��
						beta.push_back(prodRight[i]);
					if (beta.empty() || lookahead != EPSILON)//�¡٦�,a=�ţ�������a
						beta.push_back(Symbol(lookahead));//����a
					//G'�е�ÿ������ʽB����
					set<TokenType>firstbeta = First(beta);//FIRST(��a)
					for (size_t i = 0; i < productions.size(); i++) {
						if (productions[i].left != nextSym)
							continue;
						//FIRST(��a)�е�ÿ���ս����b
						for (TokenType b : firstbeta) {
							if (itemset.items.insert(LR1Item(i, 0, b)).second)//�ս��Ϊ�����
								added = true;
						}
					}
				}
			}
			else if (dotPos > int(prodRight.size()))
				throw runtime_error("LR1��Ŀ�С�λ�÷Ƿ�");
			//�������ֱ�ӷ��أ��������
		}
	}
	return itemset;
}

const LR1ItemSet Parser::Goto(const LR1ItemSet& itemset, const Symbol& sym)
{
	LR1ItemSet gotoset;//��J��ʼ��Ϊ�ռ�;
	for (const LR1Item& item : itemset.items) {//I�е�ÿ���A������X��,a]
		int dotPosition = item.dotPosition;
		int productionIndex = item.productionIndex;
		if (dotPosition < int(productions[productionIndex].right.size()) && productions[productionIndex].right[dotPosition] == sym)//X�²�Ϊ����XΪsym
			gotoset.items.insert(LR1Item(productionIndex, dotPosition + 1, item.lookahead));
	}
	return Closure(gotoset);
}

void Parser::addReduceEntry(int ItemsetsIndex)
{
	const LR1ItemSet& itemset = Itemsets[ItemsetsIndex];
	for (const LR1Item& item : itemset.items)
		if (item.dotPosition > int(productions[item.productionIndex].right.size()))
			throw runtime_error("LR1��Ŀ�С�λ�÷Ƿ�");
		else if (item.dotPosition == productions[item.productionIndex].right.size())//A->B��,a
			if (item.productionIndex)
				writeActionTable(ItemsetsIndex, item.lookahead, ActionTableEntry{ Action::reduce , item.productionIndex });//��reduce��д����
			else
				writeActionTable(ItemsetsIndex, item.lookahead, ActionTableEntry{ Action::accept , item.productionIndex });//��reduce��д����
}

void Parser::writeActionTable(int itemset, int terminal, const ActionTableEntry& entry)
{
	//���һ����Լ/��Լ��ͻʱ,ѡ���ڹ�Լ������ǰ����Ǹ���ͻ����ʽ
	//�������/��Լ��ͻʱ����ѡ�����롣���������ȷ�ؽ������Ϊ����else�����Զ�����������/��Լ��ͻ
	ActionTableEntry& currEntry = actionTable[itemset][terminal];

	if (currEntry == entry)//����д����д���޳�ͻ
		return;
	else if (currEntry.act == Action::error) {//δ��д���޳�ͻ
		currEntry = entry;//��д����
		return;
	}
	else if (currEntry.act == Action::accept || entry.act == Action::accept) {//�����ܵĳ�ͻ
		DEBUG_CERR << "LR1������accept��ͻ��Ӧ����߼�" << endl;
		return;
	}
	else if (entry.act == Action::error) {//�����ܵĳ�ͻ
		DEBUG_CERR << "��Ӧ��LR1������������error��Ӧ����߼�" << endl;
		return;
	}
	else if (currEntry.act == Action::shift && entry.act == Action::shift) {//�����ܵĳ�ͻ
		DEBUG_CERR << "LR1�������ƽ�-�ƽ���ͻ��Ӧ����߼�" << endl;
		return;
	}
	//������������¿��ܲ����ĳ�ͻ����Լ-��Լ��ͻ���ƽ�-��Լ��ͻ������ʹ�����ȼ�����򱨸��ͻ
	//�����ǰ״̬����Ϣ
	DEBUG_CERR << "��ǰ״̬� I" << itemset << ":" << endl;
	for (const LR1Item& item : Itemsets[itemset].items) {
		const Production& prod = productions[item.productionIndex];
		DEBUG_CERR << "  " << prod.left.name << " -> ";
		for (size_t j = 0; j < prod.right.size(); j++) {
			if (j == item.dotPosition)
				DEBUG_CERR << "�� ";
			DEBUG_CERR << prod.right[j].name << " ";
		}
		if (item.dotPosition == prod.right.size())
			DEBUG_CERR << "��";
		DEBUG_CERR << ", " << TokenTypeToString(item.lookahead) << endl;
	}
	//�����ͬ�ĳ�ͻ��Ϣ
	DEBUG_CERR << "״̬� I" << itemset << " ��Է��� " << TokenTypeToString(TokenType(terminal)) << " ����";
	//��Լ-��Լ��ͻ
	if (currEntry.act == Action::reduce && entry.act == Action::reduce) {//��Լ-��Լ��ͻ
		DEBUG_CERR << "LR1�������Լ-��Լ��ͻ" << endl;
		DEBUG_CERR << "	��Լ����1: ʹ�ò���ʽ " << productions[currEntry.num].left.name << " -> ";
		for (const Symbol& sym : productions[currEntry.num].right)
			DEBUG_CERR << sym.name << " ";
		DEBUG_CERR << endl << "	��Լ����2: ʹ�ò���ʽ " << productions[entry.num].left.name << " -> ";
		for (const Symbol& sym : productions[entry.num].right)
			DEBUG_CERR << sym.name << " ";
		DEBUG_CERR << endl << "�������: ��Լ/��Լ��ͻʱ��ѡ���ڹ�Լ������ǰ����Ǹ���ͻ����ʽ" << endl;
		if (entry.num < currEntry.num) {
			DEBUG_CERR << "	������Լ����2" << endl << endl;
			currEntry = entry;
		}
		else
			DEBUG_CERR << "	������Լ����1" << endl << endl;
		return;
	}
	//�ƽ�-��Լ��ͻ
	int shiftnum = currEntry.act == Action::shift ? currEntry.num : entry.num;
	int reducenum = currEntry.act == Action::reduce ? currEntry.num : entry.num;
	DEBUG_CERR << "LR1�������ƽ�-��Լ��ͻ" << endl;
	DEBUG_CERR << "�ƽ�����: �ƽ���״̬ " << shiftnum << endl;
	DEBUG_CERR << "��Լ����: ʹ�ò���ʽ " << productions[reducenum].left.name << " -> ";
	for (const Symbol& sym : productions[reducenum].right)
		DEBUG_CERR << sym.name << " ";
	DEBUG_CERR << endl;
	DEBUG_CERR << "�������: ����/��Լ��ͻʱ����ѡ������" << endl << endl;
	if (currEntry.act == Action::reduce && entry.act == Action::shift)//�ƽ�-��Լ��ͻ
		currEntry = entry; //ѡ���ƽ�����
	//else if (currEntry.act == Action::shift && entry.act == Action::reduce)//�ƽ�-��Լ��ͻ
	//	//�޲���
}

void Parser::Items()
{
	//��C��ʼ��Ϊ{CLOSURE}({[S'����S,$]});
	Itemsets.clear();
	Itemsets.push_back(Closure(LR1ItemSet{ set<LR1Item>{LR1Item(0, 0, TokenType::End)} }));//productionIndex��augmentProduction���ع�ʱ���λ��Ҫ��Ӧ

	int nonTerminalsNum = nonTerminals.size();
	actionTable.emplace_back(vector<ActionTableEntry>(TokenType::End + 1));//actionTable������Itemsetsͬ���仯
	gotoTable.emplace_back(vector<int>(nonTerminalsNum, -1));//gotoTable������Itemsetsͬ���仯

	bool added = true;
	while (added) {//ѭ��ֱ���������µ�����뵽C��
		added = false;
		for (int index = 0; index < int(Itemsets.size()); ++index) {//C�е�ÿ���I
			LR1ItemSet I = Itemsets[index];
			//ÿ���ķ�����X
			for (int i = TokenType::None + 1; i <= TokenType::End; ++i) {//ÿ���ķ�����X���ս����
				Symbol X((TokenType(i)));
				const LR1ItemSet gotoset = Goto(I, X);
				if (!gotoset.items.empty() && find(Itemsets.begin(), Itemsets.end(), gotoset) == Itemsets.end()) {//GOTO(I,X)�ǿ��Ҳ���C��
					Itemsets.push_back(gotoset);//��GOTO(I,X)����C��

					actionTable.emplace_back(vector<ActionTableEntry>(TokenType::End + 1));//actionTable������Itemsetsͬ���仯
					gotoTable.emplace_back(vector<int>(nonTerminalsNum, -1));//gotoTable������Itemsetsͬ���仯
					added = true;
				}
				if (!gotoset.items.empty())
					writeActionTable(index, i, ActionTableEntry{ Action::shift, distance(Itemsets.begin(), find(Itemsets.begin(), Itemsets.end(), gotoset)) });//��shift��д����
			}
			for (pair<const string, unsigned int> sym : nonTerminals) {//ÿ���ķ�����X�����ս����
				Symbol X = GetNonTerminal(sym.first);//////////////////////////ȥ��Symbol.name���Ժ�ʹ��Symbol X(sym.second);//////////////////////////
				const LR1ItemSet gotoset = Goto(I, X);
				vector<LR1ItemSet>::iterator it = find(Itemsets.begin(), Itemsets.end(), gotoset);//GOTO����LR1��Ŀ��������
				if (!gotoset.items.empty() && it == Itemsets.end()) {//GOTO(I,X)�ǿ��Ҳ���C��
					Itemsets.push_back(gotoset);//��GOTO(I,X)����C��

					actionTable.emplace_back(vector<ActionTableEntry>(TokenType::End + 1));//actionTable������Itemsetsͬ���仯
					gotoTable.emplace_back(vector<int>(nonTerminalsNum, -1));//gotoTable������Itemsetsͬ���仯
					added = true;
				}
				if (!gotoset.items.empty())
					gotoTable[index][sym.second] = distance(Itemsets.begin(), find(Itemsets.begin(), Itemsets.end(), gotoset));//��goto��д����
			}
			addReduceEntry(index);
		}
	}
}

void Parser::AddParseError(int line, int column, int length, const std::string& message)
{
	parseErrors.push_back({ line, column, length, message });
}

Parser::Parser(Scanner& lexer, const string filepath) :lexer(lexer), look(TokenType::None)
{
	LoadGrammar(filepath);
}

void Parser::SyntaxAnalysis()
{
	augmentProduction();
	ComputeFirsts();
	Items();

	stack<int> stateStack;//״̬ջ
	stack<Symbol> tokenStack;//����ջ
	stateStack.push(0);//�ع��ķ�����ʼ��Ԫ��ӦLR1��Ŀ����ʼ
	tokenStack.push(TokenType::End);

	GetToken();//��lookΪ��#�ĵ�һ������
	while (true) {
		int s = stateStack.top();//��s��ջ����״̬
		ActionTableEntry action = actionTable[s][look.type];//s��look��Ӧ�ж�
		if (action.act == Action::shift) {//ACTION[s,a]=������״̬��sx��
			stateStack.push(action.num);//����״̬ѹ��ջ��
			tokenStack.push(look.type);
			GetToken();
		}
		else if (action.act == Action::reduce) {//ACTION[s,a]=��ԼA����
			int betalength = productions[action.num].right.size();//|��|
			for (int i = 0; i < betalength; ++i) {//��ջ�е���|��|������;
				stateStack.pop();
				tokenStack.pop();
			}
			int t = stateStack.top();//��tΪ��ǰ��ջ��״̬;
			unsigned int Aid = productions[action.num].left.nonterminalId;//A
			stateStack.push(gotoTable[t][Aid]);//��GOTO[t,A]ѹ��ջ��
			tokenStack.push(Symbol(Aid, productions[action.num].left.name));
			reduceProductionLists.push_back(productions[action.num]);//�������ʽA����
		}
		else if (action.act == Action::accept)
			break;//�﷨�������
		else if (action.act == Action::error) {
			//����չʾ
			lexer.ProcError(string(TokenTypeToString(look.type)) + "�����Ų����ϸ������﷨����");
			//��¼����
			AddParseError(look.line, look.column, look.length, string(TokenTypeToString(look.type)) + "�����Ų����ϸ������﷨����");
			//չʾ��״̬�ɽ��ܵĴʷ���Ԫ����
			DEBUG_CERR << "Ԥ�ڵĴʷ���Ԫ: ";
			bool hasExpected = false;
			for (int i = 1; i <= int(TokenType::End); i++)
				if (actionTable[s][i].act != Action::error) {
					if (hasExpected)
						DEBUG_CERR << ", ";
					DEBUG_CERR << TokenTypeToString(TokenType(i));
					hasExpected = true;
				}
			if (!hasExpected)
				DEBUG_CERR << "�޷�ȷ��";
			DEBUG_CERR << endl;
			DEBUG_CERR << "������ǰ�ʷ���Ԫ" << endl << endl;
			if (look.type == TokenType::End)
				break;
			GetToken();
		}
		////�����ǰջ������
		//stack<Symbol> tempStack = tokenStack;
		//while (!tempStack.empty()) {
		//	cout << (tempStack.top().isTerminal ? TokenTypeToString(tempStack.top().terminalId) : tempStack.top().name) << ' ';
		//	tempStack.pop();
		//}
		//cout << endl;
	}
}

const std::vector<Production>& Parser::GetProductions() const
{
#ifdef ENABLE_NOTING_OUTPUT
	//����鿴
	//��ӡ��ȡ�Ĳ���ʽ�����ڵ���
	cout << "��ȡ�Ĳ���ʽ��" << endl;
	for (size_t i = 0; i < productions.size(); i++) {
		const Production& p = productions[i];
		cout << i << ": " << p.left.name << " -> ";
		for (const Symbol& symbol : p.right) {
			cout << symbol.name << " ";
		}
		cout << endl;
	}
#endif
	return productions;
}

const unordered_map<string, unsigned int>& Parser::GetNonTerminals() const
{
	return nonTerminals;
}

const vector<set<enum TokenType>>& Parser::GetFirsts() const
{
#ifdef ENABLE_NOTING_OUTPUT
	cout << "���з��ŵ�FIRST��" << endl;
	//�ս��
	cout << "���������ս��, FIRST(t) = {t}" << endl;
	for (int i = 0; i <= int(TokenType::End); i++) {
		TokenType token = TokenType(i);
		cout << "FIRST(" << TokenTypeToString(token) << ") = { " << TokenTypeToString(token) << " }" << endl;
	}
	//���ս��
	for (const pair<const string, unsigned int>& entry : nonTerminals) {//����ÿһ�����ս��
		cout << "FIRST(" << entry.first << ")={ ";
		bool first = true;
		for (const TokenType& token : firsts[entry.second]) {//���ս����ÿһ��FIRSTԪ��
			if (!first)
				cout << ", ";
			first = false;

			if (token == EPSILON)
				cout << "��";
			else
				cout << TokenTypeToString(token);
		}
		cout << " }" << endl;
	}
#endif
	return firsts;
}

const vector<LR1ItemSet>& Parser::GetItemsets() const
{
#ifdef ENABLE_NOTING_OUTPUT
	cout << "=============== LR1��� ===============" << endl;
	for (size_t i = 0; i < Itemsets.size(); i++) {
		cout << "I" << i << ":" << endl;
		const LR1ItemSet& itemset = Itemsets[i];
		for (const LR1Item& item : itemset.items) {
			//�������ʽ
			const Production& prod = productions[item.productionIndex];
			cout << "  " << prod.left.name << " -> ";
			//����Ҳಢ�ں���λ�ò�����
			for (size_t j = 0; j < prod.right.size(); j++) {
				if (j == item.dotPosition)
					cout << "�� ";
				cout << prod.right[j].name << " ";
			}
			//����������Ҳ࣬�������׷�ӵ�
			if (item.dotPosition == prod.right.size())
				cout << "��";
			//���ǰհ����
			cout << ", " << (item.lookahead == EPSILON ? "��" : TokenTypeToString(item.lookahead)) << endl;
		}
		cout << endl;
	}
	cout << "=======================================" << endl;
#endif
	return Itemsets;
}

const vector<vector<ActionTableEntry>>& Parser::GetActionTable() const
{
	return actionTable;
}

const vector<vector<int>>& Parser::GetGotoTable() const
{
	return gotoTable;
}

void Parser::printParsingTables() const
{
	cout << "=============== Action�� ===============" << endl;
	cout << "״̬\t";

	//����ս����ͷ
	for (int i = 0; i <= int(TokenType::End); i++) {
		TokenType type = TokenType(i);
		if (type == TokenType::None)
			continue; //����None
		cout << TokenTypeToString(type) << "\t";
	}
	cout << endl;

	//���ÿһ�еĶ���
	for (size_t i = 0; i < actionTable.size(); i++) {
		cout << i << "\t";
		for (int j = 1; j <= int(TokenType::End); j++) {
			TokenType type = TokenType(j);
			const ActionTableEntry& entry = actionTable[i][j];

			//���ݶ������������ͬ�ĸ�ʽ
			switch (entry.act) {
			case Action::shift:
				cout << "s" << entry.num << "\t";
				break;
			case Action::reduce:
				cout << "r" << entry.num << "\t";
				break;
			case Action::accept:
				cout << "acc\t";
				break;
			case Action::error:
			default:
				cout << "-\t";
				break;
			}
		}
		cout << endl;
	}

	cout << "\n=============== Goto�� ===============" << endl;
	cout << "״̬\t";

	//������ս����ͷ
	for (const auto& nt : nonTerminals) {
		cout << nt.first << "\t";
	}
	cout << endl;

	//���ÿһ�е�Ŀ��״̬
	for (size_t i = 0; i < gotoTable.size(); i++) {
		cout << i << "\t";
		for (size_t j = 0; j < gotoTable[i].size(); j++) {
			int target = gotoTable[i][j];
			cout << (target == -1 ? "-" : to_string(target)) << "\t";
		}
		cout << endl;
	}

	cout << "=======================================" << endl;
}

const std::vector<Production>& Parser::getReduceProductionLists() const
{
	return reduceProductionLists;
}

const std::vector<ParseError>& Parser::GetParseErrors() const
{
	return parseErrors;
}

void Parser::printSyntaxTree() const
{
	cout << "\n=============== �﷨�� ===============\n";

	//ʹ��ջ��ģ���Լ���̣��������ṹ
	struct TreeNode {
		Symbol symbol;              //�ڵ����
		vector<TreeNode*> children; //ʹ��ָ����⸴������
		string lexeme;              //����(���ս��)

		TreeNode(const Symbol& s) : symbol(s), lexeme("") {}
		~TreeNode() {
			for (auto child : children)
				delete child;
		}
	};

	stack<TreeNode*> nodeStack;

	//�ӹ�Լ���й����﷨��
	for (const auto& prod : reduceProductionLists) {
		TreeNode* node = new TreeNode(prod.left);

		//��ջ�е��������ʽ�Ҳ�������ͬ�Ľڵ�
		vector<TreeNode*> children;
		for (size_t i = 0; i < prod.right.size(); i++)
			if (!nodeStack.empty()) {
				children.insert(children.begin(), nodeStack.top());
				nodeStack.pop();
			}

		node->children = children;
		nodeStack.push(node);
	}

	//�ݹ��ӡ�﷨���ĸ���lambda����
	auto printTreeNode = [](const TreeNode* node, string prefix, bool isLast, auto& self) -> void {
		if (!node)
			return;

		cout << prefix;

		//��ӡ������
		cout << (isLast ? "������ " : "������ ");

		//��ӡ�ڵ�����
		cout << node->symbol.name;
		if (!node->lexeme.empty())
			cout << " (" << node->lexeme << ")";
		cout << endl;

		//Ϊ�ӽڵ�׼��ǰ׺
		string newPrefix = prefix + (isLast ? "    " : "��   ");

		//�ݹ��ӡ�ӽڵ�
		for (size_t i = 0; i < node->children.size(); i++) {
			bool lastChild = (i == node->children.size() - 1);
			self(node->children[i], newPrefix, lastChild, self);
		}
		};

	//��ӡ������
	if (!nodeStack.empty()) {
		TreeNode* rootNode = nodeStack.top();
		printTreeNode(rootNode, "", true, printTreeNode);

		//�����ڴ�
		while (!nodeStack.empty()) {
			TreeNode* node = nodeStack.top();
			nodeStack.pop();
			delete node;
		}
	}
	else
		cout << "�޷������﷨����\n";

	cout << "=======================================" << endl;
}