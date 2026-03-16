#pragma once

// Common CSS included in every page.
#define COMMON_STYLE \
"<style>" \
"body{background:#000;color:#ddd;font-family:serif;max-width:480px;margin:40px auto;padding:0 16px;text-align:center}" \
"h1{color:#8b0000;font-size:2rem;margin-bottom:4px}" \
"h2{color:#8b0000;font-size:1.4rem}" \
"a{color:#888;font-size:0.9rem}" \
"input[type=text],input[type=number],input[type=password]" \
"{background:#111;color:#ddd;border:1px solid #444;padding:8px;width:100%;box-sizing:border-box;font-size:1rem;margin:4px 0}" \
"button,input[type=submit]{background:#8b0000;color:#fff;border:none;padding:10px 24px;" \
"font-size:1rem;cursor:pointer;margin-top:8px;width:100%}" \
"button:active,input[type=submit]:active{background:#600}" \
"nav{margin-top:24px;display:flex;gap:12px;justify-content:center}" \
"label{display:block;text-align:left;margin-top:12px;font-size:0.85rem;color:#aaa}" \
"table{width:100%;border-collapse:collapse;font-size:0.85rem}" \
"td{padding:6px;border-bottom:1px solid #222;text-align:left}" \
"td:first-child{color:#aaa;white-space:nowrap}" \
"</style>"

// ---------------------------------------------------------------------------
// Home page: text input
// ---------------------------------------------------------------------------
const char HOME_HTML[] PROGMEM =
"<!DOCTYPE html><html><head>"
"<meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Ouija</title>"
COMMON_STYLE
"</head><body>"
"<h1>Ouija Board</h1>"
"<p style='color:#555;font-size:0.9rem'>The spirits are listening.</p>"
"<form action='/spell' method='POST'>"
"<input type='text' name='text' placeholder='Type your message...' autocomplete='off'>"
"<input type='submit' value='Speak'>"
"</form>"
"<nav>"
"<a href='/compass'>Compass mode</a>"
"<a href='/config'>Configuration</a>"
"<a href='/jog'>Jog</a>"
"</nav>"
"</body></html>";

// ---------------------------------------------------------------------------
// Compass page: phone orientation -> servo
// iOS 13+ requires an explicit permission request for DeviceOrientationEvent.
// ---------------------------------------------------------------------------
const char COMPASS_HTML[] PROGMEM =
"<!DOCTYPE html><html><head>"
"<meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Ouija - Compass</title>"
COMMON_STYLE
"</head><body>"
"<h1>Compass</h1>"
"<p style='color:#555;font-size:0.9rem'>Let your hand guide the spirits.</p>"
"<div id='status' style='color:#555;margin:16px 0'>Press the button to begin.</div>"
"<div id='alpha' style='font-size:3rem;color:#8b0000;margin:16px 0'>--</div>"
"<button id='btn' onclick='start()'>Begin</button>"
"<nav>"
"<a href='/'>Home</a>"
"<a href='/config'>Configuration</a>"
"</nav>"
"<script>"
"var active=false,noSleepVideo=null,lastRaw=null,contAlpha=0;"
"function enableNoSleep(){"
"  if(noSleepVideo)return;"
"  var canvas=document.createElement('canvas');"
"  canvas.width=1;canvas.height=1;"
"  canvas.getContext('2d').fillRect(0,0,1,1);"
"  noSleepVideo=document.createElement('video');"
"  noSleepVideo.setAttribute('playsinline','');"
"  noSleepVideo.setAttribute('muted','');"
"  noSleepVideo.srcObject=canvas.captureStream();"
"  noSleepVideo.play().catch(function(){});"
"}"
"function start(){"
"  if(typeof DeviceOrientationEvent!=='undefined'"
"  && typeof DeviceOrientationEvent.requestPermission==='function'){"
"    DeviceOrientationEvent.requestPermission().then(function(s){"
"      if(s==='granted'){activate();}"
"      else{document.getElementById('status').innerText='Permission denied.';}"
"    });"
"  }else{activate();}"
"}"
"function activate(){"
"  active=true;"
"  enableNoSleep();"
"  document.getElementById('btn').style.display='none';"
"  document.getElementById('status').innerText='Active';"
"  window.addEventListener('deviceorientation',function(e){"
"    var raw=e.alpha||0;"
"    if(lastRaw===null){contAlpha=raw;}else{"
"      var d=raw-lastRaw;"
"      if(d>180)d-=360;"
"      if(d<-180)d+=360;"
"      contAlpha+=d;"
"    }"
"    lastRaw=raw;"
"    document.getElementById('alpha').innerText=Math.round(contAlpha)+'deg';"
"  });"
"  setInterval(function(){"
"    if(!active)return;"
"    var a=document.getElementById('alpha').innerText;"
"    var val=parseInt(a);"
"    if(isNaN(val))return;"
"    var norm=((val%360)+360)%360;"
"    var xhr=new XMLHttpRequest();"
"    xhr.open('POST','/compass',true);"
"    xhr.setRequestHeader('Content-type','application/x-www-form-urlencoded');"
"    xhr.send('alpha='+norm);"
"  },100);"
"}"
"</script>"
"</body></html>";

