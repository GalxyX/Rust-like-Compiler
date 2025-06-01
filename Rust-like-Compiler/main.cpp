#include "LexicalAnalyzer.h"
#include "Parser.h"
#include <string>
using namespace std;

//#define BACKEND
#ifdef BACKEND
#include <nlohmann/json.hpp>
using json = nlohmann::json;
json TokensJson(const vector<Token>& tokens)
{
	json tokens_array = json::array();
	for (const auto& token : tokens) {
		json token_obj;
		token_obj["type"] = TokenTypeToString(token.type);
		// ����token����ѡ����ȷ��ֵ����
		if (token.type == Identifier)
			token_obj["value"] = get<unsigned int>(token.value);
		else if (token.type == i32_)
			token_obj["value"] = get<int>(token.value);
		else if (token.type == char_)
			token_obj["value"] = string(1, get<char>(token.value)); // ת��charΪstring
		else if (token.type == string_)
			token_obj["value"] = get<string>(token.value);
		else
			token_obj["value"] = get<int>(token.value);
		// ���λ����Ϣ
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
	json firsts_json = json::array();//[{�ַ� : FIRST()}, {�ַ� : FIRST()}, ...]
	for (const auto& map : nonTerminals) {
		json first_set = json::array();//���ַ�������firstԪ�ص�����
		for (const auto& terminal : firsts[map.second]) {
			first_set.push_back(TokenTypeToString(terminal));
		}
		firsts_json[map.first] = first_set;//{�ַ� : FIRST()}
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

		json items = json::array();//[����LR1��Ŀ��ÿ�����ʽ]
		for (const auto& item : itemsets[i].items) {
			json item_json = json::object();//{ "left": , "right": , ...}
			const Production& prod = productions[item.productionIndex];//��ȡ����ʽ
			item_json["left"] = prod.left.name;//����ʽ�� left : T
			json right_symbols = json::array();//����ʽ�Ҳ�[t1, t2, ...]
			for (size_t j = 0; j < prod.right.size(); j++) {
				right_symbols.push_back(prod.right[j].name);
			}
			item_json["right"] = right_symbols;//����ʽ�Ҳ� right : [t1, t2, ...]
			item_json["dotPosition"] = item.dotPosition;//��λ�� dotPosition : x
			item_json["lookahead"] = TokenTypeToString(item.lookahead);//ǰհ���� lookahead, T
			// �������ʽ�ַ�����ʾ
			string production_str = prod.left.name + " -> ";
			for (size_t j = 0; j < prod.right.size(); j++) {
				if (j == item.dotPosition)
					production_str += "�� ";
				production_str += prod.right[j].name + " ";
			}
			if (item.dotPosition == prod.right.size())
				production_str += "��";
			production_str += ", " + string(TokenTypeToString(item.lookahead));
			item_json["display"] = production_str;//display : T->T��T, T
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
	for (size_t i = 0; i < actionTable.size(); i++) {// ��������״̬
		json row = json::object();
		row["state"] = "I" + to_string(i);

		json actions = json::object();
		// ���������ս�����������ս����
		for (int j = 1; j <= int(TokenType::End); j++) {
			string token_name = TokenTypeToString(TokenType(j));
			const auto& entry = actionTable[i][j];

			// ���ݶ������������ַ�����ʾ
			string action_str = "-"; // Ĭ��Ϊ��
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
				// ����Ĭ��ֵ"-"
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
	for (size_t i = 0; i < gotoTable.size(); i++) {//��������״̬
		json row = json::object();
		row["state"] = "I" + to_string(i);

		json goto_entries = json::object();
		// �������з��ս��
		size_t map_idx = 0;
		for (const auto& map : nonTerminals) {
			// ���ս�����ƺͶ�Ӧ��Ŀ��״̬
			string nt_name = map.first;
			int target = gotoTable[i][map_idx];
			// ��Ŀ��״̬ת��Ϊ�ַ�����0��ձ�ʾΪ"-"��
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

		// �������ʽ���ַ�����ʾ
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

int main() {
	//�ӱ�׼�����ȡ����������EOF��β
	string program, line;
	while (getline(cin, line))
		program += line + "\n";
	//����InputBuffer����
	InputBuffer input(program);
	input.filter_comments();
	//�ʷ�����
	Scanner scanner(input);
	scanner.LexicalAnalysis();
	vector<SymbolTableEntry> symbolTable = scanner.GetSymbolTable();
	vector<Token> tokens = scanner.GetTokens();
	//����ʷ��������
	json result;
	result["tokens"] = TokensJson(tokens);
	//�﷨����
	InputBuffer syntaxInput(program);
	syntaxInput.filter_comments();
	Scanner parserScanner(syntaxInput);
	//string grammar = "rust/grammar.txt"; // �����﷨�ļ�·��
	//Parser parser(parserScanner, grammar);
	Parser parser(parserScanner, "rust/parser.galp", true);
	parser.SyntaxAnalysis();
	//���FIRST����
	result["firsts"] = FirstsJson(parser);
	//LR1��Ŀ��
	result["LR1items"] = LR1ItemsJson(parser);
	// ��ȡAction��ת��ΪJSON
	result["actiontable"] = ActionTableJson(parser);
	// ��ȡGoto��
	result["gototable"] = GotoTableJson(parser);
	// ��ȡ��Լ����ʽ����
	result["reduceProductions"] = ReduceProductionsJson(parser);
	// ����﷨��������
	result["parseErrors"] = ParseErrorsJson(parser);

	cout << result.dump(2) << endl;
	return 0;
}
#else
int main(int argc, char** argv)
{
	InputBuffer* inputb;
	filesystem::path path = "rust/test.rs";
	string grammar = "rust/grammar.txt";

	if (argc == 3 && !strcmp(argv[1], "-f"))
		path = argv[2];
	else if (argc == 2 && !strcmp(argv[1], "-s"))
		while (true) {
			string s;
			getline(cin, s);
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
	//�ʷ�����
	inputb = new(nothrow)InputBuffer(path);
	inputb->filter_comments();

	Scanner newscanner(*inputb);
	newscanner.LexicalAnalysis();
	vector<SymbolTableEntry> SymbolTable = newscanner.GetSymbolTable();
	vector<Token> tokens = newscanner.GetTokens();
	// �ڴ�ӡtokens��Ϣʱ���λ����Ϣ
	for (int i = 0; i < tokens.size(); ++i) {
		cout << " [��:" << tokens[i].line << " ��:" << tokens[i].column << " ����:" << tokens[i].length << "]    ";
		if (tokens[i].type == Identifier)
			cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<unsigned int>(tokens[i].value) << ")��" << SymbolTable[get<unsigned int>(tokens[i].value)].ID;
		else if (tokens[i].type == string_)
			cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<string>(tokens[i].value) << ')';
		else if (tokens[i].type == char_)
			cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<char>(tokens[i].value) << ')';
		else
			cout << '(' << TokenTypeToString(tokens[i].type) << ' ' << get<int>(tokens[i].value) << ')';
		cout << endl;
	}
	delete inputb;
	//�﷨����
	InputBuffer* inputp = new(nothrow)InputBuffer(path);
	inputp->filter_comments();
	Scanner pscanner(*inputp);
	//Parser newparser(pscanner, grammar);
	Parser newparser(pscanner, "rust/parser.galp", true);
	//newparser.saveToFile("rust/parser.galp");
	newparser.SyntaxAnalysis();
	newparser.printSyntaxTree();
	delete inputp;
	return 0;
}
#endif // BACKEND