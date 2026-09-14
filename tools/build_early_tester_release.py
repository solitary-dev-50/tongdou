"""沿用 v0.1.0 结构生成早期测试交付目录，只收集构建输入和发布文件。"""
from pathlib import Path
import argparse, configparser, hashlib, json, re, shutil, subprocess, sys, zipfile
import yaml
from fix_server_linux_startup import install as install_linux_startup

ROOT=Path(__file__).resolve().parents[1]
SERVER=ROOT.parent/'tongdou-server'
OUT=ROOT/'release_artifacts'
GUIDES=['TongDou_v0.1_Early_Tester_Features_and_Usage_EN.md','TongDou_v0.1_Early_Tester_Quick_Start_EN.md']

def copy(src,dst):
 dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
def write(path,text):
 path.parent.mkdir(parents=True,exist_ok=True)
 path.write_text(text,encoding='utf-8',newline='\r\n' if path.suffix=='.bat' else '\n')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def replace(s,old,new):
 assert old in s,old[:100];return s.replace(old,new)

def source_files():
 raw=subprocess.check_output(['git','ls-files','-z','--cached','--others','--exclude-standard'],cwd=SERVER)
 for name in sorted(set(raw.decode('utf-8').split('\0'))):
  if not name or not (name.startswith(('main/manager-api/','main/manager-web/','main/xiaozhi-server/','docs/docker/'))):continue
  p=Path(name)
  if any(x in p.parts for x in ('data','logs','tmp','node_modules','target','dist','__pycache__','.git','models')):continue
  if p.name in ('.env','.config.yaml','.mcp_server_settings.json') or p.suffix in ('.db','.sqlite','.pyc','.log'):continue
  if (SERVER/p).is_file():yield p

