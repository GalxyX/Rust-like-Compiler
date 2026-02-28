/* 页面数据 */
let reduceProductions = null; // 维护 reduceProductions 数据(tab3、tab4共用)
let gotoTableData = null; // 维护 goto 表数据
let actionTableData = null; // 维护 action 表数据
let syntaxErrors = null; // 维护语法错误数据
let quadRuples = null; // 维护四元式数据
let semanticErrors = null; // 维护语义错误数据
let targetCodeData = null; // 维护目标代码数据

/* 缓存数据和源代码状态 */
let lastSourceCode = ""; // 上次分析的源代码
let cachedLexicalData = null; // 缓存的词法分析结果
let cachedParseData = null; // 缓存的语法分析结果
let cachedTargetCodeData = null; // 缓存的目标代码结果
let cachedGrammarData = null; // 缓存的语法规则数据（grammar调用结果）
let sourceCodeChanged = true; // 源代码是否发生变化

/* 缓存管理：根据Rust-like-Compiler.exe的输出文件判断缓存层级 */
let cacheLevel = {
  grammar: false, // grammar调用：生成action.json、goto.json
  lexical: false, // lexical调用：生成LexicalAnalyzer.json
  parse: false, // parse调用：生成action.json、goto.json、perror.json、process.json、quads.json、serror.json
  target: false, // target调用：生成objectCode.txt + 更新所有parse级别的文件
};
/* 防抖 */
let timeout; // 词法分析防抖计时器
let activeTab = "tab1"; // 当前激活的标签页ID

// 获取 IPC 通道（仅 Electron 环境有效）
const { ipcRenderer } = require("electron");

// ─── 通用状态视图辅助函数 ────────────────────────────────────
/** 转义 HTML 特殊字符，防止拼接字符串时产生 XSS */
function escapeHtml(str) {
  return String(str).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}

/**
 * 生成空状态/错误状态的 HTML 字符串，统一使用 CSS 类，不含 inline style
 * @param {'empty'|'error'} type  - 状态类型
 * @param {string} message        - 主要提示文字
 * @param {string} [hint]         - 次要提示（可选，仅 error 类型）
 */
function stateHtml(type, message, hint = "") {
  const iconClass = type === "error" ? "warning-icon" : "nodata-icon";
  const textClass = type === "error" ? "state-text-error" : "state-text";
  const hintHtml = hint ? `<p class="state-text-hint">${escapeHtml(hint)}</p>` : "";
  return `
        <div class="state-view">
            <div class="${iconClass}"></div>
            <p class="${textClass}">${escapeHtml(message)}</p>
            ${hintHtml}
        </div>
    `;
}

// 缓存状态管理函数
function resetCache() {
  cachedLexicalData = null;
  cachedParseData = null;
  cachedTargetCodeData = null;
  // 注意：不重置cachedGrammarData和cacheLevel.grammar，因为语法规则不依赖源代码
  cacheLevel.lexical = false;
  cacheLevel.parse = false;
  cacheLevel.target = false;
  sourceCodeChanged = true;
  console.log("缓存已重置（保留语法规则缓存）");
}

// 初始化编辑器
const editor = CodeMirror(document.getElementById("editor"), {
  lineNumbers: true,
  indentUnit: 4,
  smartIndent: true,
  matchBrackets: true,
  autoCloseBrackets: true,
  value: "",
  placeholder: "在此输入 Rust-like 源代码...",
  extraKeys: {
    "Ctrl-Space": "autocomplete",
  },
});

// 监听光标移动，实时更新状态栏中的行列显示
editor.on("cursorActivity", () => {
  const cursor = editor.getCursor();
  const el = document.getElementById("cursor-pos");
  if (el) el.textContent = `行 ${cursor.line + 1}\u00a0列 ${cursor.ch + 1}`;
});

