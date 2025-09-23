#include "ObjectCodeGenerator.h"
#include <queue>
#include <functional>
using namespace std;

UseLiveInfo::UseLiveInfo() :use(-1), live(false)
{
}

UseLiveInfo::UseLiveInfo(int use, bool live) :use(use), live(live)
{
}

FlowGraph::FlowGraph() :startAddr(0)
{
}

FlowGraph::FlowGraph(size_t startAddr, vector<Quadruple> qList) :startAddr(startAddr)
{
	if (qList.empty())
		return;
	priority_queue<int, vector<int>, greater<int>> entrance;//入口语句，按照语句地址排序（可能重复）
	priority_queue<int, vector<int>, greater<int>> exit;//除入口语句外的出口语句，按照语句地址排序（不会重复）
	entrance.push(0);//入口语句：第一个语句
	for (int i = 0; i < int(qList.size()); ++i) {
		Quadruple quad = qList[i];
		if (qList[i].op[0] == 'j') {//转移语句
			if (i < int(qList.size()) - 1 && qList[i].op != "j")//不是最后一条语句&&为条件转移
				entrance.push(i + 1);//入口语句：紧跟在条件转移语句后面的语句
			entrance.push(stoi(qList[i].result) - startAddr);//入口语句：能由转移语句转移到的语句
			exit.push(i);//出口语句：转移语句
		}
		else if (qList[i].op == "hlt")
			exit.push(i);//出口语句：停语句
		else if (qList[i].op == "ret")
			exit.push(i);//出口语句：*将返回语句视为无目标（无条件）跳转语句，因其后语句不会被执行
	}
	//划分基本块
	int start = -1, end = -1;//基本块的入口语句与出口语句，此处取值：即本函数的第几条语句（而非整个中间代码四元式序号）
	while (!entrance.empty()) {
		start = entrance.top();
		if (start == qList.size())
			break;//如果跳转至函数以后，认为其无效
		entrance.pop();
		while (!entrance.empty() && start == entrance.top()) {
			entrance.pop();//entrance加入语句时未去重
			continue;
		}//此时entrance队首为真正（不同）的下一条入口语句
		while (!exit.empty() && exit.top() < start)
			exit.pop();//此时exit队首出口语句为入口语句或入口语句之后的语句，消除了之前的语句
		//判断出口语句位置
		if (entrance.empty() && exit.empty())//基本块直接到最后一句终止
			end = qList.size() - 1;
		else if (entrance.empty())
			end = exit.top();
		//exit的出队留至下一次循环，直接出队至入口语句之后的第一个出口语句
		else if (exit.empty())
			end = entrance.top() - 1;
		else {
			int entrancetop = entrance.top();
			int exittop = exit.top();
			if (entrancetop - 1 < exittop)
				end = entrancetop - 1;//出口语句：下一入口语句(不包括该入口语句)
			else if (exittop < entrancetop - 1)
				end = exittop;//出口语句：一转移语句(包括该转移语句)或一停语句(包括该停语句)
			//exit的出队留至下一次循环，直接出队至入口语句之后的第一个出口语句
			else
				end = entrancetop - 1;
			//exit的出队留至下一次循环，直接出队至入口语句之后的第一个出口语句
		}
		//根据起止位置构造基本块
		blocks.push_back({ size_t(start), size_t(end), vector<Quadruple>(qList.begin() + start,qList.begin() + end + 1), unordered_set<int>(), vector<array<UseLiveInfo, 3>>(), unordered_set<string>(), unordered_set<string>() });
	}
	//构造基本块之间转移关系
	for (int i = 0; i < int(blocks.size()); ++i) {//对于每个基本块
		BasicBlock& b = blocks[i];
		//for (int j = 0; j < int(b.quads.size()); ++j) {//查找基本块中的每个四元式
			//Quadruple& q = b.quads[j];
		//无需循环，转移语句一定是基本块内最后一句
		Quadruple& q = b.quads[b.quads.size() - 1];
		if (q.op[0] == 'j') {//找到其中的转移语句
			size_t target = stoi(q.result) - startAddr;//转移目标为函数第几句
			//找到目标基本块
			int k;
			for (k = 0; k < int(blocks.size()); ++k)
				if (/*target >= blocks[k].startAddr && */target <= blocks[k].endAddr) {//第一个基本块最后一句地址大于目标语句的基本块，为目标语句所在基本块（前提：目标语句在基本块内）
					q.result = to_string(k);//将跳转地址修改为基本块在函数内的编号
					b.successors.insert(k);
					break;
				}
			if (k == blocks.size())
				cerr << "目标代码生成错误：" << startAddr + b.quads.size() - 1 << "：(" << q.op << ", " << q.arg1 << ", " << q.arg2 << ", " << q.result << ")" << endl << endl;
		}
		//}
		if (q.op == "ret")//基本块最后一条语句不是返回语句，则无后继
			b.successors.clear();
		else if (q.op != "j" && i != blocks.size() - 1)//基本块最后一条语句不是无条件跳转，则将下一个基本块列为其后继
			b.successors.insert(i + 1);
		//successors最多只有2个出口：最后一句ret，无；最后一句条件跳转，2；普通语句或无条件跳转，1
	}
	/***************************************可继续优化***************************************
	* 每一个基本块（除第一个基本块）：如果没有进入的箭头或者仅存在自己直接指向自己的箭头，可删除该基本块并删除其为起点的弧
	* 一条弧链接的两个基本块，且作为弧起点的基本块出度为1，作为弧终点的基本块入度为1。合并（即在上一步被优化的基本块中，若存在转移语句，删除其对于生成入口语句的贡献。注意有可能其它语句也将其标记为入口语句，不应直接标记为非入口语句）
	*****************************************************************************************/
	return;
}

