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
		throw runtime_error("未定义的终结符");
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
	//打开文件
	ifstream readFile(filepath, ios::in);
	if (!readFile.is_open())
	{
		DEBUG_CERR << "无法打开语法产生式文件: " << filepath << endl;
		throw runtime_error("无法打开语法产生式文件: " + filepath);
	}
	//提取产生式
	string line;
	//读取每一行
	while (getline(readFile, line)) {
		//跳过空行
		if (line.empty())
			continue;
		//找到左右分隔符第一个->
		size_t arrowPos = line.find("->");
		if (arrowPos == string::npos) {
			DEBUG_CERR << "忽略格式不正确的行" << line << endl;
			continue;
		}
		//提取左部
		string leftstr = line.substr(0, arrowPos);
		leftstr.erase(remove_if(leftstr.begin(), leftstr.end(), [](char ch) {return ch == ' '; }), leftstr.end());//删除左部所有位置空格
		Symbol leftSymbol = GetNonTerminal(leftstr);
		//提取右部并添加产生式
		string rightstr = line.substr(arrowPos + 2);
		istringstream is(rightstr);//->右侧所有字符
		string singleSymbol;//->右侧用空格分隔的<符号>
		vector<Symbol> rightSymbols;//每个产生式右侧的所有<符号>
		while (is >> singleSymbol) {
			if (singleSymbol == "|") {//单条产生式完毕
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
					if (token == TokenType::None)//为非终结符
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
		throw runtime_error("无产生式，无法生成拓广文法");
	vector<Production>::iterator it = productions.begin();
	productions.insert(it, Production(GetNonTerminal(""), vector<Symbol>{ it->left }));
}

const Symbol Parser::GetNonTerminal(const string name)
{
	auto it = nonTerminals.find(name);
	if (it == nonTerminals.end()) {
		//在map中添加新的非终结符
		unsigned int nonterminalId = nonTerminals.size();
		nonTerminals[name] = nonterminalId;
		return Symbol(nonterminalId, name);
	}
	else
		return Symbol(it->second, name);
}

const enum TokenType Parser::GetTerminalType(const string name)
{
	//预定义终结符映射关系
	static const unordered_map<string, TokenType> terminalMap = {
		//运算符映射
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
		//关键字映射
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
		//特殊标记
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
	bool changed = true;//本轮循环所有产生式是否有FIRST集合改变
	bool noepsilon = true;//本产生式右侧每一个符号循环当前符号是否有FIRST集中包含ε
	bool allepsilon = true;//本产生式右侧每一个符号循环所有符号是否FIRST集中均包含ε
	firsts.resize(nonTerminals.size());
	while (changed) {
		changed = false;
		for (Production prod : productions) {
			const Symbol left = prod.left;
			if (left.isTerminal)
				continue;//正常情况左侧为非终结符，执行不到
			set<TokenType>& leftfirst = firsts[left.nonterminalId];//当前产生式左侧非终结符的first集合
			//产生式右侧为ε
			if (prod.right.empty()) {
				if (leftfirst.insert(EPSILON).second)//ε为新添加
					changed = true;
			}
			//产生式右侧开始为（非）终结符，为终结符在第一次循环中break
			else {
				allepsilon = true;
				for (Symbol sym : prod.right) {//产生式右侧的每一个符号
					if (sym.isTerminal) {//遇终结符则加入FIRST(该终结符)后结束
						if (leftfirst.insert(sym.terminalId).second)//终结符为新添加
							changed = true;
						allepsilon = false;
						break;//结束
					}
					else {//遇非终结符则加入FIRST(该非终结符)后继续查找
						noepsilon = true;
						for (TokenType firstele : firsts[sym.nonterminalId])//非终结符的每一个已知FIRST元素
							if (firstele != EPSILON) {//添加FIRST(该非终结符)-{ε}
								if (leftfirst.insert(firstele).second)//终结符为新添加
									changed = true;
							}
							else
								noepsilon = false;
						if (noepsilon) {//FIRST集合中如果没有ε则查找到该非终结符后结束
							allepsilon = false;
							break;
						}
					}
				}
				if (allepsilon && leftfirst.insert(EPSILON).second)//产生式右侧所有符号FIRST集均包含ε，添加ε到FIRST(左侧符号)
					changed = true;
			}
		}
	}
}

const set<enum TokenType> Parser::First(const vector<Symbol>& symbols)
{
	set<TokenType> firsteles;
	bool allepsilon = true;
	for (Symbol sym : symbols) {//句型的每一个符号
		bool noepsilon = true;
		if (sym.isTerminal) {//遇终结符则终结符加入集合后结束
			firsteles.insert(sym.terminalId);
			allepsilon = false;
			break;//结束
		}
		else {//遇非终结符
			for (TokenType firstele : firsts[sym.nonterminalId])//非终结符的每一个FIRST元素
				if (firstele != EPSILON)//添加FIRST(该非终结符)-{ε}
					firsteles.insert(firstele);
				else
					noepsilon = false;
			if (noepsilon) {//FIRST集合中如果没有ε则查找到该非终结符后结束
				allepsilon = false;
				break;
			}
		}
	}
	if (allepsilon)//symbols为ε/symbols所有符号FIRST集均包含ε，添加ε到FIRST(symbols)
		firsteles.insert(EPSILON);
	return firsteles;
}

const LR1ItemSet Parser::Closure(LR1ItemSet itemset)
{
	bool added = true;
	while (added) {
		added = false;
		//I中的每个项[A→α·Bβ,a]
		for (const LR1Item& item : itemset.items) {
			//找出每个LR1项目的产生式右部
			vector<Symbol> prodRight = productions[item.productionIndex].right;//αBβ
			int dotPos = item.dotPosition;
			if (dotPos < int(prodRight.size())) {//移进项，A→α·Bβ,a（β有可能为ε）
				Symbol nextSym = prodRight[dotPos];//B
				//仅为非终结符时继续求解
				if (!nextSym.isTerminal) {
					//求βa
					vector<Symbol> beta;//βa
					TokenType lookahead = item.lookahead;//a
					for (size_t i = dotPos + 1; i < prodRight.size(); ++i)//加入β，β=ε时未进入循环
						beta.push_back(prodRight[i]);
					if (beta.empty() || lookahead != EPSILON)//β≠ε,a=ε，不加入a
						beta.push_back(Symbol(lookahead));//加入a
					//G'中的每个产生式B→γ
					set<TokenType>firstbeta = First(beta);//FIRST(βa)
					for (size_t i = 0; i < productions.size(); i++) {
						if (productions[i].left != nextSym)
							continue;
						//FIRST(βa)中的每个终结符号b
						for (TokenType b : firstbeta) {
							if (itemset.items.insert(LR1Item(i, 0, b)).second)//终结符为新添加
								added = true;
						}
					}
				}
			}
			else if (dotPos > int(prodRight.size()))
				throw runtime_error("LR1项目中·位置非法");
			//·在最后直接返回，无需操作
		}
	}
	return itemset;
}

const LR1ItemSet Parser::Goto(const LR1ItemSet& itemset, const Symbol& sym)
{
	LR1ItemSet gotoset;//将J初始化为空集;
	for (const LR1Item& item : itemset.items) {//I中的每个项「A→α·Xβ,a]
		int dotPosition = item.dotPosition;
		int productionIndex = item.productionIndex;
		if (dotPosition < int(productions[productionIndex].right.size()) && productions[productionIndex].right[dotPosition] == sym)//Xβ不为空且X为sym
			gotoset.items.insert(LR1Item(productionIndex, dotPosition + 1, item.lookahead));
	}
	return Closure(gotoset);
}

void Parser::addReduceEntry(int ItemsetsIndex)
{
	const LR1ItemSet& itemset = Itemsets[ItemsetsIndex];
	for (const LR1Item& item : itemset.items)
		if (item.dotPosition > int(productions[item.productionIndex].right.size()))
			throw runtime_error("LR1项目中·位置非法");
		else if (item.dotPosition == productions[item.productionIndex].right.size())//A->B·,a
			if (item.productionIndex)
				writeActionTable(ItemsetsIndex, item.lookahead, ActionTableEntry{ Action::reduce , item.productionIndex });//对reduce填写表项
			else
				writeActionTable(ItemsetsIndex, item.lookahead, ActionTableEntry{ Action::accept , item.productionIndex });//对reduce填写表项
}

void Parser::writeActionTable(int itemset, int terminal, const ActionTableEntry& entry)
{
	//解决一个归约/归约冲突时,选择在规约中列在前面的那个冲突产生式
	//解决移入/归约冲突时总是选择移入。这个规则正确地解决了因为悬空else二义性而产生的移入/归约冲突
	ActionTableEntry& currEntry = actionTable[itemset][terminal];

	if (currEntry == entry)//已填写且填写项无冲突
		return;
	else if (currEntry.act == Action::error) {//未填写，无冲突
		currEntry = entry;//填写表项
		return;
	}
	else if (currEntry.act == Action::accept || entry.act == Action::accept) {//不可能的冲突
		DEBUG_CERR << "LR1分析表accept冲突，应检查逻辑" << endl;
		return;
	}
	else if (entry.act == Action::error) {//不可能的冲突
		DEBUG_CERR << "不应向LR1分析表中填入error，应检查逻辑" << endl;
		return;
	}
	else if (currEntry.act == Action::shift && entry.act == Action::shift) {//不可能的冲突
		DEBUG_CERR << "LR1分析表移进-移进冲突，应检查逻辑" << endl;
		return;
	}
	//处理正常情况下可能产生的冲突：归约-归约冲突与移进-归约冲突，可以使用优先级规则或报告冲突
	//输出当前状态集信息
	DEBUG_CERR << "当前状态项集 I" << itemset << ":" << endl;
	for (const LR1Item& item : Itemsets[itemset].items) {
		const Production& prod = productions[item.productionIndex];
		DEBUG_CERR << "  " << prod.left.name << " -> ";
		for (size_t j = 0; j < prod.right.size(); j++) {
			if (j == item.dotPosition)
				DEBUG_CERR << "· ";
			DEBUG_CERR << prod.right[j].name << " ";
		}
		if (item.dotPosition == prod.right.size())
			DEBUG_CERR << "·";
		DEBUG_CERR << ", " << TokenTypeToString(item.lookahead) << endl;
	}
	//输出共同的冲突信息
	DEBUG_CERR << "状态项集 I" << itemset << " 针对符号 " << TokenTypeToString(TokenType(terminal)) << " 发生";
	//归约-归约冲突
	if (currEntry.act == Action::reduce && entry.act == Action::reduce) {//归约-归约冲突
		DEBUG_CERR << "LR1分析表归约-归约冲突" << endl;
		DEBUG_CERR << "	规约操作1: 使用产生式 " << productions[currEntry.num].left.name << " -> ";
		for (const Symbol& sym : productions[currEntry.num].right)
			DEBUG_CERR << sym.name << " ";
		DEBUG_CERR << endl << "	规约操作2: 使用产生式 " << productions[entry.num].left.name << " -> ";
		for (const Symbol& sym : productions[entry.num].right)
			DEBUG_CERR << sym.name << " ";
		DEBUG_CERR << endl << "解决方案: 归约/归约冲突时，选择在规约中列在前面的那个冲突产生式" << endl;
		if (entry.num < currEntry.num) {
			DEBUG_CERR << "	保留规约操作2" << endl << endl;
			currEntry = entry;
		}
		else
			DEBUG_CERR << "	保留规约操作1" << endl << endl;
		return;
	}
	//移进-归约冲突
	int shiftnum = currEntry.act == Action::shift ? currEntry.num : entry.num;
	int reducenum = currEntry.act == Action::reduce ? currEntry.num : entry.num;
	DEBUG_CERR << "LR1分析表移进-归约冲突" << endl;
	DEBUG_CERR << "移进操作: 移进到状态 " << shiftnum << endl;
	DEBUG_CERR << "规约操作: 使用产生式 " << productions[reducenum].left.name << " -> ";
	for (const Symbol& sym : productions[reducenum].right)
		DEBUG_CERR << sym.name << " ";
	DEBUG_CERR << endl;
	DEBUG_CERR << "解决方案: 移入/归约冲突时总是选择移入" << endl << endl;
	if (currEntry.act == Action::reduce && entry.act == Action::shift)//移进-归约冲突
		currEntry = entry; //选择移进操作
	//else if (currEntry.act == Action::shift && entry.act == Action::reduce)//移进-归约冲突
	//	//无操作
}

void Parser::Items()
{
	//将C初始化为{CLOSURE}({[S'→·S,$]});
	Itemsets.clear();
	Itemsets.push_back(Closure(LR1ItemSet{ set<LR1Item>{LR1Item(0, 0, TokenType::End)} }));//productionIndex与augmentProduction中拓广时添加位置要对应

	int nonTerminalsNum = nonTerminals.size();
	actionTable.emplace_back(vector<ActionTableEntry>(TokenType::End + 1));//actionTable行数与Itemsets同步变化
	gotoTable.emplace_back(vector<int>(nonTerminalsNum, -1));//gotoTable行数与Itemsets同步变化

	bool added = true;
	while (added) {//循环直到不再有新的项集加入到C中
		added = false;
		for (int index = 0; index < int(Itemsets.size()); ++index) {//C中的每个项集I
			LR1ItemSet I = Itemsets[index];
			//每个文法符号X
			for (int i = TokenType::None + 1; i <= TokenType::End; ++i) {//每个文法符号X（终结符）
				Symbol X((TokenType(i)));
				const LR1ItemSet gotoset = Goto(I, X);
				if (!gotoset.items.empty() && find(Itemsets.begin(), Itemsets.end(), gotoset) == Itemsets.end()) {//GOTO(I,X)非空且不在C中
					Itemsets.push_back(gotoset);//将GOTO(I,X)加入C中

					actionTable.emplace_back(vector<ActionTableEntry>(TokenType::End + 1));//actionTable行数与Itemsets同步变化
					gotoTable.emplace_back(vector<int>(nonTerminalsNum, -1));//gotoTable行数与Itemsets同步变化
					added = true;
				}
				if (!gotoset.items.empty())
					writeActionTable(index, i, ActionTableEntry{ Action::shift, distance(Itemsets.begin(), find(Itemsets.begin(), Itemsets.end(), gotoset)) });//对shift填写表项
			}
			for (pair<const string, unsigned int> sym : nonTerminals) {//每个文法符号X（非终结符）
				Symbol X = GetNonTerminal(sym.first);//////////////////////////去除Symbol.name属性后使用Symbol X(sym.second);//////////////////////////
				const LR1ItemSet gotoset = Goto(I, X);
				vector<LR1ItemSet>::iterator it = find(Itemsets.begin(), Itemsets.end(), gotoset);//GOTO到的LR1项目集迭代器
				if (!gotoset.items.empty() && it == Itemsets.end()) {//GOTO(I,X)非空且不在C中
					Itemsets.push_back(gotoset);//将GOTO(I,X)加入C中

					actionTable.emplace_back(vector<ActionTableEntry>(TokenType::End + 1));//actionTable行数与Itemsets同步变化
					gotoTable.emplace_back(vector<int>(nonTerminalsNum, -1));//gotoTable行数与Itemsets同步变化
					added = true;
				}
				if (!gotoset.items.empty())
					gotoTable[index][sym.second] = distance(Itemsets.begin(), find(Itemsets.begin(), Itemsets.end(), gotoset));//对goto填写表项
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

	stack<int> stateStack;//状态栈
	stack<Symbol> tokenStack;//符号栈
	stateStack.push(0);//拓广文法的起始变元对应LR1项目集开始
	tokenStack.push(TokenType::End);

	GetToken();//令look为ω#的第一个符号
	while (true) {
		int s = stateStack.top();//令s是栈顶的状态
		ActionTableEntry action = actionTable[s][look.type];//s和look对应行动
		if (action.act == Action::shift) {//ACTION[s,a]=移入新状态（sx）
			stateStack.push(action.num);//将新状态压入栈中
			tokenStack.push(look.type);
			GetToken();
		}
		else if (action.act == Action::reduce) {//ACTION[s,a]=归约A→β
			int betalength = productions[action.num].right.size();//|β|
			for (int i = 0; i < betalength; ++i) {//从栈中弹出|β|个符号;
				stateStack.pop();
				tokenStack.pop();
			}
			int t = stateStack.top();//令t为当前的栈顶状态;
			unsigned int Aid = productions[action.num].left.nonterminalId;//A
			stateStack.push(gotoTable[t][Aid]);//将GOTO[t,A]压入栈中
			tokenStack.push(Symbol(Aid, productions[action.num].left.name));
			reduceProductionLists.push_back(productions[action.num]);//输出产生式A→β
		}
		else if (action.act == Action::accept)
			break;//语法分析完成
		else if (action.act == Action::error) {
			//错误展示
			lexer.ProcError(string(TokenTypeToString(look.type)) + "类别符号不符合给定的语法规则");
			//记录错误
			AddParseError(look.line, look.column, look.length, string(TokenTypeToString(look.type)) + "类别符号不符合给定的语法规则");
			//展示此状态可接受的词法单元类型
			DEBUG_CERR << "预期的词法单元: ";
			bool hasExpected = false;
			for (int i = 1; i <= int(TokenType::End); i++)
				if (actionTable[s][i].act != Action::error) {
					if (hasExpected)
						DEBUG_CERR << ", ";
					DEBUG_CERR << TokenTypeToString(TokenType(i));
					hasExpected = true;
				}
			if (!hasExpected)
				DEBUG_CERR << "无法确定";
			DEBUG_CERR << endl;
			DEBUG_CERR << "跳过当前词法单元" << endl << endl;
			if (look.type == TokenType::End)
				break;
			GetToken();
		}
		////输出当前栈中内容
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
	//输出查看
	//打印读取的产生式，用于调试
	cout << "读取的产生式：" << endl;
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
	cout << "所有符号的FIRST：" << endl;
	//终结符
	cout << "对于所有终结符, FIRST(t) = {t}" << endl;
	for (int i = 0; i <= int(TokenType::End); i++) {
		TokenType token = TokenType(i);
		cout << "FIRST(" << TokenTypeToString(token) << ") = { " << TokenTypeToString(token) << " }" << endl;
	}
	//非终结符
	for (const pair<const string, unsigned int>& entry : nonTerminals) {//对于每一个非终结符
		cout << "FIRST(" << entry.first << ")={ ";
		bool first = true;
		for (const TokenType& token : firsts[entry.second]) {//非终结符的每一个FIRST元素
			if (!first)
				cout << ", ";
			first = false;

			if (token == EPSILON)
				cout << "ε";
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
	cout << "=============== LR1项集族 ===============" << endl;
	for (size_t i = 0; i < Itemsets.size(); i++) {
		cout << "I" << i << ":" << endl;
		const LR1ItemSet& itemset = Itemsets[i];
		for (const LR1Item& item : itemset.items) {
			//输出产生式
			const Production& prod = productions[item.productionIndex];
			cout << "  " << prod.left.name << " -> ";
			//输出右侧并在合适位置插入点号
			for (size_t j = 0; j < prod.right.size(); j++) {
				if (j == item.dotPosition)
					cout << "· ";
				cout << prod.right[j].name << " ";
			}
			//如果点在最右侧，则在最后追加点
			if (item.dotPosition == prod.right.size())
				cout << "·";
			//输出前瞻符号
			cout << ", " << (item.lookahead == EPSILON ? "ε" : TokenTypeToString(item.lookahead)) << endl;
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
	cout << "=============== Action表 ===============" << endl;
	cout << "状态\t";

	//输出终结符表头
	for (int i = 0; i <= int(TokenType::End); i++) {
		TokenType type = TokenType(i);
		if (type == TokenType::None)
			continue; //跳过None
		cout << TokenTypeToString(type) << "\t";
	}
	cout << endl;

	//输出每一行的动作
	for (size_t i = 0; i < actionTable.size(); i++) {
		cout << i << "\t";
		for (int j = 1; j <= int(TokenType::End); j++) {
			TokenType type = TokenType(j);
			const ActionTableEntry& entry = actionTable[i][j];

			//根据动作类型输出不同的格式
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

	cout << "\n=============== Goto表 ===============" << endl;
	cout << "状态\t";

	//输出非终结符表头
	for (const auto& nt : nonTerminals) {
		cout << nt.first << "\t";
	}
	cout << endl;

	//输出每一行的目标状态
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
	cout << "\n=============== 语法树 ===============\n";

	//使用栈来模拟归约过程，构建树结构
	struct TreeNode {
		Symbol symbol;              //节点符号
		vector<TreeNode*> children; //使用指针避免复制问题
		string lexeme;              //词素(对终结符)

		TreeNode(const Symbol& s) : symbol(s), lexeme("") {}
		~TreeNode() {
			for (auto child : children)
				delete child;
		}
	};

	stack<TreeNode*> nodeStack;

	//从归约序列构建语法树
	for (const auto& prod : reduceProductionLists) {
		TreeNode* node = new TreeNode(prod.left);

		//从栈中弹出与产生式右部数量相同的节点
		vector<TreeNode*> children;
		for (size_t i = 0; i < prod.right.size(); i++)
			if (!nodeStack.empty()) {
				children.insert(children.begin(), nodeStack.top());
				nodeStack.pop();
			}

		node->children = children;
		nodeStack.push(node);
	}

	//递归打印语法树的辅助lambda函数
	auto printTreeNode = [](const TreeNode* node, string prefix, bool isLast, auto& self) -> void {
		if (!node)
			return;

		cout << prefix;

		//打印连接线
		cout << (isLast ? "└── " : "├── ");

		//打印节点内容
		cout << node->symbol.name;
		if (!node->lexeme.empty())
			cout << " (" << node->lexeme << ")";
		cout << endl;

		//为子节点准备前缀
		string newPrefix = prefix + (isLast ? "    " : "│   ");

		//递归打印子节点
		for (size_t i = 0; i < node->children.size(); i++) {
			bool lastChild = (i == node->children.size() - 1);
			self(node->children[i], newPrefix, lastChild, self);
		}
		};

	//打印整棵树
	if (!nodeStack.empty()) {
		TreeNode* rootNode = nodeStack.top();
		printTreeNode(rootNode, "", true, printTreeNode);

		//清理内存
		while (!nodeStack.empty()) {
			TreeNode* node = nodeStack.top();
			nodeStack.pop();
			delete node;
		}
	}
	else
		cout << "无法构建语法树！\n";

	cout << "=======================================" << endl;
}