const APP_CONFIG = {
  name: "类 Rust 编译器",
  version: "0.1.0",
};

// 同步页面标题与 Header 版本号，保证两者与此配置始终一致
document.title = APP_CONFIG.name;
const _versionEl = document.getElementById("app-version");
if (_versionEl) {
  _versionEl.textContent = "v " + APP_CONFIG.version;
}
