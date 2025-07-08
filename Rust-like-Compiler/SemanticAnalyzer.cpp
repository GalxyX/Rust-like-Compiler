#include "SemanticAnalyzer.h"
#include "Parser.h"
using namespace std;

SymbolTable::SymbolTable(SymbolTable* prevTable) : prev(prevTable), width(0)
{
}

//返回ValueType对应数据类型的字节长度
static size_t getValueTypeWidth(ValueType type) {
	switch (type) {
	case _i8:
	case _u8:
		return 1;
	case _i16:
	case _u16:
		return 2;
	case _i32:
	case _u32:
	case _f32:
		return 4;
	case _i64:
	case _u64:
	case _f64:
		return 8;
	case _i128:
	case _u128:
		return 16;
	case _isize:
	case _usize:
		return sizeof(size_t);// 依赖于目标架构，32位系统为4字节，64位系统为8字节
	case _bool:
		return 1;
	case _char:
		return 4;
	case _unit:
		return 0;       // 单元类型()不占用内存空间
	case _array:
		return 0;       // 数组大小需要额外信息，这里返回0作为占位
	case _procedure:
		return sizeof(void*);   // 函数指针大小，通常为指针大小
	case undefined:
	default:
		return 0;       // 未定义类型或错误情况
	}
}

bool SymbolTable::isExist(string symbolName)
{
	for (vector<SymbolTableEntry>::iterator it = table.begin(); it != table.end(); ++it)
		if ((*it).ID == symbolName)
			return true;
	return false;
}

void SymbolTable::put(const SymbolTableEntry& newSymbol)
{
	table.push_back(SymbolTableEntry{ newSymbol.ID, newSymbol.kind, newSymbol.type, newSymbol.isNormal, newSymbol.isAssigned/*, newSymbol.value*/, width });
	width += getValueTypeWidth(newSymbol.type);
}

SymbolTableEntry SymbolTable::get(string symbolName)
{
	for (SymbolTable* env = this; env != NULL; env = env->prev) {
		const vector<SymbolTableEntry>& stable = env->table;
		for (reverse_iterator<vector<SymbolTableEntry> ::const_iterator> rit = stable.rbegin(); rit != stable.rend(); ++rit)
			if ((*rit).ID == symbolName)
				return *rit;
	}
	return SymbolTableEntry{ "", SymbolKind::Constant, ValueType::undefined, false, false/*, 0*/, 0 };
}

bool SymbolTable::update(const SymbolTableEntry& newSymbol)
{
	for (SymbolTable* env = this; env != NULL; env = env->prev) {
		vector<SymbolTableEntry>& stable = env->table;//本层符号表
		for (reverse_iterator<vector<SymbolTableEntry> ::iterator> rit = stable.rbegin(); rit != stable.rend(); ++rit)
			if ((*rit).ID == newSymbol.ID) {
				(*rit).kind = newSymbol.kind;
				(*rit).type = newSymbol.type;
				(*rit).isNormal = newSymbol.isNormal;
				(*rit).isAssigned = newSymbol.isAssigned;
				//(*rit).value = newSymbol.value;
				(*rit).addr = width;
				width += getValueTypeWidth(newSymbol.type);
				return true;
			}
	}
	return false;
}

void SymbolTable::erase(string symbolName)
{
	int idx = -1;
	for (int i = table.size() - 1; i >= 0; --i)
		if (table[i].ID == symbolName) {
			idx = i;
			break;
		}
	if (idx != -1)
		table.erase(table.begin() + idx);
	return;
}

//void SymbolTable::addwidth(const size_t wid)
//{
//	width = wid;
//}

ProcedureTableEntry::ProcedureTableEntry(std::string ID, ValueType returntype, const std::vector<ValueType>& paratype, size_t addr) :ID(ID), returntype(returntype), paratype(paratype), addr(addr)
{
}

ASTNode::ASTNode(size_t line, size_t column) : line(line), column(column)
{
}

//string ASTNode::error(const string& s) const
//{
//	return  '(' + to_string(line) + ", " + to_string(column) + ')' + s;
//}

//Expr::Expr(size_t line, size_t column, string name, ValueType type/*, size_t width*/) :ASTNode(line, column), name(name), type(type)/*, width(width)*/
//{
//}

ExprNode::ExprNode(size_t line, size_t column, ValueType type, std::string name) : ASTNode(line, column), type(type), kind(SymbolKind::Variable), name_value(name)
{
}

ExprNode::ExprNode(size_t line, size_t column, ValueType type, TokenValue value) : ASTNode(line, column), type(type), kind(SymbolKind::Constant), name_value(value)
{
}

std::string Id::newtemp()
{
	return "T" + to_string(count++);
}

int Id::count = 0;
Id::Id(size_t line, size_t column, string name) : ASTNode(line, column), name(name)
{
}

ParaListNode::ParaListNode(size_t line, size_t column, const std::vector<ValueType>& paratype) : ASTNode(line, column), paratype(paratype)
{
}

I32num::I32num(size_t line, size_t column, int value, ValueType type) : ASTNode(line, column), value(value), type(type)
{
}

//BridgeNode::BridgeNode(size_t line = 0, size_t column = 0, SymbolKind kind = SymbolKind::Variable, std::string name = "", ValueType type = ValueType::undefined, size_t width = 0) :ASTNode(line, column), kind(kind), name(name), type(type), width(width)
//{
//
//}

BridgeNode::BridgeNode(size_t line, size_t column, SymbolKind kind, std::string name) : ASTNode(line, column), kind(kind), name(name)
{
}

BridgeNode::BridgeNode(size_t line, size_t column, ValueType type, size_t width) : ASTNode(line, column), type(type), width(width)
{
}

BridgeNode::BridgeNode(size_t line, size_t column, std::string name) : ASTNode(line, column), name(name)
{
}

Stmt::Stmt(size_t line, size_t column, bool returned) : ASTNode(line, column), returned(returned)
{
}

Stmt::Stmt(size_t line, size_t column, bool returned, const std::vector<size_t>& list, int listtype) : ASTNode(line, column), returned(returned)
{
	if (listtype == 0)
		nextlist = list;
	else if (listtype == 1)
		breaklist = list;
	else if (listtype == 2)
		continuelist = list;
}

Stmt::Stmt(size_t line, size_t column, bool returned, const std::vector<size_t>& nextlist, const std::vector<size_t>& breaklist, const std::vector<size_t>& continuelist) : ASTNode(line, column), returned(returned), nextlist(nextlist), breaklist(breaklist), continuelist(continuelist)
{
}

//Stmt::Stmt(size_t line, size_t column, bool returned, int listtype, const std::vector<BrkConListEntry>& list) : ASTNode(line, column), returned(returned)
//{
//	if (listtype == 0)
//		breaklist = list;
//	else if (listtype == 1)
//		continuelist = list;
//}

Op::Op(size_t line, size_t column, TokenType op) : ASTNode(line, column), op(op)
{
}

string Op::GetOperatorString()
{
	switch (op) {
		// 算术运算符
	case TokenType::Addition:
		return "+";
	case TokenType::Subtraction:
		return "-";
	case TokenType::Multiplication:
		return "*";
	case TokenType::Division:
		return "/";
	case TokenType::Modulo:
		return "%";
		// 比较运算符
	case TokenType::Equality:
		return "==";
	case TokenType::Inequality:
		return "!=";
	case TokenType::LessThan:
		return "<";
	case TokenType::LessOrEqual:
		return "<=";
	case TokenType::GreaterThan:
		return ">";
	case TokenType::GreaterOrEqual:
		return ">=";
		// 位运算符
	case TokenType::BitAnd:
		return "&";
	case TokenType::BitOr:
		return "|";
	case TokenType::BitXor:
		return "^";
	case TokenType::LeftShift:
		return "<<";
	case TokenType::RightShift:
		return ">>";
		// 逻辑运算符
	case TokenType::LogicalAnd:
		return "&&";
	case TokenType::LogicalOr:
		return "||";
	case TokenType::Not:
		return "!";
		// 赋值运算符
	case TokenType::Assignment:
		return "=";
	case TokenType::AdditionAssign:
		return "+=";
	case TokenType::SubtractionAssign:
		return "-=";
	case TokenType::MultiplicationAssign:
		return "*=";
	case TokenType::DivisionAssign:
		return "/=";
	case TokenType::ModuloAssign:
		return "%=";
	case TokenType::BitAndAssign:
		return "&=";
	case TokenType::BitOrAssign:
		return "|=";
	case TokenType::BitXorAssign:
		return "^=";
	case TokenType::LeftShiftAssign:
		return "<<=";
	case TokenType::RightShiftAssign:
		return ">>=";
		// 其他运算符
	case TokenType::ArrowOperator:
		return "->";
	case TokenType::DotOperator:
		return ".";
	case TokenType::RangeOperator:
		return "..";
	case TokenType::Arrowmatch:
		return "=>";
	case TokenType::ErrorPropagation:
		return "?";
	case TokenType::PatternBinding:
		return "@";
	default:
		return "unknown_op";
	}
}

Q::Q(size_t line, size_t column, size_t quad) : ASTNode(line, column), quad(quad)
{
}

B::B(size_t line, size_t column, const std::vector<size_t>& truelist, const std::vector<size_t>& falselist) : ASTNode(line, column), truelist(truelist), falselist(falselist)
{
}

P::P(size_t line, size_t column, const std::vector<size_t>& nextlist) : ASTNode(line, column), nextlist(nextlist)
{
}

ElseStmt::ElseStmt(size_t line, size_t column, bool returned, int firstquad, const std::vector<size_t>& nextlist) : Stmt(line, column, returned, nextlist), firstquad(firstquad)
{
}

IterableStructure::IterableStructure(size_t line, size_t column, ExprNode left, ExprNode right) : ASTNode(line, column), left(left), right(right)
{
}

