#include "demo/DemoWebPage.h"

namespace tongdou {

String DemoWebPage::html() const {
  String body;
  body.reserve(7600);
  body += F(R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>TongDou Demo</title>
<style>
:root{color-scheme:dark;--bg:#101010;--panel:#1b1b1b;--line:#303030;--gold:#e5b55f;--text:#f4efe4;--muted:#aaa}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font-family:Arial,sans-serif}
main{max-width:720px;margin:auto;padding:18px}h1{margin:0;font-size:28px}p{color:var(--muted)}
.status{background:var(--panel);border:1px solid var(--line);border-radius:8px;padding:14px;margin:14px 0}
.scene{font-size:18px;font-weight:700}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}
button{min-height:82px;border:1px solid var(--line);border-radius:8px;background:#222;color:var(--text);font-weight:700;font-size:15px}
button strong{display:block;font-size:28px;color:var(--gold);margin-bottom:4px}
button.stop{grid-column:span 3;background:#3a1c1c;border-color:#6b3030}
.subgrid{display:grid;grid-column:1/-1;grid-template-columns:repeat(5,1fr);gap:8px}
.subgrid button{min-height:66px;font-size:12px;background:#191919}
button:active{transform:translateY(1px);filter:brightness(.8)}
pre{white-space:pre-wrap;background:#050505;border:1px solid var(--line);border-radius:8px;padding:12px;min-height:96px}
@media(max-width:520px){.grid{grid-template-columns:repeat(2,1fr)}button.stop{grid-column:span 2}.subgrid{grid-template-columns:repeat(2,1fr)}}
</style></head><body><main>
<h1>TongDou Demo</h1>
<p>Internal overseas campaign controller. Phone stays off camera.</p>
<section class="status">
<div>Current</div><div class="scene" id="current">Loading...</div>
<p id="playing">Checking status</p>
</section>
<section class="grid">
<button onclick="play(1)"><strong>1</strong>Late-night hook</button>
<button onclick="play(2)"><strong>2</strong>Soft landing</button>
<button onclick="play(3)"><strong>3</strong>Morning wakeup</button>
<button onclick="play(4)"><strong>4</strong>Email notice</button>
<button onclick="play(5)"><strong>5</strong>Email summary</button>
<button onclick="play(6,1)"><strong>6</strong>Accountant 6-1</button>
<div class="subgrid" id="accountantVariants">
<button onclick="play(6,1)"><strong>1</strong>Clear debt</button>
<button onclick="play(6,2)"><strong>2</strong>Damage</button>
<button onclick="play(6,3)"><strong>3</strong>Bribe</button>
<button onclick="play(6,4)"><strong>4</strong>Memory</button>
<button onclick="play(6,5)"><strong>5</strong>Pi debt</button>
</div>
<button onclick="play(7)"><strong>7</strong>Campaign call</button>
<button onclick="play(8)"><strong>8</strong>Summon</button>
<button class="stop" onclick="play(0)"><strong>0</strong>Stop and idle</button>
</section>
<pre id="log">Ready</pre>
<script>
const $=id=>document.getElementById(id);
let refreshBusy=false;
let commandBusy=false;
async function fetchLimit(url,opt,ms){
  const ctl=new AbortController();
  const t=setTimeout(()=>ctl.abort(),ms);
  opt=opt||{};
  opt.signal=ctl.signal;
  try{return await fetch(url,opt);}
  finally{clearTimeout(t);}
}
async function refresh(){
  if(refreshBusy||commandBusy)return;
  refreshBusy=true;
  try{
    const r=await fetchLimit('/api/demo/status',{cache:'no-store'},1800);
    const j=await r.json();
    $('current').textContent=j.id+' · '+j.title;
    $('playing').textContent=j.playing?'Playing: '+j.scene:'Idle: '+j.scene;
  }catch(e){
    $('playing').textContent='Status update delayed';
  }finally{
    refreshBusy=false;
  }
}
async function play(id,variant){
  if(commandBusy){
    $('log').textContent='Command is still sending...';
    return;
  }
  commandBusy=true;
  const label=variant?id+'-'+variant:id;
  $('log').textContent='Sending scene '+label+'...';
  let body='id='+id;
  if(variant) body+='&variant='+variant;
  try{
    const r=await fetchLimit('/api/demo/scene',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body},4500);
    const t=await r.text();
    $('log').textContent=t;
  }catch(e){
    $('log').textContent='Command timed out. Tap again.';
  }finally{
    commandBusy=false;
  }
  refresh();
}
refresh();setInterval(refresh,2500);
</script></main></body></html>)HTML");
  return body;
}

}  // namespace tongdou
