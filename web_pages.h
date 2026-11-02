#pragma once
// Web UI. Same look and routes as the original project. Changes:
//  - buttons are built with textContent (antenna names can't inject HTML)
//  - a red banner appears if the device reports its I/O expander is missing

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Antenna Control</title>
<style>
body{background-color:#121212;color:#e0e0e0;font-family:Arial,sans-serif;text-align:center;padding:20px;margin:0}
h2{color:#00bcd4;margin-bottom:30px}
#buttons{display:flex;justify-content:center;flex-wrap:wrap;gap:15px;margin-bottom:30px}
button{padding:15px 25px;font-size:18px;border:none;border-radius:6px;cursor:pointer;min-width:160px;transition:background-color .3s,transform .1s}
.on{background-color:#4caf50;color:#fff}
.off{background-color:#f44336;color:#fff}
button:hover{opacity:.9;transform:scale(1.05)}
#settingsCog{position:fixed;top:10px;right:10px;font-size:28px;color:#00bcd4;cursor:pointer;user-select:none}
#settingsCog:hover{color:#80deea}
#warn{display:none;background:#b71c1c;color:#fff;padding:10px;border-radius:6px;max-width:500px;margin:0 auto 20px}
</style>
</head>
<body>
<div id="settingsCog" title="Settings" onclick="location.href='/settings'">&#9881;</div>
<h2>ESP32 Antenna Control</h2>
<div id="warn">I/O expander not responding - check wiring</div>
<div id="buttons"></div>
<script>
var websocket;
function initWebSocket() {
  websocket = new WebSocket(`ws://${window.location.hostname}/ws`);
  websocket.onmessage = function(event) {
    const data = JSON.parse(event.data);
    const box = document.getElementById("buttons");
    box.textContent = "";
    data.names.forEach(function(name, i) {
      const b = document.createElement("button");
      b.className = data.states[i] ? "on" : "off";
      b.textContent = name;
      b.onclick = function() { toggleRelay(i); };
      box.appendChild(b);
    });
    document.getElementById("warn").style.display = (data.io === false) ? "block" : "none";
  };
  websocket.onclose = function() { setTimeout(initWebSocket, 2000); };
}
function toggleRelay(id) {
  websocket.send(JSON.stringify({ action: "toggle", id: id }));
}
window.addEventListener('load', initWebSocket);
</script>
</body>
</html>
)rawliteral";

const char settings_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Settings - Antenna Names</title>
<style>
body{background-color:#121212;color:#e0e0e0;font-family:Arial,sans-serif;padding:20px;margin:0}
h2{color:#00bcd4;margin-bottom:20px;text-align:center}
form{max-width:400px;margin:0 auto}
label{display:block;margin:15px 0 5px 0}
input[type=text]{width:100%;padding:8px;font-size:16px;border-radius:5px;border:none;box-sizing:border-box}
button{margin-top:25px;width:100%;padding:12px;background-color:#00bcd4;border:none;border-radius:6px;font-size:18px;color:#fff;cursor:pointer;transition:background-color .3s}
button:hover{background-color:#008c9e}
a{color:#00bcd4;display:block;text-align:center;margin-top:15px;text-decoration:none}
a:hover{text-decoration:underline}
</style>
</head>
<body>
<h2>Settings - Rename Antennas</h2>
<form method="POST" action="/save-names">
<label for="name0">Antenna 1 Name:</label>
<input type="text" id="name0" name="name0" required maxlength="30">
<label for="name1">Antenna 2 Name:</label>
<input type="text" id="name1" name="name1" required maxlength="30">
<label for="name2">Antenna 3 Name:</label>
<input type="text" id="name2" name="name2" required maxlength="30">
<label for="name3">Antenna 4 Name:</label>
<input type="text" id="name3" name="name3" required maxlength="30">
<button type="submit">Save Names</button>
</form>
<a href="/">&#8592; Back to Control</a>
<script>
window.onload = function() {
  fetch('/get-names').then(r => r.json()).then(data => {
    for (let i = 0; i < 4; i++) document.getElementById('name' + i).value = data.names[i];
  });
}
</script>
</body>
</html>
)rawliteral";
