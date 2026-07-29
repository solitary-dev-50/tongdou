from pathlib import Path
import subprocess
import sys

Import("env")


def _strip_quotes(value):
    return str(value).strip().strip('"')


def _run(command, cwd):
    print("\n>>> " + " ".join(str(part) for part in command))
    subprocess.check_call([str(part) for part in command], cwd=str(cwd))


def _chunked_upload(source, target, env):
    project_dir = Path(env.subst("$PROJECT_DIR"))
    build_dir = Path(env.subst("$BUILD_DIR"))
    python = Path(_strip_quotes(env.subst("$PYTHONEXE")))
    uploader = Path(_strip_quotes(env.subst("$UPLOADER")))
    upload_port = _strip_quotes(env.subst("$UPLOAD_PORT"))
    upload_speed = _strip_quotes(env.subst("$UPLOAD_SPEED")) or "921600"
    framework_dir = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32"))
    boot_app0 = framework_dir / "tools" / "partitions" / "boot_app0.bin"

    if not upload_port:
        upload_port = env.AutodetectUploadPort()
    if not upload_port:
        raise RuntimeError("No upload port found. Use --upload-port COMx.")

    bootloader = build_dir / "bootloader.bin"
    partitions = build_dir / "partitions.bin"
    firmware = build_dir / "firmware.bin"

    for artifact in (bootloader, partitions, boot_app0, firmware):
        if not artifact.exists():
            raise RuntimeError(f"Missing upload artifact: {artifact}")

    base = [
        python,
        uploader,
        "--chip",
        "esp32s3",
        "--port",
        upload_port,
        "--baud",
        upload_speed,
        "--before",
        "default_reset",
        "--after",
        "hard_reset",
    ]

    common_flash_args = [
        "--no-compress",
        "--flash_mode",
        "dio",
        "--flash_freq",
        "80m",
        "--flash_size",
        "4MB",
    ]

    _run(
        base
        + [
            "write_flash",
            *common_flash_args,
            "0x0000",
            bootloader,
            "0x8000",
            partitions,
            "0xe000",
            boot_app0,
        ],
        project_dir,
    )

    chunk_dir = build_dir / "upload_chunks"
    chunk_dir.mkdir(parents=True, exist_ok=True)
    data = firmware.read_bytes()
    chunk_size = 64 * 1024
    for offset in range(0, len(data), chunk_size):
        chunk_path = chunk_dir / f"app_{offset:06x}.bin"
        chunk_path.write_bytes(data[offset : offset + chunk_size])
        _run(
            base
            + [
                "write_flash",
                *common_flash_args,
                hex(0x10000 + offset),
                chunk_path,
            ],
            project_dir,
        )

    print("\nchunked upload complete")


env.AddCustomTarget(
    "upload_chunked",
    dependencies=[
        "$BUILD_DIR/bootloader.bin",
        "$BUILD_DIR/partitions.bin",
        "$BUILD_DIR/firmware.bin",
    ],
    actions=[_chunked_upload],
    title="Upload Chunked",
    description="Upload the board-test firmware in short chunks for unstable native USB flashing.",
)
