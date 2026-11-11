// pages.h — webbsidorna (lagras i flash)
// Utvecklad av A. Isaksson SA7BNB — 2026
#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------- gemensam head + CSS
const char PAGE_HEAD[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="sv"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0a0e13">
<title>Temperaturgivare</title>
<style>
:root{color-scheme:dark;--h:205;--bg:#0a0e13;--card:#121922;--card2:#18212c;--ink:#eef3f8;--mut:#8696a7;--line:#222e3c;
--acc:#4fb0ff;--ok:#3ddc97;--bad:#ff6b63;--warn:#ffb648;--r:18px;--shadow:0 1px 0 rgba(255,255,255,.04) inset,0 12px 32px -18px rgba(0,0,0,.8)}
@media(prefers-color-scheme:light){:root{color-scheme:light;--bg:#eef1f5;--card:#fff;--card2:#f4f6f9;--ink:#111a24;--mut:#5f6f80;--line:#dfe5ec;
--acc:#0a6fd6;--ok:#12855a;--bad:#c5332b;--warn:#a86400;--shadow:0 1px 2px rgba(16,24,40,.05),0 14px 30px -20px rgba(16,24,40,.25)}}
*{box-sizing:border-box}
html{-webkit-text-size-adjust:100%}
body{margin:0;background:var(--bg);color:var(--ink);font:16px/1.5 system-ui,-apple-system,"Segoe UI",Roboto,"Helvetica Neue",sans-serif;-webkit-font-smoothing:antialiased}
main{max-width:560px;margin:0 auto;padding:18px 16px 28px}
header{display:flex;align-items:center;gap:12px;margin:4px 0 18px}
.logo{flex:none;width:44px;height:44px;border-radius:13px;display:grid;place-items:center;color:#fff;
background:linear-gradient(145deg,hsl(var(--h) 85% 58%),hsl(calc(var(--h) + 40) 80% 42%));box-shadow:0 8px 20px -8px hsl(var(--h) 85% 50%/.7);transition:background .8s}
.hd{flex:1;min-width:0}
h1{font-size:19px;line-height:1.2;margin:0;font-weight:650;letter-spacing:-.01em;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.sub{color:var(--mut);margin:1px 0 0;font-size:13px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
h2{font-size:12px;margin:0 0 12px;text-transform:uppercase;letter-spacing:.09em;color:var(--mut);font-weight:650}
section,.card{background:var(--card);border:1px solid var(--line);border-radius:var(--r);padding:18px;margin:0 0 14px;box-shadow:var(--shadow)}
.row{display:flex;justify-content:space-between;align-items:center;gap:10px}
.pill{flex:none;display:inline-flex;align-items:center;gap:7px;padding:6px 11px;border-radius:99px;font-size:12.5px;font-weight:600;
background:var(--card);border:1px solid var(--line);color:var(--mut);white-space:nowrap}
.pill i{width:8px;height:8px;border-radius:50%;background:currentColor}
.pill.ok{color:var(--ok)}.pill.bad{color:var(--bad)}.pill.warn{color:var(--warn)}
.pill.ok i{animation:pulse 2.4s infinite}
@keyframes pulse{0%{box-shadow:0 0 0 0 color-mix(in srgb,currentColor 55%,transparent)}70%,100%{box-shadow:0 0 0 7px transparent}}
@media(prefers-reduced-motion:reduce){*{animation:none!important;transition:none!important}}

/* flikar */
nav{display:grid;grid-template-columns:repeat(3,1fr);gap:4px;padding:4px;background:var(--card);border:1px solid var(--line);border-radius:14px;margin:0 0 14px}
nav button{background:transparent;border:0;color:var(--mut);padding:9px 6px;border-radius:10px;font-size:14px;font-weight:600}
nav button.on{background:var(--card2);color:var(--ink);box-shadow:0 0 0 1px var(--line)}

/* hero */
.hero{position:relative;overflow:hidden;padding:20px 20px 0;
background:radial-gradient(120% 90% at 100% 0%,hsl(var(--h) 90% 55%/.22),transparent 60%),var(--card);transition:background .8s}
.hero .lab{font-size:12px;text-transform:uppercase;letter-spacing:.09em;color:var(--mut);font-weight:650}
.temp{display:flex;align-items:flex-start;font-weight:250;line-height:1;margin:6px 0 4px;letter-spacing:-.04em;font-variant-numeric:tabular-nums}
.temp b{font-size:92px;font-weight:250}.temp span{font-size:40px;margin-top:10px;letter-spacing:-.02em}.temp em{font-style:normal;font-size:20px;color:var(--mut);margin:14px 0 0 6px;letter-spacing:0;font-weight:500}
.hero .mm{display:flex;gap:16px;color:var(--mut);font-size:13.5px;font-variant-numeric:tabular-nums}
.hero .mm b{color:var(--ink);font-weight:600}
.spark{display:block;width:calc(100% + 40px);margin:10px -20px 0;height:74px}
.spark .ln{fill:none;stroke:hsl(var(--h) 90% 62%);stroke-width:2;stroke-linejoin:round;stroke-linecap:round;vector-effect:non-scaling-stroke}

/* rutor */
.grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin:0 0 14px}
.tile{background:var(--card);border:1px solid var(--line);border-radius:var(--r);padding:14px 16px;box-shadow:var(--shadow);min-width:0}
.tile.w{grid-column:1/-1}
.tile .k{font-size:11.5px;text-transform:uppercase;letter-spacing:.08em;color:var(--mut);font-weight:650;display:flex;justify-content:space-between;align-items:center}
.tile .v{font-size:19px;font-weight:600;margin-top:5px;font-variant-numeric:tabular-nums;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;letter-spacing:-.01em}
.tile .s{font-size:13px;color:var(--mut);margin-top:1px;overflow-wrap:anywhere}
.chips{display:flex;flex-wrap:wrap;gap:8px;margin-top:8px}
.chip{display:inline-flex;align-items:baseline;gap:8px;padding:7px 12px;border-radius:12px;background:var(--card2);border:1px solid var(--line);font-size:13px;color:var(--mut)}
.chip b{font-size:16px;color:var(--ink);font-weight:600;font-variant-numeric:tabular-nums}
.chip.low{border-color:hsl(var(--h) 80% 55%/.7)}.chip small{color:hsl(var(--h) 80% 62%);font-weight:600}

/* formulär */
label{display:block;font-size:13.5px;color:var(--mut);margin:14px 0 6px;font-weight:500}
input[type=text],input[type=password],input[type=number],select,input[type=file]{width:100%;padding:12px 13px;border:1px solid var(--line);border-radius:12px;background:var(--card2);color:var(--ink);font:inherit}
input[type=file]{padding:9px;font-size:14px}
input:focus,select:focus,button:focus-visible{outline:2px solid var(--acc);outline-offset:2px}
.sw{display:flex;align-items:center;justify-content:space-between;gap:12px;margin:14px 0 0;color:var(--ink);font-size:15.5px;font-weight:500}
.sw input{appearance:none;-webkit-appearance:none;flex:none;width:48px;height:28px;border-radius:99px;background:var(--line);position:relative;margin:0;cursor:pointer;transition:background .2s}
.sw input::after{content:"";position:absolute;top:3px;left:3px;width:22px;height:22px;border-radius:50%;background:#fff;box-shadow:0 1px 3px rgba(0,0,0,.4);transition:transform .2s}
.sw input:checked{background:var(--ok)}.sw input:checked::after{transform:translateX(20px)}
button{font:inherit;font-weight:600;padding:12px 18px;border-radius:12px;border:1px solid transparent;background:var(--acc);color:#fff;cursor:pointer}
@media(prefers-color-scheme:dark){button{color:#04121f}}
button:active{transform:translateY(1px)}
button.ghost{background:var(--card2);border-color:var(--line);color:var(--ink)}
button.warn{background:transparent;border-color:color-mix(in srgb,var(--bad) 55%,transparent);color:var(--bad)}
button.mini{padding:6px 11px;font-size:13px;border-radius:9px;white-space:nowrap;flex:none}
button:disabled{opacity:.5}
.btns{display:flex;flex-wrap:wrap;gap:10px;margin-top:16px}.btns.full button{flex:1}
.muted{color:var(--mut);font-size:13.5px}
.ok{color:var(--ok)}.bad{color:var(--bad)}
.pin{font:600 30px ui-monospace,"SF Mono",Menlo,Consolas,monospace;letter-spacing:.06em}
dl{display:grid;grid-template-columns:auto 1fr;gap:8px 14px;margin:12px 0 0;font-size:14.5px}
dt{color:var(--mut)}dd{margin:0;text-align:right;font-variant-numeric:tabular-nums;overflow-wrap:anywhere}
.act{display:flex;justify-content:space-between;align-items:center;gap:14px;padding:14px 0;border-top:1px solid var(--line)}
.act:first-of-type{border-top:0;padding-top:2px}.act:last-child{padding-bottom:2px}
.act div{min-width:0}.act b{display:block;font-size:15px;font-weight:600}.act span{font-size:13px;color:var(--mut)}
.act button{flex:none}

/* wifi-listan */
.net{display:flex;width:100%;justify-content:space-between;align-items:center;gap:10px;background:transparent;color:var(--ink);border:0;border-top:1px solid var(--line);border-radius:0;padding:14px 2px;text-align:left;font-weight:500}
.net:first-child{border-top:0}
.net span.n{overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.sig{display:inline-flex;align-items:flex-end;gap:2px;height:14px;margin-left:8px;vertical-align:-1px}
.sig i{width:3px;background:var(--line);border-radius:1px}.sig i.on{background:var(--ink)}
.spin{display:inline-block;width:14px;height:14px;border:2px solid var(--line);border-top-color:var(--acc);border-radius:50%;animation:sp .8s linear infinite;vertical-align:-2px;margin-right:8px}
@keyframes sp{to{transform:rotate(360deg)}}

a{color:var(--acc)}
footer{text-align:center;color:var(--mut);font-size:12.5px;margin:26px 0 0;line-height:1.7}
footer b{color:var(--ink);font-weight:600;letter-spacing:.02em}
#toast{position:fixed;left:50%;bottom:calc(18px + env(safe-area-inset-bottom));transform:translateX(-50%);background:var(--ink);color:var(--bg);padding:12px 18px;border-radius:14px;font-size:14px;font-weight:500;max-width:calc(100% - 32px);box-shadow:0 14px 34px -10px rgba(0,0,0,.6);z-index:9}
[hidden]{display:none!important}
@media(max-width:380px){.temp b{font-size:76px}.temp span{font-size:34px}}
</style></head><body><main>
)HTML";

// ---------------------------------------------------------------- wifi-sidan (AP-läge)
const char PAGE_WIFI[] PROGMEM = R"HTML(
<header>
  <div class="logo"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M5 12.5a10 10 0 0 1 14 0M8.2 15.7a5.5 5.5 0 0 1 7.6 0"/><circle cx="12" cy="19" r="1.2" fill="currentColor"/></svg></div>
  <div class="hd"><h1>Anslut givaren till wifi</h1><p class="sub" id="dev">&nbsp;</p></div>
</header>

<section id="s-list">
  <div class="row"><h2 style="margin:0">Nätverk i närheten</h2><button class="ghost mini" id="rescan">Sök igen</button></div>
  <div id="nets" style="margin-top:10px"><p class="muted"><i class="spin"></i>Söker efter nätverk…</p></div>
  <div class="btns"><button class="ghost" id="manual">Dolt nätverk…</button></div>
</section>

<section id="s-form" hidden>
  <h2>Anslut</h2>
  <label for="ssid">Nätverksnamn (SSID)</label>
  <input type="text" id="ssid" maxlength="32" autocapitalize="none" autocorrect="off" spellcheck="false">
  <label for="pass">Lösenord</label>
  <input type="password" id="pass" maxlength="64" autocapitalize="none" autocorrect="off" spellcheck="false">
  <label class="sw">Visa lösenord <input type="checkbox" id="show"></label>
  <div class="btns full"><button id="go">Anslut</button><button class="ghost" id="back">Avbryt</button></div>
</section>

<section id="s-prog" hidden>
  <h2>Status</h2>
  <p id="msg" style="margin:0"></p>
  <p id="links" hidden></p>
  <div class="btns"><button class="ghost" id="again" hidden>Försök igen</button></div>
</section>

<p class="muted" style="text-align:center">Vill du ställa in rapporteringen först? <a href="/config">Öppna inställningarna</a></p>
<footer>Utvecklad av <b>A. Isaksson</b> · <b>SA7BNB</b> · 2026</footer>

<script>
const $=i=>document.getElementById(i);
let info={name:'',ap:''},lost=0;
const j=async(u,o)=>(await fetch(u,o)).json();
const SPIN='<p class="muted"><i class="spin"></i>Söker efter nätverk…</p>';
function show(id){['s-list','s-form','s-prog'].forEach(s=>$(s).hidden=(s!==id))}
function sig(r){const n=r>-55?4:r>-67?3:r>-78?2:1,s=document.createElement('span');s.className='sig';
  s.title=r+' dBm';for(let i=0;i<4;i++){const b=document.createElement('i');b.style.height=(5+i*3)+'px';if(i<n)b.className='on';s.appendChild(b)}return s}
function render(nets){
  const best={};nets.forEach(n=>{if(n.ssid&&(!best[n.ssid]||n.rssi>best[n.ssid].rssi))best[n.ssid]=n});
  const list=Object.values(best).sort((a,b)=>b.rssi-a.rssi),box=$('nets');box.textContent='';
  if(!list.length){box.innerHTML='<p class="muted">Hittade inga nätverk. Tryck på Sök igen.</p>';return}
  list.forEach(n=>{const b=document.createElement('button');b.className='net';
    const a=document.createElement('span');a.className='n';a.textContent=n.ssid;
    const r=document.createElement('span');r.textContent=n.open?'öppet ':'🔒 ';r.className='muted';r.style.flex='none';r.appendChild(sig(n.rssi));
    b.append(a,r);b.onclick=()=>pick(n.ssid,n.open);box.appendChild(b)})}
async function scan(start){
  if(start)$('nets').innerHTML=SPIN;
  try{const d=await j('/api/scan'+(start?'?start=1':''));
    if(d.state==='done'){render(d.nets);return}
    if(d.state==='idle'){setTimeout(()=>scan(1),1500);return}}catch(e){}
  setTimeout(()=>scan(0),1200)}
function pick(ssid,open){$('ssid').value=ssid;$('ssid').readOnly=!!ssid;$('pass').value='';
  $('pass').disabled=!!open;$('pass').placeholder=open?'Öppet nätverk – inget lösenord':'';
  show('s-form');(ssid?(open?$('go'):$('pass')):$('ssid')).focus()}
async function go(){const ssid=$('ssid').value.trim();if(!ssid)return;
  show('s-prog');$('again').hidden=true;$('links').hidden=true;$('msg').className='';
  $('msg').innerHTML='<i class="spin"></i>';$('msg').append('Ansluter till '+ssid+' … det kan ta upp till 20 sekunder.');
  try{await fetch('/api/connect',{method:'POST',body:new URLSearchParams({ssid:ssid,pass:$('pass').value})})}catch(e){}
  lost=0;setTimeout(check,1500)}
function links(ip){const p=$('links');p.textContent='';
  const add=u=>{const a=document.createElement('a');a.href=u;a.textContent=u;p.append(a,document.createElement('br'))};
  if(ip)add('http://'+ip+'/');add('http://'+info.name+'.local/');p.hidden=false}
async function check(){
  try{const d=await j('/api/wifi');lost=0;
    if(d.state==='connected'){$('msg').className='ok';
      $('msg').textContent='Ansluten! Givaren fick adressen '+d.ip+' och startar nu om. Anslut din telefon eller dator till samma nätverk och öppna konfigurationssidan:';
      links(d.ip);return}
    if(d.state==='failed'){$('msg').className='bad';$('msg').textContent='Det gick inte att ansluta: '+d.reason;$('again').hidden=false;return}
  }catch(e){if(++lost>8){
      $('msg').textContent='Tappade kontakten med givaren. Det är vanligt när den byter kanal för att ansluta till ditt nätverk. '+
        'Gick det bra når du den från ditt vanliga wifi på adressen nedan. Dyker nätverket '+info.ap+' upp igen misslyckades anslutningen – anslut dit och försök på nytt.';
      links('');return}}
  setTimeout(check,1500)}
$('rescan').onclick=()=>scan(1);$('manual').onclick=()=>pick('',false);
$('back').onclick=()=>show('s-list');$('again').onclick=()=>show('s-form');$('go').onclick=go;
$('show').onchange=e=>$('pass').type=e.target.checked?'text':'password';
$('pass').onkeydown=e=>{if(e.key==='Enter')go()};
j('/api/wifi').then(d=>{info=d;$('dev').textContent=d.name+(d.ssid?' · sparat nätverk '+d.ssid+' gick inte att nå':'')}).catch(()=>{});
scan(1);
</script>
</main></body></html>
)HTML";

// ---------------------------------------------------------------- konfigurationssidan
const char PAGE_CONFIG[] PROGMEM = R"HTML(
<header>
  <div class="logo"><svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14 14.8V5a2 2 0 0 0-4 0v9.8a4 4 0 1 0 4 0z"/><path d="M12 9v8"/></svg></div>
  <div class="hd"><h1 id="title">Temperaturgivare</h1><p class="sub" id="sub">&nbsp;</p></div>
  <span class="pill" id="pill"><i></i><span id="pilltxt">Ansluter…</span></span>
</header>

<nav>
  <button data-t="over" class="on">Översikt</button>
  <button data-t="set">Inställningar</button>
  <button data-t="sys">Underhåll</button>
</nav>

<!-- ============ ÖVERSIKT ============ -->
<div data-tab="over">
  <div class="card hero">
    <div class="lab">Rapporteras till temperatur.nu</div>
    <div class="temp"><b id="t-int">–</b><span id="t-dec"></span><em>°C</em></div>
    <div class="mm"><span>Lägst <b id="t-min">–</b></span><span>Högst <b id="t-max">–</b></span><span id="sparkcap" style="margin-left:auto">Samlar mätvärden…</span></div>
    <svg class="spark" id="spark" viewBox="0 0 300 74" preserveAspectRatio="none" aria-hidden="true">
      <defs><linearGradient id="g" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="hsl(var(--h) 90% 60%)" stop-opacity=".35"/><stop offset="1" stop-color="hsl(var(--h) 90% 60%)" stop-opacity="0"/></linearGradient></defs>
      <polygon id="sp-a" fill="url(#g)" points=""/><polyline id="sp-l" class="ln" points=""/>
    </svg>
  </div>

  <div class="grid">
    <div class="tile w"><div class="k">Givare</div><div class="chips" id="chips"><span class="muted">Söker…</span></div></div>
    <div class="tile"><div class="k">Senaste rapport</div><div class="v" id="rep">–</div><div class="s" id="repnext">&nbsp;</div></div>
    <div class="tile"><div class="k">Wifi <span class="sig" id="bars"></span></div><div class="v" id="wifi">–</div><div class="s" id="rssi">&nbsp;</div></div>
    <div class="tile"><div class="k">IP-adress</div><div class="v" id="ip">–</div><div class="s" id="host">&nbsp;</div></div>
    <div class="tile"><div class="k">Drifttid</div><div class="v" id="up">–</div><div class="s" id="ver">&nbsp;</div></div>
    <div class="tile w"><div class="k">Svar från servern</div><div class="s" id="repmsg" style="margin-top:6px;font-size:14px">–</div>
      <div class="btns" style="margin-top:12px"><button class="ghost" id="repnow">Rapportera nu</button></div></div>
  </div>

  <section>
    <div class="row"><h2 style="margin:0">Registrering hos temperatur.nu</h2><button class="ghost mini" id="copy">Kopiera PIN</button></div>
    <div class="pin" id="pin" style="margin-top:10px">–</div>
    <p class="muted" style="margin:6px 0 0">Registrera stationen på
      <a href="https://www.temperatur.nu/nystation" target="_blank" rel="noopener">temperatur.nu/nystation</a> med PIN-koden.</p>
    <dl><dt>MAC</dt><dd id="mac">–</dd><dt>Identitet</dt><dd id="idt">–</dd></dl>
  </section>
</div>

<!-- ============ INSTÄLLNINGAR ============ -->
<div data-tab="set" hidden>
  <section>
    <h2>Enhet</h2>
    <label for="name" style="margin-top:0">Enhetsnamn (blir även adressen namn.local)</label>
    <input type="text" id="name" maxlength="32" autocapitalize="none" spellcheck="false">
    <label for="offset">Kalibrering (°C, läggs till mätvärdet)</label>
    <input type="number" id="offset" step="0.1" min="-10" max="10" inputmode="decimal">
  </section>
  <section>
    <h2>Rapportering</h2>
    <label class="sw" style="margin-top:0">Rapportera till temperatur.nu <input type="checkbox" id="enabled"></label>
    <label for="method">Metod</label>
    <select id="method"><option value="0">PIN (som Temperatur.nu Reporter)</option>
      <option value="1">Hashkod (rapportera.php)</option></select>
    <div id="hashbox"><label for="hash">Hashkod från temperatur.nu</label>
    <input type="text" id="hash" maxlength="64" autocapitalize="none" autocorrect="off" spellcheck="false"></div>
    <label for="interval">Intervall</label>
    <select id="interval"><option value="60">Varje minut</option><option value="120">Varannan minut</option><option value="300">Var 5:e minut</option>
      <option value="600">Var 10:e minut</option><option value="900">Var 15:e minut</option><option value="1800">Var 30:e minut</option></select>
  </section>
  <section>
    <h2>Nätverk och säkerhet</h2>
    <label for="txp" style="margin-top:0">Sändeffekt wifi</label>
    <select id="txp"><option value="0">Låg, 8,5 dBm (rekommenderas)</option>
      <option value="1">Medel, 15 dBm</option><option value="2">Hög, 19,5 dBm</option></select>
    <label for="admin">Lösenord för den här sidan (användarnamn: admin)</label>
    <input type="password" id="admin" maxlength="32" autocomplete="new-password" placeholder="Lämna tomt för att inte ändra">
    <label class="sw" id="clrbox" hidden>Ta bort lösenordet <input type="checkbox" id="adminclr"></label>
  </section>
  <div class="btns full" style="margin:0 0 14px"><button id="save">Spara inställningar</button></div>
</div>

<!-- ============ UNDERHÅLL ============ -->
<div data-tab="sys" hidden>
  <section>
    <h2>Firmware</h2>
    <p class="muted" style="margin:0 0 10px">Välj en .bin-fil (Arduino IDE: Sketch → Export Compiled Binary).</p>
    <input type="file" id="fw" accept=".bin">
    <div class="btns"><button class="ghost" id="fwgo">Ladda upp firmware</button></div>
  </section>
  <section>
    <h2>Enheten</h2>
    <div class="act"><div><b>Starta om</b><span>Inga inställningar ändras.</span></div><button class="ghost" id="restart">Starta om</button></div>
    <div class="act"><div><b>Byt wifi</b><span>Glömmer nätverket och öppnar accesspunkten igen.</span></div><button class="warn" id="newwifi">Byt wifi</button></div>
    <div class="act"><div><b>Återställ inställningar</b><span>Raderar wifi och alla val. PIN och MAC behålls.</span></div><button class="warn" id="factory">Återställ</button></div>
    <div class="act"><div><b>Hård återställning</b><span>Raderar allt och ger givaren ny PIN och MAC – som en helt ny enhet.</span></div><button class="warn" id="hard">Återställ hårt</button></div>
  </section>
</div>

<footer>Utvecklad av <b>A. Isaksson</b> · <b>SA7BNB</b> · 2026<br><span id="fwv">&nbsp;</span></footer>

<div id="toast" hidden></div>
<script>
const $=i=>document.getElementById(i);
let filled=false,tt,pin='';
function toast(t){const e=$('toast');e.textContent=t;e.hidden=false;clearTimeout(tt);tt=setTimeout(()=>e.hidden=true,5000)}
function dur(s){if(s<60)return s+' s';if(s<3600)return Math.floor(s/60)+' min';const h=Math.floor(s/3600);
  return h<48?h+' h '+Math.floor(s%3600/60)+' min':Math.floor(h/24)+' dygn '+(h%24)+' h'}
const f1=v=>v.toFixed(1).replace('.',',').replace('-','−');
const post=async(u,b)=>(await fetch(u,{method:'POST',body:b})).json();
function hashvis(){$('hashbox').hidden=$('method').value!=='1'}
function tab(t){document.querySelectorAll('nav button').forEach(b=>b.classList.toggle('on',b.dataset.t===t));
  document.querySelectorAll('[data-tab]').forEach(d=>d.hidden=d.dataset.tab!==t);try{history.replaceState(null,'','#'+t)}catch(e){}}
document.querySelectorAll('nav button').forEach(b=>b.onclick=()=>tab(b.dataset.t));
if(['set','sys'].includes(location.hash.slice(1)))tab(location.hash.slice(1));
function pill(c,t){$('pill').className='pill '+c;$('pilltxt').textContent=t}
function bars(r){const n=r>-55?4:r>-67?3:r>-78?2:1,s=$('bars');s.textContent='';
  for(let i=0;i<4;i++){const b=document.createElement('i');b.style.height=(5+i*3)+'px';if(i<n)b.className='on';s.appendChild(b)}}
function spark(h,step){
  if(h.length<2){$('sp-l').setAttribute('points','');$('sp-a').setAttribute('points','');
    $('sparkcap').textContent='Samlar mätvärden…';return}
  let lo=Math.min(...h),hi=Math.max(...h);if(hi-lo<1){const m=(hi+lo)/2;lo=m-.5;hi=m+.5}
  const W=300,T=8,B=58,p=h.map((v,i)=>(i*W/(h.length-1)).toFixed(1)+','+(B-(v-lo)/(hi-lo)*(B-T)).toFixed(1));
  $('sp-l').setAttribute('points',p.join(' '));$('sp-a').setAttribute('points','0,74 '+p.join(' ')+' 300,74');
  $('sparkcap').textContent='Senaste '+dur((h.length-1)*step)}
function hue(t){return Math.round(t<=0?212:t>=30?18:212-(t/30)*194)}
async function load(){
  let d;try{d=await(await fetch('/api/state')).json()}catch(e){pill('','Ingen kontakt');return}
  if(d.temp===null){$('t-int').textContent='–';$('t-dec').textContent=''}
  else{const s=f1(d.temp).split(',');$('t-int').textContent=s[0];$('t-dec').textContent=','+s[1];
    document.documentElement.style.setProperty('--h',hue(d.temp))}
  const all=d.hist.concat(d.temp===null?[]:[d.temp]);
  $('t-min').textContent=all.length?f1(Math.min(...all))+'°':'–';$('t-max').textContent=all.length?f1(Math.max(...all))+'°':'–';
  spark(d.hist,d.hist_step);
  const c=$('chips');c.textContent='';
  if(!d.sensors.length){c.innerHTML='<span class="bad">Ingen DS18B20 hittad på GPIO4 – kontrollera koppling och pull-up.</span>'}
  else{const ok=d.sensors.filter(v=>v!==null),low=ok.length?Math.min(...ok):null;let marked=false;
    d.sensors.forEach((v,i)=>{const e=document.createElement('span');e.className='chip';
      e.append('Givare '+(i+1)+' ');const b=document.createElement('b');b.textContent=v===null?'fel':f1(v)+'°';e.appendChild(b);
      if(v!==null&&v===low&&!marked&&d.sensors.length>1){marked=true;e.classList.add('low');const s=document.createElement('small');s.textContent='rapporteras';e.appendChild(s)}
      c.appendChild(e)})}
  $('rep').textContent=d.rep_age<0?(d.enabled?'Ingen ännu':'Avstängd'):dur(d.rep_age)+' sedan';
  $('rep').className='v'+(d.rep_age>=0&&!d.rep_ok?' bad':'');
  $('repnext').textContent=d.next_in>=0?'Nästa om '+dur(d.next_in):' ';
  $('repmsg').textContent=d.rep_msg||'Ingen rapport skickad ännu.';
  $('wifi').textContent=d.ap?'AP-läge':d.ssid;$('rssi').textContent=d.ap?'Inte ansluten till wifi':d.rssi+' dBm';if(!d.ap)bars(d.rssi);
  $('ip').textContent=d.ip;$('host').textContent=d.name+'.local';
  $('up').textContent=dur(d.uptime);$('ver').textContent='Firmware '+d.fw;$('fwv').textContent='Firmware '+d.fw;
  pin=d.pin;$('pin').textContent=d.pin;$('mac').textContent=d.mac;$('idt').textContent=d.custom_id?'Egen (slumpad)':'Fabrik';
  $('title').textContent=d.name;$('sub').textContent=d.name+'.local · '+d.ip;
  if(d.ap)pill('warn','Accesspunkt');else if(!d.enabled)pill('','Rapportering av');
  else if(d.rep_age<0)pill('warn','Väntar');else if(d.rep_ok)pill('ok','Rapporterar');else pill('bad','Rapportfel');
  if(!filled){filled=true;document.title=d.name;$('name').value=d.name;$('enabled').checked=d.enabled;
    $('method').value=d.method;$('hash').value=d.hash;$('interval').value=d.interval;$('offset').value=d.offset;
    $('txp').value=d.txp;$('clrbox').hidden=!d.has_admin;hashvis()}}
$('method').onchange=hashvis;
$('copy').onclick=async()=>{try{if(navigator.clipboard&&window.isSecureContext)await navigator.clipboard.writeText(pin);
  else{const t=document.createElement('textarea');t.value=pin;t.style.position='fixed';t.style.opacity='0';document.body.appendChild(t);t.select();document.execCommand('copy');t.remove()}
  toast('PIN kopierad: '+pin)}catch(e){toast('Kunde inte kopiera – markera koden manuellt.')}};
$('save').onclick=async()=>{
  const b=new URLSearchParams({name:$('name').value,enabled:$('enabled').checked?1:0,method:$('method').value,
    hash:$('hash').value.trim(),interval:$('interval').value,offset:$('offset').value||0,txp:$('txp').value,
    admin:$('admin').value,adminclr:$('adminclr').checked?1:0});
  try{const r=await post('/api/config',b);
    toast(r.ok?(r.restart?'Sparat. Givaren startar om med nytt namn: '+r.name+'.local':'Inställningarna är sparade.'):'Fel: '+r.error);
    if(r.ok){$('admin').value='';$('adminclr').checked=false;filled=false;if(!r.restart)load()}
  }catch(e){toast('Kunde inte spara – ingen kontakt med givaren.')}};
$('repnow').onclick=async()=>{$('repnow').disabled=true;
  try{const r=await post('/api/report');toast(r.ok?'Rapporterat: '+r.msg:'Misslyckades: '+r.msg)}catch(e){toast('Ingen kontakt med givaren.')}
  $('repnow').disabled=false;load()};
$('restart').onclick=async()=>{if(confirm('Starta om givaren?')){try{await post('/api/restart')}catch(e){}toast('Startar om…')}};
$('newwifi').onclick=async()=>{if(confirm('Glöm det sparade nätverket? Givaren startar om som accesspunkt så att du kan välja ett nytt.')){
  try{await post('/api/resetwifi')}catch(e){}toast('Wifi raderat. Anslut till givarens eget nätverk för att välja nytt.')}};
$('factory').onclick=async()=>{if(confirm('Radera alla inställningar, även wifi och lösenord? PIN och MAC behålls.')){
  try{await post('/api/factory')}catch(e){}toast('Återställd. Givaren startar om som accesspunkt.')}};
$('hard').onclick=async()=>{if(confirm('Hård återställning: radera ALLT och ge givaren en ny PIN och MAC? Den gamla stationen på temperatur.nu slutar få värden från den här enheten.')){
  try{await post('/api/hardreset')}catch(e){}toast('Återställd med ny identitet. Givaren startar om som accesspunkt.')}};
$('fwgo').onclick=async()=>{const f=$('fw').files[0];if(!f){toast('Välj en .bin-fil först.');return}
  if(!confirm('Ladda upp '+f.name+' ('+Math.round(f.size/1024)+' kB)?'))return;
  $('fwgo').disabled=true;toast('Laddar upp… stäng inte sidan.');
  const fd=new FormData();fd.append('fw',f,f.name);
  try{const r=await post('/update',fd);toast(r.ok?'Klart! Givaren startar om.':'Uppdateringen misslyckades: '+r.error)}
  catch(e){toast('Uppladdningen avbröts.')}$('fwgo').disabled=false};
load();setInterval(load,5000);
</script>
</main></body></html>
)HTML";
