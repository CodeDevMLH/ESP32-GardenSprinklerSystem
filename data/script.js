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
// Warten auf DOMContentLoaded und load Events
let domReady = false;
let pageLoaded = false;

document.addEventListener('DOMContentLoaded', () => {
  domReady = true;
  if (pageLoaded) checkActionEnabled();

  const savedMode = localStorage.getItem('darkMode');
  if (savedMode) {
    switchMode(savedMode === 'enabled');
    document.getElementById('design_mode').checked = (savedMode === 'enabled');
  } else {
    const prefersDarkMode = window.matchMedia('(prefers-color-scheme: dark)').matches;
    switchMode(prefersDarkMode);
    document.getElementById('design_mode').checked = prefersDarkMode;
  }
});

window.addEventListener('load', () => {
  pageLoaded = true;
  if (domReady) checkActionEnabled();
});

// Dark Mode Umschalten
function switchMode(isDark) {
  if (isDark) {
    document.body.classList.add('dark-mode');
    localStorage.setItem('darkMode', 'enabled');
  } else {
    document.body.classList.remove('dark-mode');
    localStorage.setItem('darkMode', 'disabled');
  }
}

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
function toggleSetDarkMode() {
  const darkMode = document.getElementById("design_mode").checked;
  switchMode(darkMode);
}

// function to show all action elements
async function checkActionEnabled() {
  try {
    const response = await fetch("/checkActionEnabled");
    const data = await response.json();
    console.log("Action enabled: ", data);
    if (data.status) {
      const actionElements = document.querySelectorAll('.actionEnabled');
      actionElements.forEach(element => {
        element.style.display = "inline";
      });
    } else {
      const actionElements = document.querySelectorAll('.actionEnabled');
      actionElements.forEach(element => {
        element.style.display = "none";
      });
    }
  } catch (error) {
    console.error("Error on check action enabled:", error);
  }
}

// Function to toggle wetaher settings options
function toggleWeatherForm() {
  var weatherChannel = document.getElementById("weather-channel").value;
  var openweatherForm = document.getElementById("openweather-form");
  var meteomaticsForm = document.getElementById("meteomatics-form");

  if (weatherChannel === "openweather") {
    openweatherForm.style.display = "block";
    meteomaticsForm.style.display = "none";
  } else {
    openweatherForm.style.display = "none";
    meteomaticsForm.style.display = "block";
  }
}

async function saveweatherSettings(event, formId) {
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
    console.log("Save Weather Setting. Success: ", result.status);
    if (result.status) {
      createAlert(
        "Gespeichert",
        "Wetterdienst übernommen",
        "success",
        true,
        true
      );
    }
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
      document.getElementById("meteomatics-form").style.display = "none";
      document.getElementById("weather-op-api-key-ow").value = data.api_key;
      document.getElementById("longitude-ow").value = data.longitude;
      document.getElementById("latitude-ow").value = data.latitude;
      document.getElementById("rain_duration_past_ow").value = data.rain_duration_past;
      document.getElementById("rain_duration_forecast_ow").value = data.rain_duration_forecast;
    } else if (data.weather_channel === "meteomatics") {
      document.getElementById("weather-op-api-key-meteo").value = data.api_key;
      document.getElementById("openweather-form").style.display = "none";
      document.getElementById("meteomatics-form").style.display = "block";
      document.getElementById("weather-meteo-user").value = data.meteomatics_user;
      document.getElementById("weather-meteo-pw").value = data.meteomatics_password;
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
  const allTimersSelected =
    document.getElementById("select-all-timers").checked;
  document.getElementById("timer1-button").style.display = allTimersSelected
    ? "none"
    : "inline-block";
  document.getElementById("timer2-button").style.display = allTimersSelected
    ? "none"
    : "inline-block";
  document.getElementById("timer-all-button").style.display = allTimersSelected
    ? "inline-block"
    : "none";
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
    console.log("Emergency: ", responseData);
  } catch (error) {
    console.error("Error on emergency:", error);
  }
  createAlert(
    "Warnung!",
    "Not-Aus ausgelöst!",
    "warning",
    false,
    true
  );
  emergencyLoop();
}
//MARK: emergnecy stop
function showEmergencyStopConfirmationDialog() {
  return new Promise((resolve, reject) => {
    // Zeige den Dialog an
    const modalEmergencyOverlay = document.getElementById('modalEmergencyOverlay');
    modalEmergencyOverlay.classList.add('show');

    // Event-Handler für die Bestätigungs-Schaltfläche
    const confirmButton = document.getElementById('confirmButtonEmergency');
    confirmButton.onclick = () => {
      modalEmergencyOverlay.classList.remove('show');
      resolve(true); // Resolve das Promise bei Bestätigung
    };
  });
}

