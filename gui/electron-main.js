const { app, BrowserWindow, ipcMain, dialog } = require("electron");
const { exec, execFile, spawn } = require("child_process");
const fs = require("fs");
const path = require("path");

// 在Windows下修复控制台编码问题
if (process.platform === "win32") {
  // 设置环境变量处理中文编码
  process.env.PYTHONIOENCODING = "utf-8";

  // 设置控制台代码页为UTF-8
  exec("chcp 65001", { shell: true }, (error, stdout, stderr) => {
    if (error) {
      console.log("设置控制台编码时出现错误，但应用程序将继续运行");
    } else {
      console.log("控制台编码已设置为UTF-8");
    }
  });
}

const logMessage = console.log.bind(console);

let mainWindow;
// 判断是否为开发模式
const isDev = process.argv.includes("--dev") || process.defaultApp || /[\\/]electron-prebuilt[\\/]/.test(process.execPath) || /[\\/]electron[\\/]/.test(process.execPath);

// 禁用硬件加速，避免 GPU 子进程在某些显卡驱动下崩溃导致闪烁
// （Windows exit_code=-1073740791 即 0xC0000409 EXCEPTION_STACK_BUFFER_OVERRUN）
app.disableHardwareAcceleration();

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1400,
    height: 900,
    minWidth: 1200,
    minHeight: 700,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
      enableRemoteModule: true,
    },
    icon: path.join(__dirname, "assets/icon.png"),
    title: "类Rust编译器",
    backgroundColor: "#ffffff", // 防止显示前黑屏闪烁
    show: false, // 先隐藏，等加载完成后显示
  });

  mainWindow.loadFile("index.html");

  mainWindow.once("ready-to-show", () => {
    mainWindow.show();
    if (isDev) {
      mainWindow.webContents.openDevTools();
    }
  });

  mainWindow.on("closed", () => {
    mainWindow = null;
  });
}

