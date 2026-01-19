#ifndef WEB_PAGE_H
#define WEB_PAGE_H

// ===== Page HTML de l'IHM =====
static const char INDEX_HTML[] = R"HTML(
<!doctype html>
<html lang="fr">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Station compostage — IHM (3 bacs)</title>
  <style>
    :root{
      --bg:#0b1220; --text:#e6edf3; --muted:#9aa4b2;
      --border:rgba(255,255,255,.10); --card: rgba(255,255,255,.04);
      --shadow: 0 10px 30px rgba(0,0,0,.35); --r:16px;
      --ok:#22c55e; --warn:#f59e0b; --bad:#ef4444;

      --b1:#fb7185; /* Bac 1 */
      --b2:#60a5fa; /* Bac 2 */
      --b3:#34d399; /* Bac 3 */
      --o2:#a78bfa; /* O2 Bac1 */
    }
    *{box-sizing:border-box}
    body{
      margin:0;
      font-family: system-ui, -apple-system, Segoe UI, Roboto, Arial, sans-serif;
      background: radial-gradient(1200px 800px at 20% 0%, #13244a 0%, var(--bg) 55%) fixed;
      color:var(--text);
    }
    .wrap{max-width:1100px; margin:0 auto; padding:18px;}
    header{
      display:flex; gap:14px; align-items:center; justify-content:space-between;
      padding:14px 16px; border:1px solid var(--border); border-radius:var(--r);
      background: linear-gradient(180deg, rgba(255,255,255,.06), rgba(255,255,255,.02));
      box-shadow:var(--shadow);
    }
    h1{margin:0; font-size:18px; font-weight:900}
    .sub{margin-top:2px; color:var(--muted); font-size:12px}
    .pills{display:flex; gap:10px; flex-wrap:wrap; align-items:center; justify-content:flex-end}
    .pill{
      display:inline-flex; gap:8px; align-items:center;
      padding:8px 10px; border-radius:999px;
      border:1px solid var(--border);
      background: rgba(255,255,255,.03);
      font-size:12px; color:var(--muted);
    }
    .dot{width:10px; height:10px; border-radius:999px; background:var(--muted)}
    .dot.ok{background:var(--ok)} .dot.warn{background:var(--warn)} .dot.bad{background:var(--bad)}

    .grid{margin-top:14px; display:grid; grid-template-columns: repeat(12, 1fr); gap:12px;}
    .card{
      border:1px solid var(--border); background: var(--card);
      border-radius:var(--r); box-shadow:var(--shadow); padding:14px; min-width:0;
    }
    .span-4{grid-column: span 4;} .span-8{grid-column: span 8;}
    @media (max-width: 900px){ .span-4{grid-column: span 12;} .span-8{grid-column: span 12;} }

    .k{color:var(--muted); font-size:12px}
    .big{font-size:26px; font-weight:900; margin-top:4px}
    .unit{font-size:13px; color:var(--muted); font-weight:800; margin-left:6px}

    .row{display:flex; gap:10px; align-items:center; flex-wrap:wrap}
    .badge{
      display:inline-flex; align-items:center; gap:8px;
      padding:7px 10px; border-radius:999px; font-size:12px;
      border:1px solid var(--border);
      background: rgba(255,255,255,.03);
      color:var(--muted);
    }
    .badge strong{color:var(--text)}
    .badge.ok strong{color:var(--ok)} .badge.warn strong{color:var(--warn)} .badge.bad strong{color:var(--bad)}

    .mini{display:grid; grid-template-columns: 1fr 1fr; gap:10px; margin-top:10px;}
    .mini .box{padding:10px; border:1px solid var(--border); border-radius:12px; background: rgba(0,0,0,.12);}
    .mini .val{font-size:18px; font-weight:900; margin-top:2px}
    .na{color:var(--muted); font-weight:800}

    .tabs{display:flex; gap:10px; flex-wrap:wrap; margin-top:10px}
    .tab{
      padding:9px 12px; border-radius:12px;
      border:1px solid var(--border);
      background: rgba(255,255,255,.04);
      color:var(--text);
      cursor:pointer;
      font-weight:900;
    }
    .tab.active{outline:2px solid rgba(255,255,255,.18)}

    button{
      padding:9px 12px; border-radius:12px;
      border:1px solid var(--border);
      background: rgba(255,255,255,.05);
      color:var(--text);
      cursor:pointer;
      font-weight:800;
    }

    canvas{
      width:100%;
      height:320px;
      border-radius:var(--r);
      background: rgba(0,0,0,.15);
      border:1px solid var(--border);
      display:block;
      margin-top:10px;
      touch-action: none;
    }
    .canvasSmall{ height:220px; }

    table{width:100%; border-collapse:collapse; overflow:hidden; border-radius:12px; margin-top:10px}
    th,td{padding:8px 10px; border-bottom:1px solid var(--border); font-size:13px}
    th{color:var(--muted); text-align:left; font-weight:900}
    td{color:var(--text)}
    .foot{color:var(--muted); font-size:12px; margin-top:10px}
    .err{color: var(--bad); font-weight:900}

    .legend{
      display:flex; gap:12px; flex-wrap:wrap;
      margin-top:10px; align-items:center; justify-content:space-between;
    }
    .legendLeft{display:flex; gap:12px; flex-wrap:wrap; align-items:center}
    .legend .item{display:flex; align-items:center; gap:8px; color:var(--muted); font-size:12px}
    .swatch{width:14px; height:6px; border-radius:999px}
    .swatch.b1{background:var(--b1)}
    .swatch.b2{background:var(--b2)}
    .swatch.b3{background:var(--b3)}
    .swatch.o2{background:var(--o2)}
    .livevals{color:var(--muted); font-size:12px}

    .btnRow{display:flex; gap:10px; flex-wrap:wrap; margin-top:10px}
    .hidden{display:none !important;}
  </style>
</head>

<body>
<div class="wrap">
  <header>
    <div>
      <h1>IHM Station de compostage — 3 bacs</h1>
      <div class="sub">ESP32 en point d’accès • Page embarquée</div>
    </div>
    <div class="pills">
      <div class="pill"><span class="dot" id="dotConn"></span><span id="connTxt">Connexion : —</span></div>
      <div class="pill">Dernier update : <span id="lastSeen">—</span></div>
      <div class="pill">t (s) : <span id="ts">—</span></div>
    </div>
  </header>

  <div class="grid">
    <div class="card span-4" id="cardB1">
      <div class="row" style="justify-content:space-between">
        <div>
          <div class="k">Bac 1</div>
          <div class="foot">Temp / Hum / O₂</div>
        </div>
        <div class="badge" id="b1Badge">État : <strong id="b1State">—</strong></div>
      </div>
      <div class="mini">
        <div class="box">
          <div class="k">Température</div>
          <div class="val"><span id="b1Temp">--</span><span class="unit">°C</span></div>
        </div>
        <div class="box">
          <div class="k">Humidité</div>
          <div class="val"><span id="b1Hum">--</span><span class="unit">%</span></div>
        </div>
        <div class="box" style="grid-column: span 2;">
          <div class="k">Oxygène</div>
          <div class="val"><span id="b1O2">--</span><span class="unit">%</span></div>
        </div>
      </div>
    </div>

    <div class="card span-4" id="cardB2">
      <div class="row" style="justify-content:space-between">
        <div>
          <div class="k">Bac 2</div>
          <div class="foot">Temp / Hum</div>
        </div>
        <div class="badge" id="b2Badge">État : <strong id="b2State">—</strong></div>
      </div>
      <div class="mini">
        <div class="box">
          <div class="k">Température</div>
          <div class="val"><span id="b2Temp">--</span><span class="unit">°C</span></div>
        </div>
        <div class="box">
          <div class="k">Humidité</div>
          <div class="val"><span id="b2Hum">--</span><span class="unit">%</span></div>
        </div>
      </div>
    </div>

    <div class="card span-4" id="cardB3">
      <div class="row" style="justify-content:space-between">
        <div>
          <div class="k">Bac 3</div>
          <div class="foot">Temp / Hum</div>
        </div>
        <div class="badge" id="b3Badge">État : <strong id="b3State">—</strong></div>
      </div>
      <div class="mini">
        <div class="box">
          <div class="k">Température</div>
          <div class="val"><span id="b3Temp">--</span><span class="unit">°C</span></div>
        </div>
        <div class="box">
          <div class="k">Humidité</div>
          <div class="val"><span id="b3Hum">--</span><span class="unit">%</span></div>
        </div>
      </div>
    </div>

    <!-- Zone graphes -->
    <div class="card span-8">
      <div class="row" style="justify-content:space-between">
        <div>
          <div class="k">Graphes</div>
          <div class="big" style="margin-top:2px">
            <span id="viewTitle">Bac <span id="selBacTitle">1</span></span>
          </div>
          <div class="foot" id="viewDesc">Vue Bac (détails)</div>
          <div class="foot">Astuce : survole/touche le graphe pour lire les valeurs.</div>
        </div>

        <div class="row">
          <div class="tabs">
            <button class="tab active" id="viewBacBtn">Vue Bac</button>
            <button class="tab" id="viewCmpBtn">Comparaison</button>
          </div>
          <div class="tabs" id="bacTabs">
            <button class="tab active" id="tab1">Bac 1</button>
            <button class="tab" id="tab2">Bac 2</button>
            <button class="tab" id="tab3">Bac 3</button>
          </div>
          <button id="reload">Recharger l'historique</button>
          <button id="pauseBtn">Pause</button>
        </div>
      </div>

      <!-- Vue Bac -->
      <div id="viewBac">
        <canvas id="plotBac"></canvas>
        <div class="legend">
          <div class="legendLeft">
            <div class="item"><span class="swatch b1"></span>Temp</div>
            <div class="item"><span class="swatch b2"></span>Hum</div>
            <div class="item" id="legendO2Bac"><span class="swatch o2"></span>O₂ (Bac 1)</div>
          </div>
          <div class="livevals" id="liveVals">—</div>
        </div>
      </div>

      <!-- Vue Comparaison -->
      <div id="viewCmp" class="hidden">
        <div class="foot">Température (3 bacs)</div>
        <canvas id="plotTemp" class="canvasSmall"></canvas>

        <div class="foot" style="margin-top:10px;">Humidité (3 bacs)</div>
        <canvas id="plotHum" class="canvasSmall"></canvas>

        <div class="foot" style="margin-top:10px;">O₂ (Bac 1)</div>
        <canvas id="plotO2" class="canvasSmall"></canvas>

        <div class="legend">
          <div class="legendLeft">
            <div class="item"><span class="swatch b1"></span>Bac 1</div>
            <div class="item"><span class="swatch b2"></span>Bac 2</div>
            <div class="item"><span class="swatch b3"></span>Bac 3</div>
            <div class="item"><span class="swatch o2"></span>O₂</div>
          </div>
          <div class="livevals" id="cmpVals">—</div>
        </div>
      </div>

      <div class="foot" id="hint"></div>
      <div class="err" id="err"></div>
    </div>

    <!-- Table + CSV -->
    <div class="card span-4">
      <div class="k">Dernières mesures</div>
      <div class="foot">Table du bac sélectionné (10 dernières)</div>
      <table>
        <thead>
          <tr><th>t (s)</th><th>Temp</th><th>Hum</th><th>O₂</th></tr>
        </thead>
        <tbody id="rows"></tbody>
      </table>

      <div class="btnRow">
        <button id="csvBtn">Télécharger CSV (Bac sélectionné)</button>
      </div>

      <div class="foot">CSV = historique complet depuis la 1ère mesure.</div>
    </div>
  </div>

  <div class="foot">
    WiFi <strong>ESP32_AP</strong> puis <strong>http://192.168.10.1/</strong>
  </div>
</div>

<script>
  // ========= DOM =========
  const dotConn  = document.getElementById('dotConn');
  const connTxt  = document.getElementById('connTxt');
  const lastSeen = document.getElementById('lastSeen');
  const elTs     = document.getElementById('ts');

  const b1Temp = document.getElementById('b1Temp');
  const b1Hum  = document.getElementById('b1Hum');
  const b1O2   = document.getElementById('b1O2');
  const b2Temp = document.getElementById('b2Temp');
  const b2Hum  = document.getElementById('b2Hum');
  const b3Temp = document.getElementById('b3Temp');
  const b3Hum  = document.getElementById('b3Hum');

  const b1Badge = document.getElementById('b1Badge');
  const b2Badge = document.getElementById('b2Badge');
  const b3Badge = document.getElementById('b3Badge');
  const b1State = document.getElementById('b1State');
  const b2State = document.getElementById('b2State');
  const b3State = document.getElementById('b3State');

  const selBacTitle = document.getElementById('selBacTitle');
  const viewDesc  = document.getElementById('viewDesc');

  const liveVals = document.getElementById('liveVals');
  const cmpVals  = document.getElementById('cmpVals');
  const legendO2Bac = document.getElementById('legendO2Bac');

  const viewBacBtn = document.getElementById('viewBacBtn');
  const viewCmpBtn = document.getElementById('viewCmpBtn');
  const viewBacDiv = document.getElementById('viewBac');
  const viewCmpDiv = document.getElementById('viewCmp');
  const bacTabsDiv = document.getElementById('bacTabs');

  const tab1 = document.getElementById('tab1');
  const tab2 = document.getElementById('tab2');
  const tab3 = document.getElementById('tab3');

  const reloadBtn = document.getElementById('reload');
  const pauseBtn  = document.getElementById('pauseBtn');
  const csvBtn    = document.getElementById('csvBtn');

  const errEl  = document.getElementById('err');
  const hintEl = document.getElementById('hint');
  const tbody  = document.getElementById('rows');

  // canvases
  const canvasBac  = document.getElementById('plotBac');
  const ctxBac     = canvasBac.getContext('2d');

  const canvasTemp = document.getElementById('plotTemp');
  const ctxTemp    = canvasTemp.getContext('2d');

  const canvasHum  = document.getElementById('plotHum');
  const ctxHum     = canvasHum.getContext('2d');

  const canvasO2   = document.getElementById('plotO2');
  const ctxO2      = canvasO2.getContext('2d');

  let history = [];
  let selected = 1;
  let paused = false;
  let lastMsgMs = 0;

  let viewMode = 'bac'; // 'bac'|'cmp'
  let hoverIdxBac = null;
  let hoverIdxCmp = null;

  function nowLocal(){ return new Date().toLocaleTimeString(); }
  function setConn(state){
    dotConn.className = 'dot ' + state;
    connTxt.textContent = (state==='ok')?'Connexion : LIVE':(state==='warn')?'Connexion : instable':'Connexion : perdue';
  }

  // ===== Badges simples =====
  function rank(lvl){ return lvl==='bad'?2 : lvl==='warn'?1 : lvl==='ok'?0 : -1; }
  function worstOf(levels){
    const xs = levels.filter(x=>x);
    if(xs.length===0) return null;
    return xs.sort((a,b)=>rank(b)-rank(a))[0];
  }
  function evalTemp(t){ if(!isFinite(t)) return {lvl:null}; if(t>=65) return {lvl:'bad'}; if(t>=55) return {lvl:'warn'}; return {lvl:'ok'}; }
  function evalHum(h){ if(!isFinite(h)) return {lvl:null}; if(h<=35||h>=85) return {lvl:'bad'}; if(h<=45||h>=75) return {lvl:'warn'}; return {lvl:'ok'}; }
  function evalO2(o){ if(!isFinite(o)) return {lvl:null}; if(o<=18.5) return {lvl:'bad'}; if(o<=19.5) return {lvl:'warn'}; return {lvl:'ok'}; }
  function stateBadge(elBadge, elText, lvl){
    elText.textContent = lvl==='bad'?'ALERTE':lvl==='warn'?'WARNING':lvl==='ok'?'OK':'—';
    elBadge.classList.remove('ok','warn','bad');
    if(lvl) elBadge.classList.add(lvl);
  }

  function updateCards(d){
    elTs.textContent = d.t ?? '--';

    const b1 = d.b1 || {};
    const b1t = Number(b1.tempC), b1h = Number(b1.humPct), b1o = Number(b1.o2Pct);
    b1Temp.textContent = isFinite(b1t)? b1t.toFixed(2) : '--';
    b1Hum.textContent  = isFinite(b1h)? b1h.toFixed(2) : '--';
    b1O2.textContent   = isFinite(b1o)? b1o.toFixed(2) : '--';
    stateBadge(b1Badge, b1State, worstOf([evalTemp(b1t).lvl, evalHum(b1h).lvl, evalO2(b1o).lvl]));

    const b2 = d.b2 || {};
    const b2t = Number(b2.tempC), b2h = Number(b2.humPct);
    b2Temp.textContent = isFinite(b2t)? b2t.toFixed(2) : '--';
    b2Hum.textContent  = isFinite(b2h)? b2h.toFixed(2) : '--';
    stateBadge(b2Badge, b2State, worstOf([evalTemp(b2t).lvl, evalHum(b2h).lvl]));

    const b3 = d.b3 || {};
    const b3t = Number(b3.tempC), b3h = Number(b3.humPct);
    b3Temp.textContent = isFinite(b3t)? b3t.toFixed(2) : '--';
    b3Hum.textContent  = isFinite(b3h)? b3h.toFixed(2) : '--';
    stateBadge(b3Badge, b3State, worstOf([evalTemp(b3t).lvl, evalHum(b3h).lvl]));
  }

  // ========= View mode =========
  function setViewMode(mode){
    viewMode = mode;
    viewBacBtn.classList.toggle('active', mode==='bac');
    viewCmpBtn.classList.toggle('active', mode==='cmp');

    viewBacDiv.classList.toggle('hidden', mode!=='bac');
    viewCmpDiv.classList.toggle('hidden', mode!=='cmp');
    bacTabsDiv.classList.toggle('hidden', mode!=='bac');

    viewDesc.textContent = (mode==='bac') ? 'Vue Bac (détails)' : 'Comparaison des bacs';
    hoverIdxBac = null;
    hoverIdxCmp = null;
    redrawAll();
    if (history.length) updateCards(history[history.length - 1]);

  }
  viewBacBtn.addEventListener('click', ()=>setViewMode('bac'));
  viewCmpBtn.addEventListener('click', ()=>setViewMode('cmp'));

  // ========= Bac selection =========
  function setSelected(n){
    selected = n;
    selBacTitle.textContent = String(n);
    tab1.classList.toggle('active', n===1);
    tab2.classList.toggle('active', n===2);
    tab3.classList.toggle('active', n===3);

    legendO2Bac.style.display = (n===1) ? 'flex' : 'none';

    hoverIdxBac = null;
    refreshTable();
    redrawAll();
  }

  tab1.addEventListener('click', ()=>setSelected(1));
  tab2.addEventListener('click', ()=>setSelected(2));
  tab3.addEventListener('click', ()=>setSelected(3));
  document.getElementById('cardB1').addEventListener('click', ()=>setSelected(1));
  document.getElementById('cardB2').addEventListener('click', ()=>setSelected(2));
  document.getElementById('cardB3').addEventListener('click', ()=>setSelected(3));

  // ========= Canvas utils =========
  function resizeCanvasToDisplaySize(canvas){
    const dpr = window.devicePixelRatio || 1;
    const rect = canvas.getBoundingClientRect();
    const wantW = Math.max(300, Math.floor(rect.width * dpr));
    const wantH = Math.max(180, Math.floor(rect.height * dpr));
    if(canvas.width !== wantW || canvas.height !== wantH){
      canvas.width = wantW;
      canvas.height = wantH;
    }
  }

  function minMax(arr){
    const fin = arr.filter(v => Number.isFinite(v));
    if(fin.length === 0) return {lo:0, hi:1};
    let lo = Math.min(...fin), hi = Math.max(...fin);
    if(Math.abs(hi-lo) < 1e-9){ hi = lo + 1; }
    return {lo, hi};
  }

  function roundRect(ctx, x, y, w, h, r){
    const rr = Math.min(r, w/2, h/2);
    ctx.moveTo(x+rr, y);
    ctx.arcTo(x+w, y,   x+w, y+h, rr);
    ctx.arcTo(x+w, y+h, x,   y+h, rr);
    ctx.arcTo(x,   y+h, x,   y,   rr);
    ctx.arcTo(x,   y,   x+w, y,   rr);
    ctx.closePath();
  }

  function drawGridAxes(ctx, w, h, L, R, T, B){
    const PW = w - L - R;
    const PH = h - T - B;
    const gridX = 6, gridY = 5;

    ctx.fillStyle = 'rgba(255,255,255,0.02)';
    ctx.fillRect(0,0,w,h);

    ctx.strokeStyle = 'rgba(255,255,255,0.10)';
    ctx.lineWidth = 1;
    for(let i=0;i<=gridY;i++){
      const y = T + (PH * i / gridY);
      ctx.beginPath(); ctx.moveTo(L, y); ctx.lineTo(L+PW, y); ctx.stroke();
    }
    for(let i=0;i<=gridX;i++){
      const x = L + (PW * i / gridX);
      ctx.beginPath(); ctx.moveTo(x, T); ctx.lineTo(x, T+PH); ctx.stroke();
    }

    ctx.strokeStyle = 'rgba(255,255,255,0.25)';
    ctx.lineWidth = 1.5;
    ctx.beginPath(); ctx.moveTo(L, T); ctx.lineTo(L, T+PH); ctx.lineTo(L+PW, T+PH); ctx.stroke();

    return {PW, PH};
  }

  function drawTicksX(ctx, dpr, L, T, PH, PW, times){
    const n = times.length;
    if(n < 2) return;

    const gridX = 6;
    const xOfIdx = (i)=> L + (PW * (i/(n-1)));

    ctx.fillStyle = 'rgba(255,255,255,0.50)';
    ctx.font = `${Math.floor(11*dpr)}px system-ui`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'top';
    for(let i=0;i<=gridX;i++){
      const idx = Math.round((n-1) * (i/gridX));
      ctx.fillText(String(times[idx]), xOfIdx(idx), T+PH + Math.floor(8*dpr));
    }
    ctx.textAlign = 'left';
    ctx.fillText('t (s)', L, T+PH + Math.floor(26*dpr));
  }

  function drawLine(ctx, dpr, L, T, PW, PH, arr, vMin, vMax, stroke, width, dashed){
    const n = arr.length;
    if(n < 2) return;
    const xOfIdx = (i)=> L + (PW * (i/(n-1)));
    const yOf = (v)=> T + PH * (1 - (v - vMin) / (vMax - vMin));

    ctx.save();
    ctx.strokeStyle = stroke;
    ctx.lineWidth = width;
    ctx.setLineDash(dashed ? [Math.floor(6*dpr), Math.floor(5*dpr)] : []);
    ctx.beginPath();
    let started = false;
    for(let i=0;i<n;i++){
      const v = arr[i];
      if(!Number.isFinite(v)) continue;
      const x = xOfIdx(i);
      const y = yOf(v);
      if(!started){ ctx.moveTo(x,y); started=true; } else ctx.lineTo(x,y);
    }
    ctx.stroke();
    ctx.restore();
  }

  function drawMarker(ctx, dpr, L, T, PW, PH, i, arr, vMin, vMax, fill){
    const n = arr.length;
    if(n < 2) return;
    const v = arr[i];
    if(!Number.isFinite(v)) return;
    const x = L + (PW * (i/(n-1)));
    const y = T + PH * (1 - (v - vMin) / (vMax - vMin));
    ctx.beginPath();
    ctx.fillStyle = fill;
    ctx.arc(x, y, Math.floor(4.2*dpr), 0, Math.PI*2);
    ctx.fill();
    ctx.strokeStyle = 'rgba(0,0,0,0.35)';
    ctx.lineWidth = Math.floor(1.2*dpr);
    ctx.stroke();
  }

  function drawTooltip(ctx, dpr, L, T, PW, PH, x, y, lines){
    ctx.font = `${Math.floor(12*dpr)}px system-ui`;
    const pad = Math.floor(10*dpr);
    let boxW = 0;
    for(const s of lines) boxW = Math.max(boxW, ctx.measureText(s).width);
    boxW = Math.floor(boxW + pad*2);
    const lineH = Math.floor(16*dpr);
    const boxH = Math.floor(lines.length*lineH + pad*2);

    let bx = x + Math.floor(12*dpr);
    let by = y;
    if(bx + boxW > L+PW) bx = x - boxW - Math.floor(12*dpr);
    if(by + boxH > T+PH) by = T + Math.floor(12*dpr);

    ctx.fillStyle = 'rgba(10,16,26,0.92)';
    ctx.strokeStyle = 'rgba(255,255,255,0.15)';
    ctx.lineWidth = Math.floor(1*dpr);
    ctx.beginPath(); roundRect(ctx, bx, by, boxW, boxH, Math.floor(10*dpr));
    ctx.fill(); ctx.stroke();

    ctx.fillStyle = 'rgba(255,255,255,0.88)';
    let ty = by + pad + lineH - Math.floor(4*dpr);
    for(const s of lines){
      ctx.fillText(s, bx + pad, ty);
      ty += lineH;
    }
  }

  // ========= Séries =========
  function seriesBac(bac){
    const key = 'b'+bac;
    return {
      t: history.map(p=>p.t),
      temp: history.map(p=>Number((p[key]||{}).tempC)),
      hum:  history.map(p=>Number((p[key]||{}).humPct)),
      o2:   history.map(p=>Number((p[key]||{}).o2Pct))
    };
  }

  // ========= Vue Bac =========
  function redrawBac(){
    resizeCanvasToDisplaySize(canvasBac);
    const w = canvasBac.width, h = canvasBac.height;
    const dpr = window.devicePixelRatio || 1;

    const L = Math.floor(52 * dpr), R = Math.floor(16 * dpr), T = Math.floor(16 * dpr), B = Math.floor(40 * dpr);

    ctxBac.clearRect(0,0,w,h);
    const {PW, PH} = drawGridAxes(ctxBac, w, h, L, R, T, B);

    if(history.length < 2){
      ctxBac.fillStyle = 'rgba(255,255,255,0.55)';
      ctxBac.font = `${Math.floor(12*dpr)}px system-ui`;
      ctxBac.fillText('En attente de données…', L + Math.floor(10*dpr), T + Math.floor(20*dpr));
      return;
    }

    const S = seriesBac(selected);
    const n = S.t.length;

    const rT = minMax(S.temp);
    const rH = minMax(S.hum);
    const rO = minMax(S.o2);

    drawTicksX(ctxBac, dpr, L, T, PH, PW, S.t);

    const colTemp = 'rgba(251,113,133,0.95)';
    const colHum  = 'rgba(96,165,250,0.95)';
    const colO2   = 'rgba(167,139,250,0.95)';

    drawLine(ctxBac, dpr, L, T, PW, PH, S.temp, rT.lo-0.5, rT.hi+0.5, colTemp, Math.floor(2.4*dpr), false);
    drawLine(ctxBac, dpr, L, T, PW, PH, S.hum,  rH.lo-1,   rH.hi+1,   colHum,  Math.floor(2.0*dpr), false);
    if(selected===1){
      drawLine(ctxBac, dpr, L, T, PW, PH, S.o2,  rO.lo-0.1, rO.hi+0.1, colO2,   Math.floor(1.8*dpr), true);
    }

    const lastI = n-1;
    drawMarker(ctxBac, dpr, L, T, PW, PH, lastI, S.temp, rT.lo-0.5, rT.hi+0.5, colTemp);
    drawMarker(ctxBac, dpr, L, T, PW, PH, lastI, S.hum,  rH.lo-1,   rH.hi+1,   colHum);
    if(selected===1) drawMarker(ctxBac, dpr, L, T, PW, PH, lastI, S.o2, rO.lo-0.1, rO.hi+0.1, colO2);

    if(hoverIdxBac !== null){
      const i = Math.max(0, Math.min(n-1, hoverIdxBac));
      const x = L + (PW * (i/(n-1)));

      ctxBac.strokeStyle = 'rgba(255,255,255,0.35)';
      ctxBac.lineWidth = Math.floor(1.2*dpr);
      ctxBac.setLineDash([Math.floor(4*dpr), Math.floor(4*dpr)]);
      ctxBac.beginPath(); ctxBac.moveTo(x, T); ctxBac.lineTo(x, T+PH); ctxBac.stroke();
      ctxBac.setLineDash([]);

      drawMarker(ctxBac, dpr, L, T, PW, PH, i, S.temp, rT.lo-0.5, rT.hi+0.5, colTemp);
      drawMarker(ctxBac, dpr, L, T, PW, PH, i, S.hum,  rH.lo-1,   rH.hi+1,   colHum);
      if(selected===1) drawMarker(ctxBac, dpr, L, T, PW, PH, i, S.o2, rO.lo-0.1, rO.hi+0.1, colO2);

      const tt = S.t[i];
      const vT = S.temp[i], vH = S.hum[i], vO = S.o2[i];
      const lines = (selected===1)
        ? [`t: ${tt} s`, `Temp: ${Number.isFinite(vT)?vT.toFixed(2):'--'} °C`, `Hum: ${Number.isFinite(vH)?vH.toFixed(2):'--'} %`, `O₂: ${Number.isFinite(vO)?vO.toFixed(2):'--'} %`]
        : [`t: ${tt} s`, `Temp: ${Number.isFinite(vT)?vT.toFixed(2):'--'} °C`, `Hum: ${Number.isFinite(vH)?vH.toFixed(2):'--'} %`];

      drawTooltip(ctxBac, dpr, L, T, PW, PH, x, T+Math.floor(12*dpr), lines);
    }

    if(history.length){
      const last = history[history.length-1];
      const b = last['b'+selected] || {};
      const t = Number(b.tempC), h = Number(b.humPct), o = Number(b.o2Pct);
      liveVals.textContent = (selected===1)
        ? `Temp: ${Number.isFinite(t)?t.toFixed(2):'--'} °C • Hum: ${Number.isFinite(h)?h.toFixed(2):'--'} % • O₂: ${Number.isFinite(o)?o.toFixed(2):'--'} %`
        : `Temp: ${Number.isFinite(t)?t.toFixed(2):'--'} °C • Hum: ${Number.isFinite(h)?h.toFixed(2):'--'} %`;
    }
  }

  // ========= Comparaison =========
  function redrawCompareOne(canvas, ctx, title, field){
    resizeCanvasToDisplaySize(canvas);
    const w = canvas.width, h = canvas.height;
    const dpr = window.devicePixelRatio || 1;

    const L = Math.floor(52 * dpr), R = Math.floor(16 * dpr), T = Math.floor(16 * dpr), B = Math.floor(40 * dpr);

    ctx.clearRect(0,0,w,h);
    const {PW, PH} = drawGridAxes(ctx, w, h, L, R, T, B);

    if(history.length < 2){
      ctx.fillStyle = 'rgba(255,255,255,0.55)';
      ctx.font = `${Math.floor(12*dpr)}px system-ui`;
      ctx.fillText('En attente de données…', L + Math.floor(10*dpr), T + Math.floor(20*dpr));
      return;
    }

    const times = history.map(p=>p.t);
    const b1 = history.map(p=>Number((p.b1||{})[field]));
    const b2 = history.map(p=>Number((p.b2||{})[field]));
    const b3 = history.map(p=>Number((p.b3||{})[field]));

    const all = b1.concat(b2).concat(b3);
    const r = minMax(all);

    drawTicksX(ctx, dpr, L, T, PH, PW, times);

    const colB1 = 'rgba(251,113,133,0.95)';
    const colB2 = 'rgba(96,165,250,0.95)';
    const colB3 = 'rgba(52,211,153,0.95)';

    drawLine(ctx, dpr, L, T, PW, PH, b1, r.lo, r.hi, colB1, Math.floor(2.2*dpr), false);
    drawLine(ctx, dpr, L, T, PW, PH, b2, r.lo, r.hi, colB2, Math.floor(2.2*dpr), false);
    drawLine(ctx, dpr, L, T, PW, PH, b3, r.lo, r.hi, colB3, Math.floor(2.2*dpr), false);

    const lastI = times.length-1;
    drawMarker(ctx, dpr, L, T, PW, PH, lastI, b1, r.lo, r.hi, colB1);
    drawMarker(ctx, dpr, L, T, PW, PH, lastI, b2, r.lo, r.hi, colB2);
    drawMarker(ctx, dpr, L, T, PW, PH, lastI, b3, r.lo, r.hi, colB3);

    ctx.fillStyle = 'rgba(255,255,255,0.55)';
    ctx.font = `${Math.floor(11*dpr)}px system-ui`;
    ctx.fillText(title, L, T + Math.floor(14*dpr));

    if(hoverIdxCmp !== null){
      const n = times.length;
      const i = Math.max(0, Math.min(n-1, hoverIdxCmp));
      const x = L + (PW * (i/(n-1)));

      ctx.strokeStyle = 'rgba(255,255,255,0.35)';
      ctx.lineWidth = Math.floor(1.2*dpr);
      ctx.setLineDash([Math.floor(4*dpr), Math.floor(4*dpr)]);
      ctx.beginPath(); ctx.moveTo(x, T); ctx.lineTo(x, T+PH); ctx.stroke();
      ctx.setLineDash([]);

      drawMarker(ctx, dpr, L, T, PW, PH, i, b1, r.lo, r.hi, colB1);
      drawMarker(ctx, dpr, L, T, PW, PH, i, b2, r.lo, r.hi, colB2);
      drawMarker(ctx, dpr, L, T, PW, PH, i, b3, r.lo, r.hi, colB3);

      const tt = times[i];
      const v1 = b1[i], v2 = b2[i], v3 = b3[i];
      const suffix = (field==='tempC') ? '°C' : '%';
      const lines = [
        `t: ${tt} s`,
        `Bac 1: ${Number.isFinite(v1)?v1.toFixed(2):'--'} ${suffix}`,
        `Bac 2: ${Number.isFinite(v2)?v2.toFixed(2):'--'} ${suffix}`,
        `Bac 3: ${Number.isFinite(v3)?v3.toFixed(2):'--'} ${suffix}`,
      ];

      drawTooltip(ctx, dpr, L, T, PW, PH, x, T+Math.floor(12*dpr), lines);
      cmpVals.textContent = `${title} @ t=${tt}s  →  B1:${Number.isFinite(v1)?v1.toFixed(2):'--'}  B2:${Number.isFinite(v2)?v2.toFixed(2):'--'}  B3:${Number.isFinite(v3)?v3.toFixed(2):'--'}`;
    } else {
      const n = times.length;
      const v1 = b1[n-1], v2 = b2[n-1], v3 = b3[n-1];
      cmpVals.textContent = `${title} (dernier)  →  B1:${Number.isFinite(v1)?v1.toFixed(2):'--'}  B2:${Number.isFinite(v2)?v2.toFixed(2):'--'}  B3:${Number.isFinite(v3)?v3.toFixed(2):'--'}`;
    }
  }

  function redrawO2(canvas, ctx){
    resizeCanvasToDisplaySize(canvas);
    const w = canvas.width, h = canvas.height;
    const dpr = window.devicePixelRatio || 1;

    const L = Math.floor(52 * dpr), R = Math.floor(16 * dpr), T = Math.floor(16 * dpr), B = Math.floor(40 * dpr);

    ctx.clearRect(0,0,w,h);
    const {PW, PH} = drawGridAxes(ctx, w, h, L, R, T, B);

    if(history.length < 2){
      ctx.fillStyle = 'rgba(255,255,255,0.55)';
      ctx.font = `${Math.floor(12*dpr)}px system-ui`;
      ctx.fillText('En attente de données…', L + Math.floor(10*dpr), T + Math.floor(20*dpr));
      return;
    }

    const times = history.map(p=>p.t);
    const o2 = history.map(p=>Number((p.b1||{}).o2Pct));
    const r = minMax(o2);

    drawTicksX(ctx, dpr, L, T, PH, PW, times);

    const col = 'rgba(167,139,250,0.95)';
    drawLine(ctx, dpr, L, T, PW, PH, o2, r.lo-0.1, r.hi+0.1, col, Math.floor(2.1*dpr), true);

    const lastI = times.length-1;
    drawMarker(ctx, dpr, L, T, PW, PH, lastI, o2, r.lo-0.1, r.hi+0.1, col);

    ctx.fillStyle = 'rgba(255,255,255,0.55)';
    ctx.font = `${Math.floor(11*dpr)}px system-ui`;
    ctx.fillText('O₂ (Bac 1) %', L, T + Math.floor(14*dpr));
  }

  function redrawCompare(){
    redrawCompareOne(canvasTemp, ctxTemp, 'Temp (°C)', 'tempC');
    redrawCompareOne(canvasHum,  ctxHum,  'Hum (%)',   'humPct');
    redrawO2(canvasO2, ctxO2);
  }

  function redrawAll(){
    if(viewMode==='bac') redrawBac();
    else redrawCompare();
  }

  // ========= Table =========
  function refreshTable(){
    const last = history.slice(-10).reverse();
    tbody.innerHTML = last.map(p => {
      const b = p['b'+selected] || {};
      const t = p.t ?? '';
      const temp = Number(b.tempC);
      const hum  = Number(b.humPct);
      const o2   = Number(b.o2Pct);
      const o2Txt = (selected===1 && Number.isFinite(o2)) ? o2.toFixed(2) : '—';
      return `<tr>
        <td>${t}</td>
        <td>${Number.isFinite(temp)? temp.toFixed(2) : '—'}</td>
        <td>${Number.isFinite(hum)? hum.toFixed(2) : '—'}</td>
        <td>${o2Txt}</td>
      </tr>`;
    }).join("");
  }

  async function loadHistory(){
    errEl.textContent = '';
    hintEl.textContent = '';
    try{
      const r = await fetch('/api/history', {cache:'no-store'});
      if(!r.ok) throw new Error('HTTP ' + r.status);
      history = await r.json();
      refreshTable();
      redrawAll();
      hintEl.textContent = `Historique RAM chargé : ${history.length} points`;
    }catch(e){
      errEl.textContent = 'Échec /api/history';
      hintEl.textContent = String(e);
    }
  }

  reloadBtn.addEventListener('click', loadHistory);
  pauseBtn.addEventListener('click', ()=>{
    paused = !paused;
    pauseBtn.textContent = paused ? 'Reprendre' : 'Pause';
  });

  csvBtn.addEventListener('click', ()=>{
  window.location.href = "/csv/data";
 });


  // Hover vue bac
  function setHoverBacFromClientX(clientX){
    if(history.length < 2) return;
    const rect = canvasBac.getBoundingClientRect();
    const x = clientX - rect.left;
    const n = history.length;
    const idx = Math.round((n-1) * (x / Math.max(1, rect.width)));
    hoverIdxBac = Math.max(0, Math.min(n-1, idx));
    redrawBac();
  }
  canvasBac.addEventListener('mousemove', (e)=> setHoverBacFromClientX(e.clientX));
  canvasBac.addEventListener('mouseleave', ()=>{ hoverIdxBac = null; redrawBac(); });
  canvasBac.addEventListener('touchstart', (e)=>{ if(e.touches?.length) setHoverBacFromClientX(e.touches[0].clientX); }, {passive:true});
  canvasBac.addEventListener('touchmove', (e)=>{ if(e.touches?.length) setHoverBacFromClientX(e.touches[0].clientX); }, {passive:true});
  canvasBac.addEventListener('touchend', ()=>{ hoverIdxBac = null; redrawBac(); }, {passive:true});

  // Hover comparaison (sync)
  function setHoverCmpFromClientX(clientX, canvas){
    if(history.length < 2) return;
    const rect = canvas.getBoundingClientRect();
    const x = clientX - rect.left;
    const n = history.length;
    const idx = Math.round((n-1) * (x / Math.max(1, rect.width)));
    hoverIdxCmp = Math.max(0, Math.min(n-1, idx));
    redrawCompare();
  }
  function bindCmpHover(canvas){
    canvas.addEventListener('mousemove', (e)=> setHoverCmpFromClientX(e.clientX, canvas));
    canvas.addEventListener('mouseleave', ()=>{ hoverIdxCmp = null; redrawCompare(); });
    canvas.addEventListener('touchstart', (e)=>{ if(e.touches?.length) setHoverCmpFromClientX(e.touches[0].clientX, canvas); }, {passive:true});
    canvas.addEventListener('touchmove', (e)=>{ if(e.touches?.length) setHoverCmpFromClientX(e.touches[0].clientX, canvas); }, {passive:true});
    canvas.addEventListener('touchend', ()=>{ hoverIdxCmp = null; redrawCompare(); }, {passive:true});
  }
  bindCmpHover(canvasTemp);
  bindCmpHover(canvasHum);
  bindCmpHover(canvasO2);

  // Mode buttons
  function setSelectedAndMode(bac){
    setSelected(bac);
    setViewMode('bac');
  }
  document.getElementById('cardB1').addEventListener('dblclick', ()=>setSelectedAndMode(1));
  document.getElementById('cardB2').addEventListener('dblclick', ()=>setSelectedAndMode(2));
  document.getElementById('cardB3').addEventListener('dblclick', ()=>setSelectedAndMode(3));

  // ===== SSE =====
  function startSSE(){
    setConn('warn');
    const es = new EventSource('/events');

    es.addEventListener('open', () => setConn('ok'));
    es.addEventListener('error', () => setConn('bad'));

    es.addEventListener('sample', (ev) => {
      if(paused) return;
      lastMsgMs = Date.now();

      const d = JSON.parse(ev.data);
      lastSeen.textContent = nowLocal();

      history.push(d);
      if(history.length > 300) history.shift();

      updateCards(d);
      refreshTable();
      redrawAll();
    });

    setInterval(()=>{
      const dt = Date.now() - lastMsgMs;
      if(dt > 12000) setConn('bad');
      else if(dt > 6000) setConn('warn');
    }, 2000);
  }

  function init(){
    setSelected(1);
    setViewMode('bac');
    loadHistory();
    startSSE();
    window.addEventListener('resize', ()=>redrawAll());
  }
  init();
</script>
</body>
</html>
)HTML";

#endif