// Function to reset whole esp
async function emergencyLoop() {
  try {
    await showEmergencyStopConfirmationDialog();
    const response = await fetch("/emergencyLoop");
    const result = await response.json();
    console.log("emergency stopped!: Success: ", result.status);
    if (result.status) {
      createAlert(
        "Info!",
        "Not-Aus aufgehoben!",
        "info",
        true,
        true
      );
    } else {
      createAlert(
        "Warnung!",
        "Not-Aus konnte nicht aufgehoben werden!",
        "warning",
        false,
        true
      );
    }
  } catch (error) {
    console.error("Error on emergency stop:", error);
  }
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
    const result = await response.json();
    console.log("Set timer. Success: ", result.status);
    if (result.status) {
      createAlert(
        "Gespeichert",
        "Timereinstellungen übernommen",
        "success",
        true,
        true
      );
    }
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
    const result = await response.json();
    console.log("All timers stopped. Success: ", result.status);
    if (result.status) {
      createAlert(
        "Warnung!",
        "Alle Timer gestoppt!",
        "warning",
        false,
        true
      );
    }
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
    console.log("Set timeclock. Success: ", responseData.status);
    if (responseData.status) {
      createAlert(
        "Gespeichert",
        "Zeiteinstellungen übernommen",
        "success",
        true,
        true
      );
    }
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
    document.getElementById("valve1-button").value = data.valve1_button ? "Nord AUS" : "Nord AN";
    document.getElementById("stat_kreis1").innerText = data.valve1_button ? "AN" : "AUS";
    document.getElementById("valve2-button").value = data.valve2_button ? "Süd AUS" : "Süd AN";
    document.getElementById("stat_kreis2").innerText = data.valve2_button ? "AN" : "AUS";
    document.getElementById("action-button").value = data.action_button ? "Action AUS" : "Action AN";
    document.getElementById("stat_action").innerText = data.action_button ? "AN" : "AUS";
    setManualControlButtons();
    //console.log("updated home data");
  } catch (error) {
    console.error("Error on fetch home data:", error);
  }
}

function setManualControlButtons() {
  let button = document.getElementById("pump-button");
  if (button.value === "Pumpe AUS") {
    button.classList.remove("red");
    button.classList.add("green");
  } else {
    button.classList.remove("green");
    button.classList.add("red");
  }
  button = document.getElementById("valve1-button");
  if (button.value === "Nord AUS") {
    button.classList.remove("red");
    button.classList.add("green");
  } else {
    button.classList.remove("green");
    button.classList.add("red");
  }
  button = document.getElementById("valve2-button");
  if (button.value === "Süd AUS") {
    button.classList.remove("red");
    button.classList.add("green");
  } else {
    button.classList.remove("green");
    button.classList.add("red");
  }
  button = document.getElementById("action-button");
  if (button.value === "Action AUS") {
    button.classList.remove("red");
    button.classList.add("green");
  } else {
    button.classList.remove("green");
    button.classList.add("red");
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
        document.getElementById(`view-${day}-on-time`).innerText = data[day].on_time;
      }
      document.getElementById(`view-${day}-timer1`).innerText = data[day].timer1 + " min";
      document.getElementById(`view-${day}-timer2`).innerText = data[day].timer2 + " min";
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
    const result = await response.json();
    console.log("Reboot. Success: ", result);
    if (result.status) {
      createAlert(
        "Info",
        "Neustart wird durchgeführt!",
        "info",
        false,
        true
      );
    }
  } catch (error) {
    console.error("Error on reboot:", error);
  }
}

//MARK: reset esp
function showResetConfirmationDialog() {
  return new Promise((resolve, reject) => {
    // Zeige den Dialog an
    const modalOverlay = document.getElementById('modalOverlay');
    modalOverlay.classList.add('show');

    // Event-Handler für die Bestätigungs-Schaltfläche
    const confirmButton = document.getElementById('confirmButton');
    confirmButton.onclick = () => {
      modalOverlay.classList.remove('show');
      resolve(true); // Resolve das Promise bei Bestätigung
    };

    // Event-Handler für die Abbrechen-Schaltfläche
    const cancelButton = document.getElementById('cancelButton');
    cancelButton.onclick = () => {
      modalOverlay.classList.remove('show');
      reject(false); // Reject das Promise bei Abbrechen
    };
  });
}

// Function to reset whole esp
async function toggleReset() {
  try {
    await showResetConfirmationDialog();
    const response = await fetch("/reset");
    const responseData = await response.text();
    console.log(responseData);
    createAlert(
      "Reset",
      "ESP reset wird ausgeführt!",
      "warning",
      false,
      true
    );
  } catch (error) {
    console.error("Error on reset or reset canceled:", error);
  }
}

