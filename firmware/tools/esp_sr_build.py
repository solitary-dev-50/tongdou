"""Prepare the pinned ESP-SR wake model before PlatformIO configures upload.

The official component is downloaded into the ignored .pio cache, verified,
and reduced to the selected Chinese MultiNet model image.
"""

Import("env")

import hashlib
import importlib.util
import os
import shutil
import urllib.request
import zipfile


ESP_SR_VERSION = "1.4.2"
ESP_SR_URL = (
    "https://components.espressif.com/api/downloads/"
    "?object_id=e32acaf6-fb5e-43fa-a3a9-db278f81cae9&object_type=component"
)
ESP_SR_SHA256 = "754dbc6275435f316e805e19b12b65ed71e0f9688ca2e68929d6827dab63fa0f"
MODEL_NAME = "mn4q8_cn"
MODEL_FLASH_OFFSET = "0x10000"


project_dir = env.subst("$PROJECT_DIR")
build_dir = env.subst("$BUILD_DIR")
cache_root = os.path.join(project_dir, ".pio", "esp-sr", ESP_SR_VERSION)
archive_path = os.path.join(cache_root, "esp-sr.zip")
component_dir = os.path.join(cache_root, "esp-sr")


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def download_component():
    os.makedirs(cache_root, exist_ok=True)
    if os.path.isfile(archive_path) and sha256(archive_path) == ESP_SR_SHA256:
        return

    temporary_path = archive_path + ".download"
    last_error = None
    for attempt in range(1, 4):
        if os.path.exists(temporary_path):
            os.remove(temporary_path)
        print(
            "Downloading pinned ESP-SR %s (attempt %d/3)..."
            % (ESP_SR_VERSION, attempt)
        )
        try:
            request = urllib.request.Request(
                ESP_SR_URL, headers={"User-Agent": "TongDou-PlatformIO-Build"}
            )
            with urllib.request.urlopen(request, timeout=90) as response:
                with open(temporary_path, "wb") as destination:
                    shutil.copyfileobj(response, destination, 1024 * 1024)
            actual_hash = sha256(temporary_path)
            if actual_hash != ESP_SR_SHA256:
                raise RuntimeError(
                    "ESP-SR archive checksum mismatch: expected %s, got %s"
                    % (ESP_SR_SHA256, actual_hash)
                )
            os.replace(temporary_path, archive_path)
            return
        except Exception as error:
            last_error = error
    if os.path.exists(temporary_path):
        os.remove(temporary_path)
    raise RuntimeError("ESP-SR download failed after 3 attempts: %s" % last_error)


def extract_component():
    marker = os.path.join(component_dir, "idf_component.yml")
    if os.path.isfile(marker):
        return

    if os.path.isdir(component_dir):
        shutil.rmtree(component_dir)
    with zipfile.ZipFile(archive_path) as archive:
        cache_path = os.path.abspath(cache_root)
        for member in archive.infolist():
            target = os.path.abspath(os.path.join(cache_root, member.filename))
            if os.path.commonpath([cache_path, target]) != cache_path:
                raise RuntimeError("ESP-SR archive contains an unsafe path")
        archive.extractall(cache_root)


def load_pack_function():
    script_path = os.path.join(component_dir, "model", "pack_model.py")
    spec = importlib.util.spec_from_file_location("tongdou_esp_sr_pack", script_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.pack_models


def build_model_image():
    model_input_dir = os.path.join(build_dir, "srmodels-input")
    model_source_dir = os.path.join(
        component_dir, "model", "multinet_model", MODEL_NAME
    )
    model_image = os.path.join(build_dir, "srmodels.bin")

    if os.path.isdir(model_input_dir):
        shutil.rmtree(model_input_dir)
    os.makedirs(model_input_dir)
    shutil.copytree(model_source_dir, os.path.join(model_input_dir, MODEL_NAME))
    load_pack_function()(model_input_dir, model_image)

    image_size = os.path.getsize(model_image)
    partition_size = 0xE0000
    if image_size > partition_size:
        raise RuntimeError(
            "ESP-SR model image is %d bytes, larger than the %d-byte model partition"
            % (image_size, partition_size)
        )
    print("Wake model image: %s (%d bytes)" % (MODEL_NAME, image_size))
    return model_image


download_component()
extract_component()
model_image = build_model_image()
env.Append(FLASH_EXTRA_IMAGES=[(MODEL_FLASH_OFFSET, model_image)])
