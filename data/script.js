//MARK: Update Local Time
function updateTime() {
  const now = new Date();
  const timeString = now.toLocaleTimeString("de-DE", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  });
  document.getElementById("current-time").innerText = timeString;
}

setInterval(updateTime, 1000);
setInterval(getHomeStatus, 1000);

//MARK: Section Control
function showSection(section) {
  // hide all sections
  document.querySelectorAll(".container").forEach(function (container) {
    container.style.display = "none";
  });

  // show selected section
  document.getElementById(section + "-content").style.display = "block";

  // set active class for selected section
  document.querySelectorAll("nav a").forEach(function (link) {
    link.classList.remove("active");
  });
  document.querySelector(`nav a[href="#${section}"]`).classList.add("active");

  handleSectionActivation(section);
}

// Function to handle data loading for each section
function handleSectionActivation(section) {
  if (section === "home") {
    console.log("load home elements");
    fetchCurrentHomeData();
  } else if (section === "timer") {
    console.log("load timer elements");
    fetchCurrentTimerData();
  } else if (section === "timeclock") {
    console.log("load timeclock elements");
    fetchCurrentTimeclockSettings();
  } else if (section === "settings") {
    console.log("load settings elements");
    loadSettings();
  } else if (section === "about") {
    console.log("load about elements");
    fetchESPInfo();
  }
}

// show home section by default
//showSection("home");
function loadData() {
  showSection("home");
}

//MARK: UI Controls
// Function to toggle wetaher settings options
function toggleWeatherForm() {
  var weatherChannel = document.getElementById("weather-channel").value;
  var openweatherForm = document.getElementById("openweather-form");
  var metehomaticsForm = document.getElementById("metehomatics-form");

  if (weatherChannel === "openweather") {
    openweatherForm.style.display = "block";
    metehomaticsForm.style.display = "none";
  } else if (weatherChannel === "metehomatics") {
    openweatherForm.style.display = "none";
    metehomaticsForm.style.display = "block";
  }
}

async function saveWetherSettings(event, formId) {
  event.preventDefault();
  const form = document.getElementById(formId);
  const formData = new FormData(form);
  const json = {};
  formData.forEach((value, key) => {
    json[key] = value;
  });

  try {
    const response = await fetch("/settings", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(json),
    });
    const result = await response.json();
    console.log("Success:", result.message);
  } catch (error) {
    console.error("Error sending settings:", error);
  }
  loadSettingsWeather();
}

async function loadSettingsWeather() {
  try {
    const response = await fetch("/settings");
    const data = await response.json();    
    
    // Weather API Settings
    document.getElementById("weather-channel").value = data.weather_channel;
    if (data.weather_channel === "openweather") {
      document.getElementById("openweather-form").style.display = "block";
      document.getElementById("metehomatics-form").style.display = "none";
      document.getElementById("wether-op-api-key-ow").value = data.openweather_api_key;
      document.getElementById("longitude-ow").value = data.longitude;
      document.getElementById("latitude-ow").value = data.latitude;
      document.getElementById("rain_duration_past_ow").value = data.rain_duration_past;
      document.getElementById("rain_duration_forecast_ow").value = data.rain_duration_forecast;
    } else if (data.weather_channel === "metehomatics") {
      document.getElementById("wether-op-api-key-meteo").value = data.openweather_api_key;
      document.getElementById("openweather-form").style.display = "none";
      document.getElementById("metehomatics-form").style.display = "block";
      document.getElementById("wether-metheo-user").value = data.metehomatics_user;
      document.getElementById("wether-metheo-pw").value = data.metehomatics_password;
      document.getElementById("longitude-meteo").value = data.longitude;
      document.getElementById("latitude-meteo").value = data.latitude;
      document.getElementById("rain_duration_past_meteo").value = data.rain_duration_past;
      document.getElementById("rain_duration_forecast_meteo").value = data.rain_duration_forecast;
    }
  } catch (error) {
    console.error("Error loading settings:", error);
  }
}

