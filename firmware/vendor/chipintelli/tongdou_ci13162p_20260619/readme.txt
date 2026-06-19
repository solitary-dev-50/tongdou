1. Command Word Information Table Instructions
    In the CmdWordStructure directory, you will find two spreadsheet files: [60000]{cmd_info}.xlsx and [60000]{cmd_info_old}.xlsx.
    The appropriate table to use depends on the version of the SDK tool .\tools\ci-tool-kit.exe.
    How to Check the Tool Version:
    Open a command terminal and run the following commands:
        cd .\tools
        .\ci-tool-kit.exe -v
    Version-Based Selection Rule:
    If the reported version is 1.25 or higher, use [60000]{cmd_info}.xlsx.
    If the version is lower than 1.25, use [60000]{cmd_info_old}.xlsx.
    Note: Starting from SDK version 2.7.X, the default and recommended command word table is [60000]{cmd_info}.xlsx.

2. File Usage Instructions
    copy flie "CmdWordStructure\[60000]*.xlsx" to directory ".\projects\*_sample\firmware\user_file\cmd_info"
    copy flie "GfstCmd\[0]*.dat" and "GfstWake\[1]*.dat" to directory ".\projects\*_sample\firmware\asr"

1、命令词信息表说明
    在"CmdWordStructure"目录下，存在[60000]{cmd_info}.xlsx和[60000]{cmd_info_old}.xlsx两个表格，
    可根据SDK中 .\tools\ci-tool-kit.exe 的版本选择使用哪个表格
    .\tools\ci-tool-kit.exe 的版本查询方法如下：
    在命令词行中执行如下命令词：
        cd .\tools
        .\ci-tool-kit.exe -v
    如果查询的版本等于或大于1.25，则使用表格[60000]{cmd_info}.xlsx，否则使用表格[60000]{cmd_info_old}.xlsx
    注： 从SDK 2.7.X开始，默认使用[60000]{cmd_info}.xlsx

2、文件使用说明
    将 "CmdWordStructure\[60000]*.xlsx" 文件拷贝到SDK的 ".\projects\*_sample\firmware\user_file\cmd_info" 目录下
    将 "GfstCmd\[0]*.dat" 和 "GfstWake\[1]*.dat" 文件拷贝到SDK的 ".\projects\*_sample\firmware\asr" 目录下
