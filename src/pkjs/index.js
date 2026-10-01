var Clay = require('pebble-clay');
var clayConfig = require('./config');

var clay = new Clay(clayConfig);

// Condition ids understood by the watch (see WEATHER_* in momentous.c)
var CLEAR = 1, PARTLY_CLOUDY = 2, CLOUDY = 3, RAIN = 4, SNOW = 5, STORM = 6, FOG = 7;

// Maps an Open-Meteo WMO weather code to a watch condition id
function conditionFromCode(code) {
  if (code === 0) return CLEAR;
  if (code <= 2) return PARTLY_CLOUDY;
  if (code === 3) return CLOUDY;
  if (code === 45 || code === 48) return FOG;
  if ((code >= 71 && code <= 77) || code === 85 || code === 86) return SNOW;
  if (code >= 95) return STORM;
  return RAIN;
}

function useCelsius() {
  try {
    var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
    return settings.TempUnit === 'C';
  } catch (e) {
    return false;
  }
}

function fetchWeather() {
  navigator.geolocation.getCurrentPosition(function(pos) {
    var url = 'https://api.open-meteo.com/v1/forecast' +
      '?latitude=' + pos.coords.latitude +
      '&longitude=' + pos.coords.longitude +
      '&current=temperature_2m,weather_code' +
      '&temperature_unit=' + (useCelsius() ? 'celsius' : 'fahrenheit');

    var xhr = new XMLHttpRequest();
    xhr.onload = function() {
      try {
        var current = JSON.parse(this.responseText).current;
        Pebble.sendAppMessage({
          Temperature: Math.round(current.temperature_2m),
          WeatherCondition: conditionFromCode(current.weather_code)
        });
      } catch (e) {
        console.log('Weather parse failed: ' + e);
      }
    };
    xhr.open('GET', url);
    xhr.send();
  }, function(err) {
    console.log('Location unavailable: ' + err.message);
  }, { timeout: 15000, maximumAge: 60000 });
}

Pebble.addEventListener('ready', fetchWeather);

Pebble.addEventListener('appmessage', function(e) {
  if (e.payload.RequestWeather) {
    fetchWeather();
  }
});

// Refetch after settings change so a new temperature unit shows up right away
Pebble.addEventListener('webviewclosed', function() {
  setTimeout(fetchWeather, 1000);
});
