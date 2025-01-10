#if defined(ESP8266) || defined(ESP32)
#include <pgmspace.h>

const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE html>
<html>
  <head>
    <title>ESP32 ReflowStation</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" charset="utf-8">
    <link rel="icon" type="image/png" href="favicon.png">
    <link rel="stylesheet" type="text/css" href="style.css">
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/1.11.0/jquery.min.js"></script>
    <script src="https://code.highcharts.com/highcharts.js"></script>
    <script src="https://code.highcharts.com/modules/data.js"></script>
    <script src="https://code.highcharts.com/modules/exporting.js"></script>

  </head>
  <body>
    <div class="topnav">
      <h1>ESP32 ReflowStation</h1>
    </div>
    <div class="content">
      <div class="card-grid">
              <p class="name"><span id="error" class="error"></span></p>
              <p class="name"><span id="status" class="status"></span><span id="time" class="toggle"></span></p>
              <table>
			  <tr><td style="width: 30%;">Управление</td><td style="width: 20%;">Плата</td><td style="width: 20%;">Верх</td><td style="width: 20%;">Низ</td></tr>
			  <tr>
			  <td>
			     <table>
				  <tr>
					  <td scope="row" colspan="2"><button onclick='websocket.send("CANCEL")' class="button_red">STOP</button></td>
				  </tr>
				  <tr>
					  <td><button onclick='websocket.send("START")' class="button_green">START</button></td>
					  <td><button onclick='websocket.send("HOT_START")' class="button">HOT START</button></td>
				  </tr>
				  <tr>
					  <td scope="row" colspan="2"><button onclick='websocket.send("UP")' class="button">+ UP</button></td>
				  </tr>
				  <tr>
					  <td><button onclick='websocket.send("LEFT")' class="button"><< PROFILE</button></td>
					  <td><button onclick='websocket.send("RIGHT")' class="button">PROFILE >></button></td>
				  </tr>
				  <tr>
					  <td scope="row" colspan="2"><button onclick='websocket.send("DOWN")' class="button">-DOWN</button></td>
				  </tr>
				  </table>
			  </td>
			  <td>
				  <p>Температура: <span id="temperature_pcb" class="toggle"></span></p>
				  <p>Температура профиля: <span id="profile_pcb" class="toggle"></span></p>
				  <p><!-- <p>Мощность: <span id="power_pcb" class="toggle"></span></p>--></p>
			  </td>
			  <td>
				  <p>Температура: <span id="temperature_top" class="toggle"></span></p>
				  <p>Мощность: <span id="power_top" class="toggle"></span></p>
				  <p>Температура профиля: <span id="profile_top" class="toggle"></span></p>
			  </td>
			  <td>
				  <p>Температура: <span id="temperature_bottom" class="toggle"></span></p>
				  <p>Мощность: <span id="power_bottom" class="toggle"></span></p>
				  <p>Температура профиля: <span id="profile_bottom" class="toggle"></span></p>
			  </td>
			  </tr>
              </table>
        <div class="card">
          <a href="./profile" target="_blank"><p class="card-title" id="card-title">Temperature Chart</p></a>
          <div id="chart-temperature" class="chart-container"></div>
        </div>
      </div>
      <p id="detailsheader"></p>
      <p id="details"></p>

    </div>
    <script src="script.js"></script>
  </body>
</html>
)rawliteral";

const char style_html[] PROGMEM = R"rawliteral(
html {
  font-family: Arial, Helvetica, sans-serif;
  display: inline-block;
  text-align: center;
}
h1 {
  font-size: 1.4rem;
  color: white;
}
p {
  font-size: 1.0rem;
}
.topnav {
  overflow: hidden;
  background-color: #0A1128;
}
body {
  margin: 0;
}
.content {
  padding: 2%;
}
.card-grid {
  margin: 0 auto;
}
.card {
  background-color: white;
  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
}
.card-title {
  font-size: 1.0rem;
  font-weight: bold;
  color: #034078
}

.chart-container {
  padding-right: 5%;
  padding-left: 5%;
}
table {
    margin: 0 auto; /* or margin: 0 auto 0 auto */
	
}
.error {
   color: red;
   font-size: 2rem;
   font-weight: bold;
}
.status {
   color: blue;
}
.toggle
{
  font-size: 2rem;
  font-weight: bold;
}
.button_green
{
  font-size: 1rem;
  font-weight: bold;
  width: 150px;
  height: 50px;
  background-color: rgb(157 229 185);
}
.button_red
{
  font-size: 1rem;
  font-weight: bold;
  width: 150px;
  height: 50px;
  background-color: rgb(255 99 99);
}
.button
{
  font-size: 1rem;
  font-weight: bold;
  width: 150px;
  height: 50px;
}
)rawliteral";

const char script_html[] PROGMEM = R"rawliteral(


var host = window.location.host;
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onLoad);

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
}