//MARK: Home settings
// Function to toggle timeclock enable on home
async function toggleSetTimetable() {
  try {
    const response = await fetch("/activateAutoMode");
    const data = await response.json();
    document.getElementById("zeitsteuerung-aktiv").checked = data.status;
    console.log("Time control: ", data.status);
    if (data.status) {
      createAlert(
        "Gespeichert",
        "Zeitsteuerung aktiviert!",
        "success",
        true,
        true
      );
    } else {
      createAlert(
        "Gespeichert",
        "Zeitsteuerung deaktiviert!",
        "success",
        true,
        true);
    }
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
    console.log("Mention rain data: ", data.status);
    if (data.status) {
      createAlert(
        "Gespeichert",
        "Niederschlag berücksichtigen aktiviert!",
        "success",
        true,
        true
      );
    } else {
      createAlert(
        "Gespeichert",
        "Niederschlag berücksichtigen deaktiviert!",
        "success",
        true,
        true);
    }
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
    console.log("Mention forecast rain data: ", data.status);
    if (data.status) {
      createAlert(
        "Gespeichert",
        "Vorraussichtlichen Niederschlag berücksichtigen aktiviert!",
        "success",
        true,
        true
      );
    } else {
      createAlert(
        "Gespeichert",
        "Vorraussichtlichen Niederschlag berücksichtigen deaktiviert!",
        "success",
        true,
        true);
    }
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
    console.log("Mention sunset: ", data.status);
    if (data.status) {
      createAlert(
        "Gespeichert",
        "Dämmerung berücksichtigen aktiviert!",
        "success",
        true,
        true
      );
    } else {
      createAlert(
        "Gespeichert",
        "Dämmerung berücksichtigen deaktiviert!",
        "success",
        true,
        true);
    }
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
    button.classList.remove("red");
    button.classList.add("green");
  } else {
    button.classList.remove("green");
    button.classList.add("red");
  }
}

async function toggleValve1(button) {
  try {
    const response = await fetch("/toggleValve1");
    const data = await response.json();
    button.value = data.status ? "Nord AUS" : "Nord AN";
    document.getElementById("stat_kreis1").innerText = data.status ? "AN" : "AUS";
  } catch (error) {
    console.error("Error toggle valve 1:", error);
  }

  if (button.value === "Nord AUS") {
    button.classList.remove("red");
    button.classList.add("green");
  } else {
    button.classList.remove("green");
    button.classList.add("red");
  }
}

async function toggleValve2(button) {
  try {
    const response = await fetch("/toggleValve2");
    const data = await response.json();
    button.value = data.status ? "Süd AUS" : "Süd AN";
    document.getElementById("stat_kreis2").innerText = data.status ? "AN" : "AUS";
  } catch (error) {
    console.error("Error toggle valve 2:", error);
  }

  if (button.value === "Süd AUS") {
    button.classList.remove("red");
    button.classList.add("green");
  } else {
    button.classList.remove("green");
    button.classList.add("red");
  }
}

