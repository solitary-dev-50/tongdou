const diagnosticOutput = document.querySelector("#diagnostic-output");
const refreshStatus = document.querySelector("#refresh-status");
const syncTimeButton = document.querySelector("#sync-time");
const timeMessage = document.querySelector("#time-message");
const personalityMessage = document.querySelector("#personality-message");
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

function setActivePersonality(personality) {
  document.querySelectorAll("[data-personality]").forEach((button) => {
    const active = button.dataset.personality === personality;
    button.classList.toggle("active", active);
    button.setAttribute("aria-checked", active ? "true" : "false");
  });
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

async function syncTime() {
  if (!timeMessage) return;
  timeMessage.textContent = "同步中";
  try {
    const epoch = Math.floor(Date.now() / 1000);
    const response = await fetch("/api/time/sync", {
      method: "POST",
      headers: { "Content-Type": "application/x-www-form-urlencoded" },
      body: `epoch=${epoch}`,
    });
    timeMessage.textContent = await response.text();
    loadStatus();
  } catch (error) {
    timeMessage.textContent = `同步失败：${error}`;
  }
}

async function callMcpTool(name, args = {}) {
  if (diagnosticOutput) diagnosticOutput.textContent = `执行中：${name}`;
  try {
    const payload = {
      jsonrpc: "2.0",
      id: Date.now(),
      method: "tools/call",
      params: {
        name,
        arguments: args,
      },
    };
    const response = await fetch("/api/mcp", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });
    const text = await response.text();
    if (diagnosticOutput) diagnosticOutput.textContent = text;
    return text;
  } catch (error) {
    if (diagnosticOutput) diagnosticOutput.textContent = `MCP 调用失败：${error}`;
    return "";
  }
}

document.querySelectorAll("[data-personality]").forEach((button) => {
  button.addEventListener("click", async () => {
    const personality = button.dataset.personality;
    setActivePersonality(personality);
    if (personalityMessage) personalityMessage.textContent = "人格模式设置中";
    await callMcpTool("set_personality", { personality });
    if (personalityMessage) personalityMessage.textContent = "人格模式已发送";
  });
});

document.querySelectorAll("[data-command]").forEach((button) => {
  button.addEventListener("click", () => runDiagnostic(button.dataset.command));
});

document.querySelectorAll("[data-mcp-tool]").forEach((button) => {
  button.addEventListener("click", () => {
    const args = JSON.parse(button.dataset.mcpArgs || "{}");
    callMcpTool(button.dataset.mcpTool, args);
  });
});

refreshStatus?.addEventListener("click", loadStatus);
syncTimeButton?.addEventListener("click", syncTime);
loadStatus();