ForIri::ForIri(size_t line, size_t column, int firstquad, const std::vector<size_t>& truelist, const std::vector<size_t>& falselist, const std::string& name, const IterableStructure& iri) : ASTNode(line, column), firstquad(firstquad), truelist(truelist), falselist(falselist), name(name), iri(iri)
{
}

void SemanticAnalyzer::emit(Quadruple s)
{
	qList.push_back(s);
	++nextstat;
}

void SemanticAnalyzer::enterproc(ProcedureTable* table, ProcedureTableEntry newtable)
{
	table->push_back(newtable);
}

std::vector<size_t> SemanticAnalyzer::makelist(size_t quad)
{
	return std::vector<size_t>{quad};
}

std::vector<size_t> SemanticAnalyzer::merge(std::vector<size_t> list1, std::vector<size_t> list2)
{
	std::vector<size_t> result;
	result.reserve(list1.size() + list2.size()); // 预分配内存，提高性能
	result.insert(result.end(), list1.begin(), list1.end());
	result.insert(result.end(), list2.begin(), list2.end());
	return result;
}

void SemanticAnalyzer::backpatch(std::vector<size_t> list, size_t quad)
{
	for (const size_t& i : list)
		qList[i - START_STMT_ADDR - 1].result = to_string(quad);
}

void SemanticAnalyzer::mkleaf(size_t line, size_t column, std::string id)
{
	unique_ptr<Id> idnode = make_unique<Id>(line, column, id);
	nodeStack.push(move(idnode));
}

void SemanticAnalyzer::mkleaf(size_t line, size_t column, int val)
{
	unique_ptr<I32num> i32node = make_unique<I32num>(line, column, val);
	nodeStack.push(move(i32node));
}

void SemanticAnalyzer::mkleaf(size_t line, size_t column, TokenType op)
{
	unique_ptr<Op> opnode = make_unique<Op>(line, column, op);
	nodeStack.push(move(opnode));
}

void SemanticAnalyzer::mkleaf(size_t line, size_t column)
{
	unique_ptr<ASTNode> astnode = make_unique<ASTNode>(line, column);
	nodeStack.push(move(astnode));
}

void SemanticAnalyzer::AddSemanticError(int line, int column, const std::string& message)
{
	semanticErrors.push_back({ line, column, -1, message });
}

SemanticAnalyzer::SemanticAnalyzer() : nextstat(START_STMT_ADDR + 1)
{
	proptr.push(&procedureList);
}

SymbolTable* SemanticAnalyzer::mktable(SymbolTable* previous)
{
	try {
		SymbolTable* env = new SymbolTable(previous);
		return env;
	}
	catch (const bad_alloc& mem_fail) {
		cerr << mem_fail.what() << endl;
		return nullptr;
	}
}

//void SemanticAnalyzer::enter(SymbolTable* table, const SymbolTableEntry newSymbol)
//{
//	table->put(newSymbol);
//}

//void SemanticAnalyzer::addwidth(SymbolTable* table, const size_t width)
//{
//	table->addwidth(width);
//}