async function toggleAction(button) {
  try {
    const response = await fetch("/toggleAction");
    const data = await response.json();
    button.value = data.status ? "Action AUS" : "Action AN";
    document.getElementById("stat_action").innerText = data.status ? "AN" : "AUS";
  } catch (error) {
    console.error("Error toggle action:", error);
  }

  if (button.value === "Action AUS") {
    button.classList.remove("red");
    button.classList.add("green");
  } else {
    button.classList.remove("green");
    button.classList.add("red");
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
    document.getElementById("max-valve1-time").value = data.max_valve1_time;
    document.getElementById("max-valve2-time").value = data.max_valve2_time;
    document.getElementById("max-action-time").value = data.max_action_time;

    document.getElementById("action_active").checked = data.action_active;

    // Weather API Settings
    document.getElementById("weather-channel").value = data.weather_channel;
    if (data.weather_channel === "openweather") {
      document.getElementById("openweather-form").style.display = "block";
      document.getElementById("meteomatics-form").style.display = "none";
      document.getElementById("weather-op-api-key-ow").value = data.api_key;
      document.getElementById("longitude-ow").value = data.longitude;
      document.getElementById("latitude-ow").value = data.latitude;
      document.getElementById("rain_duration_past_ow").value = data.rain_duration_past;
      document.getElementById("rain_duration_forecast_ow").value = data.rain_duration_forecast;
    } else {
      document.getElementById("openweather-form").style.display = "none";
      document.getElementById("meteomatics-form").style.display = "block";
      document.getElementById("weather-op-api-key-meteo").value = data.api_key;
      document.getElementById("weather-meteo-user").value = data.meteomatics_user;
      document.getElementById("weather-meteo-pw").value = data.meteomatics_password;
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
    console.log("Settings saved: ", result.status);
    if (result.status) {
      createAlert(
        "Gespeichert",
        "Einstellungen gespeichert!",
        "success",
        true,
        true
      );
    }
  } catch (error) {
    console.error("Error sending settings:", error);
  }
}

async function toggleSetActionEnable() {
  const enabledStatus = document.getElementById("action_active").checked;
  const data = {
    status: enabledStatus,
  };

  try {
    const response = await fetch("/activateAction", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(data),
    });
    const result = await response.json();
    console.log("Action controlls active: ", result.status);
    if (result.status) {
      const actionElements = document.querySelectorAll('.actionEnabled');
      actionElements.forEach(element => {
        element.style.display = "inline";
      });
      createAlert(
        "Gespeichert",
        "Action Elemente werden angezeigt!",
        "success",
        true,
        true
      );
    } else {
      const actionElements = document.querySelectorAll('.actionEnabled');
      actionElements.forEach(element => {
        element.style.display = "none";
      });
      createAlert(
        "Gespeichert",
        "Action Elemente werden nicht mehr angezeigt",
        "warning",
        true,
        true);
    }
  } catch (error) {
    console.error("Error on toggle action controll active:", error);
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
    document.getElementById("esp-rrsi").textContent = data.rrsi;
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

function sendDownloadAlert() {
  createAlert(
    "Download",
    "Die Datei wird heruntergeladen!",
    "info",
    false,
    true
  );
}

//MARK: File Upload
async function uploadConfig() {
  const fileInput = document.getElementById("configfile");
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

    if (!response.ok) {
      const errorText = await response.text();
      console.error(`Error uploading file: Status ${response.status} - ${errorText}`);
      createAlert(
        "Warnung",
        `Fehler beim Hochladen: ${errorText}`,
        "warning",
        false,
        true
      );
      return;
    }

    const result = await response.json();
    if (result.status) {
      console.log("File uploaded successfully");
      createAlert(
        "Gespeichert",
        "Config erfolgreich hochgeladen!",
        "success",
        true,
        true
      );
    } else {
      console.log("File uploaded failed");
      createAlert(
        "Warnung",
        "Fehler beim Hochladen!",
        "warning",
        false,
        true
      );
    }
  } catch (error) {
    console.error("Error uploading file:", error);
    // alert("An error occurred while uploading the file");
    createAlert(
      "Warnung",
      "Fehler beim Hochladen!",
      "warning",
      false,
      true
    );
  }
}

//MARK: Alerts

function createAlert(title, message, alertType, dismissible, autoClose) {
  // Container für die Alerts finden
  var alertContainer = document.getElementById("pageMessages");

  // Erstelle das Alert-Div
  var alertDiv = document.createElement("div");
  alertDiv.className = "alert alert-" + alertType;

  // Füge den Titel und die Nachricht hinzu
  //alertDiv.innerHTML = '<strong>' + title + '</strong> ' + message;

  // Füge den Titel in einer eigenen Zeile hinzu
  if (title) {
    var titleElement = document.createElement("div");
    titleElement.style.fontWeight = "bold"; // Optional: Fettdruck für den Titel
    titleElement.textContent = title;
    alertDiv.appendChild(titleElement);
  }

  // Füge die Nachricht hinzu
  var messageElement = document.createElement("div");
  messageElement.textContent = message;
  alertDiv.appendChild(messageElement);

  // Füge den Schließen-Button hinzu, wenn dismissible
  if (dismissible) {
    var closeButton = document.createElement("span");
    closeButton.className = "close";
    closeButton.innerHTML = "&times;";
    closeButton.onclick = function () {
      fadeOut(alertDiv); // Wende den Ausfaden-Effekt an
    };
    alertDiv.appendChild(closeButton);
  }

  // Füge das Alert zum Container hinzu
  alertContainer.appendChild(alertDiv);

  // Automatisches Schließen nach 10 Sekunden, wenn aktiviert
  if (autoClose) {
    setTimeout(function () {
      fadeOut(alertDiv); // Wende den Ausfaden-Effekt an
    }, 8000);
  }
}

function fadeOut(element) {
  element.style.opacity = "0"; // Setze die Sichtbarkeit auf 0, um das Ausfaden zu starten
  setTimeout(function () {
    element.remove(); // Entferne das Element nach dem Ausfaden
  }, 500); // Wartezeit entsprechend der CSS-Transition-Dauer (0.5s)
}