function updateEditorHighlight(tokens) {
  const doc = editor.getDoc();

  doc.getAllMarks().forEach((mark) => mark.clear());

  const allContent = editor.getValue();

  // 移除多行注释
  const withoutMultiComments = allContent.replace(/\/\*[\s\S]*?\*\//g, function (match) {
    return match
      .split("\n")
      .map(() => "")
      .join("\n");
  });

  // 移除单行注释 (保持换行符)
  const withoutComments = withoutMultiComments
    .split("\n")
    .map((line) => line.replace(/\/\/.*$/, ""))
    .join("\n");

  // 预计算行数组，避免在每个 token 循环内重复 split
  const commentFreeLines = withoutComments.split("\n");

  tokens.forEach((token) => {
    const { column, length, line, type } = token;

    // 忽略无效的 token 和结束标记
    if (line < 1 || column < 1 || length < 1 || type === "End") return;

    // 获取实际代码行内容 (注意: line从1开始,需要-1)
    const lineContent = commentFreeLines[line - 1] || "";

    // 计算当前行的实际缩进宽度
    let actualIndentWidth = 0;
    for (let i = 0; i < lineContent.length; i++) {
      if (lineContent[i] !== " ") break;
      actualIndentWidth++;
    }

    // 计算 token 的实际起始列数
    let adjustedColumn = 0;
    if (actualIndentWidth) {
      adjustedColumn = actualIndentWidth + (column - 2); // 后端的 column 从 1 开始
    } else {
      adjustedColumn = column - 1;
    }

    // 如果列超出行的长度，跳过该 token
    if (adjustedColumn >= lineContent.length) return;

    // 特殊处理 string_ 类型
    let adjustedLength = length;
    if (type === "string_") {
      adjustedLength += 2; // 加上引号的长度
    }

    try {
      // 获取当前行的注释前缀长度
      let commentPrefixLength = 0;
      const multilineCommentMatch = lineContent.match(/\/\*.*\*\//);
      if (multilineCommentMatch && multilineCommentMatch.index < adjustedColumn) {
        // 如果是单行的多行注释，计算其长度
        commentPrefixLength = multilineCommentMatch[0].length;
      }

      // 特殊处理多行字符串
      if (type === "string_" && token.value && token.value.includes("\n")) {
        const lines = token.value.split("\n");
        const startLine = line - 1; // CodeMirror的行从0开始

        // 处理首行（包含开始引号）
        editor.markText({ line: startLine, ch: adjustedColumn + commentPrefixLength }, { line: startLine, ch: lineContent.length }, { className: getClassNameForTokenType(type) });

        // 处理中间行（完整行高亮）
        for (let i = 1; i < lines.length - 1; i++) {
          const currentLine = startLine + i;
          const currentContent = commentFreeLines[currentLine] || "";
          if (currentContent) {
            editor.markText({ line: currentLine, ch: 0 }, { line: currentLine, ch: currentContent.length }, { className: getClassNameForTokenType(type) });
          }
        }

        // 处理最后一行（包含结束引号）
        const lastLine = startLine + lines.length - 1;
        const lastContent = commentFreeLines[lastLine] || "";
        const lastLineEndPos = lastContent.indexOf('"') + 1;
        if (lastContent) {
          editor.markText({ line: lastLine, ch: 0 }, { line: lastLine, ch: lastLineEndPos > 0 ? lastLineEndPos : lastContent.length }, { className: getClassNameForTokenType(type) });
        }
      } else {
        // 处理普通token，加上注释前缀的长度
        editor.markText({ line: line - 1, ch: adjustedColumn + commentPrefixLength }, { line: line - 1, ch: adjustedColumn + commentPrefixLength + adjustedLength }, { className: getClassNameForTokenType(type) });
      }
    } catch (e) {
      console.error(`Highlighting error for token:`, token, e);
    }
  });
}

// 代码编辑区高亮错误
function highlightErrors(errors) {
  errors.forEach((error) => {
    if (error.line === -1 && error.column === -1) return;

    // CodeMirror 的行从 0 开始
    const line = error.line - 1;

    const lineContent = editor.getLine(line);
    if (lineContent === null || lineContent === undefined) return;

    const startPos = { line, ch: 0 };
    const endPos = { line, ch: lineContent.length };
    editor.markText(startPos, endPos, { className: "error-marker" });
  });
}

function getClassNameForTokenType(type) {
  const typeClassMap = {
    // var-keyword
    I8: "cm-var-keyword",
    U8: "cm-var-keyword",
    I16: "cm-var-keyword",
    U16: "cm-var-keyword",
    I32: "cm-var-keyword",
    U32: "cm-var-keyword",
    I64: "cm-var-keyword",
    U64: "cm-var-keyword",
    I128: "cm-var-keyword",
    U128: "cm-var-keyword",
    F32: "cm-var-keyword",
    F64: "cm-var-keyword",
    ISIZE: "cm-var-keyword",
    USIZE: "cm-var-keyword",
    BOOL: "cm-var-keyword",
    CHAR: "cm-var-keyword",
    UNIT: "cm-var-keyword",
    ARRAY: "cm-var-keyword",
    TRUE: "cm-var-keyword",
    FALSE: "cm-var-keyword",

    // ctrl-keyword
    LET: "cm-ctrl-keyword",
    IF: "cm-ctrl-keyword",
    ELSE: "cm-ctrl-keyword",
    WHILE: "cm-ctrl-keyword",
    RETURN: "cm-ctrl-keyword",
    MUT: "cm-ctrl-keyword",
    FN: "cm-ctrl-keyword",
    FOR: "cm-ctrl-keyword",
    IN: "cm-ctrl-keyword",
    LOOP: "cm-ctrl-keyword",
    BREAK: "cm-ctrl-keyword",
    CONTINUE: "cm-ctrl-keyword",

    // Identifier
    Identifier: "cm-identifier",

    // Constant
    i32_: "cm-constant",
    string_: "cm-string",
    char_: "cm-string",

    // Operator
    Assignment: "cm-operator",
    Addition: "cm-operator",
    Subtraction: "cm-operator",
    Multiplication: "cm-operator",
    Division: "cm-operator",
    Equality: "cm-operator",
    GreaterThan: "cm-operator",
    GreaterOrEqual: "cm-operator",
    LessThan: "cm-operator",
    LessOrEqual: "cm-operator",
    Inequality: "cm-operator",

    // Delimiter
    ParenthesisL: "cm-delimiter",
    ParenthesisR: "cm-delimiter",
    CurlyBraceL: "cm-delimiter",
    CurlyBraceR: "cm-delimiter",
    SquareBracketL: "cm-delimiter",
    SquareBracketR: "cm-delimiter",
    Semicolon: "cm-delimiter",
    Comma: "cm-delimiter",
    Colon: "cm-delimiter",
    ArrowOperator: "cm-delimiter",
    DotOperator: "cm-delimiter",
    RangeOperator: "cm-delimiter",

    // Quote
    DoubleQuote: "cm-quote",
    SingleQuote: "cm-quote",

    // logical-operator: ! % %= & &= && | |= || ?
    Not: "cm-logical-operator",
    Modulo: "cm-logical-operator",
    ModuloAssign: "cm-logical-operator",
    BitAnd: "cm-logical-operator",
    BitAndAssign: "cm-logical-operator",
    LogicalAnd: "cm-logical-operator",
    BitOr: "cm-logical-operator",
    BitOrAssign: "cm-logical-operator",
    LogicalOr: "cm-logical-operator",
    ErrorPropagation: "cm-logical-operator",
    // assign: *= += -= /= << <<= => >> >>= @ ^ ^=
    MultiplicationAssign: "cm-assign",
    AdditionAssign: "cm-assign",
    SubtractionAssign: "cm-assign",
    DivisionAssign: "cm-assign",
    LeftShift: "cm-assign",
    LeftShiftAssign: "cm-assign",
    Arrowmatch: "cm-assign",
    RightShift: "cm-assign",
    RightShiftAssign: "cm-assign",
    PatternBinding: "cm-assign",
    BitXor: "cm-assign",
    BitXorAssign: "cm-assign",
  };

  return typeClassMap[type] || "cm-default";
}

