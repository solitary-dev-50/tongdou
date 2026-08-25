"""Build and flash one default locale pack only for factory images."""

Import("env")

import os
import subprocess
import sys


LOCALE_FLASH_OFFSET = "0x3D0000"
LOCALE_PARTITION_SIZE = 0x1C000

project_dir = env.subst("$PROJECT_DIR")
build_dir = env.subst("$BUILD_DIR")
factory_locale = env.GetProjectOption("custom_factory_locale", "en-GB").strip()
resource_version = env.GetProjectOption(
    "custom_locale_resource_version", "2"
).strip()
pack_builder = os.path.join(project_dir, "tools", "build_locale_pack.py")
pack_image = os.path.join(build_dir, "locale_%s.bin" % factory_locale)

subprocess.run(
    [
        sys.executable,
        pack_builder,
        "--locale",
        factory_locale,
        "--version",
        resource_version,
        "--output",
        pack_image,
    ],
    check=True,
)

pack_size = os.path.getsize(pack_image)
if pack_size > LOCALE_PARTITION_SIZE:
    raise RuntimeError(
        "Factory locale pack is %d bytes, larger than the %d-byte locale partition"
        % (pack_size, LOCALE_PARTITION_SIZE)
    )

print(
    "Factory locale image: %s version=%s (%d bytes)"
    % (factory_locale, resource_version, pack_size)
)
env.Append(FLASH_EXTRA_IMAGES=[(LOCALE_FLASH_OFFSET, pack_image)])