bool FlowGraph::calBlockUseLive(BasicBlock& block)
{
	/*****************************************************************************************
	* 计算待用信息和活跃信息：
	* 1. 开始时，把基本块中各变量的符号表登记项中的待用信息栏填为“非待用”，并根据该变量在基本块出口之后是不是活跃的，把其中的活跃信息栏填为“活跃”或“非活跃”；
	* 2. 从基本块出口到基本块入口由后向前依次处理各个四元式。对每一个四元式i: A:=B op C，依次执行下面的步骤：
	* 1)   把符号表中变量A的待用信息和活跃信息附加到四元式i上；
	* 2)   把符号表中A的待用信息和活跃信息分别置为“非待用”和“非活跃”；
	* 3)   把符号表中变量B和C的待用信息和活跃信息附加到四元式i上；
	* 4)   把符号表中B和C的待用信息均置为i，活跃信息均置为“活跃”。
	*****************************************************************************************/
	unordered_map<string, UseLiveInfo> variaUseLives;//每一个变量的待用/活跃信息
	//1.初始化变量的初始状态→信息链（待用/活跃信息栏）
	unordered_set<string> newliveOut = getBlockLiveOut(block);//其余使用默认构造函数自动赋值为(^,^)
	bool changed = block.liveOut != newliveOut;//中间算法始终不变，当liveOut相同时，所有求解结果均相同
	block.liveOut = newliveOut;
	for (const string& s : block.liveOut)
		variaUseLives[s] = { -1, true };//(^,y)，其余不是出口处的活跃变量的变量均为(^,^)
	//2.从基本块出口到基本块入口由后向前依次处理各个四元式
	block.quadsUseLives.resize(block.quads.size());
	for (int i = block.quads.size() - 1; i >= 0; --i) {
		Quadruple& q = block.quads[i];
		string arg1, arg2, rst;
		//找到本式中引用点与定值点//仅两个操作数可能为引用点；仅结果可能为定值点//空表示本处不涉及引用/定制
		if (q.op == "=") {
			if (q.arg1[0] == '-' || isdigit(q.arg1[0]))
				arg1.clear();
			else
				arg1 = q.arg1;
			arg2.clear();
			rst = q.result;
		}
		else if (q.op == "j") {
			arg1.clear();
			arg2.clear();
			rst.clear();
		}
		else if (q.op[0] == 'j') {//条件转移
			if (q.arg1[0] == '-' || isdigit(q.arg1[0]))
				arg1.clear();
			else
				arg1 = q.arg1;
			if (q.arg2[0] == '-' || isdigit(q.arg2[0]))
				arg2.clear();
			else
				arg2 = q.arg2;
			rst.clear();
		}
		else if (q.op == "call") {
			arg1.clear();
			arg2.clear();
			if (q.result == "-")
				rst.clear();
			else
				rst = q.result;
		}
		else if (q.op == "para") {
			if (q.arg1[0] == '-' || isdigit(q.arg1[0]))
				arg1.clear();
			else
				arg1 = q.arg1;
			arg2.clear();
			rst.clear();
		}
		else if (q.op == "ret") {
			if (arg1 == "-" || q.arg1[0] == '-' || isdigit(q.arg1[0]))
				arg1.clear();
			else
				arg1 = q.arg1;
			arg2.clear();
			rst.clear();
		}
		else {//目前其余op均为算术，根据后续扩展扩展
			if (q.arg1[0] == '-' || isdigit(q.arg1[0]))
				arg1.clear();
			else
				arg1 = q.arg1;
			if (q.arg2[0] == '-' || isdigit(q.arg2[0]))
				arg2.clear();
			else
				arg2 = q.arg2;
			if (q.result[0] == '-' || isdigit(q.result[0]))
				rst.clear();
			else
				rst = q.result;
		}
		if (!rst.empty()) {//result处有定值变量
			block.quadsUseLives[i][0] = variaUseLives[rst];//1)   把符号表中变量A的待用信息和活跃信息附加到四元式i上；
			variaUseLives[rst] = { -1,false };//2)   把符号表中A的待用信息和活跃信息分别置为“非待用”和“非活跃”；
		}
		//3)   把符号表中变量B和C的待用信息和活跃信息附加到四元式i上；
		if (!arg1.empty())
			block.quadsUseLives[i][1] = variaUseLives[arg1];
		if (!arg2.empty())
			block.quadsUseLives[i][2] = variaUseLives[arg2];
		//4)   把符号表中B和C的待用信息均置为i，活跃信息均置为“活跃”。
		if (!arg1.empty())
			variaUseLives[arg1] = { i, true };
		if (!arg2.empty())
			variaUseLives[arg2] = { i, true };
	}
	//总结liveIn
	block.liveIn.clear();
	for (const pair<string, UseLiveInfo>& p : variaUseLives)
		if (p.second.live)
			block.liveIn.insert(p.first);
	return changed;
}

unordered_set<string> FlowGraph::getBlockLiveOut(BasicBlock& block)
{
	unordered_set<string> variables;
	for (const int& succ : block.successors)
		variables.insert(blocks[succ].liveIn.begin(), blocks[succ].liveIn.end());
	return variables;
}

void FlowGraph::calFunctionUseLive()
{
	/*****************************************************************************************
	* 从后向前遍历每个基本块，对于每个基本块倒序求解待用/活跃信息
	* ①初始化“变量的初始状态→信息链（待用/活跃信息栏）”：该基本块所有后继基本块的入口处活跃变量为该基本块出口处的活跃变量
	* ②倒序遍历基本块内每条语句，更新“附加在四元式上的待用/活跃信息表”“变量的初始状态→信息链（待用/活跃信息栏）”
	* ③保存遍历结束后的“变量的初始状态→信息链（待用/活跃信息栏）”（该基本块的入口处活跃变量），供后续①使用
	* 由于流图中可能存在环，①求解时后继基本块可能尚未求解。因此采用反复迭代直至无变化的方式
	*****************************************************************************************/
	bool changed = true;
	while (changed) {
		changed = false;
		for (int i = blocks.size() - 1; i >= 0; --i)
			if (calBlockUseLive(blocks[i]))
				changed = true;
	}
	return;
}

SymProTable::SymProTable(string ID, size_t addr, SymProTable* prev, vector<symbolInfo> symbols, vector<SymProTable*> procedures) :ID(ID), addr(addr), prev(prev), symbols(symbols), procedures(procedures)
{
}

void ObjectCodeGenerator::divideBasicBlocks()
{
	//DFS依次遍历所有函数，保证按照语句顺序遍历每个函数。对于遍历到的函数，将函数所有语句划分基本块
	stack<const SymProTable*> sptables;//存储每个函数表项
	//最外层函数（即直接定义的函数，无嵌套）的具体内容（函数表项）入栈
	sptables.push(&sympro);
	//开始DFS
	while (!sptables.empty()) {
		const SymProTable* t = sptables.top();
		sptables.pop();
		size_t startq = t->addr;//本函数起始四元式地址
		for (int i = t->procedures.size() - 1; i >= 0; --i)//逆序入栈
			sptables.push(t->procedures[i]);//递归加入全部子函数
		size_t endq = sptables.empty() ? qList.size() + START_STMT_ADDR : sptables.top()->addr;//下一函数地址（本函数结束地址）
		if (startq < endq)//仅当本函数有语句时
			flowgraphs[t->ID] = FlowGraph(startq, vector<Quadruple>(qList.begin() + (startq - START_STMT_ADDR), qList.begin() + (endq - START_STMT_ADDR)));//构造本函数的流图
	}
}

