#include "LexicalAnalyzer.h"
#include "Parser.h"
#include "ObjectCodeGenerator.h"
#include <string>
using namespace std;

//#define BACKEND
#ifdef BACKEND
#include "json.hpp"
using json = nlohmann::json;
json TokensJson(const vector<Token>& tokens)
{
	json tokens_array = json::array();
	for (const auto& token : tokens) {
		json token_obj;
		token_obj["type"] = TokenTypeToString(token.type);
		// 根据token类型选择正确的值类型
		if (token.type == Identifier)
			token_obj["value"] = get<string>(token.value);
		else if (token.type == i32_)
			token_obj["value"] = get<int>(token.value);
		else if (token.type == char_)
			token_obj["value"] = string(1, get<char>(token.value)); // 转换char为string
		else if (token.type == string_)
			token_obj["value"] = get<string>(token.value);
		else
			token_obj["value"] = get<int>(token.value);
		// 添加位置信息
		token_obj["line"] = token.line;
		token_obj["column"] = token.column;
		token_obj["length"] = token.length;
		tokens_array.push_back(token_obj);
	}
	return tokens_array;
}
json FirstsJson(const Parser& parser)
{
	const vector<set<enum TokenType>>& firsts = parser.GetFirsts();
	const unordered_map<string, unsigned int>& nonTerminals = parser.GetNonTerminals();
	json firsts_json = json::object();
	vector<pair<string, unsigned>> nts(nonTerminals.begin(), nonTerminals.end());
	sort(nts.begin(), nts.end(), [](auto& a, auto& b) { return a.first < b.first; });
	for (const auto& kv : nts) {
		json first_set = json::array();
		for (const auto& terminal : firsts[kv.second]) {
			first_set.push_back(TokenTypeToString(terminal));
		}
		firsts_json[kv.first] = first_set;
	}
	return firsts_json;
}
json LR1ItemsJson(const Parser& parser)
{
	const vector<LR1ItemSet>& itemsets = parser.GetItemsets();
	const vector<Production>& productions = parser.GetProductions();
	json itemsets_json = json::array();
	for (size_t i = 0; i < itemsets.size(); i++) {
		json itemset_json = json::object();
		itemset_json["id"] = "I" + to_string(i);

		json items = json::array();//[单个LR1项目的每个表达式]
		for (const auto& item : itemsets[i].items) {
			json item_json = json::object();//{ "left": , "right": , ...}
			const Production& prod = productions[item.productionIndex];//获取产生式
			item_json["left"] = prod.left.name;//产生式左部 left : T
			json right_symbols = json::array();//产生式右部[t1, t2, ...]
			for (size_t j = 0; j < prod.right.size(); j++) {
				right_symbols.push_back(prod.right[j].name);
			}
			item_json["right"] = right_symbols;//产生式右部 right : [t1, t2, ...]
			item_json["dotPosition"] = item.dotPosition;//点位置 dotPosition : x
			item_json["lookahead"] = TokenTypeToString(item.lookahead);//前瞻符号 lookahead, T
			// 构造产生式字符串表示
			string production_str = prod.left.name + " -> ";
			for (size_t j = 0; j < prod.right.size(); j++) {
				if (j == item.dotPosition)
					production_str += "· ";
				production_str += prod.right[j].name + " ";
			}
			if (item.dotPosition == prod.right.size())
				production_str += "·";
			production_str += ", " + string(TokenTypeToString(item.lookahead));
			item_json["display"] = production_str;//display : T->T·T, T
			items.push_back(item_json);
		}
		itemset_json["items"] = items;
		itemsets_json.push_back(itemset_json);
	}
	return itemsets_json;
}
json ActionTableJson(const Parser& parser)
{
	const auto& actionTable = parser.GetActionTable();
	json action_table_json = json::array();
	for (size_t i = 0; i < actionTable.size(); i++) {// 遍历所有状态
		json row = json::object();
		row["state"] = "I" + to_string(i);

		json actions = json::object();
		// 遍历所有终结符（包括空终结符）
		for (int j = 1; j <= int(TokenType::End); j++) {
			string token_name = TokenTypeToString(TokenType(j));
			const auto& entry = actionTable[i][j];

			// 根据动作类型生成字符串表示
			string action_str = "-"; // 默认为空
			switch (entry.act) {
			case Action::shift:
				action_str = "s" + to_string(entry.num);
				break;
			case Action::reduce:
				action_str = "r" + to_string(entry.num);
				break;
			case Action::accept:
				action_str = "acc";
				break;
			default:
				// 保持默认值"-"
				break;
			}
			actions[token_name] = action_str;
		}

		row["actions"] = actions;
		action_table_json.push_back(row);
	}
	return action_table_json;
}
json GotoTableJson(const Parser& parser)
{
	const auto& gotoTable = parser.GetGotoTable();
	const unordered_map<string, unsigned int>& nonTerminals = parser.GetNonTerminals();
	json goto_table_json = json::array();
	for (size_t i = 0; i < gotoTable.size(); i++) {//遍历所有状态
		json row = json::object();
		row["state"] = "I" + to_string(i);

		json goto_entries = json::object();
		// 遍历所有非终结符
		size_t map_idx = 0;
		for (const auto& map : nonTerminals) {
			// 非终结符名称和对应的目标状态
			string nt_name = map.first;
			int target = gotoTable[i][map_idx];
			// 将目标状态转换为字符串（0或空表示为"-"）
			goto_entries[nt_name] = (target == -1) ? "-" : to_string(target);
			map_idx++;
		}

		row["entries"] = goto_entries;
		goto_table_json.push_back(row);
	}
	return goto_table_json;
}
json ReduceProductionsJson(const Parser& parser)
{
	const auto& reduceProductions = parser.getReduceProductionLists();
	json reduce_prods_json = json::array();
	for (const auto& prod : reduceProductions) {
		json prod_json = json::object();
		prod_json["left"] = prod.left.name;

		json right = json::array();
		for (const auto& sym : prod.right) {
			right.push_back(sym.name);
		}
		prod_json["right"] = right;

		// 构造产生式的字符串表示
		string prod_str = prod.left.name + " -> ";
		for (const auto& sym : prod.right) {
			prod_str += sym.name + " ";
		}
		prod_json["display"] = prod_str;

		reduce_prods_json.push_back(prod_json);
	}
	return reduce_prods_json;
}
json ParseErrorsJson(const Parser& parser)
{
	json errors = json::array();
	for (const auto& err : parser.GetParseErrors()) {
		json obj;
		obj["line"] = err.line;
		obj["column"] = err.column;
		obj["message"] = err.message;
		errors.push_back(obj);
	}
	return errors;
}
json QuadruplesJson(const Parser& parser)
{
	json quadruples = json::array();
	int address = 100;
	for (const auto& q : parser.GetqList()) {
		json obj = json::object();
		obj["address"] = address;
		obj["op"] = q.op;
		obj["arg1"] = q.arg1;
		obj["arg2"] = q.arg2;
		obj["result"] = q.result;
		quadruples.push_back(obj);
		++address; // 递增
	}
	return quadruples;
}
json SemanticErrorsJson(const Parser& parser)
{
	json errors = json::array();
	for (const auto& error : parser.GetSemanticErrors()) {
		json obj;
		obj["line"] = error.line;
		obj["column"] = error.column;
		obj["message"] = error.message;
		errors.push_back(obj);
	}
	return errors;
}