// 检查源代码是否发生变化
function checkSourceCodeChanged() {
  const currentCode = editor.getValue();
  if (currentCode !== lastSourceCode) {
    lastSourceCode = currentCode;
    sourceCodeChanged = true;
    cachedLexicalData = null;
    cachedParseData = null;
    cachedTargetCodeData = null;
    // 重置缓存级别（但保留grammar缓存，因为它不依赖源代码）
    cacheLevel.lexical = false;
    cacheLevel.parse = false;
    cacheLevel.target = false;
    console.log("源代码已变化，清除源代码相关缓存");
    return true;
  }
  return false;
}

// 显示缓存状态信息
function showCacheStatus(api, fromCache = false) {
  const statusMessages = {
    analyze: fromCache ? "缓存命中 · 词法分析" : "调用编译器 · 词法分析中...",
    parse: fromCache ? "缓存命中 · 语法分析" : "调用编译器 · 语法分析中...",
    "target-code": fromCache ? "缓存命中 · 目标代码" : "调用编译器 · 目标代码生成中...",
    grammar: fromCache ? "缓存命中 · 语法规则" : "调用编译器 · 加载语法规则...",
  };

  const message = statusMessages[api] || "";
  if (message) {
    console.log(message);

    const envInfo = document.getElementById("environment-info");
    if (envInfo) {
      envInfo.textContent = message;
      envInfo.style.color = fromCache ? "#17a2b8" : "#f59e0b";

      // 2 秒后恢复就绪状态
      setTimeout(() => {
        envInfo.textContent = "编译器就绪";
        envInfo.style.color = "#22c55e";
      }, 2000);
    }
  }
}

