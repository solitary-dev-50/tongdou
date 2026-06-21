const personality = document.querySelector("#personality");
const diagnosticOutput = document.querySelector("#diagnostic-output");
const refreshStatus = document.querySelector("#refresh-status");
const wifiStatus = document.querySelector("#wifi-status");
const timeStatus = document.querySelector("#time-status");

async function loadStatus() {
  try {
    const response = await fetch("/api/status");
    const status = await response.json();
    wifiStatus.textContent = status.wifiConnected
      ? "已连接"
      : status.portal
        ? "配网模式"
        : "连接中";
    timeStatus.textContent = status.timeReady ? status.now : "未校时";
  } catch (error) {
    wifiStatus.textContent = "读取失败";
    timeStatus.textContent = "读取失败";
  }
}

async function runDiagnostic(command) {
  diagnosticOutput.textContent = `执行中：${command}`;
  try {
    const body = new URLSearchParams({ command });
    const response = await fetch("/api/diagnostic", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body,
    });
    diagnosticOutput.textContent = await response.text();
  } catch (error) {
    diagnosticOutput.textContent = `诊断失败：${error}`;
  }
}

personality?.addEventListener("change", (event) => {
  console.log("personality changed", event.target.value);
});

document.querySelectorAll("[data-command]").forEach((button) => {
  button.addEventListener("click", () => runDiagnostic(button.dataset.command));
});

refreshStatus?.addEventListener("click", loadStatus);
loadStatus();