void ObjectCodeGenerator::calFunctionsUseLive()
{
	//DFS依次遍历所有函数，保证按照语句顺序遍历每个函数。对于遍历到的函数，为其中基本块创建待用/活跃信息
	stack<const SymProTable*> sptables;//存储每个函数表项
	//最外层函数（即直接定义的函数，无嵌套）的具体内容（函数表项）入栈
	for (int i = sympro.procedures.size() - 1; i >= 0; --i)//逆序入栈
		sptables.push(sympro.procedures[i]);
	//开始DFS
	while (!sptables.empty()) {
		const SymProTable* t = sptables.top();
		sptables.pop();
		for (int i = t->procedures.size() - 1; i >= 0; --i)//逆序入栈
			sptables.push(t->procedures[i]);//递归加入全部子函数
		flowgraphs[t->ID].calFunctionUseLive();//构造本函数的待用/活跃信息
	}
}

inline void ObjectCodeGenerator::WriteObjectCode(const string& op, const string& data1)
{
	outfile << '\t' << op << "\t" /*(op.size() < 4 ? "\t\t" : "\t")*/ << data1 << endl;
}

inline void ObjectCodeGenerator::WriteObjectCode(const string& op, const string& data1, const string& data2)
{
	outfile << '\t' << op << "\t" << data1 << ",\t" << data2 << endl;
}

inline void ObjectCodeGenerator::WriteObjectCode(const string& op, const string& data1, const string& data2, const string& data3)
{
	outfile << '\t' << op << "\t" << data1 << ",\t" << data2 << ",\t" << data3 << endl;
}

inline void ObjectCodeGenerator::WriteObjectCode(const string& op, const string& data1, const string& data2, const string& data3, const string& comment)
{
	outfile << '\t' << op << "\t" << data1 << ",\t" << data2 << ",\t" << data3 << "\t#" << comment << endl;
}

/*****************************************************************************************
* 返回值：
* INT_MIN：错误，操作结果为数字，为数字分配寄存器
* -1：分配寄存器失败
* ≥0：AVAILABLE_REG_NAMES中下标
*
* 说明：
* 四元式无论arg1，arg2为数字还是变量名均可分配。数字与变量均按算法规则同一处理，算法中如“B与A是同一个标识符”等判定也不会满足，不会发生问题。
*****************************************************************************************/
int ObjectCodeGenerator::getReg(const Quadruple& I, const array<UseLiveInfo, 3>& UseLives)
{
	//无操作数直接返回
	if (I.result == "-" || I.result[0] == '-' || isdigit(I.result[0]))
		return INT_MIN;
	//I: op result, arg1, arg2//必须为A:=B op C形式
	/*****************************************************************************************
	* GETREG(i: A:=B op C) 返回一个用来存放A的值的寄存器
	* 1 如果B的现行值在某个寄存器Ri中，RVALUE[Ri]中只包含B，此外，或者B与A是同一个标识符，或者B的现行值在执行四元式A:=B op C之后不会再引用，则选取Ri为所需要的寄存器R，并转4；
	* 2 如果有尚未分配的寄存器，则从中选取一个Ri为所需要的寄存器R，并转4；
	* 3 从已分配的寄存器中选取一个Ri为所需要的寄存器R。最好使得Ri满足以下条件：
	*		占用Ri的变量的值也同时存放在该变量的贮存单元中，
	* 		或者在基本块中要在最远的将来才会引用到或不会引用到。
	*	要不要为Ri中的变量生成存数指令？
	*		对RVALUE[Ri]中每一变量M，如果M不是A，或者如果M是A又是C，但不是B并且B也不在RVALUE[Ri]中，则
	*		(1) 如果AVALUE[M]不包含M，则生成目标代码 ST Ri，M
	*		(2) 如果M是B，或者M是C但同时B也在RVALUE[Ri]中，则令AVALUE[M]={M, Ri} ，否则令AVALUE[M]={M}
	*		(3) 删除RVALUE[Ri]中的M
	* 4 给出R，返回。
	*****************************************************************************************/
	int reg = -1;//结果
	//尽可能用B独占的寄存器
	if (I.arg1[0] != '-' && !isdigit(I.arg1[0]))//仅当B是变量而不是立即数时
		for (int i = 0; i < int(Rvalue.size()); ++i) {
			unordered_set<string>rval = Rvalue[i];
			if (rval.size() == 1 && rval.count(I.arg1) == 1) //RVALUE[Ri]中只包含B
				reg = i;
			else
				continue;
			//if (reg != -1)//仅当前执行reg = i;后会执行以下语句，因此无需判定reg != -1
			if (I.result == I.arg1)//B与A是同一个标识符
				return reg;//选取Ri为所需要的寄存器R
			else if (UseLives[1].use == -1) {//B的现行值在执行四元式A:=B op C之后不会再引用
				if (UseLives[1].live == true && Avalue[I.arg1].find(-1) == Avalue[I.arg1].end())//如果B活跃，存储//如果AVALUE[B]不包含B，则生成目标代码ST Ri, B
					storeVariable(I.arg1, AVAILABLE_REG_NAMES[i]);
				return reg;//选取Ri为所需要的寄存器R
			}
			else
				reg = -1;
		}
	//尽可能用空闲寄存器
	for (int i = 0; i < int(Rvalue.size()); ++i)
		if (Rvalue[i].empty())
			return i;
	//抢占用非空闲寄存器//求解RVALUE[Ri]中每个变量，是否全部在贮存单元中；最远在何处会引用到
	bool previousAllInMemory = false;//选中的Ri的RVALUE[Ri]中每个变量是否全部在贮存单元中
	int maxUse = -1;//选中的Ri的RVALUE[Ri]中每个变量的最近使用的四元式编号（基本块内0开始）
	for (int i = 0; i < int(Rvalue.size()); ++i) {
		bool allInMemory = true;
		int minUseTime = INT_MAX;
		for (const string& var : Rvalue[i]) {//遍历RVALUE[Ri]中每个变量
			if (Avalue[var].find(-1) == Avalue[var].end())
				allInMemory = false;//检查变量是否在内存中有备份
			int nextUse;//查找该变量在当前基本块中的下一次引用
			for (nextUse = currentquad; nextUse < int(currentblock->quads.size()); ++nextUse)//遍历当前基本块剩余的四元式（含本语句使用）
				if (currentblock->quads[nextUse].arg1 == var || currentblock->quads[nextUse].arg2 == var)//遇到第一条使用的即找到
					break;
			if (nextUse == currentblock->quads.size())
				if (currentblock->liveOut.find(var) != currentblock->liveOut.end())// 如果在当前基本块中没有找到使用，检查是否为出口活跃变量
					nextUse = currentblock->quads.size(); // 设为基本块结束位置
				else
					nextUse = INT_MAX;//不会引用到
			minUseTime = min(minUseTime, nextUse);
		}
		//// 优先选择条件：1. 所有变量都在内存中有备份的寄存器；2. 下次使用时间最远的寄存器
		//if (bestReg == -1 || (allInMemory && !previousAllInMemory) || (allInMemory == previousAllInMemory && minUseTime > maxUse)) {
		//	bestReg = i;
		//	maxUse = minUseTime;
		//	previousAllInMemory = allInMemory;
		//}
		if (maxUse < minUseTime) {//此处仅选择在基本块中要在最远的将来才会引用到或不会引用到
			reg = i;
			maxUse = minUseTime;
		}
	}
	//抢占用非空闲寄存器的情况下，为Ri中的变量生成存数指令
	if (reg == -1)
		cerr << "无法为变量" << I.result << "选择寄存器" << endl;
	for (auto it = Rvalue[reg].begin(); it != Rvalue[reg].end(); ) {//对RVALUE[Ri]中每一变量M
		const string& var = *it;
		if (var != I.result || var == I.result && var == I.arg2 && var != I.arg1 && Rvalue[reg].find(I.arg1) == Rvalue[reg].end()) {//如果M不是A，或者如果M是A又是C，但不是B并且B也不在RVALUE[Ri]中
			//如果AVALUE[M]不包含M，生成目标代码ST Ri，M
			if (Avalue[var].find(-1) == Avalue[var].end())
				storeVariable(var, AVAILABLE_REG_NAMES[reg]);
			//如果M是B，或者M是C但同时B也在RVALUE[Ri]中，则令AVALUE[M]={M, Ri} ，否则令AVALUE[M]={M}
			if (var == I.arg1 || var == I.arg2 && Rvalue[reg].find(I.arg1) != Rvalue[reg].end())//如果M是B，或者M是C但同时B也在RVALUE[Ri]中
				Avalue[var] = { -1, reg };//AVALUE[M] = { M, Ri }
			else
				Avalue[var] = { -1 };//AVALUE[M] = { M }
			//删除RVALUE[Ri]中的M
			//Rvalue[reg].erase(var);
			it = Rvalue[reg].erase(it);
		}
		else
			++it;
	}
	return reg;
}

