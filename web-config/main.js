// UUIDs from ESP32 BleHandler.cpp
const SERVICE_UUID = "91bad492-b950-4226-aa2b-4ed124237670";
const CHAR_SSID_UUID = "91bad492-b950-4226-aa2b-4ed124237671";
const CHAR_PASS_UUID = "91bad492-b950-4226-aa2b-4ed124237672";
const CHAR_STATION_UUID = "91bad492-b950-4226-aa2b-4ed124237673";
const CHAR_REFRESH_UUID = "91bad492-b950-4226-aa2b-4ed124237674";
const CHAR_ACTION_UUID = "91bad492-b950-4226-aa2b-4ed124237675";

let device = null;
let server = null;
let service = null;

// UI Elements
const connectBtn = document.getElementById('connect-btn');
const writeBtn = document.getElementById('write-btn');
const writeRebootBtn = document.getElementById('write-reboot-btn');
const statusBadge = document.getElementById('status-badge');
const landingView = document.getElementById('landing-view');
const configView = document.getElementById('config-view');
const toastContainer = document.getElementById('toast-container');

// Inputs
const wifiSsidInput = document.getElementById('wifi-ssid');
const wifiPassInput = document.getElementById('wifi-pass');
const stationInput = document.getElementById('station-name');
const refreshInput = document.getElementById('refresh-rate');

// Mirror Elements
const mirrorStation = document.getElementById('mirror-station');

// --- UTILS ---

function showToast(message, type = 'info') {
  const toast = document.createElement('div');
  toast.className = `toast ${type}`;
  toast.textContent = message;
  toastContainer.appendChild(toast);
  setTimeout(() => toast.remove(), 4000);
}

function updateMirror() {
  mirrorStation.textContent = (stationInput.value || "ZÜRICH HB").toUpperCase();
}

stationInput.addEventListener('input', updateMirror);

// --- BLE LOGIC ---

async function connect() {
  try {
    statusBadge.textContent = "Connecting...";

    device = await navigator.bluetooth.requestDevice({
      filters: [{ name: 'SBB_Display_Config' }],
      optionalServices: [SERVICE_UUID]
    });

    device.addEventListener('gattserverdisconnected', onDisconnected);

    server = await device.gatt.connect();
    service = await server.getPrimaryService(SERVICE_UUID);

    showToast("Connected to SBB Display", "success");
    statusBadge.textContent = "Connected";
    statusBadge.classList.add('connected');

    // Initial Read
    await readAllSettings();

    // Switch View
    landingView.classList.remove('active');
    configView.classList.add('active');

  } catch (error) {
    console.error(error);
    statusBadge.textContent = "Disconnected";
    showToast("Connection failed: " + error.message, "error");
  }
}

function onDisconnected() {
  showToast("Device disconnected", "info");
  statusBadge.textContent = "Disconnected";
  statusBadge.classList.remove('connected');
  configView.classList.remove('active');
  landingView.classList.add('active');
}

async function readCharacteristic(uuid) {
  const char = await service.getCharacteristic(uuid);
  const value = await char.readValue();
  return new TextDecoder().decode(value);
}

async function writeCharacteristic(uuid, value) {
  const char = await service.getCharacteristic(uuid);
  const encoder = new TextEncoder();
  await char.writeValue(encoder.encode(value));
}

async function readAllSettings() {
  try {
    wifiSsidInput.value = await readCharacteristic(CHAR_SSID_UUID);
    stationInput.value = await readCharacteristic(CHAR_STATION_UUID);
    refreshInput.value = await readCharacteristic(CHAR_REFRESH_UUID);
    updateMirror();
  } catch (e) {
    console.warn("Could not read some characteristics", e);
  }
}

async function writeSettings(reboot = false) {
  const btn = reboot ? writeRebootBtn : writeBtn;
  btn.classList.add('btn-loading');
  btn.disabled = true;

  try {
    // 1. Write values
    await writeCharacteristic(CHAR_SSID_UUID, wifiSsidInput.value);

    if (wifiPassInput.value) {
      await writeCharacteristic(CHAR_PASS_UUID, wifiPassInput.value);
    }

    await writeCharacteristic(CHAR_STATION_UUID, stationInput.value);
    await writeCharacteristic(CHAR_REFRESH_UUID, refreshInput.value);

    // 2. Verify (Read back)
    showToast("Verifying settings...", "info");
    const readSsid = await readCharacteristic(CHAR_SSID_UUID);
    const readStation = await readCharacteristic(CHAR_STATION_UUID);

    if (readSsid === wifiSsidInput.value && readStation === stationInput.value) {
      showToast("Settings verified and saved!", "success");
    } else {
      throw new Error("Verification failed. Values mismatch.");
    }

    // 3. Optional Reboot
    if (reboot) {
      showToast("Rebooting device...", "info");
      await writeCharacteristic(CHAR_ACTION_UUID, "SAVE");
      // Device will disconnect automatically
    }

  } catch (error) {
    console.error(error);
    showToast("Write failed: " + error.message, "error");
  } finally {
    btn.classList.remove('btn-loading');
    btn.disabled = false;
  }
}

// --- EVENTS ---

connectBtn.addEventListener('click', connect);
writeBtn.addEventListener('click', () => writeSettings(false));
writeRebootBtn.addEventListener('click', () => writeSettings(true));