// 调用exe文件的API函数
async function executeAPI(api) {
  const code = editor.getValue();

  const codeChanged = checkSourceCodeChanged();

  // 智能缓存检查和调用策略
  try {
    let result;

    switch (api) {
      case "grammar":
        // grammar调用不依赖源代码，可以持久缓存
        if (cachedGrammarData && cacheLevel.grammar) {
          console.log("缓存命中：语法规则数据已缓存，跳过exe调用");
          showCacheStatus(api, true);
          return cachedGrammarData;
        }

        console.log("⚙️ 缓存未命中：首次调用 Rust-like-Compiler.exe grammar");
        result = await ipcRenderer.invoke("execute-grammar");

        if (result.success) {
          cachedGrammarData = result.data;
          cacheLevel.grammar = true;
          console.log("✅ 语法规则数据已缓存，下次将跳过exe调用");
          return result.data;
        }
        break;

      case "analyze":
        // 检查是否可以使用缓存
        if (!codeChanged && cachedLexicalData && cacheLevel.lexical) {
          console.log("缓存命中：词法分析结果已缓存，跳过exe调用");
          showCacheStatus(api, true);
          return cachedLexicalData;
        }

        // 检查是否可以从更高级缓存中获取数据
        if (!codeChanged && cachedParseData && (cacheLevel.parse || cacheLevel.target)) {
          console.log("智能缓存：从语法分析缓存中提取词法分析结果");
          showCacheStatus(api, true);
          // 从parse结果中提取词法分析数据
          const lexicalFromParse = {
            tokens: cachedParseData.tokens || [],
          };
          cachedLexicalData = lexicalFromParse;
          cacheLevel.lexical = true;
          return lexicalFromParse;
        }

        console.log("⚙️ 缓存未命中：调用 Rust-like-Compiler.exe lexical");
        showCacheStatus(api, false);
        result = await ipcRenderer.invoke("execute-lexical", code);

        if (result.success) {
          cachedLexicalData = result.data;
          cacheLevel.lexical = true;
          sourceCodeChanged = false;
          console.log("✅ 词法分析结果已缓存");
          return result.data;
        }
        break;

      case "parse":
        // 检查是否可以使用缓存
        if (!codeChanged && cachedParseData && cacheLevel.parse) {
          console.log("缓存命中：语法分析结果已缓存，跳过exe调用");
          showCacheStatus(api, true);
          return cachedParseData;
        }

        // 检查是否可以从target缓存中获取数据
        if (!codeChanged && cachedTargetCodeData && cacheLevel.target) {
          console.log("智能缓存：从目标代码缓存中提取语法分析结果");
          showCacheStatus(api, true);
          // target调用会更新所有parse级别的文件，所以可以复用
          if (cachedParseData) {
            return cachedParseData;
          }
        }

        console.log("⚙️ 缓存未命中：调用 Rust-like-Compiler.exe parse");
        showCacheStatus(api, false);
        result = await ipcRenderer.invoke("execute-parser", code);

        if (result.success) {
          cachedParseData = result.data;
          cacheLevel.parse = true;
          // parse调用也会生成词法分析结果
          if (result.data.tokens) {
            cachedLexicalData = { tokens: result.data.tokens };
            cacheLevel.lexical = true;
            console.log("✅ 同时缓存了词法分析结果");
          }
          sourceCodeChanged = false;
          console.log("✅ 语法分析结果已缓存");
          return result.data;
        }
        break;

      case "target-code":
        // 检查是否可以使用缓存
        if (!codeChanged && cachedTargetCodeData && cacheLevel.target) {
          console.log("缓存命中：目标代码结果已缓存，跳过exe调用");
          showCacheStatus(api, true);
          return cachedTargetCodeData;
        }

        console.log("⚙️ 缓存未命中：调用 Rust-like-Compiler.exe target");
        showCacheStatus(api, false);
        result = await ipcRenderer.invoke("execute-target-code", code);

        if (result.success) {
          cachedTargetCodeData = result.data;
          cacheLevel.target = true;

          // target调用会更新所有parse级别的数据，所以可以更新相关缓存
          if (result.data.parseData) {
            cachedParseData = result.data.parseData;
            cacheLevel.parse = true;
            console.log("✅ 同时更新了语法分析缓存");

            // 同时更新词法分析缓存
            if (result.data.parseData.tokens) {
              cachedLexicalData = { tokens: result.data.parseData.tokens };
              cacheLevel.lexical = true;
              console.log("✅ 同时更新了词法分析缓存");
            }
          }

          sourceCodeChanged = false;
          console.log("✅ 目标代码结果已缓存，下次相同源代码将跳过所有exe调用");
          return result.data;
        }
        break;

      default:
        throw new Error(`未知的API: ${api}`);
    }

    if (!result.success) {
      throw new Error(result.error);
    }
  } catch (error) {
    console.error(`API调用失败 (${api}):`, error);

    // 显示错误对话框（直接展示 exe 返回的错误信息）
    await ipcRenderer.invoke("show-error", "执行失败", `${api} 执行失败:\n${error.message}`);

    throw error;
  }
}

// 渲染 goto 和 action 表
function renderTable(tableHeaderId, tableBodyId, data) {
  const tableHeaders = document.getElementById(tableHeaderId);
  const tableBody = document.getElementById(tableBodyId);

  tableHeaders.innerHTML = "";
  tableBody.innerHTML = "";

  // 安全保护：data 为空时直接返回，已清空的表格留空即可
  if (!data || data.length === 0) {
    return;
  }

  var columns = [];
  let dataKey = ""; // GOTO 表用 "entries"，ACTION 表用 "actions"

  if (tableHeaderId === "goto-table-headers" && data[0].entries) {
    columns = ["state", ...Object.keys(data[0].entries).filter((column) => column.trim() !== "")];
    dataKey = "entries";
  } else if (tableHeaderId === "action-table-headers" && data[0].actions) {
    columns = ["state", ...Object.keys(data[0].actions).filter((column) => column.trim() !== "")];
    dataKey = "actions";
  }

  if (columns.length === 0) return;

  columns.forEach((column) => {
    const th = document.createElement("th");
    th.textContent = column;
    tableHeaders.appendChild(th);
  });

  data.forEach((row) => {
    const tr = document.createElement("tr");

    const stateCell = document.createElement("td");
    stateCell.textContent = row.state;
    tr.appendChild(stateCell);

    columns.slice(1).forEach((column) => {
      const td = document.createElement("td");
      td.textContent = row[dataKey][column] || "-"; // 无数据时显示 '-'
      tr.appendChild(td);
    });

    tableBody.appendChild(tr);
  });
}

// 渲染"移进-规约"表
function renderReduceTable(tableHeaderId, tableBodyId, data) {
  const tab3Content = document.getElementById("tab3");
  if (data.length === 0) {
    tab3Content.innerHTML = stateHtml("empty", "数据暂无");
  } else {
    tab3Content.innerHTML = `
            <div class="data-table-container">
                <table class="data-table">
                    <thead>
                        <tr class="table-headers" id="reduce-table-headers"></tr>
                    </thead>
                    <tbody class="table-body" id="reduce-table-body"></tbody>
                </table>
            </div>
        `;

    const tableHeaders = document.getElementById(tableHeaderId);
    const tableBody = document.getElementById(tableBodyId);

    tableHeaders.innerHTML = "";
    tableBody.innerHTML = "";

    const columns = ["序号", "产生式左部", "产生式右部", "完整产生式"];
    columns.forEach((column) => {
      const th = document.createElement("th");
      th.textContent = column;
      tableHeaders.appendChild(th);
    });

    data.forEach((row, index) => {
      const tr = document.createElement("tr");

      const indexCell = document.createElement("td");
      indexCell.textContent = index + 1; // 序号从 1 开始
      tr.appendChild(indexCell);

      const leftCell = document.createElement("td");
      leftCell.textContent = row.left || "-";
      tr.appendChild(leftCell);

      const rightCell = document.createElement("td");
      rightCell.textContent = row.right.length > 0 ? row.right.join(" ") : "-";
      tr.appendChild(rightCell);

      const displayCell = document.createElement("td");
      displayCell.textContent = row.display || "-";
      tr.appendChild(displayCell);

      tableBody.appendChild(tr);
    });
  }
}