/*****************************************************************************************
* 返回值：
* INT_MIN：错误，无操作数，为空分配寄存器
* -1：分配寄存器失败
* -2：不分配寄存器
* ≥0：AVAILABLE_REG_NAMES中下标
*
* 说明：
* 在getReg的基础上进行修改，为满足B与C作为源操作数，在MIPS中可能必须在寄存器中的需求，分配只读寄存器（只读寄存器在此处的含义为：只需有值而无需考虑在本指令执行过程中被写，与getReg区别）
* 因此，在MIPS中某些源操作数需在寄存器中这种思想下，为实现简便，本类中生成的代码如A:=B op C即使其中一个源操作数可以为立即数，也将其先存至寄存器。统一操作，牺牲目标代码性能，使生成过程中未讨论B与C谁是立即数谁不是（对应四种不同的目标代码生成方式）
* 进一步，对于B操作数，对于ConvertProcedureCode中的微调后的目标代码生成算法，其中第3步“如果B'≠R，则生成目标代码LD R, B'”，其意义在于，如果B不在任一寄存器中，需要将B随意load入一个寄存器中以继续运算，那么直接将其load入A分配的寄存器就好了（因为对于MIPS，目标寄存器不一定要与源寄存器相同，因此只要B在任一寄存器中，即使B'≠R也无需转移）
*
* 考虑到这些特殊需求，定义参数pos含义：
* 1：为B分配只读寄存器
* 2：为C分配只读寄存器
* -1：找到一个Rvalue中有B的寄存器作为B的只读寄存器返回，如果没有也不用再分配并load入新寄存器，直接在未来生成目标代码时，load入A的寄存器就好了
* （1和2的区别仅在于，当B或C为数字时，分配至为立即数保留的寄存器（"$t8", "$t9"），规定B分配至$t8，C分配至$t9。无其余区别）
*****************************************************************************************/
int ObjectCodeGenerator::getReadonlyReg(string var, int pos)
{
	//无操作数直接返回
	if (var == "-")
		return INT_MIN;
	//为立即数固定分配$t8、$t9寄存器
	if (var[0] == '-' || isdigit(var[0]))
		if (pos == 1 || pos == -1) {
			WriteObjectCode("li", "$t8", var);
			return AVAILABLE_REG_NUM;
		}
		else if (pos == 2) {
			WriteObjectCode("li", "$t9", var);
			return AVAILABLE_REG_NUM + 1;
		}
		else {
			cerr << "getReadonlyReg错误的指定位置：" << currtable->ID << "："
				<< currentblock->quads[currentquad].op << currentblock->quads[currentquad].arg1 << currentblock->quads[currentquad].arg2 << currentblock->quads[currentquad].result << endl;
		}
	//目标为返回一个存储var值的寄存器；若没有，则选择一个寄存器存储，并将寄存器中原值写入内存。无后续其它运算/写等操作，因此无需getReg的众多复杂判断
	/*****************************************************************************************
	* GETREG(i: A:=B op C) 返回一个用来存放A的值的寄存器
	* 1 如果B的现行值在某个寄存器Ri中（RVALUE[Ri]中包含B），则选取任一Ri为所需要的寄存器R，并转4；
	* 2 如果有尚未分配的寄存器，则从中选取一个Ri为所需要的寄存器R，并转4；
	* 3 从已分配的寄存器中选取一个Ri为所需要的寄存器R。最好使得Ri满足以下条件：
	*		占用Ri的变量的值也同时存放在该变量的贮存单元中，
	* 		或者在基本块中要在最远的将来才会引用到或不会引用到。
	*	要不要为Ri中的变量生成存数指令？都要
	*		对RVALUE[Ri]中每一变量M，生成目标代码 ST Ri，M
	* 4 给出R，返回。
	*****************************************************************************************/
	//尽可能用有该变量的寄存器
	for (const int& reg : Avalue[var])//遍历变量var的AVALUE（unordered_set<int>）
		if (reg != -1)
			return reg;
	if (pos == -1)//如果没有含B的寄存器，不分配，直接将B通过load入A分配的寄存器中（为B分配A未来分配到的寄存器），直接运算
		return -2;
	int reg = -1;
	//尽可能用空闲寄存器
	for (int i = 0; i < int(Rvalue.size()); ++i)
		if (Rvalue[i].empty()) {
			reg = i;
			break;
		}
	//抢占用非空闲寄存器//求解RVALUE[Ri]中每个变量，是否全部在贮存单元中；最远在何处会引用到
	if (reg == -1) {
		bool previousAllInMemory = false;//选中的Ri的RVALUE[Ri]中每个变量是否全部在贮存单元中
		int maxUse = -1;//选中的Ri的RVALUE[Ri]中每个变量的最近使用的四元式编号（基本块内0开始）
		for (int i = 0; i < int(Rvalue.size()); ++i) {
			bool allInMemory = true;
			int minUseTime = INT_MAX;
			for (const string& var : Rvalue[i]) {//遍历RVALUE[Ri]中每个变量
				if (Avalue[var].find(-1) == Avalue[var].end())
					allInMemory = false;//检查变量是否在内存中有备份
				int nextUse;//查找该变量在当前基本块中的下一次引用
				for (nextUse = currentquad; nextUse < int(currentblock->quads.size()); ++nextUse)//遍历当前基本块剩余的四元式（含本语句使用）
					if (currentblock->quads[nextUse].arg1 == var || currentblock->quads[nextUse].arg2 == var)//遇到第一条使用的即找到
						break;
				if (nextUse == currentblock->quads.size())
					if (currentblock->liveOut.find(var) != currentblock->liveOut.end())// 如果在当前基本块中没有找到使用，检查是否为出口活跃变量
						nextUse = currentblock->quads.size(); // 设为基本块结束位置
					else
						nextUse = INT_MAX;//不会引用到
				minUseTime = min(minUseTime, nextUse);
			}
			//// 优先选择条件：1. 所有变量都在内存中有备份的寄存器；2. 下次使用时间最远的寄存器
			//if (bestReg == -1 || (allInMemory && !previousAllInMemory) || (allInMemory == previousAllInMemory && minUseTime > maxUse)) {
			//	bestReg = i;
			//	maxUse = minUseTime;
			//	previousAllInMemory = allInMemory;
			//}
			if (maxUse < minUseTime) {//此处仅选择在基本块中要在最远的将来才会引用到或不会引用到
				reg = i;
				maxUse = minUseTime;
			}
		}
		//抢占用非空闲寄存器的情况下，为Ri中的变量生成存数指令
		if (reg == -1)
			cerr << "无法为变量" << var << "选择寄存器" << endl;
		for (const string& v : Rvalue[reg]) {//对RVALUE[Ri]中每一变量M
			//始终生成目标代码ST Ri，M
			if (Avalue[v].find(-1) == Avalue[v].end())
				storeVariable(v, AVAILABLE_REG_NAMES[reg]);
			Avalue[v] = { -1 };//AVALUE[M] = { M }
			//删除RVALUE[Ri]中的M（直接移植函数最后赋值）
			//Rvalue[reg].erase(var);
		}
	}
	loadVariable(var, AVAILABLE_REG_NAMES[reg]);
	Rvalue[reg] = { var };
	Avalue[var].insert(reg);
	return reg;
}