app.whenReady().then(() => {
  createWindow();

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

// 所有窗口关闭时退出应用（除了macOS）
app.on("window-all-closed", () => {
  if (process.platform !== "darwin") {
    app.quit();
  }
});

// 获取exe文件路径
function getExePath(exeName) {
  if (isDev) {
    // 开发模式：从当前目录
    return path.join(__dirname, exeName);
  } else {
    // 生产模式：尝试多个可能的路径
    const possiblePaths = [
      // 1. 尝试从项目根目录（对于npm start）
      path.join(__dirname, exeName),
      // 2. 尝试从resources目录（对于打包后的应用）
      path.join(process.resourcesPath, exeName),
      // 3. 尝试从app.asar.unpacked目录（某些打包情况）
      path.join(process.resourcesPath, "app.asar.unpacked", exeName),
    ];

    for (const exePath of possiblePaths) {
      if (fs.existsSync(exePath)) {
        return exePath;
      }
    }

    // 如果都不存在，返回第一个路径（会在后续检查中报错）
    return possiblePaths[0];
  }
}

// 串行执行队列：保证同一时刻只有一个 exe 实例在运行
let _exeQueue = Promise.resolve();

function executeExe(exeName, args = []) {
  const task = _exeQueue.then(
    () => _executeExeCore(exeName, args),
    () => _executeExeCore(exeName, args), // 前一个任务失败时也继续执行
  );
  // 让队列尾指向当前任务（吞掉 rejection，避免 UnhandledPromiseRejection）
  _exeQueue = task.then(
    () => {},
    () => {},
  );
  return task;
}

function _executeExeCore(exeName, args = []) {
  return new Promise((resolve, reject) => {
    const exePath = getExePath(exeName);

    if (!fs.existsSync(exePath)) {
      reject(new Error(`${exeName} 文件不存在: ${exePath}`));
      return;
    }

    logMessage(`正在执行: ${exeName}`);
    logMessage(`文件路径: ${exePath}`);
    logMessage(`命令行参数: ${args.join(" ")}`);

    // 获取正确的工作目录
    let workingDir;
    if (isDev) {
      workingDir = __dirname;
    } else {
      // 在打包后的应用中，使用exe文件所在的目录
      workingDir = path.dirname(exePath);
    }

    // 在Windows系统中设置正确的编码
    const execOptions = {
      cwd: workingDir,
      timeout: 30000, // 30秒超时
      encoding: "utf8",
    };

    // Windows系统需要特殊处理编码
    if (process.platform === "win32") {
      execOptions.encoding = "buffer"; // 使用buffer来获取原始数据
    }

    execFile(exePath, args, execOptions, (error, stdout, stderr) => {
      if (error) {
        // 区分不同的失败类型，提供更准确的错误信息
        if (error.code === 3) {
          // Windows CRT abort()：exit code 3，通常是输入导致编译器内部断言触发
          const msg = `${exeName} 运行时崩溃（abort，退出码 3）。输入内容可能触发了编译器内部错误，请检查代码是否合法。`;
          console.error(msg);
          reject(new Error(msg));
        } else if (error.code === "ENOENT") {
          reject(new Error(`${exeName} 文件不存在或无法启动: ${exePath}`));
        } else if (error.killed || error.signal) {
          reject(new Error(`${exeName} 执行超时或被系统终止（signal: ${error.signal}）`));
        } else {
          console.error(`执行 ${exeName} 失败 (code=${error.code}):`, error);
          reject(error);
        }
        return;
      }

      // Windows系统中处理编码转换
      let stdoutStr = stdout;
      let stderrStr = stderr;

      if (process.platform === "win32" && Buffer.isBuffer(stdout)) {
        // 在Windows下将GBK编码转换为UTF-8
        try {
          const iconv = require("iconv-lite");
          stdoutStr = iconv.decode(stdout, "cp936"); // cp936是GBK编码
          if (stderr && Buffer.isBuffer(stderr)) {
            stderrStr = iconv.decode(stderr, "cp936");
          }
        } catch (iconvError) {
          // 如果iconv-lite不可用，使用默认的UTF-8解码
          stdoutStr = stdout.toString("utf8");
          stderrStr = stderr ? stderr.toString("utf8") : "";
        }
      }

      if (stderrStr) {
        console.warn(`${exeName} 警告:`, stderrStr);
      }

      logMessage(`${exeName} 执行成功:`, stdoutStr);
      resolve(stdoutStr);
    });
  });
}

// 读取JSON文件
function readJsonFile(jsonFileName) {
  return new Promise((resolve, reject) => {
    try {
      // 根据运行环境确定rust目录的路径
      let rustDir;
      if (isDev) {
        rustDir = path.join(__dirname, "rust", "result");
      } else {
        // 在打包后的应用中，rust目录在resources目录下
        rustDir = path.join(process.resourcesPath, "rust", "result");
      }

      const jsonPath = path.join(rustDir, jsonFileName);

      logMessage(`尝试读取JSON文件: ${jsonPath}`);

      if (!fs.existsSync(jsonPath)) {
        const errorMsg = `${jsonFileName} 文件不存在: ${jsonPath}`;
        console.error(errorMsg);
        reject(new Error(errorMsg));
        return;
      }

      const data = fs.readFileSync(jsonPath, "utf8");
      const jsonData = JSON.parse(data);
      logMessage(`成功读取 ${jsonFileName}`);
      resolve(jsonData);
    } catch (error) {
      const errorMsg = `读取 ${jsonFileName} 失败: ${error.message}`;
      console.error(errorMsg);
      reject(new Error(errorMsg));
    }
  });
}

// 确保rust目录和result子目录存在
function ensureRustDirectory() {
  // 根据运行环境确定rust目录的路径
  let rustDir, resultDir;
  if (isDev) {
    rustDir = path.join(__dirname, "rust");
    resultDir = path.join(__dirname, "rust", "result");
  } else {
    // 在打包后的应用中，rust目录在resources目录下
    rustDir = path.join(process.resourcesPath, "rust");
    resultDir = path.join(process.resourcesPath, "rust", "result");
  }

  if (!fs.existsSync(rustDir)) {
    fs.mkdirSync(rustDir, { recursive: true });
    console.log("创建rust目录:", rustDir);
  }

  if (!fs.existsSync(resultDir)) {
    fs.mkdirSync(resultDir, { recursive: true });
    console.log("创建rust/result目录:", resultDir);
  }

  return { rustDir, resultDir };
}

// 将源代码保存到rust/test.rs文件
function saveSourceCodeToFile(sourceCode) {
  return new Promise((resolve, reject) => {
    try {
      ensureRustDirectory();

      // 根据运行环境确定rust目录的路径
      let rustDir;
      if (isDev) {
        rustDir = path.join(__dirname, "rust");
      } else {
        // 在打包后的应用中，rust目录在resources目录下
        rustDir = path.join(process.resourcesPath, "rust");
      }

      const sourceFilePath = path.join(rustDir, "test.rs");

      fs.writeFileSync(sourceFilePath, sourceCode, "utf8");
      logMessage("源代码已保存到:", sourceFilePath);
      resolve(sourceFilePath);
    } catch (error) {
      reject(new Error(`保存源代码文件失败: ${error.message}`));
    }
  });
}

// IPC处理器：执行词法分析
ipcMain.handle("execute-lexical", async (event, code) => {
  try {
    logMessage("开始执行词法分析...");
    ensureRustDirectory(); // 确保rust目录存在

    // 先保存源代码到rust/test.rs
    await saveSourceCodeToFile(code);

    // 执行词法分析器，传入lexical参数
    await executeExe("Rust-like-Compiler.exe", ["lexical"]);

    const tokens = await readJsonFile("LexicalAnalyzer.json");
    logMessage("词法分析完成");
    return { success: true, data: { tokens: tokens } };
  } catch (error) {
    console.error("词法分析失败:", error);
    return { success: false, error: error.message };
  }
});

// IPC处理器：执行语法分析
ipcMain.handle("execute-parser", async (event, code) => {
  try {
    logMessage("开始执行语法分析...");
    ensureRustDirectory(); // 确保rust目录存在

    // 先保存源代码到rust/test.rs
    await saveSourceCodeToFile(code);

    // 执行语法分析器，传入parse参数
    await executeExe("Rust-like-Compiler.exe", ["parse"]);

    // 读取生成的多个JSON文件并合并结果
    const [processData, quadsData, parseErrorsData, semanticErrorsData] = await Promise.allSettled([
      readJsonFile("process.json"), // 移进-归约过程
      readJsonFile("quads.json"), // 中间代码
      readJsonFile("perror.json"), // 语法分析错误
      readJsonFile("serror.json"), // 语义分析错误
    ]);

    const result = {
      reduceProductions: processData.status === "fulfilled" ? processData.value : [],
      quadruples: quadsData.status === "fulfilled" ? quadsData.value : [],
      parseErrors: parseErrorsData.status === "fulfilled" ? parseErrorsData.value : [],
      semanticErrors: semanticErrorsData.status === "fulfilled" ? semanticErrorsData.value : [],
    };

    logMessage("语法分析完成");
    return { success: true, data: result };
  } catch (error) {
    console.error("语法分析失败:", error);
    return { success: false, error: error.message };
  }
});

// IPC处理器：执行语法规则分析
ipcMain.handle("execute-grammar", async (event) => {
  try {
    logMessage("开始执行语法规则分析...");
    ensureRustDirectory(); // 确保rust目录存在

    // 执行语法规则分析，传入grammar参数
    await executeExe("Rust-like-Compiler.exe", ["grammar"]);

    // 读取生成的GOTO表和ACTION表JSON文件
    const [gotoData, actionData] = await Promise.allSettled([readJsonFile("goto.json"), readJsonFile("action.json")]);

    const result = {
      gototable: gotoData.status === "fulfilled" ? gotoData.value : [],
      actiontable: actionData.status === "fulfilled" ? actionData.value : [],
    };

    logMessage("语法规则分析完成");
    return { success: true, data: result };
  } catch (error) {
    console.error("语法规则分析失败:", error);
    return { success: false, error: error.message };
  }
});

// IPC处理器：执行目标代码生成
ipcMain.handle("execute-target-code", async (event, code) => {
  try {
    logMessage("开始执行目标代码生成...");
    ensureRustDirectory(); // 确保rust目录存在

    // 先保存源代码到rust/test.rs
    await saveSourceCodeToFile(code);

    // 执行目标代码生成，传入target参数
    await executeExe("Rust-like-Compiler.exe", ["target"]);

    // 根据运行环境确定路径
    let targetCodePath;
    if (isDev) {
      targetCodePath = path.join(__dirname, "rust", "result", "objectCode.txt");
    } else {
      targetCodePath = path.join(process.resourcesPath, "rust", "result", "objectCode.txt");
    }

    logMessage(`尝试读取目标代码文件: ${targetCodePath}`);

    if (!fs.existsSync(targetCodePath)) {
      const errorMsg = `objectCode.txt 文件不存在: ${targetCodePath}`;
      console.error(errorMsg);
      throw new Error(errorMsg);
    }

    const targetCodeContent = fs.readFileSync(targetCodePath, "utf8");
    logMessage("成功读取目标代码文件");

    return { success: true, data: { targetCode: targetCodeContent } };
  } catch (error) {
    console.error("目标代码生成失败:", error);
    return { success: false, error: error.message };
  }
});

// IPC处理器：显示错误对话框
ipcMain.handle("show-error", async (event, title, message) => {
  const result = await dialog.showMessageBox(mainWindow, {
    type: "error",
    title: title,
    message: message,
    buttons: ["确定"],
  });
  return result;
});

// IPC处理器：显示信息对话框
ipcMain.handle("show-info", async (event, title, message) => {
  const result = await dialog.showMessageBox(mainWindow, {
    type: "info",
    title: title,
    message: message,
    buttons: ["确定"],
  });
  return result;
});

// IPC处理器：获取应用信息
ipcMain.handle("get-app-info", async () => {
  return {
    version: app.getVersion(),
    name: app.getName(),
    isDev: isDev,
    platform: process.platform,
    arch: process.arch,
  };
});

// 处理未捕获的异常
process.on("uncaughtException", (error) => {
  console.error("未捕获的异常:", error);
  if (mainWindow) {
    dialog.showErrorBox("应用错误", `发生未捕获的异常: ${error.message}`);
  }
});

logMessage("Electron主进程启动完成");