// 渲染语法分析树
function renderSyntaxTree(data) {
  const tab4Content = document.getElementById("tab4");
  tab4Content.innerHTML = `<div id="syntax-tree-container"></div>`;

  const reduceProductionList = encodeURIComponent(JSON.stringify(data));
  const errorList = encodeURIComponent(JSON.stringify(syntaxErrors));
  const iframe = document.createElement("iframe");
  iframe.src = `syntax-tree.html?reduceProduction=${reduceProductionList}&syntaxErrors=${errorList}`;
  iframe.width = "100%";
  iframe.height = "100%";
  iframe.style.border = "none";
  iframe.style.display = "block";

  const container = document.getElementById("syntax-tree-container");
  container.appendChild(iframe);
}

// 渲染中间代码生成的四元式
function renderQuadRuple() {
  const tab5Content = document.getElementById("tab5");

  if (!quadRuples || quadRuples.length === 0) {
    tab5Content.innerHTML = stateHtml("empty", "数据暂无");
    return;
  }

  tab5Content.innerHTML = `
        <div class="data-table-container">
            <table class="data-table">
                <thead>
                    <tr class="table-headers" id="quadruple-table-headers"></tr>
                </thead>
                <tbody class="table-body" id="quadruple-table-body"></tbody>
            </table>
        </div>
    `;
  const tableHeaders = document.getElementById("quadruple-table-headers");
  const tableBody = document.getElementById("quadruple-table-body");

  tableHeaders.innerHTML = "";
  tableBody.innerHTML = "";

  const columns = ["地址", "四元式"];
  columns.forEach((column) => {
    const th = document.createElement("th");
    th.textContent = column;
    tableHeaders.appendChild(th);
  });

  quadRuples.forEach((quadruple) => {
    const tr = document.createElement("tr");

    const addressTd = document.createElement("td");
    addressTd.textContent = quadruple.address || "-";
    tr.appendChild(addressTd);

    // 四元式格式：(op, arg1, arg2, result)，优先使用结构化字段
    const displayTd = document.createElement("td");
    if (quadruple.op && quadruple.result && quadruple.arg1) {
      const arg2 = quadruple.arg2 || "-";
      displayTd.textContent = `(${quadruple.op}, ${quadruple.arg1}, ${arg2}, ${quadruple.result})`;
    } else {
      displayTd.textContent = quadruple.display || "-";
    }
    tr.appendChild(displayTd);

    tableBody.appendChild(tr);
  });
}

// 渲染目标代码
function renderTargetCode() {
  const tab6Content = document.getElementById("tab6");

  if (!targetCodeData || targetCodeData.trim() === "") {
    tab6Content.innerHTML = stateHtml("empty", "数据暂无");
    return;
  }

  // 解析目标代码为指令行数组，保留制表符缩进
  const codeLines = targetCodeData
    .trim()
    .split("\n")
    .filter((line) => line !== "");

  // 构建代码查看器 HTML，全部使用 CSS 类
  const rowsHtml = codeLines.map((line, index) => `<tr class="code-line-row"><td class="line-num">${index + 1}</td><td class="line-code">${highlightMIPSInstruction(line)}</td></tr>`).join("");

  const svgCopyIcon = `<svg width="13" height="13" viewBox="0 0 24 24" fill="currentColor"><path d="M16 1H4c-1.1 0-2 .9-2 2v14h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z"/></svg>`;

  tab6Content.innerHTML = `
        <div class="code-viewer">
            <div class="code-viewer-body">
                <table class="code-line-table">
                    <colgroup><col class="line-num-col"><col></colgroup>
                    <thead><tr>
                        <th class="col-linenum">行号</th>
                        <th class="col-mips">
                            <div class="thead-mips-row">
                                <span class="thead-mips-left">
                                    MIPS 指令
                                    <span class="line-count-badge">${codeLines.length} 行</span>
                                </span>
                                <button class="copy-btn" id="copy-target-code">
                                    ${svgCopyIcon}
                                    复制代码
                                </button>
                            </div>
                        </th>
                    </tr></thead>
                    <tbody>${rowsHtml}</tbody>
                </table>
            </div>
        </div>
    `;

  // 绑定复制按钮
  const copyButton = document.getElementById("copy-target-code");
  if (copyButton) {
    copyButton.addEventListener("click", () => {
      const svgCopy = svgCopyIcon;
      const svgOk = `<svg width="13" height="13" viewBox="0 0 24 24" fill="currentColor"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>`;
      const svgFail = `<svg width="13" height="13" viewBox="0 0 24 24" fill="currentColor"><path d="M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z"/></svg>`;
      navigator.clipboard
        .writeText(targetCodeData)
        .then(() => {
          copyButton.innerHTML = svgOk + " 已复制";
          copyButton.classList.add("copy-btn-ok");
          setTimeout(() => {
            copyButton.innerHTML = svgCopy + " 复制代码";
            copyButton.classList.remove("copy-btn-ok");
          }, 2000);
        })
        .catch(() => {
          copyButton.innerHTML = svgFail + " 复制失败";
          copyButton.classList.add("copy-btn-fail");
          setTimeout(() => {
            copyButton.innerHTML = svgCopy + " 复制代码";
            copyButton.classList.remove("copy-btn-fail");
          }, 2000);
        });
    });
  }
}