int ObjectCodeGenerator::findSymbolIndex(const std::string& name, const vector<symbolInfo>& currsymtable)
{
	for (int i = 0; i < int(currsymtable.size()); ++i)
		if (currsymtable[i].ID == name)
			return i;
	return -1;
}

void ObjectCodeGenerator::storeLiveOuts()
{
	//存储基本块出口的活跃变量
	for (const string& livesym : currentblock->liveOut) {
		int j = findSymbolIndex(livesym, currtable->symbols);
		if (j == -1)
			cerr << "未找到标识符" << livesym << endl;
		if (Avalue[livesym].find(-1) == Avalue[livesym].end())//需要存储
			for (int regn : Avalue[livesym])
				if (regn != -1) {
					WriteObjectCode("sw", AVAILABLE_REG_NAMES[regn], to_string(/*(j + 1)*/int(currtable->symbols.size() - j) * 4) + "($sp)");
					break; //找到第一个不为-1的值后退出循环
				}
	}
}

void ObjectCodeGenerator::loadVariable(const std::string& var, const std::string& regn)
{
	int j = findSymbolIndex(var, currtable->symbols);
	if (j == -1)
		cerr << "未找到标识符" << var << endl;
	WriteObjectCode("lw", regn, to_string(int(currtable->symbols.size() - j) * 4) + "($sp)");
}

void ObjectCodeGenerator::storeVariable(const std::string& var, const std::string& regn)
{
	int j = findSymbolIndex(var, currtable->symbols);
	if (j == -1)
		cerr << "未找到标识符" << var << endl;
	WriteObjectCode("sw", regn, to_string(int(currtable->symbols.size() - j) * 4) + "($sp)");
}

void ObjectCodeGenerator::ConvertStartupCode()
{
	//outfile << ".data" << endl;
	//int i = 0;
	//int firstFunction = sympro.procedures[0]->addr - START_STMT_ADDR;//第一条函数语句的位置//认为所有内层函数在本层函数全部语句之后定义
	////MIPS汇编均为4字节数据
	//for (const symbolInfo& sym : sympro.symbols) {
	//	outfile << sym.ID << ":\t.word\t" 
	//}
	outfile << ".text" << endl
		<< ".globl _start" << endl << endl
		<< "_start:" << endl
		<< "\tlui\t$sp\t0x7fff\t\t\t\t\t# 设置栈的高16位" << endl
		<< "\tori\t$sp,\t$sp,\t0xeffc\t\t# 栈从0x10000000开始" << endl
		//<< "\tli\t$sp,\t0x10000000\t\t# 栈指针初始化到0x10000000" << endl
		<< "\taddi\t$sp,\t$sp,\t-4" << endl//预留一个返回地址的空间
		<< "\tjal\tmain\t\t\t\t\t\t# 跳转到main函数" << endl
		<< "\tli\t$v0,\t10\t\t\t\t\t# 系统调用号（退出）" << endl
		<< "\tsyscall" << endl << endl;
}

