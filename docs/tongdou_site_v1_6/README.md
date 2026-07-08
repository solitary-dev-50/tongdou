# 铜豆独立站 v1.6.1 中文说明

这个包是 `gettongdou.com` 的自动收集版独立站。

它不是普通静态网页，而是：

```text
前台落地页 + 表单收集 + Cloudflare Pages Functions + D1 数据库 + 管理员导出接口
```

当前已经支持：

```text
GET  /              铜豆独立站首页
POST /api/leads     用户提交 Waitlist / Tester 申请
GET  /api/leads     管理员读取全部线索 JSON
GET  /api/export    管理员导出 CSV / JSON 备份
```

---

## 1. 文件结构

```text
tongdou_site_v1_6/
  index.html                         独立站首页
  privacy.html                       隐私政策
  tester-agreement.html              海外测试者内容授权协议
  thank-you.html                     提交成功页
  README.md                          当前中文说明
  wrangler.toml                      Cloudflare Pages / D1 配置

  db/
    schema.sql                       D1 数据库建表 SQL

  functions/
    api/
      leads.js                       /api/leads 接口
      export.js                      /api/export 接口

  tools/
    tongdou_launch_war_room_v3_4_cn.html
                                      冷启动指挥中心，可远程同步 /api/leads

  assets/
    tongdou-hero-placeholder.png
    tongdou-work-reminder-placeholder.png
    tongdou-hardware-closeup-placeholder.png
    tongdou-founder-edition-placeholder.png
    tongdou-tester-desk-placeholder.png
```

---

## 2. 独立站支持视频吗？

支持。

当前 v1.6.1 包里**没有内置真实视频文件**，因为视频文件通常很大，不适合直接塞进 zip 里。但网页本身完全可以放视频，最适合放这些格式：

```text
.mp4      最通用，优先用
.webm     体积更小，浏览器兼容也不错
```

推荐视频规格：

```text
首屏 / 故事区视频：16:9，1920×1080 或 1280×720
产品细节视频：4:3 或 1:1
文件大小：最好控制在 5MB～20MB
编码：H.264 MP4
```

放视频的位置建议：

```text
assets/videos/tongdou-hero-loop.mp4
assets/videos/tongdou-desk-reminder.mp4
assets/videos/tongdou-hardware-closeup.mp4
assets/videos/tongdou-founder-edition.mp4
assets/videos/tongdou-tester-desk.mp4
```

如果要把某张图片换成视频，可以把类似这段：

```html
<img src="assets/tongdou-hero-placeholder.png" alt="TongDou placeholder hero render on a dark desk" />
```

换成：

```html
<video class="site-video" autoplay muted loop playsinline poster="assets/tongdou-hero-placeholder.png">
  <source src="assets/videos/tongdou-hero-loop.mp4" type="video/mp4" />
</video>
```

注意：

```text
autoplay 必须配 muted，否则很多浏览器不会自动播放。
playsinline 是给手机浏览器用的，避免点开全屏。
poster 是视频加载前显示的封面图。
```

---

## 3. 哪些位置应该放什么图片 / 视频？

### A. Hero 首屏

当前首屏右侧的主角是：

```text
OLED 表情模拟器
```

这里**不要再堆大图**。首屏已经用 OLED 模拟器承载“铜豆是活的”这个第一印象。

建议：

```text
首屏保留 OLED Emulator，不放静态大图。
```

以后如果要加视频，也不要放在右侧主视觉里，而是放到下一屏 `What it is` 区域。

---

### B. What it is / Story 区域

当前图片：

```text
assets/tongdou-hero-placeholder.png
```

用途：

```text
展示铜豆整体外观
让用户第一眼看清楚：黄铜骨架 + OLED 脸 + 齿轮轮子 + 裸露 PCB
```

以后建议替换为：

```text
真实产品桌面照片
或者 10～20 秒循环视频
```

推荐内容：

```text
铜豆在暗色桌面上眨眼
OLED 表情有一点小坏笑
轮子轻轻动一下
灯光偏暖，桌面真实，不要太像玩具广告
```

适合文件名：

```text
assets/tongdou-hero-real.jpg
assets/videos/tongdou-hero-loop.mp4
```

---

### C. Work Reminder / 使用场景区域

当前图片：

```text
assets/tongdou-work-reminder-placeholder.png
```

用途：

```text
表达“深夜加班、铜豆提醒你休息/下班”
```

以后建议替换为：

```text
真实桌面场景照片
短视频更好
```

推荐内容：

```text
电脑、键盘、咖啡杯、暖色台灯
人趴在桌边或手扶额头
铜豆在桌上看着你，像在吐槽：够了，别干了
```

适合文件名：

```text
assets/tongdou-work-reminder-real.jpg
assets/videos/tongdou-work-reminder.mp4
```

---

### D. Behind the Brass — Maker Letter 区域

当前这里主要是文字信件。

建议补一张“创作者/工作台”照片，但不要太商业。

推荐内容：

```text
烙铁
黄铜骨架
PCB
焊接夹具
桌面凌乱但真实
手可以出现，但不一定露脸
```

