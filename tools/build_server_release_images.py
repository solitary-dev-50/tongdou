"""Build the three versioned TongDou images on a publisher's machine only."""
import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
VOICEPRINT_DOCKERFILE = ROOT / 'tools/release_templates/server_runtime/Dockerfile-voiceprint-release'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True, help='Reviewed TongDou Server source tree or source package')
    parser.add_argument('--version', required=True, help='Fixed release tag, for example v0.1.3')
    parser.add_argument('--namespace', default='tongdou', help='Existing image namespace; must be writable by publisher')
    parser.add_argument('--push', action='store_true', help='Push all three images after all builds succeed')
    parser.add_argument('--vad-model', type=Path, help='Reviewed silero_vad.onnx file when the source package omits models')
    parser.add_argument('--asr-model-dir', type=Path, help='Reviewed SenseVoiceSmall directory when the source package omits models')
    args = parser.parse_args()
    if not re.fullmatch(r'v?[0-9]+\.[0-9]+\.[0-9]+(?:[-.][A-Za-z0-9.-]+)?', args.version):
        parser.error('Version must be a fixed release tag; latest is not allowed.')
    if not re.fullmatch(r'[a-z0-9][a-z0-9._/-]*', args.namespace):
        parser.error('Namespace must be a Docker image repository prefix.')
    source = args.source.resolve()
    required = [source / 'Dockerfile-server', source / 'Dockerfile-web',
                source / 'main/xiaozhi-server/docker/voiceprint/start_voiceprint.py']
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        parser.error('Missing build inputs: ' + ', '.join(missing))
    model_relative = Path('main/xiaozhi-server/models/snakers4_silero-vad/src/silero_vad/data/silero_vad.onnx')
    model = args.vad_model.resolve() if args.vad_model else source / model_relative
    if not model.is_file() or model.stat().st_size < 1024:
        parser.error('Missing Silero VAD model; supply the reviewed binary with --vad-model.')
    asr_relative = Path('main/xiaozhi-server/models/SenseVoiceSmall')
    asr_model = args.asr_model_dir.resolve() if args.asr_model_dir else source / asr_relative
    for filename in ('model.pt', 'config.yaml', 'configuration.json', 'chn_jpn_yue_eng_ko_spectok.bpe.model', 'am.mvn'):
        minimum_size = 1024 if filename == 'model.pt' else 2
        if not (asr_model / filename).is_file() or (asr_model / filename).stat().st_size < minimum_size:
            parser.error('Missing SenseVoiceSmall input ' + filename + '; supply --asr-model-dir.')
    images = [('server', source / 'Dockerfile-server'),
              ('manager', source / 'Dockerfile-web'),
              ('voiceprint', VOICEPRINT_DOCKERFILE)]
    for name, dockerfile in images:
        tag = f'{args.namespace}/{name}:{args.version}'
        print('Building ' + tag, flush=True)
        if name == 'server' and (args.vad_model or args.asr_model_dir):
            # Add the required models to a temporary context; never alter frozen inputs.
            with tempfile.TemporaryDirectory(prefix='tongdou-server-build-') as temporary:
                context = Path(temporary) / 'source'
                shutil.copytree(source, context)
                destination = context / model_relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(model, destination)
                shutil.copytree(asr_model, context / asr_relative, dirs_exist_ok=True)
                subprocess.run(['docker', 'build', '-f', str(context / dockerfile.name), '-t', tag, str(context)], check=True)
        else:
            subprocess.run(['docker', 'build', '-f', str(dockerfile), '-t', tag, str(source)], check=True)
    if args.push:
        for name, _ in images:
            tag = f'{args.namespace}/{name}:{args.version}'
            print('Pushing ' + tag, flush=True)
            subprocess.run(['docker', 'push', tag], check=True)
    else:
        print('Images built locally. Pass --push after registry login to upload them.')


if __name__ == '__main__':
    main()