// MIPS指令语法高亮函数
function highlightMIPSInstruction(instruction) {
  if (!instruction || instruction.trim() === "") return "";

  // MIPS指令关键字
  const instructions = ["add", "addi", "sub", "mult", "div", "and", "andi", "or", "ori", "xor", "xori", "sll", "srl", "sra", "sllv", "srlv", "srav", "jr", "jalr", "mfhi", "mflo", "lw", "sw", "lh", "sh", "lb", "sb", "lui", "beq", "bne", "blez", "bgtz", "bltz", "bgez", "j", "jal", "syscall", "nop", "move", "li", "la", "slt", "slti"];

  // 标签模式 (行首的标签)
  const labelPattern = /^(\w+):/;

  // 检查是否是标签行
  const labelMatch = instruction.match(labelPattern);
  if (labelMatch) {
    return instruction.replace(labelPattern, `<span style="color: #d73a49; font-weight: bold;">$1:</span>`);
  }

  // 简化的高亮方法，避免占位符冲突
  let highlighted = instruction;

  // 1. 先处理指令关键字（第一个单词）
  const words = instruction.split(/\s+/);
  if (words.length > 0) {
    const firstWord = words[0].toLowerCase();
    if (instructions.includes(firstWord)) {
      // 使用更精确的替换，只替换第一个出现的指令
      const instructionRegex = new RegExp(`^(\\s*)(${words[0]})(\\s|$)`, "i");
      highlighted = highlighted.replace(instructionRegex, `$1<span style="color: #005cc5; font-weight: bold;">$2</span>$3`);
    }
  }

  // 2. 处理寄存器 - 使用全局替换但避免已经在HTML标签中的内容
  highlighted = highlighted.replace(/(\$\w+|\$\d+)(?![^<]*>)/g, '<span style="color: #e36209; font-weight: 500;">$1</span>');

  // 3. 处理立即数 - 避免替换HTML标签中的数字
  highlighted = highlighted.replace(/\b(-?\d+)(?![^<]*>|[^<]*<\/span>)/g, '<span style="color: #032f62;">$1</span>');

  // 4. 处理逗号和括号 - 避免替换HTML标签中的符号
  highlighted = highlighted.replace(/([,\(\)])(?![^<]*>)/g, '<span style="color: #6f42c1;">$1</span>');

  return highlighted;
}

// 渲染错误列表
function renderError(errors, tab, table_headers, table_body, no_semantic) {
  const tabContent = document.getElementById(tab);
  const errorInfo = tab === "tab3" ? "代码语法存在错误，查看下方错误列表" : tab === "tab4" ? "代码语法存在错误，语法树不可用，查看下方错误列表" : tab === "tab6" ? "代码存在错误，目标代码不可用，查看下方错误列表" : no_semantic === true ? "代码语法存在错误，中间代码不可用，查看下方错误列表" : "代码语义存在错误，查看下方错误列表";

  // tab4 默认 padding:0（为iframe设置），显示错误时需要补回 padding
  const innerHtml = `
        <div class="state-view-compact">
            <div class="warning-icon"></div>
            <p class="state-text-error">${errorInfo}</p>
        </div>
        <div class="data-table-container error-table-container">
            <table class="data-table error-table">
                <thead>
                    <tr class="table-headers" id="${table_headers}"></tr>
                </thead>
                <tbody class="table-body" id="${table_body}"></tbody>
            </table>
        </div>
    `;
  tabContent.innerHTML = tab === "tab4" ? `<div class="error-pad">${innerHtml}</div>` : innerHtml;

  const tableHeaders = document.getElementById(table_headers);
  const tableBody = document.getElementById(table_body);
  if (!tableHeaders || !tableBody) return;

  const headers = ["序号", "说明", "位置"];
  tableHeaders.innerHTML = headers.map((h) => `<th>${h}</th>`).join("");

  // 创建表格行（对键值进行转义）
  tableBody.innerHTML = errors
    .map((error, index) => {
      const position = `(${error.line},${error.column})`;
      return `<tr><td>${index + 1}</td><td>${escapeHtml(error.message)}</td><td>${escapeHtml(position)}</td></tr>`;
    })
    .join("");
}

