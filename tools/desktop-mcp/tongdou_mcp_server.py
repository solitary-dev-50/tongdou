#!/usr/bin/env python3
"""Tiny desktop MCP server for Tong Dou.

This server exposes a small tool set to a desktop LLM client and forwards the
actual device work to Tong Dou's local HTTP API.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import dataclass
from typing import Any, Callable


PROTOCOL_VERSION = "2024-11-05"
SERVER_NAME = "tongdou-desktop"
SERVER_VERSION = "0.1.0"
DEFAULT_BASE_URL = "http://192.168.4.1"
DEFAULT_TIMEOUT_SECONDS = 5


@dataclass
class ToolResult:
    text: str
    is_error: bool = False


class TongDouClient:
    def __init__(self, base_url: str, timeout_seconds: int = DEFAULT_TIMEOUT_SECONDS) -> None:
        self.base_url = base_url.rstrip("/")
        self.timeout_seconds = timeout_seconds

    def get_status(self) -> str:
        return self._request("GET", "/api/status")

    def list_reminders(self) -> str:
        return self._request("GET", "/api/reminders")

    def create_reminder(self, due_at: int, text: str) -> str:
        body = urllib.parse.urlencode({"dueAt": due_at, "text": text}).encode("utf-8")
        return self._request("POST", "/api/reminders", body)

    def delete_reminder(self, reminder_id: int) -> str:
        body = urllib.parse.urlencode({"id": reminder_id}).encode("utf-8")
        return self._request("POST", "/api/reminders/delete", body)

    def _request(self, method: str, path: str, body: bytes | None = None) -> str:
        headers = {}
        if body is not None:
            headers["Content-Type"] = "application/x-www-form-urlencoded"

        request = urllib.request.Request(
            self.base_url + path,
            data=body,
            headers=headers,
            method=method,
        )
        try:
            with urllib.request.urlopen(request, timeout=self.timeout_seconds) as response:
                return response.read().decode("utf-8", errors="replace")
        except urllib.error.HTTPError as error:
            detail = error.read().decode("utf-8", errors="replace")
            raise RuntimeError(f"Tong Dou HTTP {error.code}: {detail}") from error
        except urllib.error.URLError as error:
            raise RuntimeError(f"Cannot reach Tong Dou at {self.base_url}: {error}") from error


class ToolRegistry:
    def __init__(self, client: TongDouClient) -> None:
        self.client = client
        self.tools: dict[str, tuple[dict[str, Any], Callable[[dict[str, Any]], ToolResult]]] = {}
        self._register_tools()

    def list_tools(self) -> list[dict[str, Any]]:
        return [entry[0] for entry in self.tools.values()]

    def call(self, name: str, arguments: dict[str, Any]) -> ToolResult:
        entry = self.tools.get(name)
        if entry is None:
            return ToolResult(f"Unknown tool: {name}", True)
        try:
            return entry[1](arguments)
        except Exception as error:  # Keep MCP server alive even when the device is offline.
            return ToolResult(str(error), True)

    def _add_tool(
        self,
        name: str,
        description: str,
        properties: dict[str, dict[str, Any]],
        required: list[str],
        callback: Callable[[dict[str, Any]], ToolResult],
    ) -> None:
        self.tools[name] = (
            {
                "name": name,
                "description": description,
                "inputSchema": {
                    "type": "object",
                    "properties": properties,
                    "required": required,
                },
            },
            callback,
        )

    def _register_tools(self) -> None:
        self._add_tool(
            "get_status",
            "Get Tong Dou network, time and device status.",
            {},
            [],
            lambda _: ToolResult(self.client.get_status()),
        )
        self._add_tool(
            "list_reminders",
            "List reminders saved on Tong Dou.",
            {},
            [],
            lambda _: ToolResult(self.client.list_reminders()),
        )
        self._add_tool(
            "create_reminder",
            "Create a reminder on Tong Dou. dueAt is Unix time in seconds.",
            {
                "dueAt": {"type": "integer"},
                "text": {"type": "string"},
            },
            ["dueAt", "text"],
            self._create_reminder,
        )
        self._add_tool(
            "delete_reminder",
            "Delete a reminder from Tong Dou by id.",
            {
                "id": {"type": "integer"},
            },
            ["id"],
            self._delete_reminder,
        )

    def _create_reminder(self, arguments: dict[str, Any]) -> ToolResult:
        due_at = int(arguments.get("dueAt", 0))
        text = str(arguments.get("text", "")).strip()
        if due_at <= 0 or not text:
            return ToolResult("dueAt and text are required", True)
        return ToolResult(self.client.create_reminder(due_at, text))

    def _delete_reminder(self, arguments: dict[str, Any]) -> ToolResult:
        reminder_id = int(arguments.get("id", 0))
        if reminder_id <= 0:
            return ToolResult("id is required", True)
        return ToolResult(self.client.delete_reminder(reminder_id))


class McpServer:
    def __init__(self, registry: ToolRegistry) -> None:
        self.registry = registry

    def handle(self, message: dict[str, Any]) -> dict[str, Any] | None:
        method = message.get("method")
        request_id = message.get("id")
        if method is None or str(method).startswith("notifications/"):
            return None

        try:
            if message.get("jsonrpc") != "2.0":
                return self._error(request_id, "Invalid JSON-RPC version")
            if method == "initialize":
                return self._result(
                    request_id,
                    {
                        "protocolVersion": PROTOCOL_VERSION,
                        "capabilities": {"tools": {}},
                        "serverInfo": {"name": SERVER_NAME, "version": SERVER_VERSION},
                    },
                )
            if method == "tools/list":
                return self._result(request_id, {"tools": self.registry.list_tools()})
            if method == "tools/call":
                params = message.get("params") or {}
                name = str(params.get("name", ""))
                arguments = params.get("arguments") or {}
                result = self.registry.call(name, arguments)
                return self._result(
                    request_id,
                    {
                        "content": [{"type": "text", "text": result.text}],
                        "isError": result.is_error,
                    },
                )
            return self._error(request_id, f"Method not implemented: {method}")
        except Exception as error:
            return self._error(request_id, str(error))

    def _result(self, request_id: Any, result: dict[str, Any]) -> dict[str, Any]:
        return {"jsonrpc": "2.0", "id": request_id, "result": result}

    def _error(self, request_id: Any, message: str) -> dict[str, Any]:
        return {"jsonrpc": "2.0", "id": request_id, "error": {"message": message}}


def read_message() -> dict[str, Any] | None:
    headers: dict[str, str] = {}
    while True:
        line = sys.stdin.buffer.readline()
        if line == b"":
            return None
        line = line.strip()
        if not line:
            break
        name, _, value = line.decode("ascii", errors="replace").partition(":")
        headers[name.lower()] = value.strip()

    length = int(headers.get("content-length", "0"))
    if length <= 0:
        return None
    payload = sys.stdin.buffer.read(length)
    return json.loads(payload.decode("utf-8"))


def write_message(message: dict[str, Any]) -> None:
    payload = json.dumps(message, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    sys.stdout.buffer.write(f"Content-Length: {len(payload)}\r\n\r\n".encode("ascii"))
    sys.stdout.buffer.write(payload)
    sys.stdout.buffer.flush()


def run_stdio(server: McpServer) -> None:
    while True:
        message = read_message()
        if message is None:
            return
        response = server.handle(message)
        if response is not None:
            write_message(response)


def run_self_test(server: McpServer) -> int:
    initialize = server.handle({"jsonrpc": "2.0", "id": 1, "method": "initialize"})
    tools = server.handle({"jsonrpc": "2.0", "id": 2, "method": "tools/list"})
    if not initialize or initialize.get("result", {}).get("serverInfo", {}).get("name") != SERVER_NAME:
        print("self-test failed: initialize")
        return 1
    tool_names = [tool["name"] for tool in tools["result"]["tools"]]
    expected = {"create_reminder", "list_reminders", "delete_reminder", "get_status"}
    if set(tool_names) != expected:
        print(f"self-test failed: tools={tool_names}")
        return 1
    print("OK: desktop MCP server self-test passed")
    return 0


def build_server() -> McpServer:
    base_url = os.environ.get("TONGDOU_BASE_URL", DEFAULT_BASE_URL)
    timeout = int(os.environ.get("TONGDOU_TIMEOUT_SECONDS", str(DEFAULT_TIMEOUT_SECONDS)))
    return McpServer(ToolRegistry(TongDouClient(base_url, timeout)))


def main() -> int:
    parser = argparse.ArgumentParser(description="Tong Dou desktop MCP server")
    parser.add_argument("--self-test", action="store_true", help="run protocol self-test and exit")
    args = parser.parse_args()

    server = build_server()
    if args.self_test:
        return run_self_test(server)
    run_stdio(server)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
