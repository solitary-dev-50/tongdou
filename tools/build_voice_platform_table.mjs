import fs from "node:fs/promises";
import path from "node:path";
import { SpreadsheetFile, Workbook } from "@oai/artifact-tool";

const outputDir = path.resolve("outputs", "voice-platform");
await fs.mkdir(outputDir, { recursive: true });

const workbook = Workbook.create();
const sheet = workbook.worksheets.add("铜豆命令词");

const headers = [
  "语义标签",
  "最小功能词",
  "功能泛化词",
  "功能类型",
  "播报语句",
  "播报模式",
  "发送协议",
  "接收协议",
];

const rows = [
  [1, "你好铜豆", "你好铜豆", "唤醒词", "在", "主", "", ""],
  [1, "小铜豆", "小铜豆", "唤醒词", "在", "主", "", ""],
  [2, "立刻闭嘴", "立刻闭嘴", "命令词", "行行行，我闭嘴。", "主", "AA 03", ""],
  [2, "安静模式", "安静模式", "命令词", "行行行，我闭嘴。", "主", "AA 03", ""],
  [3, "推迟十分钟", "推迟十分钟", "命令词", "行，十分钟后再抓你。", "主", "AA 04", ""],
  [3, "等会儿再提醒", "等会儿再提醒", "命令词", "行，十分钟后再抓你。", "主", "AA 04", ""],
  [4, "确认", "确认", "命令词", "收到。", "主", "AA 10", ""],
  [4, "知道了", "知道了", "命令词", "收到。", "主", "AA 10", ""],
  [4, "收到", "收到", "命令词", "收到。", "主", "AA 10", ""],
  [5, "休息五分钟", "休息五分钟", "命令词", "行，休息五分钟。", "主", "AA 11", ""],
  [5, "我要休息一下", "我要休息一下", "命令词", "行，休息五分钟。", "主", "AA 11", ""],
  [6, "开始提醒我", "开始提醒我", "命令词", "说吧，本豆记着。", "主", "AA 12", ""],
  [6, "帮我记一下", "帮我记一下", "命令词", "说吧，本豆记着。", "主", "AA 12", ""],
  [7, "取消提醒", "取消提醒", "命令词", "行，这事先放过你。", "主", "AA 13", ""],
  [7, "不用提醒了", "不用提醒了", "命令词", "行，这事先放过你。", "主", "AA 13", ""],
  [90, "测试串口", "测试串口", "命令词", "测试。", "主", "AA F0", ""],
  [91, "测试动作", "测试动作", "命令词", "测试动作。", "主", "AA F1", ""],
];

sheet.getRange("A1:H1").values = [headers];
sheet.getRangeByIndexes(1, 0, rows.length, headers.length).values = rows;

sheet.getRange("A1:H1").format = {
  fill: "#1F4E78",
  font: { bold: true, color: "#FFFFFF" },
  wrapText: true,
};
sheet.getRangeByIndexes(0, 0, rows.length + 1, headers.length).format.borders = {
  preset: "all",
  style: "thin",
  color: "#D9D9D9",
};
sheet.getRange("A:A").format.columnWidth = 10;
sheet.getRange("B:B").format.columnWidth = 18;
sheet.getRange("C:C").format.columnWidth = 30;
sheet.getRange("D:D").format.columnWidth = 12;
sheet.getRange("E:E").format.columnWidth = 26;
sheet.getRange("F:F").format.columnWidth = 10;
sheet.getRange("G:H").format.columnWidth = 18;
sheet.getRangeByIndexes(0, 0, rows.length + 1, headers.length).format.wrapText = true;
sheet.freezePanes.freezeRows(1);

const inspect = await workbook.inspect({
  kind: "table",
  range: "铜豆命令词!A1:H18",
  include: "values",
  tableMaxRows: 20,
  tableMaxCols: 8,
});
console.log(inspect.ndjson);

const errors = await workbook.inspect({
  kind: "match",
  searchTerm: "#REF!|#DIV/0!|#VALUE!|#NAME\\?|#N/A",
  options: { useRegex: true, maxResults: 50 },
});
console.log(errors.ndjson);

await workbook.render({ sheetName: "铜豆命令词", range: "A1:H18", scale: 1, format: "png" });

const output = await SpreadsheetFile.exportXlsx(workbook);
const outPath = path.join(outputDir, "铜豆语音平台命令词配置表-修正版.xlsx");
await output.save(outPath);
console.log(outPath);
