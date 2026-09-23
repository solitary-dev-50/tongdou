"""验证发布入口不会把错主板、旧版本或无标识文件发到升级通道。"""

import importlib.util
from pathlib import Path
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "publish_ota_image.py"
spec = importlib.util.spec_from_file_location("publish_ota_image", SCRIPT)
publisher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(publisher)


class PublishIdentityTest(unittest.TestCase):
    def check_image(self, payload, model, version="0.1.3"):
        with tempfile.TemporaryDirectory() as directory:
            image = Path(directory) / "firmware.bin"
            image.write_bytes(payload)
            publisher.validate_image_identity(image, model, version)

    def test_matching_boards(self):
        for model in publisher.MODELS.values():
            with self.subTest(model=model):
                self.check_image(f"tongdou-image:{model}:0.1.3;\0".encode(), model)

    def test_cross_board_rejected_in_both_directions(self):
        models = list(publisher.MODELS.values())
        for actual, selected in (models, models[::-1]):
            with self.subTest(actual=actual), self.assertRaises(SystemExit):
                self.check_image(f"tongdou-image:{actual}:0.1.3;\0".encode(), selected)

    def test_stale_version_rejected(self):
        with self.assertRaises(SystemExit):
            self.check_image(b"tongdou-image:tongdou-esp32s3:0.1.2;\0", "tongdou-esp32s3")

    def test_missing_or_conflicting_identity_rejected(self):
        for payload in (b"old unmarked image", b"tongdou-image:tongdou-esp32s3:0.1.3;\0"
                        b"tongdou-image:tongdou-esp32s3-v9.4:0.1.3;\0"):
            with self.subTest(payload=payload), self.assertRaises(SystemExit):
                self.check_image(payload, "tongdou-esp32s3")


if __name__ == "__main__":
    unittest.main()