// 执行语法分析
async function getParser() {
  const tab3Content = document.getElementById("tab3");
  const tab4Content = document.getElementById("tab4");
  const tab5Content = document.getElementById("tab5");

  tab3Content.innerHTML = `
        <div class="loading-container">
            <div class="loading"></div>
            <p>语法分析进行中，正在获取"移进-规约"过程...</p>
        </div>
    `;
  tab4Content.innerHTML = `
        <div class="loading-container">
            <div class="loading"></div>
            <p>语法分析进行中，正在获取语法分析树...</p>
        </div>
    `;
  tab5Content.innerHTML = `
        <div class="loading-container">
            <div class="loading"></div>
            <p>语义分析进行中，正在生成中间代码...</p>
        </div>
    `;

  try {
    console.log("开始语法分析...");
    const data = await executeAPI("parse");
    console.log("语法分析完成:", data);

    reduceProductions = data.reduceProductions || [];
    syntaxErrors = data.parseErrors || [];
    quadRuples = data.quadruples || [];
    semanticErrors = data.semanticErrors || [];

    console.log("语法分析执行完成");
  } catch (error) {
    console.error("Error executing Parser:", error);

    // 显示错误状态
    tab4Content.innerHTML = tab3Content.innerHTML = tab5Content.innerHTML = stateHtml("error", `执行失败：${error.message}`, "请检查exe文件是否存在");
    return;
  }

  if (syntaxErrors && syntaxErrors.length > 0) {
    // 移进-规约、语法树、中间代码三个标签同步显示语法错误
    renderError(syntaxErrors, "tab3", "syntax-error-header", "syntax-error-body", false);
    renderError(syntaxErrors, "tab4", "tab4-syntax-error-header", "tab4-syntax-error-body", false);
    highlightErrors(syntaxErrors);
    // 语法错误了语义一定错误
    renderError(syntaxErrors, "tab5", "semantic-error-header", "semantic-error-body", true);
    return;
  }

  if (reduceProductions && reduceProductions.length > 0) {
    renderReduceTable("reduce-table-headers", "reduce-table-body", reduceProductions);
    renderSyntaxTree(reduceProductions);
  } else {
    // 没有数据时显示空状态
    tab3Content.innerHTML = tab4Content.innerHTML = stateHtml("empty", "暂无数据");
  }

  // 语义分析结果
  if (semanticErrors && semanticErrors.length > 0) {
    renderError(semanticErrors, "tab5", "semantic-error-header", "semantic-error-body", false);
    highlightErrors(semanticErrors);
  } else {
    renderQuadRuple();
  }
}

// 执行目标代码生成
async function getTargetCode() {
  const tab6Content = document.getElementById("tab6");

  tab6Content.innerHTML = `
        <div class="loading-container">
            <div class="loading"></div>
            <p>正在分析源代码...</p>
        </div>
    `;

  // 第一步：确保语法/语义分析结果是最新的（有缓存时不重复调用exe）
  try {
    const parseData = await executeAPI("parse");

    // 更新全局错误变量（确保与 parse 结果同步）
    syntaxErrors = parseData.parseErrors || [];
    semanticErrors = parseData.semanticErrors || [];
    quadRuples = parseData.quadruples || [];
    reduceProductions = parseData.reduceProductions || [];
  } catch (error) {
    console.error("目标代码生成：语法分析失败:", error);
    tab6Content.innerHTML = stateHtml("error", `分析失败：${error.message}`, "请检查exe文件是否存在");
    return;
  }

  // 第二步：若存在语法错误，目标代码不可生成，显示具体错误
  if (syntaxErrors && syntaxErrors.length > 0) {
    renderError(syntaxErrors, "tab6", "tab6-error-header", "tab6-error-body", false);
    return;
  }

  // 第三步：若存在语义错误，目标代码不可生成，显示具体错误
  if (semanticErrors && semanticErrors.length > 0) {
    renderError(semanticErrors, "tab6", "tab6-error-header", "tab6-error-body", false);
    return;
  }

  // 第四步：无错误，正式生成目标代码
  tab6Content.innerHTML = `
        <div class="loading-container">
            <div class="loading"></div>
            <p>目标代码生成中，正在生成MIPS汇编代码...</p>
        </div>
    `;

  try {
    console.log("开始目标代码生成...");
    const data = await executeAPI("target-code");
    console.log("目标代码生成完成:", data);

    targetCodeData = data.targetCode || "";
    renderTargetCode();
  } catch (error) {
    console.error("Error executing target code generation:", error);
    tab6Content.innerHTML = stateHtml("error", `生成失败：${error.message}`, "请检查exe文件是否存在和源代码是否正确");
  }
}