这个区域的图不要太花哨，它要服务那封信的情绪。

适合文件名：

```text
assets/tongdou-maker-workbench.jpg
```

---

### E. Hardware 区域

当前图片：

```text
assets/tongdou-hardware-closeup-placeholder.png
```

用途：

```text
证明铜豆不是概念图，而是真实硬件
```

以后建议替换为真实特写：

```text
OLED 表情屏
24GHz 毫米波雷达板
英飞凌 IM72D128 PDM 麦克风
ESP32-S3 主控
N20 全金属减速电机
1200mAh 电池
1mm 黄铜激光切割骨架
```

推荐内容：

```text
一张总特写 + 若干局部图
不要只拍漂亮外壳，要拍螺丝、排线、PCB、传感器、轮子
```

适合文件名：

```text
assets/tongdou-hardware-closeup-real.jpg
assets/tongdou-oled-closeup.jpg
assets/tongdou-radar-board.jpg
assets/tongdou-brass-frame.jpg
```

---

### F. Editions / Founder Edition 区域

当前图片：

```text
assets/tongdou-founder-edition-placeholder.png
```

用途：

```text
展示紫金创始人版 / 定制创始人 PCB 版的高级感
```

以后建议替换为：

```text
紫金 PCB 实拍
编号铭牌
创始人版裸板
定制丝印示意
```

推荐内容：

```text
黑色背景
紫色沉金 PCB
黄铜件
编号 / Founder’s Edition 感觉
不要太像奢侈品，要保持极客收藏感
```

适合文件名：

```text
assets/tongdou-founder-edition-real.jpg
assets/tongdou-purple-gold-pcb.jpg
assets/tongdou-custom-founder-pcb.jpg
```

---

### G. Tester 区域

当前图片：

```text
assets/tongdou-tester-desk-placeholder.png
```

用途：

```text
吸引开发者、创客、内容创作者申请测试
```

以后建议替换为：

```text
拍摄桌面 / 创客桌面 / 测评场景
```

推荐内容：

```text
相机
三脚架
烙铁
笔记本电脑
零件盒
铜豆在中间，像正在被测试/拍摄
```

适合文件名：

```text
assets/tongdou-tester-desk-real.jpg
assets/videos/tongdou-tester-desk.mp4
```

---

### H. Open Source / GitHub 区域

当前没有单独大图。

建议以后补一张：

```text
PCB 设计截图
GitHub README 截图
固件代码截图
BOM 表片段
```

这会增强极客信任感。

适合文件名：

```text
assets/tongdou-open-source-files.jpg
assets/tongdou-pcb-layout.jpg
```

---

## 4. 当前页面里已经用到的图片位置

`index.html` 目前用到这些资源：

```text
assets/tongdou-hero-placeholder.png
位置：What it is / Story 区域
作用：整体产品主视觉

assets/tongdou-work-reminder-placeholder.png
位置：使用场景区域
作用：深夜工作提醒场景

assets/tongdou-hardware-closeup-placeholder.png
位置：Hardware 区域
作用：硬件细节特写

assets/tongdou-founder-edition-placeholder.png
位置：Editions / Founder 区域
作用：创始人版视觉

assets/tongdou-tester-desk-placeholder.png
位置：Tester 区域
作用：测试者/创作者场景
```

还有一个 Open Graph 分享图：

```html
<meta property="og:image" content="assets/tongdou-hero-placeholder.png" />
```

这个会影响 Discord、X、Facebook、Telegram 等平台分享链接时显示的封面。

正式发布前建议换成：

```text
assets/tongdou-og-image.jpg
```

推荐尺寸：

```text
1200×630
```

---

## 5. 图片替换规则

最简单的替换方法：

```text
保持原文件名不变
直接用真实图片覆盖 assets/ 里的 placeholder 图片
```

例如：

```text
用真实主视觉图覆盖：
assets/tongdou-hero-placeholder.png

用真实硬件特写覆盖：
assets/tongdou-hardware-closeup-placeholder.png
```

这样不用改 HTML。

如果你想保留 placeholder，同时使用新文件名，就要改 `index.html` 里的 `src="..."`。

---

## 6. 图片尺寸建议

```text
主视觉 / 场景图：16:9，1920×1080 或 1600×900
硬件特写：4:3，1600×1200 或 1440×1080
Founder 图：4:3 或 1:1
Open Graph 分享图：1200×630
图片格式：jpg / webp / png
```

建议：

```text
照片优先 jpg / webp
透明图才用 png
网页大图尽量压缩到 300KB～1.5MB
不要一张图 5MB 以上
```

---

## 7. Cloudflare 环境变量

在 Cloudflare Pages 项目里配置：

```text
ADMIN_TOKEN=你自己生成的长密钥
DISCORD_WEBHOOK_URL=你的 Discord Webhook 地址，可选
DISCORD_NOTIFY_ALL=false
```

说明：

```text
ADMIN_TOKEN：后台读取 / 导出数据用，不能公开
DISCORD_WEBHOOK_URL：高价值线索推送到 Discord，用 Secret 保存
DISCORD_NOTIFY_ALL：false 表示只推送高价值线索
```

