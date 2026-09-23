"""直接测试相邻服务器源码的型号选择；网络、日志和认证使用替身，不启动服务。"""

import ast
import asyncio
import glob
import importlib.util
import json
import os
from pathlib import Path
import re
import tempfile
import time
from types import SimpleNamespace
from typing import Dict, List
import unittest
from unittest.mock import Mock, AsyncMock

SERVER = Path(os.environ.get(
    "TONGDOU_TEST_SERVER_DIR",
    str(Path(__file__).resolve().parents[4] / "tongdou-server" / "main" / "xiaozhi-server"),
))


@unittest.skipUnless((SERVER / "core/api/ota_handler.py").is_file(),
                     "需要相邻 tongdou-server 源码，或设置 TONGDOU_TEST_SERVER_DIR")
class BoardRoutingTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        catalog_path = SERVER / "core/api/firmware_catalog.py"
        spec = importlib.util.spec_from_file_location("catalog_under_test", catalog_path)
        catalog = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(catalog)
        handler_path = SERVER / "core/api/ota_handler.py"
        tree = ast.parse(handler_path.read_text(encoding="utf-8"))
        # 保留真实处理类，不加载无关的语音模型等服务器依赖。
        tree.body = [node for node in tree.body
                     if isinstance(node, ast.ClassDef) and node.name == "OTAHandler"]
        namespace = dict(glob=glob, json=json, os=os, re=re, time=time,
                         Dict=Dict, List=List, FirmwareFile=catalog.FirmwareFile,
                         inspect_firmware_file=catalog.inspect_firmware_file,
                         parse_version=catalog.parse_version,
                         select_firmware=catalog.select_firmware,
                         BaseHandler=object, TAG="test",
                         get_local_ip=lambda: "127.0.0.1",
                         get_vision_url=lambda config: "http://localhost:8003/mcp/vision/explain",
                         web=SimpleNamespace(Response=SimpleNamespace))
        exec(compile(tree, str(handler_path), "exec"), namespace)
        cls.handler_type = namespace["OTAHandler"]

    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.handler = object.__new__(self.handler_type)
        self.handler.bin_dir = self.directory.name
        self.handler._bin_cache = {"updated_at": 0, "ttl": 0, "files_by_model": {}}
        self.handler.config = {"server": {"websocket": "ws://localhost:8000/xiaozhi/v1/"}}
        self.handler.logger = Mock()
        self.handler.auth_enable = False
        self.handler._add_cors_headers = Mock()

    def add_image(self, model, version):
        name = f"{model}_{version}.bin"
        (Path(self.directory.name) / name).write_bytes(name.encode())

    def offer(self, model, version, recovery=False):
        headers = {"device-id": "test-device", "client-id": "test-client"}
        if recovery:
            headers["device-recovery"] = "1"
        request = SimpleNamespace(method="POST", headers=headers, text=AsyncMock(
            return_value=json.dumps({"board": {"type": model},
                                     "application": {"version": version}})))
        response = asyncio.run(self.handler.handle_post(request))
        return json.loads(response.text)["firmware"]

    def test_upgrade_and_recovery_stay_on_each_board(self):
        models = ("tongdou-esp32s3", "tongdou-esp32s3-v9.4")
        for model in models:
            for version in ("0.1.2", "0.1.3"):
                self.add_image(model, version)
        for model in models:
            with self.subTest(model=model):
                normal = self.offer(model, "0.1.2")
                self.assertTrue(normal["url"].endswith(f"/{model}_0.1.3.bin"))
                rollback = self.offer(model, "0.1.3", recovery=True)
                self.assertTrue(rollback["url"].endswith(f"/{model}_0.1.2.bin"))
                self.assertEqual(self.offer(model, "0.1.3")["url"], "")

    def test_other_board_never_fills_missing_channel(self):
        for available, missing in (("tongdou-esp32s3", "tongdou-esp32s3-v9.4"),
                                   ("tongdou-esp32s3-v9.4", "tongdou-esp32s3")):
            for path in Path(self.directory.name).glob("*.bin"):
                path.unlink()
            self.add_image(available, "9.9.9")
            for recovery in (False, True):
                with self.subTest(missing=missing, recovery=recovery):
                    self.assertEqual(self.offer(missing, "0.1.3", recovery)["url"], "")


if __name__ == "__main__":
    unittest.main()
