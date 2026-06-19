# Tong Dou Desktop MCP Server

This is the tiny desktop-side MCP server for Tong Dou.

It exposes four tools:

- `create_reminder`
- `list_reminders`
- `delete_reminder`
- `get_status`

Set the Tong Dou device address before starting:

```powershell
$env:TONGDOU_BASE_URL = "http://192.168.4.1"
python tools/desktop-mcp/tongdou_mcp_server.py
```

Run the local protocol self-test:

```powershell
python tools/desktop-mcp/tongdou_mcp_server.py --self-test
```