void ObjectCodeGenerator::ConvertProcedureCode(/*const string& procedure, const vector<symbolInfo>& symbols*/)
{
	string& procedure = currtable->ID;
	vector<symbolInfo>& symbols = currtable->symbols;
	outfile << procedure << ":" << endl;//函数名：
	FlowGraph& fgraph = flowgraphs[procedure];
	vector<BasicBlock>& bblocks = fgraph.blocks;
	//函数入口固定语句（完成栈帧）
	int paranum;
	for (paranum = 0; paranum < int(symbols.size()) && symbols[paranum].isNormal; ++paranum)//统计形参个数
		;
	//存储返回地址
	WriteObjectCode("sw", "$ra", to_string(4 * (paranum + 1)) + "($sp)");
	//为局部变量预留空间
	if ((paranum = symbols.size() - paranum) > 0)//有局部变量
		WriteObjectCode("addi", "$sp", "$sp", to_string(-4 * paranum), "为局部变量预留空间");
	// 如果为变量设置未初始化值
	//for (const symbolInfo& s : symbols) {
	//	WriteObjectCode(outfile, "li", "$a0", "0xcc");
	//	WriteObjectCode(outfile, "sw", "$a0", "($sp)");
	//	WriteObjectCode(outfile, "addi", "$sp", "$sp", "-4");
	//}
	//开始翻译具体每一句语句
	for (int bindex = 0; bindex < int(bblocks.size()); ++bindex) {
		//清除RVALUE, AVALUE
		Avalue.clear();
		for_each(Rvalue.begin(), Rvalue.end(), [](auto& regSet) {
			regSet.clear();
			});
		//处理基本块
		currentblock = &bblocks[bindex];
		outfile << procedure << "_block" << bindex << ":" << endl;//块名：
		/*****************************************************************************************
		* 对每个四元式: i: A:=B op C，依次执行：
		* +		为C获取只读寄存器：MIPS与原算法的差别在于三个操作数必须均在寄存器中，原算法保证了A和B在运算时发生在寄存器中。在语句执行前添加将C值存储在寄存器中，（无需避免后续选中该寄存器，源寄存器和目标寄存器可以相同，这点与原算法相同）。因此在后续算法执行中，与原算法无区别
		* 1.	以四元式: i: A:=B op C 为参数，调用函数过程GETREG(i: A:=B op C)，返回一个寄存器R，用作存放A的寄存器。
		* 2.	利用AVALUE[B]和AVALUE[C]，确定B和C现行值的存放位置B’和C’。如果其现行值在寄存器中，则把寄存器取作B’和C’
		* 3.	如果B’≠R，则生成目标代码：
		*			LD  R,  B’
		*			op  R,  C’
		*		否则生成目标代码 op R, C’
		*		如果B’或C’为R，则删除AVALUE[B]或AVALUE[C]中的R。
		* 4.	令AVALUE[A]={R}, RVALUE[R]={A}。
		* 5.	若B或C的现行值在基本块中不再被引用，也不是基本块出口之后的活跃变量，且其现行值在某寄存器Rk中，则删除RVALUE[Rk]中的B或C以及AVALUE[B]或AVALUE[C] 中的Rk ，使得该寄存器不再为B或C占用。
		*****************************************************************************************/
		vector<string> parameters;//除存储连续的几句para所有参数名，以便按照符号表顺序压栈以外；使用其中是否存储变量名这一信息，在call阶段判断是否已经通过para保存现场（若没有para需要在call保存现场）。使用前提：认为para语句与call语句连续，本call调用函数的参数一定由其前距离最近的一组para完成。因此在call时清空parameters，para时加入参数名即可
		for (currentquad = 0; currentquad < int(currentblock->quads.size()); ++currentquad) {//对于每一条语句
			const Quadruple& q = currentblock->quads[currentquad];
			const unordered_map<string, string> MIPS_OP = {
				{"+", "add"}, {"-", "sub"}, {"*", "mul"}, {"/", "div"}, {"=", "move"}, {"j<", "blt"}, {"j<=", "ble"}, {"j>", "bgt"}, {"j>=", "bge"}, {"j==", "beq"}, {"j=", "beq"}, {"j!=", "bne"},
			};
			if (q.op == "=") {
				int regA;
				if (q.arg1[0] == '-' || isdigit(q.arg1[0])) {//数字B赋值给变量A，分配寄存器并直接li赋值
					regA = getReg(q, currentblock->quadsUseLives[currentquad]);//（R）
					WriteObjectCode("li", AVAILABLE_REG_NAMES[regA], q.arg1);
				}
				else {//变量B赋值给变量A
					int regB = getReadonlyReg(q.arg1, -1);//（B'）
					regA = getReg(q, currentblock->quadsUseLives[currentquad]);//（R）
					if (regB == -2)//B没存储在任一寄存器中
						loadVariable(q.arg1, AVAILABLE_REG_NAMES[regA]);//regA = B;
					else if (regA != regB)//B存储在某一寄存器中
						WriteObjectCode(MIPS_OP.at(q.op), AVAILABLE_REG_NAMES[regA], AVAILABLE_REG_NAMES[regB]);//op  R,  B'
				}
				//if (regA == regB || regA == regC)
				if (q.arg1 != "-" && q.arg1[0] != '-' && !isdigit(q.arg1[0]))
					Avalue[q.arg1].erase(regA);//如果B’或C’为R，删除AVALUE[B]或AVALUE[C]中的R。
				Avalue[q.result] = { regA };//AVALUE[A] = { R }
				Rvalue[regA] = { q.result };//RVALUE[R] = { A }
				if (q.arg1 != "-" && q.arg1[0] != '-' && !isdigit(q.arg1[0]))
					if (currentblock->quadsUseLives[currentquad][1].use == -1 && currentblock->quadsUseLives[currentquad][1].live == false) {//若B或C的现行值在基本块中不再被引用，也不是基本块出口之后的活跃变量
						for (const int& rk : Avalue[q.arg1])//且其现行值在某寄存器Rk中
							Rvalue[rk].erase(q.arg1);
						Avalue[q.arg1].clear();//则删除RVALUE[Rk]中的B或C以及AVALUE[B]或AVALUE[C]中的Rk ，使得该寄存器不再为B或C占用
					}
			}
			else if (q.op == "j") {
				storeLiveOuts();
				WriteObjectCode("j", procedure + "_block" + q.result);
			}
			else if (q.op[0] == 'j') {
				storeLiveOuts();
				int regC = getReadonlyReg(q.arg2, 2);//（C'）
				int regB = getReadonlyReg(q.arg1, 1);//（B'）
				WriteObjectCode(MIPS_OP.at(q.op), AVAILABLE_REG_NAMES[regB], AVAILABLE_REG_NAMES[regC], procedure + "_block" + q.result);
			}
			/*****************************************************************************************
			* 栈帧：（$sp）指向下一使用单元（未使用）
			* ***************************
			* 局部变量（m）								被调用函数预留空间（被调用函数初始）											被调用函数销毁（ret）
			* ***************************
			* 形式单元（n）								调用函数保存（para开辟空间并保存）												被调用函数销毁（ret）
			* ***************************
			* 返回地址（1）								被调用函数保存（被调用函数初始），调用函数的para（有参函数）或call预留空间		被调用函数销毁（ret）
			* ***************************
			* 寄存器保护区（AVAILABLE_REG_NAMES长度）	调用函数保存（call保存，para（有参函数）或call预留空间）						调用函数销毁（call）
			* ***************************
			* *********调用函数栈帧……
			* ***************************
			*****************************************************************************************/
			else if (q.op == "call") {
				if (parameters.empty())
					WriteObjectCode("addi", "$sp", "$sp", to_string(-4 * int(sizeof(AVAILABLE_REG_NAMES) / sizeof(string) + 1)), "预留无参数函数返回地址及现场的空间");//预留无参数函数返回地址及现场的空间
				//保护寄存器现场
				for (int j = 0; j < sizeof(AVAILABLE_REG_NAMES) / sizeof(string); ++j)
					WriteObjectCode("sw", AVAILABLE_REG_NAMES[j], to_string((int(parameters.size()) + j + 2) * 4) + "($sp)");
				//找到所要调用函数名
				int pindex;
				for (pindex = 0; pindex < int(currtable->prev->procedures.size()); ++pindex)//调用同级函数
					if (currtable->prev->procedures[pindex]->addr == stoi(q.arg1)) {
						WriteObjectCode("jal", currtable->prev->procedures[pindex]->ID);//生成调用函数语句
						break;
					}
				if (pindex == currtable->prev->procedures.size())
					cerr << procedure << "函数：" << currentblock->quads[currentquad].op << currentblock->quads[currentquad].arg1 << currentblock->quads[currentquad].arg2 << currentblock->quads[currentquad].result << "未找到地址" << q.arg1 << "对应处的函数" << endl;
				parameters.clear();//清空函数调用参数列表，为下一函数调用准备
				//恢复寄存器现场
				for (int j = 0; j < sizeof(AVAILABLE_REG_NAMES) / sizeof(string); ++j)
					WriteObjectCode("lw", AVAILABLE_REG_NAMES[j], to_string((j + 1) * 4) + "($sp)");
				//销毁栈帧（寄存器保护区）
				WriteObjectCode("addi", "$sp", "$sp", to_string(4 * (sizeof(AVAILABLE_REG_NAMES) / sizeof(string))), "销毁栈帧（寄存器保护区）");
				//获取返回值（销毁栈帧后，保证sp满足storeVariable默认的位置）
				if (q.result != "-") {//有返回值，执行q.resut=$v0赋值
					//storeVariable(q.result, "$v0");
					int regA = getReg({ "=", "-", "-", q.result }, currentblock->quadsUseLives[currentquad]);//（R）
					WriteObjectCode(MIPS_OP.at("="), AVAILABLE_REG_NAMES[regA], "$v0");//op  R,  B'
					Avalue[q.result] = { regA };//AVALUE[A] = { R }
					Rvalue[regA] = { q.result };//RVALUE[R] = { A }
				}
			}
			else if (q.op == "para") {
				//保护现场
				WriteObjectCode("addi", "$sp", "$sp", to_string(-4 * int(sizeof(AVAILABLE_REG_NAMES) / sizeof(string) + 1)), "预留返回地址及现场的空间");//预留一个返回地址的空间
				//形式单元
				parameters = { q.arg1 };//在中间代码中，参数按声明顺序逆序生成（从后向前），因此中间代码中在后的参数距离栈顶较远（最后声明参数紧贴栈帧中返回地址处），因此需要知道参数总个数后，才能确定所有参数位置
				while (currentquad + 1 < int(currentblock->quads.size()) && currentblock->quads[currentquad + 1].op == "para")//获取所有参数
					parameters.push_back(currentblock->quads[++currentquad].arg1);
				WriteObjectCode("addi", "$sp", "$sp", to_string(-4 * int(parameters.size())), "预留参数空间，↓存储参数");
				for (int j = 0; j < int(parameters.size()); ++j) {//对于每一个形式单元
					int regB = getReadonlyReg(parameters[j], 1);
					WriteObjectCode("sw", AVAILABLE_REG_NAMES[regB], to_string((j + 1) * 4) + "($sp)");
				}
			}
			else if (q.op == "ret") {
				//存储返回值至$v0
				if (q.arg1 != "-")//有返回值
					if (q.arg1[0] == '-' || isdigit(q.arg1[0]))//返回数字
						WriteObjectCode("li", "$v0", q.arg1);
					else {//返回变量
						int regB = getReadonlyReg(q.arg1, -1);
						if (regB == -2)
							loadVariable(q.arg1, "$v0");
						else
							WriteObjectCode("move", "$v0", AVAILABLE_REG_NAMES[regB]);
					}
				//获取返回地址
				WriteObjectCode("lw", "$ra", to_string(4 * (symbols.size() + 1)) + "($sp)");
				//销毁栈帧（形式单元、局部变量、返回地址）
				WriteObjectCode("addi", "$sp", "$sp", to_string(4 * (symbols.size() + 1)), "销毁栈帧（形式单元、局部变量、返回地址）");
				//返回
				WriteObjectCode("jr", "$ra");
			}
			else {//目前其余op均为算术，根据后续扩展扩展//加减乘除
				int regC = getReadonlyReg(q.arg2, 2);//（C'）
				int regB = getReadonlyReg(q.arg1, -1);//（B'）
				int regA = getReg(q, currentblock->quadsUseLives[currentquad]);//（R）
				if (regB == -2) {//B不为立即数，且在寄存器中均无值
					loadVariable(q.arg1, AVAILABLE_REG_NAMES[regA]);//regB = regA;//LD  R,  B’
					WriteObjectCode(MIPS_OP.at(q.op), AVAILABLE_REG_NAMES[regA], AVAILABLE_REG_NAMES[regA], AVAILABLE_REG_NAMES[regC]);//op  R,  C’
				}
				else
					WriteObjectCode(MIPS_OP.at(q.op), AVAILABLE_REG_NAMES[regA], AVAILABLE_REG_NAMES[regB], AVAILABLE_REG_NAMES[regC]);//op  R,  C’
				//if (regA == regB || regA == regC)
				if (q.arg1 != "-" && q.arg1[0] != '-' && !isdigit(q.arg1[0]))
					Avalue[q.arg1].erase(regA);//如果B’或C’为R，删除AVALUE[B]或AVALUE[C]中的R。
				if (q.arg2 != "-" && q.arg2[0] != '-' && !isdigit(q.arg2[0]))
					Avalue[q.arg2].erase(regA);//如果B’或C’为R，删除AVALUE[B]或AVALUE[C]中的R。
				Avalue[q.result] = { regA };//AVALUE[A] = { R }
				Rvalue[regA] = { q.result };//RVALUE[R] = { A }
				if (q.arg1 != "-" && q.arg1[0] != '-' && !isdigit(q.arg1[0]))
					if (currentblock->quadsUseLives[currentquad][1].use == -1 && /*currentblock->quadsUseLives[currentquad][1].live == false（i=i+1错误）*/currentblock->liveOut.find(q.arg1) == currentblock->liveOut.end()) {//若B或C的现行值在基本块中不再被引用，也不是基本块出口之后的活跃变量
						for (const int& rk : Avalue[q.arg1])//且其现行值在某寄存器Rk中
							Rvalue[rk].erase(q.arg1);
						Avalue[q.arg1].clear();//则删除RVALUE[Rk]中的B或C以及AVALUE[B]或AVALUE[C]中的Rk ，使得该寄存器不再为B或C占用
					}
				if (q.arg2 != "-" && q.arg2[0] != '-' && !isdigit(q.arg2[0]))
					if (currentblock->quadsUseLives[currentquad][2].use == -1 && /*currentblock->quadsUseLives[currentquad][2].live == false*/currentblock->liveOut.find(q.arg2) == currentblock->liveOut.end()) {//若B或C的现行值在基本块中不再被引用，也不是基本块出口之后的活跃变量
						for (const int& rk : Avalue[q.arg2])//且其现行值在某寄存器Rk中
							Rvalue[rk].erase(q.arg2);
						Avalue[q.arg2].clear();//则删除RVALUE[Rk]中的B或C以及AVALUE[B]或AVALUE[C]中的Rk ，使得该寄存器不再为B或C占用
					}
			}
		}
		//存储基本块出口的活跃变量
		if ((currentblock->quads.end() - 1)->op[0] != 'j')//跳转语句结束的基本块要在跳转之前存储
			storeLiveOuts();
	}
}