def stage(v):
 fw=OUT/f'TongDou_Application_Firmware_v{v}_Early_Tester'
 srv=OUT/f'TongDou_Server_v{v}_Early_Tester'
 if fw.exists() or srv.exists():raise SystemExit('Destination already exists; review it instead of overwriting a release.')
 baseline=OUT/'TongDou_Application_Firmware_v0.1.0_Early_Tester'
 fw.mkdir();srv.mkdir()
 shutil.copytree(baseline/'Flash_Tool',fw/'Flash_Tool')
 batch=(baseline/'FLASH_TONGDOU.bat').read_text(encoding='utf-8').replace('0.1.0',v)
 batch=batch.replace('hold BOOT, tap RESET, then release BOOT','use the power switch; this board has no RESET button')
 batch=batch.replace('Hold BOOT, tap RESET, then release BOOT.','Use the power switch. See QUICK_FLASH_GUIDE_EN.md for BOOT recovery.')
 batch=batch.replace('reminders, and other settings','reminders, motor calibration, and other settings')
 write(fw/'FLASH_TONGDOU.bat',batch)
 build=ROOT/'firmware/.pio/build'
 inputs=[(0x0,build/'esp32s3_factory/bootloader.bin','bootloader.bin'),
 (0x8000,build/'esp32s3_factory/partitions.bin','partitions.bin'),
 (0xe000,Path.home()/'.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin','boot_app0.bin'),
 (0x10000,build/'esp32s3_factory/srmodels.bin','srmodels.bin'),
 (0x100000,build/'esp32s3_recovery/firmware.bin','recovery.bin'),
 (0x1d0000,build/'esp32s3_factory/firmware.bin','application.bin'),
 (0x3d0000,build/'esp32s3_factory/locale_en-GB.bin','locale_en-GB.bin')]
 for _,src,name in inputs:copy(src,fw/'Images'/name)
 cmd=[str(fw/'Flash_Tool/esptool.exe'),'--chip','esp32s3','merge-bin','--flash-mode','dio','--flash-freq','80m','--flash-size','4MB','--pad-to-size','4MB','-o',str(fw/f'TongDou_Application_Firmware_v{v}.bin')]
 for offset,_,name in inputs:cmd.extend([hex(offset),str(fw/'Images'/name)])
 subprocess.run(cmd,check=True)
 # USB and remote packages use the exact same application, not separate build fingerprints.
 copy(fw/'Images/application.bin',fw/'OTA'/f'tongdou-esp32s3_{v}.bin')
 layout='TongDou v'+v+' - 4 MB complete USB image at 0x0\n\n'
 layout+='\n'.join(f'{offset:#08x}  Images/{name}' for offset,_,name in inputs)
 layout+='\n\nPartition table:\n'+(ROOT/'firmware/partitions/tongdou_n4r2_wake.csv').read_text()
 layout+='\nOTA/ contains application only; never flash that file at 0x0.\n'
 write(fw/'FLASH_LAYOUT.txt',layout)
 for name in GUIDES:copy(ROOT/name,fw/name)
 copy(ROOT/'docs/Early_Tester_Guide.md',fw/'Early_Tester_Guide.md')
 copy(ROOT/'docs/v0.1.2测试版交付说明.md',fw/'RELEASE_NOTES_CN.md')
 for name in ('README.md','QUICK_FLASH_GUIDE_CN.md','QUICK_FLASH_GUIDE_EN.md'):
  copy(ROOT/'tools/release_templates/firmware'/name,fw/name)
 source_manifest={str(p.relative_to(ROOT/'firmware')).replace('\\','/'):sha(p) for p in (ROOT/'firmware').rglob('*') if p.is_file() and (p.relative_to(ROOT/'firmware').parts[0] in ('src','include','tools','partitions') or p.name=='platformio.ini')}
 write(fw/'BUILD_INFO.json',json.dumps({'version':v,'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT).decode().strip(),'working_tree_included':True,'firmware_sources':source_manifest},ensure_ascii=False,indent=2)+'\n')
 old=OUT/'TongDou_Server_v0.1.0_Early_Tester'
 for name in ('.env.example','.gitignore','.dockerignore','START_TONGDOU_SERVER.bat','STOP_TONGDOU_SERVER.bat','STATUS_TONGDOU_SERVER.bat','Dockerfile-web'):
  write(srv/name,(old/name).read_text(encoding='utf-8').replace('v0.1.0','v'+v))
 for name in ('status_tongdou_server.ps1',):copy(old/'scripts'/name,srv/'scripts'/name)
 for name in ('mcp_server_settings.example.json','.config.yaml.example'):copy(old/'data'/name,srv/'data'/name)
 sources={}
 for rel in source_files():
  copy(SERVER/rel,srv/rel);sources[rel.as_posix()]=sha(SERVER/rel)
 # The upstream template carries a shared weather key; it is not a tester credential.
 config_path=srv/'main/xiaozhi-server/config.yaml'
 config_text=config_path.read_text(encoding='utf-8')
 weather=yaml.safe_load(config_text)['plugins']['get_weather']
 for key in ('api_key','api_host'):
  weather_value=weather.get(key)
  if weather_value:config_text=config_text.replace(str(weather_value),'')
 write(config_path,config_text)
 copy(SERVER/'Dockerfile-server',srv/'Dockerfile-server')
 copy(SERVER/'LICENSE',srv/'LICENSE')
 # Current build source and runtime dependencies; never copy the running service's data.
 for name in ['powermem-worker-integration.md','powermem-integration.md','powermem-replay-adapter.md','voiceprint-integration.md']:
  copy(SERVER/'docs'/name,srv/'docs'/name)
 for name in GUIDES:copy(ROOT/name,srv/name)
 copy(ROOT/'docs/Early_Tester_Guide.md',srv/'docs/Early_Tester_Guide.md')
 copy(ROOT/'docs/v0.1.2测试版交付说明.md',srv/'RELEASE_NOTES_CN.md')
 copy(ROOT/'tools/release_templates/server/README.md',srv/'README.md')
 write(srv/'SOURCE_MANIFEST.json',json.dumps({'version':v,'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=SERVER).decode().strip(),'working_tree_included':True,'files':sources,'package_overrides':{'main/xiaozhi-server/config.yaml':'Removed upstream shared weather key and host; configure locally.'}},ensure_ascii=False,indent=2)+'\n')
 env=(srv/'.env.example').read_text();env+='\nVOICEPRINT_API_TOKEN=GENERATED_ON_FIRST_START\nTONGDOU_VOICEPRINT_PORT=8105\n'
 write(srv/'.env.example',env)
 compose=yaml.safe_load((old/'docker-compose.yml').read_text())
 services=compose['services']
 services['tongdou-voice']['image']='tongdou/server:v'+v
 services['tongdou-manager']['image']='tongdou/manager:v'+v
 current=yaml.safe_load((SERVER/'main/xiaozhi-server/docker-compose_all.yml').read_text())
 vp=current['services']['voiceprint-api']
 vp.pop('container_name',None);vp.pop('networks',None)
 vp['image']='tongdou/voiceprint:v'+v;vp['build']['context']='.'
 vp['depends_on']={'tongdou-db':{'condition':'service_healthy'}}
 vp['ports']=['127.0.0.1:${TONGDOU_VOICEPRINT_PORT:-8105}:8005']
 vp['environment']={'TZ':'${TZ:-Asia/Shanghai}','VOICEPRINT_API_TOKEN':'${VOICEPRINT_API_TOKEN}','VOICEPRINT_MYSQL_HOST':'tongdou-db','VOICEPRINT_MYSQL_PASSWORD':'${MYSQL_ROOT_PASSWORD}'}
 vp['volumes']=['voiceprint-data:/app/data','voiceprint-model-cache:/root/.cache/modelscope','./main/xiaozhi-server/docker/voiceprint/start_voiceprint.py:/opt/tongdou/start_voiceprint.py:ro']
 services['voiceprint-api']=vp
 services['tongdou-voice']['depends_on']['voiceprint-api']={'condition':'service_healthy'}
 services['tongdou-manager']['environment']['VOICEPRINT_API_ENDPOINT']='http://voiceprint-api:8005/voiceprint/health?key=${VOICEPRINT_API_TOKEN}'
 services['tongdou-db']['environment']['MYSQL_ROOT_HOST']='%'
 services['tongdou-db']['volumes'].append('./main/xiaozhi-server/mysql-init:/docker-entrypoint-initdb.d:ro')
 compose['volumes'].update({'voiceprint-data':{'name':'${COMPOSE_PROJECT_NAME:-tongdou-v010}-voiceprint-data'},'voiceprint-model-cache':{'name':'${COMPOSE_PROJECT_NAME:-tongdou-v010}-voiceprint-model-cache'}})
 write(srv/'docker-compose.yml',yaml.safe_dump(compose,allow_unicode=True,sort_keys=False))
 launcher=(old/'scripts/start_tongdou_server.ps1').read_text(encoding='utf-8').replace('v0.1.0','v'+v)
 launcher='param([switch]$NoBrowser)\n\n'+launcher
 key_block='''$envValues = Read-EnvFile '.env'
if (-not $envValues['VOICEPRINT_API_TOKEN'] -or $envValues['VOICEPRINT_API_TOKEN'] -eq 'GENERATED_ON_FIRST_START') {
    $voiceprintToken = New-LocalDatabasePassword
    $envText = Get-Content -Raw -LiteralPath '.env'
    if ($envText -match '(?m)^VOICEPRINT_API_TOKEN=') {
        $envText = [regex]::Replace($envText, '(?m)^VOICEPRINT_API_TOKEN=.*$', "VOICEPRINT_API_TOKEN=$voiceprintToken")
    } else {
        $envText += "`nVOICEPRINT_API_TOKEN=$voiceprintToken`n"
    }
    [System.IO.File]::WriteAllText((Join-Path $root '.env'), $envText, [System.Text.UTF8Encoding]::new($false))
    $envValues = Read-EnvFile '.env'
}
if ($envValues['VOICEPRINT_API_TOKEN'] -notmatch '^[A-Za-z0-9_-]{32,}$') {
    Stop-WithError 'VOICEPRINT_API_TOKEN must have at least 32 letters, numbers, underscores or hyphens.'
}'''
 launcher=replace(launcher,"$envValues = Read-EnvFile '.env'",key_block)
 # Existing v0.1.0 database volumes also need the separate voiceprint schema.
 before="$hasQWeatherKey = $envValues.ContainsKey('TONGDOU_QWEATHER_API_KEY')"
 launcher=replace(launcher,before,"Get-Content -Raw -LiteralPath 'main/xiaozhi-server/mysql-init/01-voiceprint.sql' | & docker compose --env-file .env -p $project exec -T -e \"MYSQL_PWD=$dbPassword\" tongdou-db mysql -uroot *> $null\nif ($LASTEXITCODE -ne 0) { Stop-WithError 'Could not initialize the voiceprint database.' }\n\n"+before)
 # On upgrade, refresh only the package-owned voiceprint endpoint; leave custom endpoints alone.
 before='$websocketUrl = "ws://${lanIp}:${wsPort}/xiaozhi/v1/"'
 extra='''$voiceprintEndpoint64 = Convert-ToBase64Utf8 ("http://voiceprint-api:8005/voiceprint/health?key=" + $envValues['VOICEPRINT_API_TOKEN'])
$voiceprintSql = "UPDATE sys_params SET param_value=CONVERT(FROM_BASE64('$voiceprintEndpoint64') USING utf8mb4) WHERE param_code='server.voice_print' AND (param_value IS NULL OR param_value='' OR param_value='null' OR param_value LIKE 'http://voiceprint-api:8005/%');"
$voiceprintSql | & docker compose --env-file .env -p $project exec -T -e "MYSQL_PWD=$dbPassword" tongdou-db mysql -uroot $dbName *> $null
if ($LASTEXITCODE -ne 0) { Stop-WithError 'Could not configure the voiceprint endpoint.' }
& docker compose --env-file .env -p $project exec -T tongdou-redis redis-cli HDEL 'sys:params' 'server.voice_print' *> $null
'''
 launcher=replace(launcher,before,extra+'\n'+before)
 before="[System.IO.File]::WriteAllText($imageMarker,"
 pos=launcher.index(before)
 launcher=launcher[:pos]+f"if (-not (Test-DockerImage 'tongdou/voiceprint:v{v}')) {{\n    & docker compose --env-file .env -p $project build voiceprint-api\n    if ($LASTEXITCODE -ne 0) {{ Stop-WithError 'Could not build the local voiceprint service.' }}\n}}\n"+launcher[pos:]
 launcher=replace(launcher,'Start-Process "http://localhost:${dashboardPort}"','if (-not $NoBrowser) { Start-Process "http://localhost:${dashboardPort}" }')
 write(srv/'scripts/start_tongdou_server.ps1',launcher)
 install_linux_startup(srv)
 for folder in ('mysql','redis','uploadfile'):(srv/folder).mkdir(exist_ok=True)
 print('STAGED',fw,srv,sep='\n')

def archive(v):
 for name in (f'TongDou_Application_Firmware_v{v}_Early_Tester',f'TongDou_Server_v{v}_Early_Tester'):
  folder=OUT/name
  target=OUT/(name+'.zip')
  if target.exists():raise SystemExit(f'Archive already exists: {target}')
  paths=sorted(p for p in folder.rglob('*') if p.is_file() and p.name!='SHA256SUMS.txt')
  forbidden=[p for p in paths if p.name in ('.env','.config.yaml','.mcp_server_settings.json') or p.suffix in ('.db','.sqlite','.log','.pyc')]
  assert not forbidden,forbidden
  write(folder/'SHA256SUMS.txt',''.join(f'{sha(p)}  {p.relative_to(folder).as_posix()}\n' for p in paths))
  with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
   for p in sorted(folder.rglob('*')):z.write(p,p.relative_to(OUT).as_posix())
  with zipfile.ZipFile(target) as z:assert z.testzip() is None
  print(target,sha(target),target.stat().st_size)

if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--stage',action='store_true');parser.add_argument('--archive',action='store_true');args=parser.parse_args()
 ini=configparser.ConfigParser();ini.read(ROOT/'firmware/platformio.ini');v=ini['env:esp32s3']['custom_firmware_version']
 if args.stage:stage(v)
 if args.archive:archive(v)
