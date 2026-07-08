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
button:active{transform:translateY(1px);filter:brightness(.8)}
pre{white-space:pre-wrap;background:#050505;border:1px solid var(--line);border-radius:8px;padding:12px;min-height:96px}
@media(max-width:520px){.grid{grid-template-columns:repeat(2,1fr)}button.stop{grid-column:span 2}}
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
<button onclick="play(6)"><strong>6</strong>Cheeky dance</button>
<button onclick="play(7)"><strong>7</strong>Campaign call</button>
<button onclick="play(8)"><strong>8</strong>Summon</button>
<button class="stop" onclick="play(0)"><strong>0</strong>Stop and idle</button>
</section>
<pre id="log">Ready</pre>
<script>
const $=id=>document.getElementById(id);
async function refresh(){
  const r=await fetch('/api/demo/status');
  const j=await r.json();
  $('current').textContent=j.id+' · '+j.title;
  $('playing').textContent=j.playing?'Playing: '+j.scene:'Idle: '+j.scene;
}
async function play(id){
  $('log').textContent='Sending scene '+id+'...';
  const r=await fetch('/api/demo/scene',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'id='+id});
  const t=await r.text();
  $('log').textContent=t;
  await refresh();
}
refresh();setInterval(refresh,900);
</script></main></body></html>)HTML");
  return body;
}

}  // namespace tongdou