ObjectCodeGenerator::ObjectCodeGenerator(string intermediateCodes, string symproTable) :sympro("", 0, NULL, vector<symbolInfo>(), vector<SymProTable*>()), currtable(NULL), currentblock(NULL), currentquad(0)
{
	//输入四元式
	ifstream infile(intermediateCodes);
	infile >> START_STMT_ADDR;
	string op, arg1, arg2, result;
	while (infile >> op >> arg1 >> arg2 >> result)
		qList.push_back({ op, arg1, arg2, result });
	infile.close();
	//构造符号表
	infile.open(symproTable);
	stack<SymProTable*> tblstk;//存储每一个待处理函数的外层函数
	tblstk.push(NULL);
	//根据DFS结果构造
	while (!tblstk.empty()) {
		SymProTable* parent = tblstk.top();
		tblstk.pop();
		string pID, vID;
		size_t paddr, vaddr;
		int num, vtype, vnormal;
		//输入本函数的名字与起始四元式地址
		infile >> pID >> paddr;
		//输入本函数自己定义的变量信息
		infile >> num;
		vector<symbolInfo> symbols;
		while (num-- > 0) {
			infile >> vID >> vaddr >> vtype >> vnormal;
			symbols.push_back({ vID, vaddr, ValueType(vtype), bool(vnormal) });
		}
		//输入子函数信息（首先输入子函数个数）
		infile >> num;
		//构造当前函数函数表
		SymProTable* proc = new SymProTable(pID, paddr, parent, symbols, vector<SymProTable*>());
		//关联本函数与外层函数
		if (parent)//不是最外层函数
			parent->procedures.insert(parent->procedures.begin(), proc);//头插法，语义分析时倒序输出至文件
		else {
			sympro = *proc;
			delete proc;
			proc = &sympro;
		}
		//递归输入全部子函数信息
		while (num-- > 0)
			tblstk.push(proc);
	}
	infile.close();
}