// ---------------------------------------------------------------------------
// Config page: uses %PLACEHOLDERS% replaced by ESPAsyncWebServer template processor
// ---------------------------------------------------------------------------
const char CONFIG_HTML[] PROGMEM =
"<!DOCTYPE html><html><head>"
"<meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Ouija - Config</title>"
COMMON_STYLE
"</head><body>"
"<h1>Configuration</h1>"
"<form action='/config' method='POST'>"
"<label>Servo speed (PWM units/s)</label>"
"<input type='number' name='speed' value='%SPEED%'>"
"<label>Movement threshold (PWM units)</label>"
"<input type='number' name='threshold' value='%THRESHOLD%'>"
"<label>Compass start (degrees)</label>"
"<input type='number' name='compassStart' value='%COMPASS_START%'>"
"<label>Compass end (degrees)</label>"
"<input type='number' name='compassEnd' value='%COMPASS_END%'>"
"<label>Letter pause (ms)</label>"
"<input type='number' name='letterPause' value='%LETTER_PAUSE%'>"
"<label>Space pause (ms)</label>"
"<input type='number' name='spacePause' value='%SPACE_PAUSE%'>"
"<input type='submit' value='Save'>"
"</form>"
"<nav>"
"<a href='/'>Home</a>"
"<a href='/compass'>Compass mode</a>"
"<a href='/jog'>Jog</a>"
"</nav>"
"</body></html>";

// ---------------------------------------------------------------------------
// WiFi setup page (served in AP mode only)
// ---------------------------------------------------------------------------
const char WIFI_SETUP_HTML[] PROGMEM =
"<!DOCTYPE html><html><head>"
"<meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Ouija - WiFi Setup</title>"
COMMON_STYLE
"</head><body>"
"<h1>WiFi Setup</h1>"
"<p style='color:#555;font-size:0.9rem'>Connect the spirits to your network.</p>"
"<form action='/wifi-setup' method='POST'>"
"<label>Network name (SSID)</label>"
"<input type='text' name='ssid' placeholder='Your WiFi name'>"
"<label>Password</label>"
"<input type='password' name='password' placeholder='Your WiFi password'>"
"<input type='submit' value='Connect'>"
"</form>"
"</body></html>";

// ---------------------------------------------------------------------------
// Jog page: directly set a PWM value for calibration
// %PWM_MIN%, %PWM_MAX%, %PWM_CURRENT% are replaced at serve time.
// ---------------------------------------------------------------------------
const char JOG_HTML[] PROGMEM =
"<!DOCTYPE html><html><head>"
"<meta charset='utf-8'>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"<title>Ouija - Jog</title>"
COMMON_STYLE
"</head><body>"
"<h1>Jog</h1>"
"<p style='color:#555;font-size:0.9rem'>Send the planchette to an exact PWM position.</p>"
"<p style='color:#aaa;font-size:0.85rem'>Range: <b>%PWM_MIN%</b> &ndash; <b>%PWM_MAX%</b> &mu;s</p>"
"<form action='/jog' method='POST'>"
"<label>Target PWM (&mu;s)</label>"
"<input type='number' name='pwm' min='%PWM_MIN%' max='%PWM_MAX%' value='%PWM_CURRENT%'>"
"<input type='submit' value='Move'>"
"</form>"
"<nav>"
"<a href='/'>Home</a>"
"<a href='/config'>Configuration</a>"
"</nav>"
"</body></html>";

// ---------------------------------------------------------------------------
// Post-save confirmation (redirects home after 2s)
// ---------------------------------------------------------------------------
const char SAVED_HTML[] PROGMEM =
"<!DOCTYPE html><html><head>"
"<meta charset='utf-8'>"
"<meta http-equiv='refresh' content='2;url=/'>"
"<title>Saved</title>"
COMMON_STYLE
"</head><body>"
"<h1>Saved</h1>"
"<p style='color:#555'>Redirecting...</p>"
"</body></html>";