void SemanticAnalyzer::analyze(const Production& prod)
{
	//0.1 变量声明内部----------<变量声明内部>->mut <ID>
	if (prod.left.name == "Identifier_inside" && prod.right.size() == 2 && prod.right[0].name == TokenTypeToString(TokenType::MUT) && prod.right[1].name == TokenTypeToString(TokenType::Identifier)) {
		unique_ptr<Id> idnode = unique_ptr<Id>(static_cast<Id*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> mutnode = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<BridgeNode> nonterminal = make_unique<BridgeNode>(mutnode->line, mutnode->column, SymbolKind::Variable, idnode->name);
		nodeStack.push(move(nonterminal));
	}
	//0.2 类型----------<类型>->i32
	else if (prod.left.name == "Type" && prod.right.size() == 1 && prod.right[0].name == TokenTypeToString(TokenType::I32)) {
		unique_ptr<ASTNode> astnode = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<BridgeNode> nonterminal = make_unique<BridgeNode>(astnode->line, astnode->column, ValueType::_i32, 4);
		nodeStack.push(move(nonterminal));
	}
	//0.3 可赋值元素----------<可赋值元素> -> <ID>
	else if (prod.left.name == "Assignable_element" && prod.right.size() == 1 && prod.right[0].name == TokenTypeToString(TokenType::Identifier)) {
		unique_ptr<Id> idnode = unique_ptr<Id>(static_cast<Id*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<BridgeNode> nonterminal = make_unique<BridgeNode>(idnode->line, idnode->column, idnode->name);
		nodeStack.push(move(nonterminal));
	}
	//1.1_1_1 基础程序----------Program  -> <M> <声明串>
	else if (prod.left.name == "Program" && prod.right.size() == 2 && prod.right[0].name == "M" && prod.right[1].name == "DeclarationString") {
		unique_ptr<ASTNode> astnode1 = move(nodeStack.top());
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(astnode1->line, astnode1->column);
		nodeStack.push(move(astnode));

		SymbolTable* t = tblptr.top();
		//addwidth(t, offset.top());
		delete t;//////////////////////////////////////////////////到底要不要保留符号表？？？？？？？？
		tblptr.pop();
		//offset.pop();
	}
	//1.1_1_2 基础程序----------<M>  -> 空
	else if (prod.left.name == "M" && prod.right.size() == 0) {
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(0, 0);
		nodeStack.push(move(astnode));

		SymbolTable* t = mktable(NULL);
		tblptr.push(t);
		//offset.push(0);
	}
	//1.1_2 基础程序----------<声明串> -> 空 | <声明> <声明串>
	else if (prod.left.name == "DeclarationString" && prod.right.size() == 0) {
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(0, 0);
		nodeStack.push(move(astnode));
	}
	else if (prod.left.name == "DeclarationString" && prod.right.size() == 2 && prod.right[0].name == "Declaration" && prod.right[1].name == "DeclarationString") {
		nodeStack.pop();
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(astnode0->line, astnode0->column);
		nodeStack.push(move(astnode));
	}
	//1.1_3 基础程序----------<声明> -> <函数声明>
	else if (prod.left.name == "Declaration" && prod.right.size() == 1 && prod.right[0].name == "Function_Declaration") {
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(astnode0->line, astnode0->column);
		nodeStack.push(move(astnode));
	}
	//1.1_4_1 基础程序----------<函数声明> -> N <函数头声明> <语句块>
	else if (prod.left.name == "Function_Declaration" && prod.right.size() == 3 && prod.right[0].name == "N" && prod.right[1].name == "Function_Head_Declaration" && prod.right[2].name == "Line_Block") {
		unique_ptr<Stmt> stmtnode2 = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode1 = move(nodeStack.top());
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(astnode1->line, astnode1->column);
		nodeStack.push(move(astnode));

		//检查break/continue是否均出现在循环体内
		if (!stmtnode2->breaklist.empty())
			AddSemanticError(astnode1->line, astnode1->column, "break语句只能在循环或开关中使用");
		if (!stmtnode2->continuelist.empty())
			AddSemanticError(astnode1->line, astnode1->column, "continue语句只能在循环中使用");
		SymbolTable* t = tblptr.top();
		//检查是否推断出所有标识符的类型
		for (const SymbolTableEntry& sym : t->table)
			if (sym.type == ValueType::none)
				AddSemanticError(astnode1->line, astnode1->column, "变量\"" + sym.ID + "\"无法根据函数上下文进行类型推导");
		//addwidth(t, offset.top());
		tblptr.pop();
		//offset.pop();
		proptr.pop();
		ProcedureTable* outPtable = proptr.top();//定义本函数的上一层函数的函数表
		if (!stmtnode2->returned) {
			if ((*outPtable)[outPtable->size() - 1].returntype != ValueType::none)
				AddSemanticError(astnode1->line, astnode1->column, "返回值类型与函数类型不匹配");
			backpatch(stmtnode2->nextlist, nextstat);
			emit(Quadruple{ "ret", "-", "-", "-" });
		}
	}
	//1.1_4_2 基础程序----------N -> 空
	else if (prod.left.name == "N" && prod.right.size() == 0) {
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(0, 0);
		nodeStack.push(move(astnode));

		SymbolTable* t = mktable(tblptr.top());
		tblptr.push(t);
		//offset.push(0);
	}
	//1.1_5 基础程序----------<函数头声明> ->  fn <ID> '(' <形参列表> ')'
	else if (prod.left.name == "Function_Head_Declaration" && prod.right.size() == 5 && prod.right[0].name == TokenTypeToString(TokenType::FN) && prod.right[1].name == TokenTypeToString(TokenType::Identifier) && prod.right[2].name == TokenTypeToString(TokenType::ParenthesisL) && prod.right[3].name == "Formal_Parameter_List" && prod.right[4].name == TokenTypeToString(TokenType::ParenthesisR)) {
		nodeStack.pop();
		unique_ptr<ParaListNode> paranode3 = unique_ptr<ParaListNode>(static_cast<ParaListNode*>(nodeStack.top().release()));
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<Id> idnode1 = unique_ptr<Id>(static_cast<Id*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(astnode0->line, astnode0->column);
		nodeStack.push(move(astnode));

		enterproc(proptr.top(), { idnode1->name, ValueType::none, paranode3->paratype, nextstat });
		ProcedureTable out = *proptr.top();
		proptr.push(&out[out.size() - 1].subProcedureTable);
	}
	//1.1_6 基础程序----------<形参列表>-> 空
	else if (prod.left.name == "Formal_Parameter_List" && prod.right.size() == 0) {
		unique_ptr<ParaListNode> paranode = make_unique<ParaListNode>(0, 0, vector<ValueType>());
		nodeStack.push(move(paranode));
	}
	//1.1_7 基础程序----------<语句块> -> '{' <语句串> '}'
	else if (prod.left.name == "Line_Block" && prod.right.size() == 3 && prod.right[0].name == TokenTypeToString(TokenType::CurlyBraceL) && prod.right[1].name == "Line_String" && prod.right[2].name == TokenTypeToString(TokenType::CurlyBraceR)) {
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode2 = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode = make_unique<Stmt>(astnode0->line, astnode0->column, stmtnode2->returned, stmtnode2->nextlist, stmtnode2->breaklist, stmtnode2->continuelist);
		nodeStack.push(move(stmtnode));
	}
	//1.1_8 基础程序----------<语句串> -> 空
	else if (prod.left.name == "Line_String" && prod.right.size() == 0) {
		unique_ptr<Stmt> astnode = make_unique<Stmt>(0, 0, false);
		nodeStack.push(move(astnode));
	}
	//1.2_1 语句（前置规则1.1）----------<语句串> -> <语句> <Q> <语句串>
	else if (prod.left.name == "Line_String" && prod.right.size() == 3 && prod.right[0].name == "Line" && prod.right[1].name == "Q" && prod.right[2].name == "Line_String") {
		unique_ptr<Stmt> stmtnode1 = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Q> qnode = unique_ptr<Q>(static_cast<Q*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode0 = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode = make_unique<Stmt>(stmtnode0->line, stmtnode0->column, stmtnode0->returned || stmtnode1->returned, stmtnode1->nextlist, merge(stmtnode0->breaklist, stmtnode1->breaklist), merge(stmtnode0->continuelist, stmtnode1->continuelist));
		nodeStack.push(move(stmtnode));

		backpatch(stmtnode0->nextlist, qnode->quad);//回填<语句>的nextlist
		//ProcedureTable* nowPtable = proptr.top();//本层函数函数表
		//proptr.pop();
		//ProcedureTable* outPtable = proptr.top();//定义本函数的上一层函数的函数表
		//proptr.push(nowPtable);//加回去
		//if (!(*outPtable)[outPtable->size() - 1].isRturned)
		//	(*outPtable)[outPtable->size() - 1].isRturned = stmtnode0->returned;
	}
	//1.2_1 语句（前置规则1.1）----------<语句>-> ';'
	else if (prod.left.name == "Line" && prod.right.size() == 1 && prod.right[0].name == TokenTypeToString(TokenType::Semicolon)) {
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode = make_unique<Stmt>(astnode0->line, astnode0->column, false);
		nodeStack.push(move(stmtnode));
	}
	//1.3_1 返回语句（前置规则1.2）----------<语句>-> <返回语句>
	else if (prod.left.name == "Line" && prod.right.size() == 1 && prod.right[0].name == "Return_Line") {
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode = make_unique<Stmt>(astnode0->line, astnode0->column, true);
		nodeStack.push(move(stmtnode));
	}
	//1.3_2 返回语句（前置规则1.2）----------<返回语句> -> return ';'
	else if (prod.left.name == "Return_Line" && prod.right.size() == 2 && prod.right[0].name == TokenTypeToString(TokenType::RETURN) && prod.right[1].name == TokenTypeToString(TokenType::Semicolon)) {
		nodeStack.pop();
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(astnode0->line, astnode0->column);
		nodeStack.push(move(astnode));

		ProcedureTable* nowPtable = proptr.top();//本层函数函数表
		proptr.pop();
		const ProcedureTable& outPtable = *proptr.top();//定义本函数的上一层函数的函数表
		proptr.push(nowPtable);//加回去
		ValueType returntype = outPtable[outPtable.size() - 1].returntype;//本函数返回类型
		if (ValueType::none != returntype)//检查返回类型是否匹配
			AddSemanticError(astnode0->line, astnode0->column, "返回值类型与函数类型不匹配");
		emit({ "ret", "-", "-", "-" });
	}
	//1.4_1 函数输入（前置规则0.1、0.2、1.1）----------<形参列表>-> <形参> | <形参> ',' <形参列表>
	else if (prod.left.name == "Formal_Parameter_List" && prod.right.size() == 1 && prod.right[0].name == "Formal_Parameter") {
		//unique_ptr<ParaListNode> paranode0 = unique_ptr<ParaListNode>(static_cast<ParaListNode*>(nodeStack.top().release()));
		//nodeStack.pop();
		//unique_ptr<ParaListNode> paranode = make_unique<ParaListNode>(paranode0->line, paranode0->column, paranode0->paratype);
		//nodeStack.push(move(paranode));
	}
	else if (prod.left.name == "Formal_Parameter_List" && prod.right.size() == 3 && prod.right[0].name == "Formal_Parameter" && prod.right[1].name == TokenTypeToString(TokenType::Comma) && prod.right[2].name == "Formal_Parameter_List") {
		unique_ptr<ParaListNode> paranode2 = unique_ptr<ParaListNode>(static_cast<ParaListNode*>(nodeStack.top().release()));
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<ParaListNode> paranode0 = unique_ptr<ParaListNode>(static_cast<ParaListNode*>(nodeStack.top().release()));
		nodeStack.pop();
		paranode2->paratype.insert(paranode2->paratype.end(), paranode0->paratype.begin(), paranode0->paratype.end());//合并形参列表
		unique_ptr<ParaListNode> paranode = make_unique<ParaListNode>(paranode0->line, paranode0->column, paranode2->paratype);
		nodeStack.push(move(paranode));
	}
	//1.4_2 函数输入（前置规则0.1、0.2、1.1）----------<形参> -> <变量声明内部> ':' <类型>
	else if (prod.left.name == "Formal_Parameter" && prod.right.size() == 3 && prod.right[0].name == "Identifier_inside" && prod.right[1].name == TokenTypeToString(TokenType::Colon) && prod.right[2].name == "Type") {
		unique_ptr<BridgeNode> nonterminal1 = unique_ptr<BridgeNode>(static_cast<BridgeNode*>(nodeStack.top().release()));
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<BridgeNode> nonterminal0 = unique_ptr<BridgeNode>(static_cast<BridgeNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ParaListNode> paranode = make_unique<ParaListNode>(nonterminal0->line, nonterminal0->column, vector<ValueType>{ nonterminal1->type });
		nodeStack.push(move(paranode));

		tblptr.top()->put(SymbolTableEntry{ nonterminal0->name, SymbolKind::Variable, nonterminal1->type, true, true/*, 0xcc*/, tblptr.top()->width });
		//size_t off = offset.top() + getValueTypeWidth(nonterminal1->type);
		//offset.pop();
		//offset.push(off);
	}
	//1.5_1 函数输出（前置规则0.2、1.3、3.1）----------<函数头声明>->fn <ID> '(' <形参列表> ')' '->' <类型>
	else if (prod.left.name == "Function_Head_Declaration" && prod.right.size() == 7 && prod.right[0].name == TokenTypeToString(TokenType::FN) && prod.right[1].name == TokenTypeToString(TokenType::Identifier) && prod.right[2].name == TokenTypeToString(TokenType::ParenthesisL) && prod.right[3].name == "Formal_Parameter_List" && prod.right[4].name == TokenTypeToString(TokenType::ParenthesisR) && prod.right[5].name == TokenTypeToString(TokenType::ArrowOperator) && prod.right[6].name == "Type") {
		unique_ptr<BridgeNode> nonterminal6 = unique_ptr<BridgeNode>(static_cast<BridgeNode*>(nodeStack.top().release()));
		nodeStack.pop();
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<ParaListNode> paranode2 = unique_ptr<ParaListNode>(static_cast<ParaListNode*>(nodeStack.top().release()));
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<Id> idnode = unique_ptr<Id>(static_cast<Id*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(astnode0->line, astnode0->column);
		nodeStack.push(move(astnode));

		enterproc(proptr.top(), { idnode->name, nonterminal6->type, paranode2->paratype, nextstat });
		ProcedureTable& outPtable = *proptr.top();
		proptr.push(&outPtable[outPtable.size() - 1].subProcedureTable);
	}
	//1.5_2 函数输出（前置规则0.2、1.3、3.1）----------<返回语句> -> return <表达式> ';'
	else if (prod.left.name == "Return_Line" && prod.right.size() == 3 && prod.right[0].name == TokenTypeToString(TokenType::RETURN) && prod.right[1].name == "Expression" && prod.right[2].name == TokenTypeToString(TokenType::Semicolon)) {
		nodeStack.pop();
		unique_ptr<ExprNode> nonterminal2 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();

		//提前保存line和column的值
		int line = int(astnode0->line);
		int column = int(astnode0->column);

		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(astnode0->line, astnode0->column);
		nodeStack.push(move(astnode));

		ProcedureTable* nowPtable = proptr.top();//本层函数函数表
		proptr.pop();
		const ProcedureTable& outPtable = *proptr.top();//定义本函数的上一层函数的函数表
		proptr.push(nowPtable);//加回去
		ValueType returntype = outPtable[outPtable.size() - 1].returntype;//本函数返回类型
		string emitvalue;//emit第二个参数
		if (nonterminal2->kind == SymbolKind::Constant) {
			if (nonterminal2->type != returntype)//检查返回类型是否匹配
				AddSemanticError(line, column, "返回值类型与函数类型不匹配");
			switch (nonterminal2->type) {
			case ValueType::_i32:
				emitvalue = to_string(get<int>(get<TokenValue>(nonterminal2->name_value)));
				break;
				//根据支持类型添加新case
			default:
				emitvalue = get<string>(get<TokenValue>(nonterminal2->name_value));
				break;
			}
		}
		else {
			if (!tblptr.top()->isExist(get<string>(nonterminal2->name_value)))
				AddSemanticError(line, column, "未定义标识符" + get<string>(nonterminal2->name_value));
			else if (tblptr.top()->get(get<string>(nonterminal2->name_value)).type != returntype)//检查返回类型是否匹配
				AddSemanticError(line, column, "返回值类型与函数类型不匹配");
			emitvalue = get<string>(nonterminal2->name_value);
		}
		emit({ "ret", emitvalue, "-", "-" });
	}
	//2.1_1 变量声明语句（前置规则0.1、0.2、1.2）----------<语句> -> <变量声明语句>
	else if (prod.left.name == "Line" && prod.right.size() == 1 && prod.right[0].name == "Identifier_Line") {
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode = make_unique<Stmt>(astnode0->line, astnode0->column, false);
		nodeStack.push(move(stmtnode));
	}
	//2.1_2 变量声明语句（前置规则0.1、0.2、1.2）----------<变量声明语句> ->  let <变量声明内部> ':'  <类型> ';'
	else if (prod.left.name == "Identifier_Line" && prod.right.size() == 5 && prod.right[0].name == TokenTypeToString(TokenType::LET) && prod.right[1].name == "Identifier_inside" && prod.right[2].name == TokenTypeToString(TokenType::Colon) && prod.right[3].name == "Type" && prod.right[4].name == TokenTypeToString(TokenType::Semicolon)) {
		nodeStack.pop();
		unique_ptr<BridgeNode> nonterminal3 = unique_ptr<BridgeNode>(static_cast<BridgeNode*>(nodeStack.top().release()));//类型（type, width）
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<BridgeNode> nonterminal1 = unique_ptr<BridgeNode>(static_cast<BridgeNode*>(nodeStack.top().release()));//变量声明内部（kind, name）
		nodeStack.pop();
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(astnode0->line, astnode0->column);
		nodeStack.push(move(astnode));

		SymbolTable* stable = tblptr.top();//当前的符号表
		bool defined = stable->isExist(nonterminal1->name);//当前符号表是否存在该变量名
		if (defined)//更新
			stable->update({ nonterminal1->name, nonterminal1->kind, nonterminal3->type, false, false/*, monostate{}*/, 0 });
		else
			stable->put({ nonterminal1->name, nonterminal1->kind, nonterminal3->type, false, false/*, monostate{}*/, 0 });
	}
	//2.1_3 变量声明语句（前置规则0.1、0.2、1.2）----------<变量声明语句> ->  let <变量声明内部>  ';'
	else if (prod.left.name == "Identifier_Line" && prod.right.size() == 3 && prod.right[0].name == TokenTypeToString(TokenType::LET) && prod.right[1].name == "Identifier_inside" && prod.right[2].name == TokenTypeToString(TokenType::Semicolon)) {
		nodeStack.pop();
		unique_ptr<BridgeNode> nonterminal1 = unique_ptr<BridgeNode>(static_cast<BridgeNode*>(nodeStack.top().release()));//变量声明内部（kind, name）
		nodeStack.pop();
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(astnode0->line, astnode0->column);
		nodeStack.push(move(astnode));

		SymbolTable* stable = tblptr.top();//当前的符号表
		bool defined = stable->isExist(nonterminal1->name);//当前符号表是否存在该变量名
		if (defined)//更新
			stable->update({ nonterminal1->name, nonterminal1->kind, ValueType::none, false, false/*, monostate{}*/, 0 });
		else
			stable->put({ nonterminal1->name, nonterminal1->kind, ValueType::none, false, false/*, monostate{}*/, 0 });
	}
	//2.2_1 赋值语句（前置规则0.3、1.2、3.1）----------<语句>-> <赋值语句>
	else if (prod.left.name == "Line" && prod.right.size() == 1 && prod.right[0].name == "Assign_Line") {
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode = make_unique<Stmt>(astnode0->line, astnode0->column, false);
		nodeStack.push(move(stmtnode));
	}
	//2.2_2 赋值语句（前置规则0.3、1.2、3.1）----------<赋值语句> -> <可赋值元素> '='<表达式> ';'
	else if (prod.left.name == "Assign_Line" && prod.right.size() == 4 && prod.right[0].name == "Assignable_element" && prod.right[1].name == TokenTypeToString(TokenType::Assignment) && prod.right[2].name == "Expression" && prod.right[3].name == TokenTypeToString(TokenType::Semicolon)) {
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));;
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<BridgeNode> nonterminal0 = unique_ptr<BridgeNode>(static_cast<BridgeNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(nonterminal0->line, nonterminal0->column);
		nodeStack.push(move(astnode));

		SymbolTable* stable = tblptr.top();//当前的符号表
		SymbolTableEntry identifier = stable->get(nonterminal0->name);
		if (identifier.type == ValueType::undefined)//符号表中没有
			AddSemanticError(nonterminal0->line, nonterminal0->column, "未定义标识符" + nonterminal0->name);
		if (identifier.kind == SymbolKind::Constant)//可赋值元素为常量
			AddSemanticError(nonterminal0->line, nonterminal0->column, "表达式必须是可修改的左值");
		else if (exprnode->kind == SymbolKind::Variable) {//表达式如果为变量，检查是否声明或赋值
			SymbolTableEntry exprid = stable->get(get<string>(exprnode->name_value));
			if (exprid.type == ValueType::undefined)
				AddSemanticError(nonterminal0->line, nonterminal0->column, "未定义标识符\"" + get<string>(exprnode->name_value) + '\"');
			if (exprid.isAssigned == false)
				AddSemanticError(nonterminal0->line, nonterminal0->column, "表达式右值\"" + get<string>(exprnode->name_value) + "\"未赋值");
		}
		if (identifier.type != exprnode->type)//等号两侧类型不匹配
			AddSemanticError(nonterminal0->line, nonterminal0->column, "不存在从\"右值\"到\"左值\"的适当转换函数");
		if (identifier.type == none) {
			identifier.type = exprnode->type;
			identifier.isAssigned = true;
			stable->update(identifier);
		}
		//if (identifier.type == ValueType::undefined)//符号表中没有
		//	AddSemanticError(nonterminal0->line, nonterminal0->column, "未定义标识符\"" + nonterminal0->name + '\"');
		//ValueType exprtype;
		//if (identifier.kind == SymbolKind::Constant) {//可赋值元素为常量
		//	AddSemanticError(nonterminal0->line, nonterminal0->column, "表达式必须是可修改的左值");
		//	exprtype = exprnode->type;
		//}
		//if (exprnode->kind == SymbolKind::Variable) {//表达式如果为变量，检查是否声明或赋值
		//	SymbolTableEntry exprid = stable->get(get<string>(exprnode->name_value));
		//	if (exprid.type == ValueType::undefined)
		//		AddSemanticError(nonterminal0->line, nonterminal0->column, "未定义标识符\"" + get<string>(exprnode->name_value) + '\"');
		//	if (exprid.isAssigned == false)
		//		AddSemanticError(nonterminal0->line, nonterminal0->column, "表达式右值未赋值");
		//	exprtype = exprid.type;
		//}
		//if (identifier.type != exprtype)//等号两侧类型不匹配
		//	AddSemanticError(nonterminal0->line, nonterminal0->column, "不存在从\"右值\"到\"左值\"的适当转换函数");
		//if (identifier.type == none) {
		//	identifier.type = exprtype;
		//	identifier.isAssigned = true;
		//	stable->update(identifier);
		//}
		emit(Quadruple{ "=", exprnode->kind == SymbolKind::Constant ? to_string(get<int>(get<TokenValue>(exprnode->name_value))) : get<string>(exprnode->name_value), "-", nonterminal0->name });//只实现i32
	}
	//2.3_1 变量声明赋值语句（前置规则0.1、0.2、0.3、1.2、3.1）----------<语句>-> <变量声明赋值语句>
	else if (prod.left.name == "Line" && prod.right.size() == 1 && prod.right[0].name == "Identifier_Assign_Line") {
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode = make_unique<Stmt>(astnode0->line, astnode0->column, false);
		nodeStack.push(move(stmtnode));
	}
	//2.3_2 赋值语句（前置规则0.3、1.2、3.1）----------<变量声明赋值语句> -> let <变量声明内部> ':'  <类型> '='<表达式> ';'
	else if (prod.left.name == "Identifier_Assign_Line" && prod.right.size() == 7 && prod.right[0].name == TokenTypeToString(TokenType::LET) && prod.right[1].name == "Identifier_inside" && prod.right[2].name == TokenTypeToString(TokenType::Colon) && prod.right[3].name == "Type" && prod.right[4].name == TokenTypeToString(TokenType::Assignment) && prod.right[5].name == "Expression" && prod.right[6].name == TokenTypeToString(TokenType::Semicolon)) {
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));;
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<BridgeNode> nonterminal3 = unique_ptr<BridgeNode>(static_cast<BridgeNode*>(nodeStack.top().release()));
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<BridgeNode> nonterminal1 = unique_ptr<BridgeNode>(static_cast<BridgeNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(astnode0->line, astnode0->column);
		nodeStack.push(move(astnode));

		SymbolTable* stable = tblptr.top();//当前的符号表
		if (exprnode->kind == SymbolKind::Variable) {//表达式如果为变量，检查是否声明或赋值
			SymbolTableEntry exprid = stable->get(get<string>(exprnode->name_value));
			if (exprid.type == ValueType::undefined)
				AddSemanticError(astnode0->line, astnode0->column, "未定义标识符" + get<string>(exprnode->name_value));
			if (exprid.isAssigned == false)
				AddSemanticError(astnode0->line, astnode0->column, "表达式未赋值");
		}
		if (nonterminal3->type != exprnode->type)//<类型>与<表达式>类型不匹配
			AddSemanticError(astnode0->line, astnode0->column, "不存在从\"右值\"到\"左值\"的适当转换函数");

		bool defined = stable->isExist(nonterminal1->name);//当前符号表是否存在该变量名
		if (defined)//更新
			stable->update({ nonterminal1->name, nonterminal1->kind, nonterminal3->type, false, true, 0 });
		else
			stable->put({ nonterminal1->name, nonterminal1->kind, nonterminal3->type, false, true, 0 });
		emit(Quadruple{ "=", exprnode->kind == SymbolKind::Constant ? to_string(get<int>(get<TokenValue>(exprnode->name_value))) : get<string>(exprnode->name_value), "-", nonterminal1->name });//只实现i32
	}
	//2.3_3 赋值语句（前置规则0.3、1.2、3.1）----------<变量声明赋值语句> -> let <变量声明内部> '='<表达式> ';'
	else if (prod.left.name == "Identifier_Assign_Line" && prod.right.size() == 5 && prod.right[0].name == TokenTypeToString(TokenType::LET) && prod.right[1].name == "Identifier_inside" && prod.right[2].name == TokenTypeToString(TokenType::Assignment) && prod.right[3].name == "Expression" && prod.right[4].name == TokenTypeToString(TokenType::Semicolon)) {
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));;
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<BridgeNode> nonterminal1 = unique_ptr<BridgeNode>(static_cast<BridgeNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = make_unique<ASTNode>(astnode0->line, astnode0->column);
		nodeStack.push(move(astnode));

		SymbolTable* stable = tblptr.top();//当前的符号表
		if (exprnode->kind == SymbolKind::Variable) {//表达式如果为变量，检查是否声明或赋值
			SymbolTableEntry exprid = stable->get(get<string>(exprnode->name_value));
			if (exprid.type == ValueType::undefined)
				AddSemanticError(astnode0->line, astnode0->column, "未定义标识符" + get<string>(exprnode->name_value));
			if (exprid.isAssigned == false)
				AddSemanticError(astnode0->line, astnode0->column, "表达式未赋值");
		}
		bool defined = stable->isExist(nonterminal1->name);//当前符号表是否存在该变量名
		if (defined)//更新
			stable->update({ nonterminal1->name, nonterminal1->kind, exprnode->type, false, true, 0 });
		else
			stable->put({ nonterminal1->name, nonterminal1->kind, exprnode->type, false, true, 0 });

		emit(Quadruple{ "=", exprnode->kind == SymbolKind::Constant ? to_string(get<int>(get<TokenValue>(exprnode->name_value))) : get<string>(exprnode->name_value), "-", nonterminal1->name });//只实现i32
	}
	//3.1_1 基本表达式（前置规则0.3）----------<语句> -> <表达式> ';'
	else if (prod.left.name == "Line" && prod.right.size() == 2 && prod.right[0].name == "Expression" && prod.right[1].name == TokenTypeToString(TokenType::Semicolon)) {
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));;
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode = make_unique<Stmt>(exprnode->line, exprnode->column, false);
		nodeStack.push(move(stmtnode));
	}
	//3.1_2 基本表达式（前置规则0.3）----------<表达式> -> <加法表达式>
	else if (prod.left.name == "Expression" && prod.right.size() == 1 && prod.right[0].name == "Plus_Expression") {
		//unique_ptr<ExprNode> exprnode0 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));;
		//nodeStack.pop();
		//unique_ptr<ExprNode> exprnode = make_unique<ExprNode>(exprnode0->line, exprnode0->column, exprnode0->name, exprnode0->type, exprnode0->kind, exprnode0->value);
		//nodeStack.push(move(exprnode));
	}
	//3.1_3 基本表达式（前置规则0.3）----------<加法表达式> ->  <项> 
	else if (prod.left.name == "Plus_Expression" && prod.right.size() == 1 && prod.right[0].name == "Term") {
		//unique_ptr<ExprNode> exprnode0 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));;
		//nodeStack.pop();
		//unique_ptr<ExprNode> exprnode = make_unique<ExprNode>(exprnode0->line, exprnode0->column, exprnode0->name, exprnode0->type, exprnode0->kind, exprnode0->value);
		//nodeStack.push(move(exprnode));
	}
	//3.1_4 基本表达式（前置规则0.3）----------<项> -> <因子>
	else if (prod.left.name == "Term" && prod.right.size() == 1 && prod.right[0].name == "Factor") {
		//unique_ptr<ExprNode> exprnode0 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));;
		//nodeStack.pop();
		//unique_ptr<ExprNode> exprnode = make_unique<ExprNode>(exprnode0->line, exprnode0->column, exprnode0->name, exprnode0->type, exprnode0->kind, exprnode0->value);
		//nodeStack.push(move(exprnode));
	}
	//3.1_5 基本表达式（前置规则0.3）----------<因子> -> <元素> 
	else if (prod.left.name == "Factor" && prod.right.size() == 1 && prod.right[0].name == "Element") {
		//unique_ptr<ExprNode> exprnode0 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));;
		//nodeStack.pop();
		//unique_ptr<ExprNode> exprnode = make_unique<ExprNode>(exprnode0->line, exprnode0->column, exprnode0->name, exprnode0->type, exprnode0->kind, exprnode0->value);
		//nodeStack.push(move(exprnode));
	}
	//3.1_6 基本表达式（前置规则0.3）----------<元素> -> <NUM>  | <可赋值元素> | '(' <表达式> ')'
	else if (prod.left.name == "Element" && prod.right.size() == 1 && prod.right[0].name == TokenTypeToString(TokenType::i32_)) {
		unique_ptr<I32num> i32node = unique_ptr<I32num>(static_cast<I32num*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode = make_unique<ExprNode>(i32node->line, i32node->column, i32node->type, i32node->value);
		nodeStack.push(move(exprnode));
	}
	else if (prod.left.name == "Element" && prod.right.size() == 1 && prod.right[0].name == "Assignable_element") {
		unique_ptr<BridgeNode> nonterminal = unique_ptr<BridgeNode>(static_cast<BridgeNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode = make_unique<ExprNode>(nonterminal->line, nonterminal->column, tblptr.top()->get(nonterminal->name).type, nonterminal->name);
		nodeStack.push(move(exprnode));
	}
	else if (prod.left.name == "Element" && prod.right.size() == 3 && prod.right[0].name == TokenTypeToString(TokenType::ParenthesisL) && prod.right[1].name == "Expression" && prod.right[2].name == TokenTypeToString(TokenType::ParenthesisR)) {
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode1 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode;
		if (exprnode1->kind == SymbolKind::Constant)
			exprnode = make_unique<ExprNode>(astnode->line, astnode->column, exprnode1->type, get<TokenValue>(exprnode1->name_value));
		else
			exprnode = make_unique<ExprNode>(astnode->line, astnode->column, exprnode1->type, get<string>(exprnode1->name_value));
		nodeStack.push(move(exprnode));
	}
	//3.2_1 表达式增加计算和比较（前置规则3.1）----------<表达式> -> <表达式> <比较运算符> <加法表达式>
	else if (prod.left.name == "Expression" && prod.right.size() == 3 && prod.right[0].name == "Expression" && prod.right[1].name == "Comparison" && prod.right[2].name == "Plus_Expression") {
		unique_ptr<ExprNode> exprnode2 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Op> opnode = unique_ptr<Op>(static_cast<Op*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode0 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();

		if (exprnode0->kind == SymbolKind::Variable && exprnode0->type == ValueType::undefined)//两个操作数中任一未声明
			AddSemanticError(exprnode0->line, exprnode0->column, "未定义标识符\"" + get<string>(exprnode0->name_value) + '\"');
		if (exprnode2->kind == SymbolKind::Variable && exprnode2->type == ValueType::undefined)//两个操作数中任一未声明
			AddSemanticError(exprnode2->line, exprnode2->column, "未定义标识符\"" + get<string>(exprnode2->name_value) + '\"');
		if (exprnode0->type == ValueType::none)//两个操作数类型任一无类型
			AddSemanticError(exprnode0->line, exprnode0->column, string("\"") + opnode->GetOperatorString() + "\"左表达式必须包含算术、未区分范围的枚举或指针类型，但它不具有类型");
		if (exprnode2->type == ValueType::none)//两个操作数类型任一无类型
			AddSemanticError(exprnode2->line, exprnode2->column, string("\"") + opnode->GetOperatorString() + "\"右表达式必须包含算术、未区分范围的枚举或指针类型，但它不具有类型");
		if (exprnode0->type != ValueType::_i32 && exprnode0->type != ValueType::_bool)
			AddSemanticError(exprnode0->line, exprnode0->column, string("比较表达式左侧\"") + opnode->GetOperatorString() + "\"应具有i32或布尔类型");
		if (exprnode2->type != ValueType::_i32 && exprnode2->type != ValueType::_bool)//两个操作数类型不匹配;
			AddSemanticError(exprnode2->line, exprnode2->column, string("比较表达式右侧\"") + opnode->GetOperatorString() + "\"应具有i32或布尔类型");
		string newtempname = Id::newtemp();
		tblptr.top()->put(SymbolTableEntry{ newtempname, SymbolKind::Variable, ValueType::_bool, false, true, 0 });//目前只实现i32
		unique_ptr<ExprNode> exprnode = make_unique<ExprNode>(exprnode0->line, exprnode0->column, ValueType::_bool, newtempname);
		emit(Quadruple{ string("j") + opnode->GetOperatorString(),
			exprnode0->kind == SymbolKind::Constant ? to_string(get<int>(get<TokenValue>(exprnode0->name_value))) : get<string>(exprnode0->name_value),
			exprnode2->kind == SymbolKind::Constant ? to_string(get<int>(get<TokenValue>(exprnode2->name_value))) : get<string>(exprnode2->name_value),
			to_string(nextstat + 3) });
		emit(Quadruple{ "=", "0", "-", newtempname });
		emit(Quadruple{ "j", "-", "-", to_string(nextstat + 2) });
		emit(Quadruple{ "=", "1", "-", newtempname });
		nodeStack.push(move(exprnode));
	}
	//3.2_2 表达式增加计算和比较（前置规则3.1）----------<加法表达式> -> <加法表达式> <加减运算符> <项>
	else if (prod.left.name == "Plus_Expression" && prod.right.size() == 3 && prod.right[0].name == "Plus_Expression" && prod.right[1].name == "AS_operator" && prod.right[2].name == "Term") {
		unique_ptr<ExprNode> exprnode2 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Op> opnode = unique_ptr<Op>(static_cast<Op*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode0 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();

		if (exprnode0->kind == SymbolKind::Variable && exprnode0->type == ValueType::undefined)//两个操作数中任一未声明
			AddSemanticError(exprnode0->line, exprnode0->column, "未定义标识符\"" + get<string>(exprnode0->name_value) + '\"');
		if (exprnode2->kind == SymbolKind::Variable && exprnode2->type == ValueType::undefined)//两个操作数中任一未声明
			AddSemanticError(exprnode2->line, exprnode2->column, "未定义标识符\"" + get<string>(exprnode2->name_value) + '\"');
		if (exprnode0->type == ValueType::none)//两个操作数类型任一无类型
			AddSemanticError(exprnode0->line, exprnode0->column, string("\"") + opnode->GetOperatorString() + "\"左表达式必须包含算术、未区分范围的枚举或指针类型，但它不具有类型");
		if (exprnode2->type == ValueType::none)//两个操作数类型任一无类型
			AddSemanticError(exprnode2->line, exprnode2->column, string("\"") + opnode->GetOperatorString() + "\"右表达式必须包含算术、未区分范围的枚举或指针类型，但它不具有类型");
		if (exprnode0->type != exprnode2->type)//两个操作数类型不匹配;
			AddSemanticError(exprnode0->line, exprnode0->column, string("没有与左右表达式类型匹配的\"") + opnode->GetOperatorString() + "\"运算符");
		string newtempname = Id::newtemp();
		tblptr.top()->put(SymbolTableEntry{ newtempname, SymbolKind::Variable, ValueType::_i32, false, true, 0 });//目前只实现i32
		unique_ptr<ExprNode> exprnode = make_unique<ExprNode>(exprnode0->line, exprnode0->column, ValueType::_i32, newtempname);
		emit(Quadruple{ opnode->GetOperatorString(),
			exprnode0->kind == SymbolKind::Constant ? to_string(get<int>(get<TokenValue>(exprnode0->name_value))) : get<string>(exprnode0->name_value),
			exprnode2->kind == SymbolKind::Constant ? to_string(get<int>(get<TokenValue>(exprnode2->name_value))) : get<string>(exprnode2->name_value),
			newtempname });
		nodeStack.push(move(exprnode));
	}
	//3.2_3 表达式增加计算和比较（前置规则3.1）----------<项> -> <项> <乘除运算符> <因子>
	else if (prod.left.name == "Term" && prod.right.size() == 3 && prod.right[0].name == "Term" && prod.right[1].name == "MD_operator" && prod.right[2].name == "Factor") {
		unique_ptr<ExprNode> exprnode2 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Op> opnode = unique_ptr<Op>(static_cast<Op*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode0 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();

		if (exprnode0->kind == SymbolKind::Variable && exprnode0->type == ValueType::undefined)//两个操作数中任一未声明
			AddSemanticError(exprnode0->line, exprnode0->column, "未定义标识符\"" + get<string>(exprnode0->name_value) + '\"');
		if (exprnode2->kind == SymbolKind::Variable && exprnode2->type == ValueType::undefined)//两个操作数中任一未声明
			AddSemanticError(exprnode2->line, exprnode2->column, "未定义标识符\"" + get<string>(exprnode2->name_value) + '\"');
		if (exprnode0->type == ValueType::none)//两个操作数类型任一无类型
			AddSemanticError(exprnode0->line, exprnode0->column, string("\"") + opnode->GetOperatorString() + "\"左表达式必须包含算术、未区分范围的枚举或指针类型，但它不具有类型");
		if (exprnode2->type == ValueType::none)//两个操作数类型任一无类型
			AddSemanticError(exprnode2->line, exprnode2->column, string("\"") + opnode->GetOperatorString() + "\"右表达式必须包含算术、未区分范围的枚举或指针类型，但它不具有类型");
		if (exprnode0->type != exprnode2->type)//两个操作数类型不匹配;
			AddSemanticError(exprnode0->line, exprnode0->column, string("没有与左右表达式类型匹配的\"") + opnode->GetOperatorString() + "\"运算符");
		string newtempname = Id::newtemp();
		tblptr.top()->put(SymbolTableEntry{ newtempname, SymbolKind::Variable, ValueType::_i32, false, true, 0 });//目前只实现i32
		unique_ptr<ExprNode> exprnode = make_unique<ExprNode>(exprnode0->line, exprnode0->column, ValueType::_i32, newtempname);
		emit(Quadruple{ opnode->GetOperatorString(),
			exprnode0->kind == SymbolKind::Constant ? to_string(get<int>(get<TokenValue>(exprnode0->name_value))) : get<string>(exprnode0->name_value),
			exprnode2->kind == SymbolKind::Constant ? to_string(get<int>(get<TokenValue>(exprnode2->name_value))) : get<string>(exprnode2->name_value),
			newtempname });
		nodeStack.push(move(exprnode));
	}
	//3.2_4 表达式增加计算和比较（前置规则3.1）----------<比较运算符> -> '<' | '<=' | '>' | '>='  | '==' | '!='
	else if (prod.left.name == "Comparison" && prod.right.size() == 1 && (TokenTypeToString(TokenType::LessThan) || TokenTypeToString(TokenType::LessOrEqual) || prod.right[0].name == TokenTypeToString(TokenType::Equality) || TokenTypeToString(TokenType::Inequality) || TokenTypeToString(TokenType::GreaterThan) || TokenTypeToString(TokenType::GreaterOrEqual))) {
		unique_ptr<Op> opnode0 = unique_ptr<Op>(static_cast<Op*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Op> opnode = make_unique<Op>(opnode0->line, opnode0->column, opnode0->op);
		nodeStack.push(move(opnode));
	}
	//3.2_5 表达式增加计算和比较（前置规则3.1）----------<加减运算符> -> '+' | '-'
	else if (prod.left.name == "AS_operator" && prod.right.size() == 1 && (prod.right[0].name == TokenTypeToString(TokenType::Addition) || prod.right[0].name == TokenTypeToString(TokenType::Subtraction))) {
		unique_ptr<Op> opnode0 = unique_ptr<Op>(static_cast<Op*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Op> opnode = make_unique<Op>(opnode0->line, opnode0->column, opnode0->op);
		nodeStack.push(move(opnode));
	}
	//3.2_6 表达式增加计算和比较（前置规则3.1）----------<乘除运算符> -> '*' | '/'
	else if (prod.left.name == "MD_operator" && prod.right.size() == 1 && (prod.right[0].name == TokenTypeToString(TokenType::Multiplication) || prod.right[0].name == TokenTypeToString(TokenType::Division))) {
		unique_ptr<Op> opnode0 = unique_ptr<Op>(static_cast<Op*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Op> opnode = make_unique<Op>(opnode0->line, opnode0->column, opnode0->op);
		nodeStack.push(move(opnode));
	}
	//3.3_1 函数调用（前置规则3.1）----------<元素> -> <ID> '(' <实参列表> ')'
	else if (prod.left.name == "Element" && prod.right.size() == 4 && prod.right[0].name == TokenTypeToString(TokenType::Identifier) && prod.right[1].name == TokenTypeToString(TokenType::ParenthesisL) && prod.right[2].name == "Real_Parameter_List" && prod.right[3].name == TokenTypeToString(TokenType::ParenthesisR)) {
		nodeStack.pop();
		unique_ptr<ParaListNode> paranode = unique_ptr<ParaListNode>(static_cast<ParaListNode*>(nodeStack.top().release()));
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<Id> idnode = unique_ptr<Id>(static_cast<Id*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode;

		//找到当前函数的函数表项
		stack<ProcedureTable*> tempprolist;
		const ProcedureTableEntry* currentp = NULL;
		while (currentp == NULL && !proptr.empty()) {
			ProcedureTable* ptable = proptr.top();
			tempprolist.push(ptable);
			proptr.pop();
			//for (const ProcedureTableEntry& tempp : *ptable)
			//	if (tempp.ID == idnode->name)
			//		currentp = &tempp;
			for (size_t i = 0; i < ptable->size(); ++i) {
				const ProcedureTableEntry& tempp = (*ptable)[i];
				if (tempp.ID == idnode->name) {
					currentp = &tempp;
					break;
				}
			}
		}
		while (!tempprolist.empty()) {
			ProcedureTable* ptable = tempprolist.top();
			proptr.push(ptable);
			tempprolist.pop();
		}
		if (currentp == NULL) {
			AddSemanticError(idnode->line, idnode->column, "未定义标识符\"" + idnode->name + '\"');
			exprnode = make_unique<ExprNode>(idnode->line, idnode->column, ValueType::none, string(""));
		}
		else {
			if (currentp->paratype != paranode->paratype)
				AddSemanticError(idnode->line, idnode->column, "函数\"" + idnode->name + "\"调用形参与实参不符");
			if (currentp->returntype == ValueType::none) {
				emit(Quadruple{ "call", to_string(currentp->addr), "-", "-" });
				exprnode = make_unique<ExprNode>(idnode->line, idnode->column, ValueType::none, string(""));
			}
			else {
				string newtempname = Id::newtemp();
				tblptr.top()->put(SymbolTableEntry{ newtempname, SymbolKind::Variable, currentp->returntype, false, true, 0 });//目前只实现i32
				exprnode = make_unique<ExprNode>(idnode->line, idnode->column, currentp->returntype, newtempname);
				emit(Quadruple{ "call", to_string(currentp->addr), "-", newtempname });
			}
		}
		nodeStack.push(move(exprnode));
	}
	//3.3_2 函数调用（前置规则3.1）----------<实参列表>-> 空 | <表达式> | <表达式> ',' <实参列表>
	else if (prod.left.name == "Real_Parameter_List" && prod.right.size() == 0) {
		unique_ptr<ParaListNode> paranode = make_unique<ParaListNode>(0, 0, vector<ValueType>());
		nodeStack.push(move(paranode));
	}
	else if (prod.left.name == "Real_Parameter_List" && prod.right.size() == 1 && prod.right[0].name == "Expression") {
		unique_ptr<ExprNode> exprnode = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();

		if (exprnode->kind == SymbolKind::Variable && exprnode->type == ValueType::undefined)
			AddSemanticError(exprnode->line, exprnode->column, "未定义标识符\"" + get<string>(exprnode->name_value) + '\"');
		unique_ptr<ParaListNode> paranode = make_unique<ParaListNode>(exprnode->line, exprnode->column, vector<ValueType>{ exprnode->type });
		nodeStack.push(move(paranode));
		emit(Quadruple{ "para", exprnode->kind == SymbolKind::Constant ? to_string(get<int>(get<TokenValue>(exprnode->name_value))) : get<string>(exprnode->name_value), "-", "-" });
	}
	else if (prod.left.name == "Real_Parameter_List" && prod.right.size() == 3 && prod.right[0].name == "Expression" && prod.right[1].name == TokenTypeToString(TokenType::Comma) && prod.right[2].name == "Real_Parameter_List") {
		unique_ptr<ParaListNode> paranode2 = unique_ptr<ParaListNode>(static_cast<ParaListNode*>(nodeStack.top().release()));
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();

		if (exprnode->kind == SymbolKind::Variable && exprnode->type == ValueType::undefined)
			AddSemanticError(exprnode->line, exprnode->column, "未定义标识符\"" + get<string>(exprnode->name_value) + '\"');
		paranode2->paratype.push_back(exprnode->type);//合并实参列表
		unique_ptr<ParaListNode> paranode = make_unique<ParaListNode>(exprnode->line, exprnode->column, paranode2->paratype);
		nodeStack.push(move(paranode));
		emit(Quadruple{ "para", exprnode->kind == SymbolKind::Constant ? to_string(get<int>(get<TokenValue>(exprnode->name_value))) : get<string>(exprnode->name_value), "-", "-" });
	}
	//4.1_1 选择结构（前置规则1.2、3.1）----------<语句> -> <if语句>
	else if (prod.left.name == "Line" && prod.right.size() == 1 && prod.right[0].name == "IF_Line") {
		//unique_ptr<Stmt> stmtnode0 = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		//nodeStack.pop();
		//unique_ptr<Stmt> stmtnode = make_unique<Stmt>(*stmtnode0);
		//nodeStack.push(move(stmtnode));
	}
	//4.1_2 选择结构（前置规则1.2、3.1）----------<if语句> ->  if <表达式> <B> <Q> <语句块> <else部分>
	else if (prod.left.name == "IF_Line" && prod.right.size() == 6 && prod.right[0].name == TokenTypeToString(TokenType::IF) && prod.right[1].name == "Expression" && prod.right[2].name == "B" && prod.right[3].name == "Q" && prod.right[4].name == "Line_Block" && prod.right[5].name == "ELSE_Part") {
		unique_ptr<ElseStmt> elsestmtnode5 = unique_ptr<ElseStmt>(static_cast<ElseStmt*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode4 = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Q> qnode3 = unique_ptr<Q>(static_cast<Q*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<B> bnode2 = unique_ptr<B>(static_cast<B*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode1 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode0 = move(nodeStack.top());
		nodeStack.pop();

		backpatch(bnode2->truelist, qnode3->quad);
		unique_ptr<Stmt> stmtnode;
		if (elsestmtnode5->firstquad != -1) {
			backpatch(bnode2->falselist, elsestmtnode5->firstquad);
			stmtnode = make_unique<Stmt>(astnode0->line, astnode0->column, stmtnode4->returned && elsestmtnode5->returned, merge(stmtnode4->nextlist, elsestmtnode5->nextlist));
		}
		else
			stmtnode = make_unique<Stmt>(astnode0->line, astnode0->column, stmtnode4->returned && elsestmtnode5->returned, merge(stmtnode4->nextlist, bnode2->falselist));
		nodeStack.push(move(stmtnode));
	}
	//4.1_3 选择结构（前置规则1.2、3.1）----------<else部分> -> 空 | <P> else <Q> <语句块>
	else if (prod.left.name == "ELSE_Part" && prod.right.size() == 0) {
		unique_ptr<ElseStmt> elsestmtnode = make_unique<ElseStmt>(0, 0, false, -1);
		nodeStack.push(move(elsestmtnode));
	}
	else if (prod.left.name == "ELSE_Part" && prod.right.size() == 4 && prod.right[0].name == "P" && prod.right[1].name == TokenTypeToString(TokenType::ELSE) && prod.right[2].name == "Q" && prod.right[3].name == "Line_Block") {
		unique_ptr<Stmt> stmtnode = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Q> qnode = unique_ptr<Q>(static_cast<Q*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<P> pnode = unique_ptr<P>(static_cast<P*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ElseStmt> elsestmtnode = make_unique<ElseStmt>(astnode->line, astnode->column, stmtnode->returned, qnode->quad, merge(pnode->nextlist, stmtnode->nextlist));
		nodeStack.push(move(elsestmtnode));
	}
	//4.2 增加else if（前置规则4.1）----------<else部分> -> <P> else if <Q> <表达式> <B> <Q> <语句块> <else部分>
	else if (prod.left.name == "ELSE_Part" && prod.right.size() == 9 && prod.right[0].name == "P" && prod.right[1].name == TokenTypeToString(TokenType::ELSE) && prod.right[2].name == TokenTypeToString(TokenType::IF) && prod.right[3].name == "Q" && prod.right[4].name == "Expression" && prod.right[5].name == "B" && prod.right[6].name == "Q" && prod.right[7].name == "Line_Block" && prod.right[8].name == "ELSE_Part") {
		unique_ptr<ElseStmt> elsestmtnode7 = unique_ptr<ElseStmt>(static_cast<ElseStmt*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Q> qnode5 = unique_ptr<Q>(static_cast<Q*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<B> bnode = unique_ptr<B>(static_cast<B*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Q> qnode3 = unique_ptr<Q>(static_cast<Q*>(nodeStack.top().release()));
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = move(nodeStack.top());
		nodeStack.pop();
		unique_ptr<P> pnode = unique_ptr<P>(static_cast<P*>(nodeStack.top().release()));
		nodeStack.pop();

		backpatch(bnode->truelist, qnode5->quad);
		unique_ptr<Stmt> elsestmtnode;
		if (elsestmtnode7->firstquad != -1) {
			backpatch(bnode->falselist, elsestmtnode7->firstquad);
			elsestmtnode = make_unique<ElseStmt>(astnode->line, astnode->column, stmtnode->returned && elsestmtnode7->returned, qnode3->quad, merge(merge(stmtnode->nextlist, pnode->nextlist), elsestmtnode7->nextlist));
		}
		else
			elsestmtnode = make_unique<ElseStmt>(astnode->line, astnode->column, stmtnode->returned && elsestmtnode7->returned, qnode3->quad, merge(merge(stmtnode->nextlist, pnode->nextlist), bnode->falselist));
		nodeStack.push(move(elsestmtnode));
	}
	//4.0 选择结构（前置规则1.2、3.1）----------<B> -> 空 | <Q> -> 空 | <P> -> 空
	else if (prod.left.name == "B" && prod.right.size() == 0) {
		size_t trueaddr = nextstat;
		emit(Quadruple{ "j=", tblptr.top()->table[tblptr.top()->table.size() - 1].ID, to_string(1), to_string(0) });
		size_t falseaddr = nextstat;
		emit(Quadruple{ "j", "-", "-", to_string(0) });
		unique_ptr<B> bnode = make_unique<B>(0, 0, vector<size_t>{trueaddr}, vector<size_t>{falseaddr});
		nodeStack.push(move(bnode));
	}
	else if (prod.left.name == "Q" && prod.right.size() == 0) {
		unique_ptr<Q> qnode = make_unique<Q>(0, 0, nextstat);
		nodeStack.push(move(qnode));
	}
	else if (prod.left.name == "P" && prod.right.size() == 0) {
		unique_ptr<P> stmtnode = make_unique<P>(0, 0, makelist(nextstat));
		nodeStack.push(move(stmtnode));
		emit(Quadruple{ "j", "-", "-", to_string(0) });
	}
	//5.1 while循环结构（前置规则1.2、3.1）----------<语句> -> <循环语句>
	else if (prod.left.name == "Line" && prod.right.size() == 1 && prod.right[0].name == "Cycle_Line") {
		//unique_ptr<Stmt> stmtnode0 = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		//nodeStack.pop();
		//unique_ptr<Stmt> stmtnode = make_unique<Stmt>(*stmtnode0);
		//nodeStack.push(move(stmtnode));
	}
	//5.1 while循环结构（前置规则1.2、3.1）----------<循环语句> -> <while语句>
	else if (prod.left.name == "Cycle_Line" && prod.right.size() == 1 && prod.right[0].name == "While_Line") {
		//unique_ptr<Stmt> stmtnode0 = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		//nodeStack.pop();
		//unique_ptr<Stmt> stmtnode = make_unique<Stmt>(*stmtnode0);
		//nodeStack.push(move(stmtnode));
	}
	//5.1 while循环结构（前置规则1.2、3.1）----------<while语句>  -> while <Q> <表达式> <B> <Q> <语句块>
	else if (prod.left.name == "While_Line" && prod.right.size() == 6 && prod.right[0].name == TokenTypeToString(TokenType::WHILE) && prod.right[1].name == "Q" && prod.right[2].name == "Expression" && prod.right[3].name == "B" && prod.right[4].name == "Q" && prod.right[5].name == "Line_Block") {
		unique_ptr<Stmt> stmtnode5 = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Q> qnode4 = unique_ptr<Q>(static_cast<Q*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<B> bnode3 = unique_ptr<B>(static_cast<B*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode2 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Q> qnode1 = unique_ptr<Q>(static_cast<Q*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode0 = unique_ptr<ASTNode>(static_cast<ASTNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode = make_unique<Stmt>(astnode0->line, astnode0->column, false, merge(bnode3->falselist, stmtnode5->breaklist));
		nodeStack.push(move(stmtnode));

		backpatch(stmtnode5->nextlist, qnode4->quad);
		backpatch(bnode3->truelist, qnode4->quad);
		backpatch(stmtnode5->continuelist, qnode1->quad);
		emit(Quadruple{ "j","-","-", to_string(qnode1->quad) });
	}
	//5.2_1 for循环结构（前置规则5.1）----------<循环语句> -> <for语句> 
	else if (prod.left.name == "Cycle_Line" && prod.right.size() == 1 && prod.right[0].name == "For_Line") {
		//unique_ptr<Stmt> stmtnode0 = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		//nodeStack.pop();
		//unique_ptr<Stmt> stmtnode = make_unique<Stmt>(*stmtnode0);
		//nodeStack.push(move(stmtnode));
	}
	//5.2_2_1 for循环结构（前置规则5.1）----------<for变量迭代结构>->  for <变量声明内部> in  <可迭代结构>
	else if (prod.left.name == "Identifier_Iterative_Structure" && prod.right.size() == 4 && prod.right[0].name == TokenTypeToString(TokenType::FOR) && prod.right[1].name == "Identifier_inside" && prod.right[2].name == TokenTypeToString(TokenType::IN) && prod.right[3].name == "Iterative_Structure") {
		unique_ptr<IterableStructure> irinode = unique_ptr<IterableStructure>(static_cast<IterableStructure*>(nodeStack.top().release()));
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<BridgeNode> nonterminal = unique_ptr<BridgeNode>(static_cast<BridgeNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = unique_ptr<ASTNode>(static_cast<ASTNode*>(nodeStack.top().release()));
		nodeStack.pop();

		SymbolTable* stable = tblptr.top();
		stable->put(SymbolTableEntry{ nonterminal->name, SymbolKind::Variable, irinode->left.type, false, true, 0 });
		emit(Quadruple{ "=", irinode->left.kind == SymbolKind::Constant ? to_string(get<int>(get<TokenValue>(irinode->left.name_value))) : get<string>(irinode->left.name_value), "-", nonterminal->name });
		int firstquad = nextstat;
		vector<size_t> truelist = { nextstat };
		emit(Quadruple{ "j<", nonterminal->name, irinode->right.kind == SymbolKind::Constant ? to_string(get<int>(get<TokenValue>(irinode->right.name_value))) : get<string>(irinode->right.name_value), to_string(0) });
		vector<size_t> falselist = { nextstat };
		emit(Quadruple{ "j", "-", "-", to_string(0) });
		unique_ptr<ForIri> foririnode = make_unique<ForIri>(astnode->line, astnode->column, firstquad, truelist, falselist, nonterminal->name, *irinode);
		nodeStack.push(move(foririnode));
	}
	//5.2_2_2 for循环结构（前置规则5.1）----------<for语句>-> <for变量迭代结构> <Q> <语句块>
	else if (prod.left.name == "For_Line" && prod.right.size() == 3 && prod.right[0].name == "Identifier_Iterative_Structure" && prod.right[1].name == "Q" && prod.right[2].name == "Line_Block") {
		unique_ptr<Stmt> stmtnode2 = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Q> qnode = unique_ptr<Q>(static_cast<Q*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ForIri> foririnode = unique_ptr<ForIri>(static_cast<ForIri*>(nodeStack.top().release()));
		nodeStack.pop();

		backpatch(foririnode->truelist, qnode->quad);
		backpatch(stmtnode2->nextlist, foririnode->firstquad);
		backpatch(stmtnode2->continuelist, foririnode->firstquad);
		/*无法解决重影错误
		for (int i;;) {
			float i;
		}
		*/
		SymbolTableEntry sym = tblptr.top()->get(foririnode->name);
		if (sym.type == ValueType::_i32)
			emit(Quadruple{ "+", foririnode->name, to_string(1), foririnode->name });
		else if (sym.type == ValueType::_array)
			;//???????????????????????????????????????????????????????????????????????????????
		else {
			//支持其余类型时添加
		}
		emit(Quadruple{ "j", "-", "-", to_string(foririnode->firstquad) });
		unique_ptr<Stmt> stmtnode = make_unique<Stmt>(foririnode->line, foririnode->column, false, merge(foririnode->falselist, stmtnode2->breaklist));
		nodeStack.push(move(stmtnode));
	}
	//5.2 for循环结构（前置规则5.1）----------<可迭代结构> ->  <表达式> '..' <表达式>
	else if (prod.left.name == "Iterative_Structure" && prod.right.size() == 3 && prod.right[0].name == "Expression" && prod.right[1].name == TokenTypeToString(TokenType::RangeOperator) && prod.right[2].name == "Expression") {
		unique_ptr<ExprNode> exprnode2 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();
		nodeStack.pop();
		unique_ptr<ExprNode> exprnode0 = unique_ptr<ExprNode>(static_cast<ExprNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<IterableStructure> irinode = make_unique<IterableStructure>(exprnode0->line, exprnode0->column, *exprnode0, *exprnode2);
		nodeStack.push(move(irinode));
	}
	//5.3_1 loop循环结构（前置规则5.1）----------<循环语句> -> <loop语句>
	else if (prod.left.name == "Cycle_Line" && prod.right.size() == 1 && prod.right[0].name == "Loop_Line") {
		//unique_ptr<Stmt> stmtnode0 = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		//nodeStack.pop();
		//unique_ptr<Stmt> stmtnode = make_unique<Stmt>(*stmtnode0);
		//nodeStack.push(move(stmtnode));
	}
	//5.3_2 loop循环结构（前置规则5.1）----------<loop语句>->loop <Q> <语句块>
	else if (prod.left.name == "Loop_Line" && prod.right.size() == 3 && prod.right[0].name == TokenTypeToString(TokenType::LOOP) && prod.right[1].name == "Q" && prod.right[2].name == "Line_Block") {
		unique_ptr<Stmt> stmtnode2 = unique_ptr<Stmt>(static_cast<Stmt*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Q> qnode = unique_ptr<Q>(static_cast<Q*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = unique_ptr<ASTNode>(static_cast<ASTNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode = make_unique<Stmt>(astnode->line, astnode->column, false, stmtnode2->breaklist);
		nodeStack.push(move(stmtnode));

		backpatch(stmtnode2->nextlist, qnode->quad);
		backpatch(stmtnode2->continuelist, qnode->quad);
		emit(Quadruple{ "j", "-", "-", to_string(qnode->quad) });
	}
	//5.4 增加break和continue（前置规则5.1）----------<语句> -> break ';' | continue ';'
	else if (prod.left.name == "Line" && prod.right.size() == 2 && prod.right[0].name == TokenTypeToString(TokenType::BREAK) && prod.right[1].name == TokenTypeToString(TokenType::Semicolon)) {
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = unique_ptr<ASTNode>(static_cast<ASTNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode = make_unique<Stmt>(astnode->line, astnode->column, false, vector<size_t>{ nextstat }, 1);
		nodeStack.push(move(stmtnode));
		emit({ "j", "-", "-", to_string(0) });
	}
	else if (prod.left.name == "Line" && prod.right.size() == 2 && prod.right[0].name == TokenTypeToString(TokenType::CONTINUE) && prod.right[1].name == TokenTypeToString(TokenType::Semicolon)) {
		nodeStack.pop();
		unique_ptr<ASTNode> astnode = unique_ptr<ASTNode>(static_cast<ASTNode*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<Stmt> stmtnode = make_unique<Stmt>(astnode->line, astnode->column, false, vector<size_t>{ nextstat }, 2);
		nodeStack.push(move(stmtnode));
		emit({ "j", "-", "-", to_string(0) });
	}
	//6.1 声明不可变变量（前置规则0.2）----------<变量声明内部> -> <ID>
	else if (prod.left.name == "Identifier_inside" && prod.right.size() == 1 && prod.right[0].name == TokenTypeToString(TokenType::Identifier)) {
		unique_ptr<Id> idnode = unique_ptr<Id>(static_cast<Id*>(nodeStack.top().release()));
		nodeStack.pop();
		unique_ptr<BridgeNode> nonterminal = make_unique<BridgeNode>(idnode->line, idnode->column, SymbolKind::Constant, idnode->name);
		nodeStack.push(move(nonterminal));
	}
	else {
		AddSemanticError(INT_MAX, INT_MAX, "产生式未匹配，未执行语义分析");
	}
}

void SemanticAnalyzer::mkleaf(Token token)
{
	if (token.type == TokenType::Identifier)
		mkleaf(token.line, token.column, get<string>(token.value));
	else if (token.type == TokenType::i32_)
		mkleaf(token.line, token.column, get<int>(token.value));
	else if (token.type == TokenType::Addition || token.type == TokenType::Subtraction || token.type == TokenType::Multiplication || token.type == TokenType::Division || token.type == TokenType::Equality || token.type == TokenType::GreaterThan || token.type == TokenType::GreaterOrEqual || token.type == TokenType::LessThan || token.type == TokenType::LessOrEqual || token.type == TokenType::Inequality)
		mkleaf(token.line, token.column, token.type);
	else
		mkleaf(token.line, token.column);
	return;
}

void SemanticAnalyzer::addJumpToMain()
{
	for (const ProcedureTableEntry& prod : procedureList)
		if (prod.ID == "main") {
			qList.emplace(qList.begin(), Quadruple{ "j", "-", "-", to_string(prod.addr) });
			return;
		}
	qList.emplace(qList.begin(), Quadruple{ "j", "-", "-", "-" });
	AddSemanticError(0, 0, "程序缺少main函数");
}

const std::vector<Quadruple>& SemanticAnalyzer::GetqList() const
{
	return qList;
}

const std::vector<ParseError>& SemanticAnalyzer::GetSemanticErrors() const
{
	return semanticErrors;
}