// Function to toggle timer options
function toggleTimerOptions() {
  const allTimersSelected = document.getElementById("select-all-timers").checked;
  document.getElementById("timer1-button").style.display = allTimersSelected ? "none" : "inline-block";
  document.getElementById("timer2-button").style.display = allTimersSelected ? "none" : "inline-block";
  document.getElementById("timer-all-button").style.display = allTimersSelected ? "inline-block" : "none";
}

// Function to reset timeclock settings to defaults
function resetTimeclockSettings() {
  const days = [
    "monday",
    "tuesday",
    "wednesday",
    "thursday",
    "friday",
    "saturday",
    "sunday",
  ];

  days.forEach((day) => {
    // Reset checkbox
    document.getElementById(`${day}-on-time-switch`).checked = false;

    // Reset time input
    document.getElementById(`${day}-on-time`).value = "";

    // Reset timer inputs
    document.getElementById(`${day}-timer1`).value = "30";
    document.getElementById(`${day}-timer2`).value = "30";

    // send "updated" settings
    saveTimeclockSettings();
  });
}

//MARK: Communication
// Emergency stop handler
async function emergency() {
  try {
    const response = await fetch("/emergency");
    const responseData = await response.text();
    console.log("Success:", responseData);
  } catch (error) {
    console.error("Error on emergency:", error);
  }
  alert("NOT-AUS aktiviert!");
}

//MARK: Timer
// Function to send timer
async function sendTimer(timer) {
  const timerValue = document.getElementById("timerslider").value;
  const timerOnTimeSwitch = document.getElementById("timer-on-time-switch").checked;
  const timerOnTime = document.getElementById("timer-on-time").value;
  const timmerAll = document.getElementById("select-all-timers").checked;

  const data = {
    timer: timer,
    value: timerValue,
    onTimeEN: timerOnTimeSwitch,
    onTime: timerOnTime,
    all: timmerAll,
  };

  try {
    const response = await fetch("/set-timer", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(data),
    });
    const responseData = await response.json();
    console.log("Success:", responseData.status);
  } catch (error) {
    console.error("Error on send timer:", error);
  }
}

// Function to fetch and update current timer data
async function fetchCurrentTimerData() {
  try {
    const response = await fetch(`/get-timer`);
    const data = await response.json();
    document.getElementById("timervalue").innerText = data.value;
    document.getElementById("timerslider").value = data.value;
    document.getElementById("timer-on-time-switch").checked = data.onTimeEN;
    document.getElementById("timer-on-time").value = data.onTime;
    document.getElementById("timervalue1").innerText = data.value;
  } catch (error) {
    console.error("Error on fetch current timer:", error);
  }
}

// Function to stop all timers
async function stopTimer() {
  try {
    const response = await fetch(`/stop-timer`);
    const responseData = await response.text();
    console.log("Success:", responseData);
  } catch (error) {
    console.error("Error on stop timer:", error);
  }
}

//MARK: Timetable
// Functions to fetch and update current timeclock data
function getDaySettings(day) {
  return {
    onTimeSwitch: document.getElementById(`${day}-on-time-switch`).checked,
    onTime: document.getElementById(`${day}-on-time`).value,
    timer1: document.getElementById(`${day}-timer1`).value,
    timer2: document.getElementById(`${day}-timer2`).value,
  };
}

async function saveTimeclockSettings() {
  const timeclockSettings = {
    monday: getDaySettings("monday"),
    tuesday: getDaySettings("tuesday"),
    wednesday: getDaySettings("wednesday"),
    thursday: getDaySettings("thursday"),
    friday: getDaySettings("friday"),
    saturday: getDaySettings("saturday"),
    sunday: getDaySettings("sunday"),
  };

  try {
    const response = await fetch(`/set-timeclock`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(timeclockSettings),
    });
    const responseData = await response.json();
    console.log("Success:", responseData.status);
  } catch (error) {
    console.error("Error on send timeclock:", error);
  }
}

