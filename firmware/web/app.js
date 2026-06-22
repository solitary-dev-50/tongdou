const personality = document.querySelector("#personality");
const diagnosticOutput = document.querySelector("#diagnostic-output");
const refreshStatus = document.querySelector("#refresh-status");
const wifiStatus = document.querySelector("#wifi-status");
const timeStatus = document.querySelector("#time-status");
const modeStatus = document.querySelector("#mode-status");
const hardwareStatus = document.querySelector("#hardware-status");
const capabilityStatus = document.querySelector("#capability-status");

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
    modeStatus.textContent = modeText(status.systemMode);
    hardwareStatus.textContent = status.hardware?.degraded ? "降级" : "正常";
    capabilityStatus.textContent = capabilityText(status.capabilities);
  } catch (error) {
    wifiStatus.textContent = "读取失败";
    timeStatus.textContent = "读取失败";
    modeStatus.textContent = "读取失败";
    hardwareStatus.textContent = "读取失败";
    capabilityStatus.textContent = "读取失败";
  }
}

function modeText(mode) {
  return (
    {
      normal: "正常",
      portal: "配网",
      connecting: "联网中",
      degraded: "降级",
    }[mode] || mode
  );
}

function capabilityText(capabilities = {}) {
  const items = [];
  if (capabilities.reminders) items.push("提醒");
  if (capabilities.web) items.push("网页");
  if (capabilities.visual) items.push("视觉");
  if (capabilities.sound) items.push("声音");
  if (capabilities.voiceInput) items.push("语音输入");
  return items.length > 0 ? items.join(" / ") : "无";
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
