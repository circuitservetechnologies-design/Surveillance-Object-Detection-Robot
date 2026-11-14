#pragma once
#include <Arduino.h>

// Keep all HTML, CSS and JavaScript in this separate header file.
// Do not paste this webpage content directly into the .ino file.
const char webpage[] PROGMEM = R"ROBOTHTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32-CAM Robot</title>
<style>
  * { box-sizing: border-box; }
  body {
    margin: 0; padding: 18px; background: #101820; color: #f4f7fb;
    font-family: Arial, sans-serif; text-align: center;
  }
  main { max-width: 720px; margin: auto; }
  h1 { margin: 8px 0 18px; font-size: 25px; }
  .panel {
    background: #1b2a36; border-radius: 14px; padding: 16px;
    margin: 14px 0; box-shadow: 0 4px 14px #0005;
  }
  #stream {
    width: 100%; max-width: 640px; min-height: 180px;
    background: #080d12; border-radius: 10px; object-fit: contain;
  }
  .controls {
    display: grid; grid-template-columns: repeat(3, 1fr);
    gap: 10px; max-width: 330px; margin: 14px auto 4px;
  }
  button {
    border: 0; border-radius: 10px; padding: 17px 10px;
    color: white; background: #1677d2; font-size: 19px;
    font-weight: bold; cursor: pointer; touch-action: none;
  }
  button:active { background: #0b4f91; }
  .stop { background: #c83232; }
  .action { font-size: 15px; margin: 5px; padding: 13px 18px; }
  #status { color: #9dd7ff; min-height: 20px; }
  .hint { color: #bdc9d3; font-size: 13px; line-height: 1.5; }
</style>
</head>
<body>
<main>
  <h1>Wi-Fi Controlled Surveillance Robot</h1>

  <section class="panel">
    <h3>Live Camera</h3>
    <img id="stream" alt="Camera stream will appear here">
    <p id="cameraStatus" class="hint">Connecting to camera...</p>
    <button class="action" onclick="capturePhoto()">Capture Photo</button>
  </section>

  <section class="panel">
    <h3>Robot Movement</h3>
    <div class="controls">
      <span></span>
      <button onpointerdown="startMove('F')" onpointerup="stopMove()"
              onpointerleave="stopMove()" onpointercancel="stopMove()">▲</button>
      <span></span>

      <button onpointerdown="startMove('L')" onpointerup="stopMove()"
              onpointerleave="stopMove()" onpointercancel="stopMove()">◀</button>
      <button class="stop" onclick="stopMove()">STOP</button>
      <button onpointerdown="startMove('R')" onpointerup="stopMove()"
              onpointerleave="stopMove()" onpointercancel="stopMove()">▶</button>

      <span></span>
      <button onpointerdown="startMove('B')" onpointerup="stopMove()"
              onpointerleave="stopMove()" onpointercancel="stopMove()">▼</button>
      <span></span>
    </div>
    <p id="status">Ready</p>
    <p class="hint">Press and hold a direction to move. Release to stop.</p>
  </section>
</main>

<script>
const streamImage = document.getElementById('stream');
const cameraStatus = document.getElementById('cameraStatus');
const statusText = document.getElementById('status');

streamImage.src = 'http://' + location.hostname + ':81/stream';
streamImage.onload = () => cameraStatus.textContent = 'Camera connected';
streamImage.onerror = () => cameraStatus.textContent =
  'Stream not available. Check the ESP32-CAM serial monitor and refresh this page.';

let activeDirection = '';
let commandBusy = false;

async function sendCommand(direction) {
  try {
    const response = await fetch('/move?dir=' + direction, { cache: 'no-store' });
    if (!response.ok) throw new Error('HTTP ' + response.status);
    statusText.textContent = direction === 'S' ? 'Stopped' : 'Moving: ' + direction;
  } catch (error) {
    statusText.textContent = 'Command failed: ' + error.message;
  }
}

function startMove(direction) {
  activeDirection = direction;
  sendCommand(direction);
}

function stopMove() {
  if (activeDirection !== '') {
    activeDirection = '';
    sendCommand('S');
  }
}

async function capturePhoto() {
  try {
    statusText.textContent = 'Capturing photo...';
    const response = await fetch('/capture', { cache: 'no-store' });
    if (!response.ok) throw new Error('Capture failed: HTTP ' + response.status);
    const blob = await response.blob();
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = 'robot_photo.jpg';
    document.body.appendChild(link);
    link.click();
    link.remove();
    URL.revokeObjectURL(url);
    statusText.textContent = 'Photo downloaded';
  } catch (error) {
    statusText.textContent = error.message;
  }
}

window.addEventListener('blur', stopMove);
document.addEventListener('visibilitychange', () => {
  if (document.hidden) stopMove();
});
</script>
</body>
</html>
)ROBOTHTML";