function onOpen(event) {
    console.log('Connection opened');
    websocket.send("getData");
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {

  console.log(event.data);
  

  var myObj = JSON.parse(event.data);
  if (myObj["cmd"] == 2) {
    displayTemperature(myObj);
    plotTemperature(myObj);
  }
  else if (myObj["cmd"] == 4) { // Status
    displayStatus(myObj);
  }
  else if (myObj["cmd"] == 5) { // Time
    displaySec(myObj);
  }
  else if (myObj["cmd"] == 6) { // Status // Time
    displayStatus(myObj);
    displaySec(myObj);
  }
  else if (myObj["cmd"] == 7) { // ReloadProfile
   GetProfile();
  }

  if (myObj["beep"] > 0) {
    play();
  }
  
  $("#error").html(myObj["TextError"]);
  
  $("#card-title").html(myObj["profile_name"]);

}

function onLoad(event) {
    initWebSocket();
}

function toggle(){
    websocket.send('toggle');
}

var chart;

function GetProfile() {

  $.getJSON('./profile.json', function(chartData) {

    chart = new Highcharts.Chart({
    chart:{
      renderTo:'chart-temperature',
      zoomType: "x",
      type: 'line'
    },
      title: {
        text: undefined
      },
      xAxis: {
        //type: 'datetime',
        //dateTimeLabelFormats: { second: '%H:%M:%S' }
      },
      yAxis: {
        title: {
          text: 'Temperature (°C)'
        }
      },
      credits: {
        enabled: false
      },
            series: chartData
          });

    
    reload_csv('./file?name=/log_save.csv&action=download');

  });
}

GetProfile();
listFiles();

function displayTemperature(jsonValue) {
  var keys = Object.keys(jsonValue);
  //console.log(keys);
  $("#temperature_top").html(jsonValue["temp_top"] + ' °C');
  $("#power_top").html(jsonValue["power_top"] + ' %');
  $("#profile_top").html(jsonValue["setpoint_top"] + ' °C');
  $("#temperature_bottom").html(jsonValue["temp_bottom"] + ' °C');
  $("#power_bottom").html(jsonValue["power_bottom"] + ' %');
  $("#profile_bottom").html(jsonValue["setpoint_bottom"] + ' °C');
  $("#temperature_pcb").html(jsonValue["temp_pcb"] + ' °C');
 // $("#power_pcb").html(jsonValue["power_pcb"] + '%');
  $("#profile_pcb").html(jsonValue["setpoint_pcb"] + ' °С');
}

function displayStatus(jsonValue) {
  var keys = Object.keys(jsonValue);
  //console.log(keys);
  $("#status").html(" Status: "+jsonValue["status"]);
}

function displaySec(jsonValue) {
  var keys = Object.keys(jsonValue);
  //console.log(keys);
  $("#time").html(" ProfileSec: " +jsonValue["time"]);
}

function plotTemperature(jsonValue) {

  var keys = Object.keys(jsonValue);
  //console.log(keys);

  chart.series[3].addPoint([jsonValue["time"], jsonValue["chart_temp_top"]], true, false, true);
  chart.series[4].addPoint([jsonValue["time"], jsonValue["chart_temp_bottom"]], true, false, true);
  chart.series[5].addPoint([jsonValue["time"], jsonValue["chart_temp_pcb"]], true, false, true);

  chart.series[6].addPoint([jsonValue["time"], jsonValue["power_top"]], true, false, true);
  chart.series[7].addPoint([jsonValue["time"], jsonValue["power_bottom"]], true, false, true);
  chart.series[8].addPoint([jsonValue["time"], jsonValue["power_pcb"]], true, false, true);

  chart.series[9].addPoint([jsonValue["time"], jsonValue["delta_top"]], true, false, true);
  chart.series[10].addPoint([jsonValue["time"], jsonValue["delta_bottom"]], true, false, true);
  chart.series[11].addPoint([jsonValue["time"], jsonValue["delta_pcb"]], true, false, true);
}

function play() {
  var audio = new Audio('./dong.wav');
  audio.play();
}

function reload_csv(url) {
    $.get(url, function(data) {

      var temp_top = [];
      var temp_bottom = [];
      var temp_pcb = [];
      var power_top = [];
      var power_bottom = [];
      var power_pcb = [];
      var delta_top = [];
      var delta_bottom = [];
      var delta_pcb = [];

      var lines = data.split('\n');
      $.each(lines, function(lineNo, line) {
                  var items = line.split(',');

          if (items[0] > 0) { 
              temp_top.push([ parseFloat(items[0]), parseFloat(items[1])]); 
              temp_bottom.push([ parseFloat(items[0]), parseFloat(items[2])]); 
              temp_pcb.push([ parseFloat(items[0]), parseFloat(items[3])]); 
              power_top.push([ parseFloat(items[0]), parseFloat(items[4])]); 
              power_bottom.push([ parseFloat(items[0]), parseFloat(items[5])]); 
              power_pcb.push([ parseFloat(items[0]), parseFloat(items[6])]); 
              delta_top.push([ parseFloat(items[0]), parseFloat(items[7])]); 
              delta_bottom.push([ parseFloat(items[0]), parseFloat(items[8])]); 
              delta_pcb.push([ parseFloat(items[0]), parseFloat(items[9])]); 
          }
      });
      
      chart.series[3].setData(temp_top);
      chart.series[4].setData(temp_bottom);
      chart.series[5].setData(temp_pcb);
      chart.series[6].setData(power_top);
      chart.series[7].setData(power_bottom);
      chart.series[8].setData(power_pcb);
      chart.series[9].setData(delta_top);
      chart.series[10].setData(delta_bottom);
      chart.series[11].setData(delta_pcb);

    });
}

function fileButton(filename, action) {
  var urltocall = "/file?name=/" + filename + "&action=" + action;
  xmlhttp=new XMLHttpRequest();
  if (action == "delete") {
    xmlhttp.open("GET", urltocall, false);
    xmlhttp.send();
    document.getElementById("status").innerHTML = xmlhttp.responseText;
    xmlhttp.open("GET", "/listfiles", false);
    xmlhttp.send();
    document.getElementById("details").innerHTML = xmlhttp.responseText;
  }
  if (action == "write") {
    xmlhttp.open("GET", urltocall, false);
    xmlhttp.send();
  }
  if (action == "load") {
    reload_csv('/file?name=/'+filename+'&action=download');
  }
  if (action == "download") {
    document.getElementById("status").innerHTML = "";
    window.open(urltocall,"_blank");
  }
}


function listFiles() {
  xmlhttp=new XMLHttpRequest();
  xmlhttp.open("GET", "./listfiles", false);
  xmlhttp.send();
  document.getElementById("detailsheader").innerHTML = "<h3>Files<h3>";
  document.getElementById("details").innerHTML ='<p><button onclick="showUploadButtonFancy()">Upload File</button></p>';
  document.getElementById("details").innerHTML = document.getElementById("details").innerHTML + xmlhttp.responseText;
}

function showUploadButtonFancy() {
  document.getElementById("detailsheader").innerHTML = "<h3>Upload File<h3>";
  document.getElementById("status").innerHTML = "";
  var uploadform = "<form method = \"POST\" action = \"/\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"data\"/><input type=\"submit\" name=\"upload\" value=\"Upload\" title = \"Upload File\"></form>";
  document.getElementById("details").innerHTML = uploadform;
  var uploadform =
  "<form id=\"upload_form\" enctype=\"multipart/form-data\" method=\"post\">" +
  "<input type=\"file\" name=\"file1\" id=\"file1\" onchange=\"uploadFile()\"><br>" +
  "<progress id=\"progressBarUpload\" value=\"0\" max=\"100\" style=\"width:300px;\"></progress>" +
  "<h3 id=\"status\"></h3>" +
  "<p id=\"loaded_n_total\"></p>" +
  "</form>";
  document.getElementById("details").innerHTML = uploadform;
}
function _(el) {
  return document.getElementById(el);
}

function uploadFile() {
  var file = _("file1").files[0];
  // alert(file.name+" | "+file.size+" | "+file.type);
  var formdata = new FormData();
  formdata.append("file1", file);
  var ajax = new XMLHttpRequest();
  ajax.upload.addEventListener("progress", progressHandler, false);
  ajax.addEventListener("load", completeHandler, false); // doesnt appear to ever get called even upon success
  ajax.addEventListener("error", errorHandler, false);
  ajax.addEventListener("abort", abortHandler, false);
  ajax.open("POST", "/");
  ajax.send(formdata);
}

function completeHandler(event) {
  _("status").innerHTML = "Upload Complete";
  _("progressBarUpload").value = 0;
  xmlhttp=new XMLHttpRequest();
  xmlhttp.open("GET", "/listfiles", false);
  xmlhttp.send();
  document.getElementById("status").innerHTML = "File Uploaded";
  document.getElementById("detailsheader").innerHTML = "<h3>Files<h3>";
  document.getElementById("details").innerHTML = xmlhttp.responseText;
}
function errorHandler(event) {
  _("status").innerHTML = "Upload Failed";
}
function abortHandler(event) {
  _("status").innerHTML = "inUpload Aborted";
}

function progressHandler(event) {
  _("loaded_n_total").innerHTML = "Uploaded " + event.loaded + " bytes";
  var percent = (event.loaded / event.total) * 100;
  _("progressBarUpload").value = Math.round(percent);
 
  _("status").innerHTML = Math.round(percent) + "%% uploaded... please wait";
  if (percent >= 100) {
    _("status").innerHTML = "Please wait, writing file to filesystem";
  }
}


)rawliteral";


#endif