// Function to fetch and update current timeclock data
async function fetchCurrentTimeclockSettings() {
  try {
    const response = await fetch(`/get-timeclock`);
    const data = await response.json();
    
    // Update each day's settings
    for (let day in data) {
      document.getElementById(`${day}-on-time-switch`).checked = data[day].onTimeSwitch;
      document.getElementById(`${day}-on-time`).value = data[day].onTime;
      document.getElementById(`${day}-timer1`).value = data[day].timer1;
      document.getElementById(`${day}-timer2`).value = data[day].timer2;
    }
  } catch (error) {
    console.error("Error on fetch timeclock:", error);
  }
}

//MARK: Home data
// Function to fetch and update current home data
function fetchCurrentHomeData() {
  getHomeStatus();
  getHomeTimetable();
}


async function getHomeStatus() {
  try {
    const response = await fetch("/home-status");
    const data = await response.json();
    document.getElementById("temperature").innerText = data.temperature + " °C";
    document.getElementById("humidity").innerText = data.humidity + " %";
    document.getElementById("ground_temp").innerText = data.ground_temp + " °C";
    document.getElementById("ground_humidity").innerText = data.ground_humidity + " %";
    document.getElementById("nds24h").innerText = data.nds24h + " mm";
    document.getElementById("nds24h-text").innerText = data.nds24h_text;
    document.getElementById("nds24h-next").innerText = data.nds24h_next + " mm";
    document.getElementById("nds24h-next-text").innerText = data.nds24h_next_text;
    document.getElementById("kreis1-timer-stat").innerText = data.kreis1_timer;
    document.getElementById("kreis2-timer-stat").innerText = data.kreis2_timer;
    document.getElementById("zeitsteuerung-aktiv").checked = data.time_control;
    document.getElementById("nds-data").checked = data.nds_active;
    document.getElementById("nds-data-forecast").checked = data.nds_active_forecast;
    document.getElementById("mention-sunset").checked = data.mentions_sunset;
    document.getElementById("pump-button").value = data.pump_button ? "Pumpe AUS" : "Pumpe AN";
    document.getElementById("stat_pumpe").innerText = data.pump_button ? "AN" : "AUS";
    document.getElementById("vent1-button").value = data.vent1_button ? "Ventil 1 AUS" : "Ventil 1 AN";
    document.getElementById("stat_kreis1").innerText = data.vent1_button ? "AN" : "AUS";
    document.getElementById("vent2-button").value = data.vent2_button ? "Ventil 2 AUS" : "Ventil 2 AN";
    document.getElementById("stat_kreis2").innerText = data.vent2_button ? "AN" : "AUS";
    setManualControlButtons();
  } catch (error) {
    console.error("Error on fetch home data:", error);
  }
}

function setManualControlButtons() {
  let button = document.getElementById("pump-button");
  if (button.value === "Pumpe AUS") {
    button.classList.remove('red');
    button.classList.add('green');
  } else {
    button.classList.remove('green');
    button.classList.add('red');
  }
  button = document.getElementById("vent1-button");
  if (button.value === "Ventil 1 AUS") {
    button.classList.remove('red');
    button.classList.add('green');
  } else {
    button.classList.remove('green');
    button.classList.add('red');
  }
  button = document.getElementById("vent2-button");
  if (button.value === "Ventil 2 AUS") {
    button.classList.remove('red');
    button.classList.add('green');
  } else {
    button.classList.remove('green');
    button.classList.add('red');
  }
}

async function getHomeTimetable() {
  try {
    const response = await fetch("/home-timetable");
    const data = await response.json();
    for (let day in data) {
      if (data[day].on === true) {
        document.getElementById(`enabled-${day}`).innerText = "AN";
      } else {
        document.getElementById(`enabled-${day}`).innerText = "AUS";
      }
      if (data[day].on_time === "") {
        document.getElementById(`view-${day}-on-time`).innerText = "--:--";
      } else {
        document.getElementById(`view-${day}-on-time`).innerText =
          data[day].on_time;
      }
      document.getElementById(`view-${day}-timer1`).innerText =
        data[day].timer1 + " min";
      document.getElementById(`view-${day}-timer2`).innerText =
        data[day].timer2 + " min";
    }
  } catch (error) {
    console.error("Error on fetch timetable:", error);
  }
}