/* 监听编辑器变化 */
// 词法高亮 + 按当前激活标签决定是否触发分析
editor.on("change", () => {
  clearTimeout(timeout);

  sourceCodeChanged = true;

  timeout = setTimeout(async () => {
    try {
      // 词法分析：用于编辑器语法着色（与标签无关，始终执行）
      console.log("开始词法分析...");
      const data = await executeAPI("analyze");

      if (data && data.tokens) {
        updateEditorHighlight(data.tokens);
        console.log("词法分析完成，tokens数量:", data.tokens.length);
      }

      // 仅在当前可见标签页需要时才调用更重的分析
      if (activeTab === "tab3" || activeTab === "tab4" || activeTab === "tab5") {
        getParser();
      } else if (activeTab === "tab6") {
        getTargetCode();
      }
      // tab1/tab2：GOTO/ACTION 表不依赖源代码内容，无需重新加载
    } catch (error) {
      console.error("词法分析失败:", error);

      const doc = editor.getDoc();
      doc.getAllMarks().forEach((mark) => mark.clear());
    }
  }, 500); // 500毫秒不变化后触发分析
});
/* tab切换控制 */
document.addEventListener("DOMContentLoaded", async () => {
  const envInfo = document.getElementById("environment-info");
  envInfo.textContent = "正在初始化编译器...";
  envInfo.style.color = "var(--text-muted)";

  const tabs = document.querySelectorAll(".tab");
  const tabContents = document.querySelectorAll(".tab-content");

  // 页面挂载时直接加载语法规则
  const gotoTabContent = document.getElementById("tab1");
  const actionTabContent = document.getElementById("tab2");
  gotoTabContent.innerHTML = `
                <div class="loading-container">
                    <div class="loading"></div>
                    <p>语法规则解析中，GOTO表正在加载...</p>
                </div>
            `;

  try {
    console.log("开始加载语法规则...");

    // 检查是否有缓存的语法规则数据
    let data;
    if (cachedGrammarData && cacheLevel.grammar) {
      data = cachedGrammarData;
      // 缓存路径：短暂提示后直接就绪
      if (envInfo) {
        envInfo.textContent = "缓存命中 · 语法规则";
        envInfo.style.color = "#17a2b8";
        setTimeout(() => {
          envInfo.textContent = "编译器就绪";
          envInfo.style.color = "#22c55e";
        }, 1500);
      }
    } else {
      data = await executeAPI("grammar");
      // 非缓存路径：加载完成后明确标记为就绪
      if (envInfo) {
        envInfo.textContent = "编译器就绪";
        envInfo.style.color = "#22c55e";
      }
    }

    console.log("语法规则加载完成:", data);

    gotoTableData = data.gototable || [];
    actionTableData = data.actiontable || [];

    if (gotoTableData.length === 0) {
      throw new Error("GOTO表数据为空");
    }

    gotoTabContent.innerHTML = `
                    <div class="data-table-container">
                        <table class="data-table">
                            <thead>
                                <tr class="table-headers" id="goto-table-headers"></tr>
                            </thead>
                            <tbody class="table-body" id="goto-table-body"></tbody>
                        </table>
                    </div>
                `;
    renderTable("goto-table-headers", "goto-table-body", gotoTableData);
  } catch (error) {
    console.error("Error fetching GOTOtable data:", error);
    if (envInfo) {
      envInfo.textContent = "初始化失败，请重启应用";
      envInfo.style.color = "#ef4444";
    }
    const errHtml = stateHtml("error", "加载失败，请重试");
    gotoTabContent.innerHTML = errHtml;
    actionTabContent.innerHTML = errHtml;
  }

  // Tab 切换逻辑
  tabs.forEach((tab) => {
    tab.addEventListener("click", async () => {
      tabs.forEach((t) => {
        t.classList.remove("active");
        t.setAttribute("aria-selected", "false");
      });
      tabContents.forEach((content) => content.classList.add("hidden"));

      tab.classList.add("active");
      tab.setAttribute("aria-selected", "true");
      const targetId = tab.getAttribute("data-tab");
      activeTab = targetId; // 跟踪当前激活的标签
      const targetContent = document.getElementById(targetId);
      targetContent.classList.remove("hidden");

      if (targetId === "tab2") {
        const actionTabContent = document.getElementById("tab2");
        if (!actionTableData || actionTableData.length === 0) {
          actionTabContent.innerHTML = stateHtml("empty", "数据暂无", "ACTION 表尚未加载，请等待初始化完成");
          return;
        }
        actionTabContent.innerHTML = `
                            <div class="data-table-container">
                                <table class="data-table">
                                    <thead>
                                        <tr class="table-headers" id="action-table-headers"></tr>
                                    </thead>
                                    <tbody class="table-body" id="action-table-body"></tbody>
                                </table>
                            </div>
                        `;
        renderTable("action-table-headers", "action-table-body", actionTableData);
      } else if (targetId === "tab3" || targetId === "tab4" || targetId === "tab5") {
        getParser();
      } else if (targetId === "tab6") {
        getTargetCode();
      }
    });
  });

  // ── 拖拽分割线逻辑 ──────────────────────────────────────
  const containerEl = document.querySelector(".container");
  const editorPanel = document.querySelector(".editor-panel");
  const tabsPanel = document.querySelector(".tabs-container");
  const resizerEl = document.getElementById("resizer");

  let isResizing = false;
  let dragStartX = 0;
  let dragStartW = 0;

  resizerEl.addEventListener("mousedown", (e) => {
    isResizing = true;
    dragStartX = e.clientX;
    dragStartW = editorPanel.getBoundingClientRect().width;
    resizerEl.classList.add("dragging");
    document.body.style.cursor = "col-resize";
    document.body.style.userSelect = "none";
    tabsPanel.style.pointerEvents = "none"; // 防止 iframe 等子元素抢事件
    e.preventDefault();
  });

  document.addEventListener("mousemove", (e) => {
    if (!isResizing) return;
    const containerW = containerEl.getBoundingClientRect().width;
    const resizerW = 8; // .resizer 实际 CSS 宽度
    const minW = 180;
    const maxW = containerW - minW - resizerW;
    const newW = Math.max(minW, Math.min(maxW, dragStartW + (e.clientX - dragStartX)));
    editorPanel.style.flex = "none";
    editorPanel.style.width = newW + "px";
    tabsPanel.style.flex = "1";
  });

  document.addEventListener("mouseup", () => {
    if (!isResizing) return;
    isResizing = false;
    resizerEl.classList.remove("dragging");
    document.body.style.cursor = "";
    document.body.style.userSelect = "";
    tabsPanel.style.pointerEvents = "";
  });

  // 双击分割线恢复默认 1:1 比例
  resizerEl.addEventListener("dblclick", () => {
    editorPanel.style.flex = "1";
    editorPanel.style.width = "";
    tabsPanel.style.flex = "1";
  });
});
