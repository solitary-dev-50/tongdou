"""Link the pinned ESP-SR binary libraries into the application."""

Import("env", "projenv")

import os


ESP_SR_VERSION = "1.4.2"

project_dir = env.subst("$PROJECT_DIR")
build_dir = env.subst("$BUILD_DIR")
component_dir = os.path.join(
    project_dir, ".pio", "esp-sr", ESP_SR_VERSION, "esp-sr"
)
include_paths = [
    os.path.join(component_dir, "src", "include"),
    os.path.join(component_dir, "include", "esp32s3"),
]
env.Append(CPPPATH=include_paths)
projenv.Append(CPPPATH=include_paths)

esp_sr_env = projenv.Clone()
esp_sr_env.Append(CPPPATH=include_paths)
esp_sr_glue = esp_sr_env.BuildLibrary(
    os.path.join(build_dir, "esp-sr-glue"),
    os.path.join(component_dir, "src"),
    "+<model_path.c> +<esp_mn_speech_commands.c>",
)

library_dir = os.path.join(component_dir, "lib", "esp32s3")
env.Prepend(
    LIBS=[
        esp_sr_glue,
        env.File(os.path.join(library_dir, "libhufzip.a")),
        env.File(os.path.join(library_dir, "libdl_lib.a")),
        env.File(os.path.join(library_dir, "libfst.a")),
        env.File(os.path.join(library_dir, "libc_speech_features.a")),
        env.File(os.path.join(library_dir, "libesp_audio_front_end.a")),
        env.File(os.path.join(library_dir, "libesp_audio_processor.a")),
        env.File(os.path.join(library_dir, "libmultinet.a")),
        env.File(os.path.join(library_dir, "libwakenet.a")),
    ]
)