//MARK: System
// Function to set reboot esp
async function toggleRestart() {
  try {
    const response = await fetch("/restart");
    const responseData = await response.text();
    console.log("Success:", responseData);
  } catch (error) {
    console.error("Error on reboot:", error);
  }
  alert("Gerät wird neu gestartet!");
}

// Function to reset whole esp
async function toggleReset() {
  if (confirm("Reset ausführen?")) {
    try {
      const response = await fetch("/reset");
      const responseData = await response.text();
      console.log("Success:", responseData);
      alert("Gerät wird zurückgesetzt!");
    } catch (error) {
      console.error("Error on reset:", error);
    }
  } else {
    console.log("Reset canceled!");
  }
}

// Function to toggle timeclock enable on home
async function toggleSetTimetable() {
  try {
    const response = await fetch("/activateAutoMode");
    const data = await response.json();
    document.getElementById("zeitsteuerung-aktiv").checked = data.status;
  } catch (error) {
    console.error("Error on toggle automatic mode:", error);
  }
}

// Function to toggle mention rain on home
async function toggleRain() {
  try {
    const response = await fetch("/activateRain");
    const data = await response.json();
    document.getElementById("nds-data").checked = data.status;
  } catch (error) {
    console.error("Error on toggle rain mention:", error);
  }
}

// Function to toggle mention rain on home
async function toggleRainForecast() {
  try {
    const response = await fetch("/activateRainForecast");
    const data = await response.json();
    document.getElementById("nds-data-forecast").checked = data.status;
  } catch (error) {
    console.error("Error on toggle rain mention forecast:", error);
  }
}

// Function to toggle mention sunset on home
async function toggleSunset() {
  try {
    const response = await fetch("/mentionSunset");
    const data = await response.json();
    document.getElementById("mention-sunset").checked = data.status;
  } catch (error) {
    console.error("Error on toggle sunset mention:", error);
  }
}

//MARK: Toggle Relays
async function togglePump(button) {
  try {
    const response = await fetch("/togglePump");
    const data = await response.json();
    button.value = data.status ? "Pumpe AUS" : "Pumpe AN";
    document.getElementById("stat_pumpe").innerText = data.status ? "AN" : "AUS";
  } catch (error) {
    console.error("Error toggle pump:", error);
  }

  if (button.value === "Pumpe AUS") {
    button.classList.remove('red');
    button.classList.add('green');
  } else {
    button.classList.remove('green');
    button.classList.add('red');
  }
}


async function toggleVent1(button) {
  try {
    const response = await fetch("/toggleVent1");
    const data = await response.json();
    button.value = data.status ? "Ventil 1 AUS" : "Ventil 1 AN";
    document.getElementById("stat_kreis1").innerText = data.status ? "AN" : "AUS";
  } catch (error) {
    console.error("Error toggle vent 1:", error);
  }

  if (button.value === "Ventil 1 AUS") {
    button.classList.remove('red');
    button.classList.add('green');
  } else {
    button.classList.remove('green');
    button.classList.add('red');
  }
}

async function toggleVent2(button) {
  try {
    const response = await fetch("/toggleVent2");
    const data = await response.json();
    button.value = data.status ? "Ventil 2 AUS" : "Ventil 2 AN";
    document.getElementById("stat_kreis2").innerText = data.status ? "AN" : "AUS";
  } catch (error) {
    console.error("Error toggle vent 2:", error);
  }

  if (button.value === "Ventil 2 AUS") {
    button.classList.remove('red');
    button.classList.add('green');
  } else {
    button.classList.remove('green');
    button.classList.add('red');
  }
}