void ObjectCodeGenerator::ConvertObjectCode(string objectFile)
{
	divideBasicBlocks();
	calFunctionsUseLive();

	outfile.open(objectFile);
	ConvertStartupCode();

	//DFS依次遍历所有函数，保证按照语句顺序遍历每个函数。对于遍历到的函数，为其中基本块创建待用/活跃信息
	stack<SymProTable*> sptables;//存储每个函数表项
	//最外层函数（即直接定义的函数，无嵌套）的具体内容（函数表项）入栈
	for (int i = sympro.procedures.size() - 1; i >= 0; --i)//逆序入栈
		sptables.push(sympro.procedures[i]);
	//开始DFS
	while (!sptables.empty()) {
		currtable = sptables.top();
		sptables.pop();
		for (int i = currtable->procedures.size() - 1; i >= 0; --i)//逆序入栈
			sptables.push(currtable->procedures[i]);//递归加入全部子函数
		//翻译函数
		ConvertProcedureCode();
	}
}

ObjectCodeGenerator::~ObjectCodeGenerator()
{
	outfile.close();
	//使用递归lambda表达式释放SymProTable内存
	function<void(SymProTable*)> deleteSymProTable = [&](SymProTable* table) {
		if (table == nullptr)
			return;
		//递归释放所有子函数
		for (SymProTable* proc : table->procedures)
			deleteSymProTable(proc);
		//释放当前表
		delete table;
		};
	//递归释放 sympro 中的所有 procedures
	for (SymProTable* proc : sympro.procedures)
		deleteSymProTable(proc);
}
