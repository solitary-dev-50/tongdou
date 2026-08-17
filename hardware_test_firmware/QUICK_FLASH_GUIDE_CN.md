# 铜豆 V9 硬件测试固件快速烧录指南

这份说明给普通用户用：只烧录已经编译好的固件，不需要安装
PlatformIO、ESP-IDF、Python 或编译器。

## 推荐方法

使用发布 ZIP 里的“一键烧录”。

1. 解压 ZIP。
2. 用 USB 数据线把铜豆 V9 连接到 Windows 电脑。
3. 双击：

```text
FLASH_TONGDOU.bat
```

脚本会自动完成：

- 使用包内的乐鑫官方 `esptool.exe`
- 自动寻找 Espressif 串口设备
- 把 `TongDou_V9_Hardware_Test_v1.0.bin` 烧录到 `0x0`
- 烧录完成后复位铜豆
- 窗口保留，并明确显示 `SUCCESS` 或 `ERROR`

## 怎么确认烧录成功

重启后，铜豆应该会开启测试热点：

```text
TongDou-BoardTest
```

手机或电脑连接这个热点，然后打开：

```text
http://192.168.4.1/motor
```

能看到铜豆板级测试网页，就说明硬件测试固件已经运行。

## 自动烧录失败时怎么办

按下面步骤检查，然后再次运行 `FLASH_TONGDOU.bat`：

1. 换一根 USB 数据线，很多线只能充电，不能传数据。
2. 关闭 PlatformIO、串口监视器、串口助手等占用串口的软件。
3. 打开 Windows 设备管理器，确认能看到 COM 口。
4. 必要时让 ESP32-S3 进入下载模式：
   - 按住 BOOT。
   - 点一下 RESET。
   - 松开 BOOT。
5. 换一个 USB 口再试。

## ZIP 里有什么

```text
TongDou_V9_Hardware_Test_v1.0.bin
FLASH_TONGDOU.bat
Flash_Tool/esptool.exe
Flash_Tool/LICENSE
Flash_Tool/SOURCE_AND_LICENSE.txt
QUICK_FLASH_GUIDE_EN.md
QUICK_FLASH_GUIDE_CN.md
README.md
```

烧录工具使用乐鑫官方 esptool Windows 独立版本。授权和源码信息保留在
`Flash_Tool/` 目录里。