//MARK: Settings
//load settings defaults
async function loadSettings() {
  try {
    const response = await fetch("/settings");
    const data = await response.json();
    document.getElementById("zeitserver").value = data.ntp;
    document.getElementById("wifi-ssid").value = data.ssid;
    document.getElementById("hostname").value = data.hostname;

    document.getElementById("precipitation-level").value = data.precipitation_level;
    document.getElementById("precipitation-level-forecast").value = data.precipitation_level_forecast;
    
    document.getElementById("max-pump-time").value = data.max_pump_time;
    document.getElementById("max-vent1-time").value = data.max_vent1_time;
    document.getElementById("max-vent2-time").value = data.max_vent2_time;
    
    
    // Weather API Settings
    document.getElementById("weather-channel").value = data.weather_channel;
    if (data.weather_channel === "openweather") {
      document.getElementById("openweather-form").style.display = "block";
      document.getElementById("metehomatics-form").style.display = "none";
      document.getElementById("wether-op-api-key-ow").value = data.openweather_api_key;
      document.getElementById("longitude-ow").value = data.longitude;
      document.getElementById("latitude-ow").value = data.latitude;
      document.getElementById("rain_duration_past_ow").value = data.rain_duration_past;
      document.getElementById("rain_duration_forecast_ow").value = data.rain_duration_forecast;
    } else if (data.weather_channel === "metehomatics") {
      document.getElementById("wether-op-api-key-meteo").value = data.openweather_api_key;
      document.getElementById("openweather-form").style.display = "none";
      document.getElementById("metehomatics-form").style.display = "block";
      document.getElementById("wether-metheo-user").value = data.metehomatics_user;
      document.getElementById("wether-metheo-pw").value = data.metehomatics_password;
      document.getElementById("longitude-meteo").value = data.longitude;
      document.getElementById("latitude-meteo").value = data.latitude;
      document.getElementById("rain_duration_past_meteo").value = data.rain_duration_past;
      document.getElementById("rain_duration_forecast_meteo").value = data.rain_duration_forecast;
    }
  } catch (error) {
    console.error("Error loading settings:", error);
  }
}

async function saveSettings(event, formId) {
  event.preventDefault();
  const form = document.getElementById(formId);
  const formData = new FormData(form);
  const json = {};
  formData.forEach((value, key) => {
    json[key] = value;
  });

  try {
    const response = await fetch("/settings", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(json),
    });
    const result = await response.json();
    console.log("Success:", result.message);
    alert(result.message);
  } catch (error) {
    console.error("Error sending settings:", error);
  }
}

//MARK: Chip info
async function fetchESPInfo() {
  try {
    const response = await fetch("/chip-info");
    const data = await response.json();

    document.getElementById("version").textContent = data.version;
    document.getElementById("esp-ip").textContent = data.ip;
    document.getElementById("esp-mac").textContent = data.mac;
    document.getElementById("esp-subnet").textContent = data.subnet;
    document.getElementById("AP-ssid").textContent = data.ssid;
    document.getElementById("esp-gateway").textContent = data.gateway;
    document.getElementById("esp-hostname").textContent = data.hostname;
    document.getElementById("esp-kernel").textContent = data.kernel;
    document.getElementById("esp-chip-id").textContent = data.chip_id;
    document.getElementById("esp-flash-id").textContent = data.flash_id;
    document.getElementById("esp-flash").textContent = data.flash_size + " KB";
    document.getElementById("esp-sketch-size").textContent = data.sketch_size + " KB";
    document.getElementById("esp-free-sketch").textContent = data.free_space + " KB";
    document.getElementById("esp-heap").textContent = data.free_heap + " KB";
    document.getElementById("esp-FS").textContent = data.fs_size + " KB";
    document.getElementById("esp-free-FS").textContent = data.fs_used + " KB";
    document.getElementById("esp-freq").textContent = data.cpu_freq + " MHz";
    document.getElementById("esp-runtime").textContent = data.esp_runtime;
  } catch (error) {
    console.error("Error on fetch ESP Chip info:", error);
  }
}

//MARK: File Upload
async function uploadConfig() {
  const fileInput = document.getElementById("config-file");
  const file = fileInput.files[0];

  if (!file) {
    alert("Please select a file to upload");
    return;
  }

  const formData = new FormData();
  formData.append("config", file);

  try {
    const response = await fetch("/upload", {
      method: "POST",
      body: formData,
    });

    if (response.ok) {
      alert("File uploaded successfully");
    } else {
      alert("Failed to upload file");
    }
  } catch (error) {
    console.error("Error uploading file:", error);
    alert("An error occurred while uploading the file");
  }
}