int main(int argc, char** argv)
{
	system("chcp 65001");
	filesystem::path path = "rust/test.rs";//Rust源程序路径
	string grammar = "rust/grammar.txt";//语法分析语法文件路径

	if (argc >= 2 && string(argv[1]) == "lexical") {
		//词法分析
		InputBuffer* inputb;
		inputb = new(nothrow)InputBuffer(path);
		inputb->filter_comments();
		Scanner scanner(*inputb);
		scanner.LexicalAnalysis();
		//输出词法分析结果
		vector<Token> tokens = scanner.GetTokens();
		delete inputb;
		ofstream of1("rust/result/LexicalAnalyzer.json");
		of1 << TokensJson(tokens).dump(2) << '\n';
		of1.close();
	}
	else {
		//语法分析
		InputBuffer syntaxInput(path);
		syntaxInput.filter_comments();
		Scanner parserScanner(syntaxInput);
		//Parser parser(parserScanner, grammar);
		Parser parser(parserScanner, "rust/parser.galp", true);
		parser.SyntaxAnalysis();
		////输出FIRST集合
		//result["firsts"] = FirstsJson(parser);
		////LR1项目集
		//result["LR1items"] = LR1ItemsJson(parser);
		// 获取Action表并转换为JSON
		ofstream of2("rust/result/action.json");
		of2 << ActionTableJson(parser).dump(2) << '\n';
		of2.close();
		// 获取Goto表
		ofstream of3("rust/result/goto.json");
		of3 << GotoTableJson(parser).dump(2) << '\n';
		of3.close();

		if (argc >= 2 && string(argv[1]) == "grammar") {
			return 0;
		}

		// 获取规约产生式序列
		ofstream of4("rust/result/process.json");
		of4 << ReduceProductionsJson(parser).dump(2) << '\n';
		of4.close();
		// 输出语法分析错误
		ofstream of5("rust/result/perror.json");
		of5 << ParseErrorsJson(parser).dump(2) << '\n';
		of5.close();
		// 输出四元式
		ofstream of6("rust/result/quads.json");
		of6 << QuadruplesJson(parser).dump(2) << '\n';
		of6.close();
		// 输出语义错误
		ofstream of7("rust/result/serror.json");
		of7 << SemanticErrorsJson(parser).dump(2) << '\n';
		of7.close();

		if (argc >= 2 && string(argv[1]) == "parse") {
			return 0;
		}
		else if (argc < 2 || argc >= 2 && string(argv[1]) == "target") {
			//语义分析
			parser.resultToFile();
			if (parser.GetSemanticErrors().empty()) {//当无语法及语义错误时，执行目标代码生成
				ObjectCodeGenerator generator("rust/intermediateCodes.txt", "rust/symTable.txt");
				generator.ConvertObjectCode("rust/result/objectCode.txt");
			}
			else {
				ofstream of8("rust/result/objectCode.txt", ofstream::out | ofstream::trunc);
				of8.close();
			}
		}

		return 0;
	}
}
#else
int main(int argc, char** argv)
{
	system("chcp 65001");
	filesystem::path path = "rust/test.rs";//Rust源程序路径
	string grammar = "rust/grammar.txt";//语法分析语法文件路径
	////词法分析
	//InputBuffer* inputb;
	//inputb = new(nothrow)InputBuffer(path);
	//inputb->filter_comments();
	//Scanner newscanner(*inputb);
	//newscanner.LexicalAnalysis();
	////词法分析部分结果输出
	//vector<Token> tokens = newscanner.GetTokens();
	//// 在打印tokens信息时添加位置信息
	//for (unsigned int i = 0; i < tokens.size(); ++i) {
	//	cout << " [行:" << tokens[i].line << " 列:" << tokens[i].column << " 长度:" << tokens[i].length << "]    ";
	//	if (tokens[i].type == Identifier)
	//		cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<string>(tokens[i].value) << ")："/* << SymbolTable[get<unsigned int>(tokens[i].value)].ID*/;
	//	else if (tokens[i].type == string_)
	//		cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<string>(tokens[i].value) << ')';
	//	else if (tokens[i].type == char_)
	//		cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<char>(tokens[i].value) << ')';
	//	else
	//		cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<int>(tokens[i].value) << ')';
	//	cout << endl;
	//}
	//delete inputb;

	//语法分析
	InputBuffer* inputp = new(nothrow)InputBuffer(path);
	inputp->filter_comments();
	Scanner pscanner(*inputp);
	//Parser newparser(pscanner, grammar);
	//newparser.saveToFile("rust/parser.galp");
	Parser newparser(pscanner, "rust/parser.galp", true);
	newparser.SyntaxAnalysis();
	//语法分析部分结果输出
	//newparser.printSyntaxTree();
	//int address = 100;
	//for (const auto& q : newparser.GetqList()) {
	//	cout << address << ":";
	//	cout << "(" << q.op << ", " << q.arg1 << ", " << q.arg2 << ", " << q.result << ")" << endl;
	//	address++;
	//}
	//for (const auto& err : newparser.GetSemanticErrors())
	//	std::cout << "Error at line " << err.line << ", column " << err.column << ", length " << err.length << ": " << err.message << std::endl;

	//目标代码生成
	newparser.resultToFile();
	delete inputp;
	if (newparser.GetSemanticErrors().empty()) {//当无语法及语义错误时，执行目标代码生成
		ObjectCodeGenerator q("rust/intermediateCodes.txt", "rust/symTable.txt");
		q.ConvertObjectCode();
	}

	return 0;
}
#endif // BACKEND