---

## 8. D1 数据库绑定

D1 数据库名建议：

```text
tongdou-leads
```

Pages Functions 绑定名必须是：

```text
TONGDOU_DB
```

代码里访问的是：

```js
env.TONGDOU_DB
```

所以绑定名不能写成 `DB`，也不能写成 `tongdou_db`。

建表 SQL 在：

```text
db/schema.sql
```

---

## 9. API 使用方式

### 用户提交线索

```text
POST https://gettongdou.com/api/leads
```

前端表单会自动提交，不需要手动调用。

### 管理员读取线索

```text
https://gettongdou.com/api/leads?token=你的ADMIN_TOKEN
```

正常返回：

```json
{
  "ok": true,
  "count": 0,
  "leads": []
}
```

### 导出 JSON

```text
https://gettongdou.com/api/export?format=json&token=你的ADMIN_TOKEN
```

### 导出 CSV

```text
https://gettongdou.com/api/export?format=csv&token=你的ADMIN_TOKEN
```

---

## 10. 高价值线索通知规则

满足任一条件，就属于高价值线索：

```text
type = tester
tier = founder
tier = custom_founder
price >= 199
social 不为空
can_post_review = true
```

默认只推送高价值线索。

如果想所有提交都推送 Discord：

```text
DISCORD_NOTIFY_ALL=true
```

---

## 11. 部署方式

因为这个项目用了：

```text
functions/api/leads.js
functions/api/export.js
```

所以不要只用网页后台拖 ZIP。

推荐用 Wrangler：

```powershell
npx.cmd wrangler login
npx.cmd wrangler pages deploy . --project-name=tongdou --branch=main --commit-dirty=true
```

部署成功后日志应包含：

```text
Compiled Worker successfully
Uploading Functions bundle
Deployment complete
```

---

## 12. 测试清单

上线后依次测试：

```text
1. 首页能打开
2. /api/leads?token=ADMIN_TOKEN 返回 ok:true
3. 提交一个 Waitlist 邮箱
4. 再查 /api/leads，count 变成 1
5. 提交一个 Tester 申请
6. Tester 数据里 tester_agreement_accepted=true
7. 重复提交同一个 email，应该更新，不应该新增重复行
8. /api/export?format=json 能导出 JSON
9. /api/export?format=csv 能导出 CSV
10. 如果配置了 Discord，高价值线索应有提醒
```

---

## 13. 现有功能保留

```text
OLED 表情模拟器
右下角嘴欠 Gremlin
档位卡片点击后自动滚到 Waitlist 表单
极简 Waitlist 表单
硬核 Tester 表单
主动勾选 Tester Agreement
5 条精简版 Tester Agreement
本地 JSON / CSV 备份工具
远程 API 自动收集
```

---

## 14. 安全注意事项

```text
不要把 ADMIN_TOKEN 写进 index.html
不要把 DISCORD_WEBHOOK_URL 写进 index.html
不要截图公开 ADMIN_TOKEN
GET /api/leads 和 GET /api/export 必须带管理员 Token
POST /api/leads 是公开入口，但服务端会校验字段
```

---

## 15. 后续建议

下一步可以继续补：

```text
1. 用真实照片替换 placeholder 图片
2. 给 What it is 区域加入 10～20 秒循环视频
3. 给 Hardware 区域加入真实硬件细节图
4. 配置真实 GitHub 链接
5. 配置真实 Discord 邀请链接
6. 把本地提醒脚本改成 API 模式
7. 冷启动指挥中心直接远程同步 /api/leads
```
 
---

## 16. 后台作战室页面

后台作战室页面已放在：

```text
/admin/war-room.html
```

线上访问路径：

```text
https://gettongdou.com/admin/war-room.html
```

页面里的远程 API 默认地址是：

```text
https://gettongdou.com/api/leads
```

`ADMIN_TOKEN` 不会写死在页面里，必须由管理员在页面里手动输入。

安全注意事项：

```text
不要把 ADMIN_TOKEN 写进 HTML、README、截图或公开 issue。
不要把后台页当成公开运营页面传播。
后台页只是一个内部工具，真正的数据权限仍然由 /api/leads 的 ADMIN_TOKEN 控制。
如果怀疑 ADMIN_TOKEN 泄露，要立刻在 Cloudflare Pages 里更换密钥。
```

---

## 17. 首页叙事顺序

首页现在按故事推进，不按普通电商详情页推进。

当前顺序是：

```text
1. 首屏介绍 TongDou 是谁
2. What it is / Story：说明它是桌面上的小陪伴
3. Not here to manage you：说明它不是监控和效率工具
4. You can build one yourself：把用户带到亲手制作这件事
5. Behind the Brass / A Letter from the Maker：Maker 自述，作为故事和套件信息之间的过渡
6. What's in the box：进入盒内套件和硬件信息
7. Editions / Tester / Join：再进入众筹档位、测试者申请和留资
```

注意：

```text
Maker Letter 不放价格、不放早鸟、不放购买按钮。
顶部导航的 Maker Letter 锚点仍然指向 #maker-letter。
```
