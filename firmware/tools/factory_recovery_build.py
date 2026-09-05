"""Build the fixed recovery image and add it to factory flashing."""

Import("env")

import os
import subprocess


RECOVERY_FLASH_OFFSET = "0x100000"
RECOVERY_PARTITION_SIZE = 0xD0000

project_dir = env.subst("$PROJECT_DIR")
python_executable = env.subst("$PYTHONEXE")
recovery_environment = "esp32s3_recovery"
recovery_image = os.path.join(
    project_dir, ".pio", "build", recovery_environment, "firmware.bin"
)

subprocess.run(
    [
        python_executable,
        "-m",
        "platformio",
        "run",
        "--environment",
        recovery_environment,
    ],
    cwd=project_dir,
    check=True,
)

recovery_size = os.path.getsize(recovery_image)
if recovery_size > RECOVERY_PARTITION_SIZE:
    raise RuntimeError(
        "Recovery image is %d bytes, larger than the %d-byte recovery partition"
        % (recovery_size, RECOVERY_PARTITION_SIZE)
    )

print("Recovery image: %s (%d bytes)" % (recovery_image, recovery_size))
env.Append(FLASH_EXTRA_IMAGES=[(RECOVERY_FLASH_OFFSET, recovery_image)])
