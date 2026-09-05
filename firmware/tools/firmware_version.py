"""Expose the single configured firmware version to the application code."""

Import("env")


version = env.GetProjectOption("custom_firmware_version").strip()
parts = version.split(".")
if not version or any(not part.isdigit() for part in parts):
    raise RuntimeError("custom_firmware_version must contain only numeric dot segments")

env.Append(
    CPPDEFINES=[
        ("TONGDOU_FIRMWARE_VERSION", env.StringifyMacro(version)),
    ]
)
print("Tong Dou firmware version: %s" % version)
