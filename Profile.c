#if defined(ESP8266) || defined(ESP32)
#include <pgmspace.h>

const char profile_html[] PROGMEM = R"rawliteral(
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
    <style>
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
  //font-size: 1.4rem;
}
.topnav {
  overflow: hidden;
  background-color: #0A1128;
}
body {
  margin: 0;
}
.content {
  padding: 1%;
}
.card-grid {
  margin: 0 auto;
}
.card {
  margin: 0 auto;
  background-color: white;
  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
  width: 80%;
}
.card-title {
  //float:left;
  font-size: 1.0rem;
  font-weight: bold;
  color: #034078;
  width: 80%;
}

.chart-container {
  padding-right: 5%;
  padding-left: 5%;
  width: 70%;
}

#table_container {
	#display:inline;
}

.table {
    margin: 10px;
    float:left;
	width: 300px;
	margin-bottom: 20px;
	border: 1px solid #dddddd;
	border-collapse: collapse; 
	font-size: 13px;
}
table th {
	font-weight: bold;
	padding: 5px;
	background: #efefef;
	border: 1px solid #dddddd;
}
table td {
	border: 1px solid #dddddd;
	padding: 2px;
}

.info {
   color: green;
}
.status {
   color: blue;
}

input {
  width: 30px;
  border: 1px solid black;
 // min-width: 30px;
}

	</style>
	
  </head>
  <body>
    <div class="topnav">
      <h1>ESP32 ReflowStation</h1>
    </div>
    <div class="content">
      <div class="card-grid">
	          <button onclick="SaveProfile()">Сохранить профиль</button>
			  <p>
			  <input type="file" name="frmFile" id="frmFile" onchange="readFile(this)" style="width: 250px;"/>
			  </p>
			  <p><button onclick="exportProfile()">Экспортировать профиль</button>
			  <a href="" id="linkForSavingFile" style="display:none" /></a></p>
			  
              <p class="name"><span id="info" class="info"></span></p>
              <p class="name"><span id="status" class="status"></span></p>
        <table align="center"><tr><td>
		<div id="table_container">
		    <table id="table_settings" class="table"><tbody>
			  <tr><td>Параметр</td><td>Значение</td></tr>
			  <tr><td>Наименование</td><td><input type="text" name="aliasprofile" id="aliasprofile" style="width: 100px;"></td></tr>
			  <tr><td>Количество шагов ВИ и НИ</td><td><input type="text" name="profile_steps" id="profile_steps"></td></tr>
			  <tr><td>Размер стола</td><td><input type="text" name="table_size" id="table_size"></td></tr>
			  <tr><td>Пропорциональный коэффициент ВИ</td><td><input type="text" name="kp1" id="kp1"></td></tr>
			  <tr><td>Интегральный коэффициент     ВИ</td><td><input type="text" name="ki1" id="ki1"></td></tr>
			  <tr><td>Дифференциальный коэффициент ВИ</td><td><input type="text" name="kd1" id="kd1"></td></tr>
			  <tr><td>Пропорциональный коэффициент НИ</td><td><input type="text" name="kp2" id="kp2"></td></tr>
			  <tr><td>Интегральный коэффициент     НИ</td><td><input type="text" name="ki2" id="ki2"></td></tr>
			  <tr><td>Пропорциональный коэффициент НИ</td><td><input type="text" name="kd2" id="kd2"></td></tr>
			  <tr><td>Пропорциональный коэффициент Платы</td><td><input type="text" name="kp3" id="kp3"></td></tr>
			  <tr><td>Интегральный коэффициент     Платы</td><td><input type="text" name="ki3" id="ki3"></td></tr>
			  <tr><td>Дифференциальный коэффициент Платы</td><td><input type="text" name="kd3" id="kd3"></td></tr>
			  <tr><td>Максимальная коррекция температуры ВИ</td><td><input type="text" name="max_correction_top" id="max_correction_top"></td></tr>
			  <tr><td>Максимальная коррекция температуры НИ</td><td><input type="text" name="max_correction_bottom" id="max_correction_bottom"></td></tr>
			  <tr><td>Максимальное отклонение температуры Платы после которого включаеся Авто Пауза (0-250)</td><td><input type="text" name="max_pcb_delta" id="max_pcb_delta"></td></tr>
			  <tr><td>Длительность автопаузы</td><td><input type="text" name="hold_lenght" id="hold_lenght"></td></tr>
			  <tr><td>Коэфициент участия верха</td><td><input type="text" name="participation_rate_top" id="participation_rate_top"></td></tr>
			  
			  </tbody></table>
		  </div>
		</td></tr></table>
          <div class="card">
          <p class="card-title">Profile</p>
          <div id="chart-temperature" class="chart-container"></div>
        </div>
      </div>
    </div>
    <script>
	
	
var host = window.location.host;

var chart;
var table = document.getElementById("table_container");
var profile_id;

var nodeArray=new Array();  

function listTable(table_name) {
  var Ret = new Array();
  var time="";
  var temp="";
  for (var i = 1, row; row = document.getElementById(table_name+"-calc").rows[i]; i++) {
	  time = time + document.getElementById(table_name+"-time-"+(i-1)).value + ",";
	  
	  if (i == 1) {
	    temp = temp + document.getElementById(table_name+"-temp-start-0").value + ",";
	  }
	  else {
	    temp = temp + row.cells[2].innerHTML + ",";
	  }
	} 
   Ret["time"] = time;
   Ret["temp"] = temp;
   return Ret;
}

function exportProfile() {


	let arr = [];

	arr["aliasprofile"] = document.getElementById('aliasprofile').value;

	var arr_tmp = listTable("top");
	arr.push(["time_step_top", arr_tmp["time"]]);
	arr.push(["temperature_step_top", arr_tmp["temp"]]);

	arr_tmp = listTable("bottom");
	arr.push(["time_step_bottom", arr_tmp["time"]]);
	arr.push(["temperature_step_bottom", arr_tmp["temp"]]);

	arr_tmp = listTable("pcb");
	arr.push(["time_step_pcb", arr_tmp["time"]]);
	arr.push(["temperature_step_pcb", arr_tmp["temp"]]);

	arr.push(["profile_steps", document.getElementById('profile_steps').value]);
	arr.push(["table_size", document.getElementById('table_size').value]);
	arr.push(["kp1", document.getElementById('kp1').value]);
	arr.push(["ki1", document.getElementById('ki1').value]);
	arr.push(["kd1", document.getElementById('kd1').value]);
	arr.push(["kp2", document.getElementById('kp2').value]);
	arr.push(["ki2", document.getElementById('ki2').value]);
	arr.push(["kd2", document.getElementById('kd2').value]);
	arr.push(["kp3", document.getElementById('kp3').value]);
	arr.push(["ki3", document.getElementById('ki3').value]);
	arr.push(["kd3", document.getElementById('kd3').value]);
	arr.push(["max_correction_top", document.getElementById('max_correction_top').value]);
	arr.push(["max_correction_bottom", document.getElementById('max_correction_bottom').value]);
	arr.push(["max_pcb_delta", document.getElementById('max_pcb_delta').value]);
	arr.push(["hold_lenght", document.getElementById('hold_lenght').value]);
	arr.push(["participation_rate_top", document.getElementById('participation_rate_top').value]);
  arr.push(["aliasprofile", document.getElementById('aliasprofile').value]);

    var data = arr.map(item => item.join(':')).join('\n');

    var a = document.getElementById("linkForSavingFile");
    var file = new Blob([data], {
      type: 'plain/text'
    });
    a.href = URL.createObjectURL(file);
    a.download = document.getElementById('aliasprofile').value+".txt";
    a.click();
}


function clearTable(table_name) {

   for (var i = 1, row; row = document.getElementById(table_name+"-calc").rows[i]; i++) {
	  document.getElementById(table_name+"-time-"+(i-1)).value = 0;
	  if (i == 1) {
	    document.getElementById(table_name+"-temp-start-0").value = 0;
	  }
	  else {
	    row.cells[2].innerHTML = 0;
	  }
	}
}

function readFile(input) {
  const graph_param = [
	  "time_step_top",
	  "temperature_step_top",
	  "time_step_bottom",
	  "temperature_step_bottom",
	  "time_step_pcb",
	  "temperature_step_pcb",
  ];

  var file = input.files[0];

  var reader = new FileReader();

  reader.readAsText(file);

  reader.onload = function() {
    //console.log(reader.result);

	clearTable("top");
	clearTable("bottom");
	clearTable("pcb");
	
	Calc("top", 1);
	Calc("bottom", 1);
	Calc("pcb", 1);
	
	var lines = reader.result.split('\n');
    for(var line = 0; line < lines.length-1; line++){
	  var linesSpace = lines[line].split(':');
	  
	  if (!graph_param.includes(linesSpace[0])) {
	    document.getElementById(linesSpace[0]).value = linesSpace[1];
	  }
	  else
	  {
        var linesSpace2 = linesSpace[0].split('_');
	    var table_name = linesSpace2[2]; 
		var table_type = linesSpace2[0]; 
		
	    var lines_gr = linesSpace[1].split(',');
		for(var line_gr = 0; line_gr < lines_gr.length-1; line_gr++){
		  
	      if (table_type == "time") {
		    document.getElementById(table_name+"-time-"+line_gr).value = lines_gr[line_gr];
		   }
		   if (table_type == "temperature") {
		    if (line_gr == 0) {
			 document.getElementById(table_name+"-temp-start-0").value = lines_gr[line_gr];
            } else {			
		      var row = document.getElementById(table_name+"-calc").rows[line_gr+1];
			  row.cells[2].innerHTML = lines_gr[line_gr];
			}
		   }
		}
	   
	  }
    }
	Calc("top", 1);
	Calc("bottom", 1);
	Calc("pcb", 1);
  };

  reader.onerror = function() {
    console.log(reader.error);
  };

}

function getRandomNumber(min, max) {
      return Math.floor(Math.random() * (max - min) + min)
  }  

function TableSettings() {
	$.getJSON('./profile_settings.json', function(Data) {
	  document.getElementById('profile_steps').value = Data['profile_steps'];
	  document.getElementById('table_size').value = Data['table_size'];
	  document.getElementById('kp1').value = Data['kp1'];
	  document.getElementById('ki1').value = Data['ki1'];
	  document.getElementById('kd1').value = Data['kd1'];
	  document.getElementById('kp2').value = Data['kp2'];
	  document.getElementById('ki2').value = Data['ki2'];
	  document.getElementById('kd2').value = Data['kd2'];
	  document.getElementById('kp3').value = Data['kp3'];
	  document.getElementById('ki3').value = Data['ki3'];
	  document.getElementById('kd3').value = Data['kd3'];
	  document.getElementById('max_correction_top').value = Data['max_correction_top'];
	  document.getElementById('max_correction_bottom').value = Data['max_correction_bottom'];
	  document.getElementById('max_pcb_delta').value = Data['max_pcb_delta'];
	  document.getElementById('hold_lenght').value = Data['hold_lenght'];
	  document.getElementById('participation_rate_top').value = Data['participation_rate_top'];
	  document.getElementById('status').innerHTML = "[ "+ Data['currentProfile'] + " ] " + Data['alias'];
	  document.getElementById('aliasprofile').value = Data['alias'];
	  profile_id = Data['currentProfile'];
	});

}

function TableCreate(profile_row_id, table_name) {

  $.getJSON('./profile.json', function(Data) {
	
    tbl  = document.createElement('table');
    tbl.setAttribute('id', table_name+'-calc');
	tbl.setAttribute('class', 'table');
	
	tr = tbl.insertRow();
	
	td = tr.insertCell();
	td.innerHTML = "Номер шага";
    td = tr.insertCell();
	td.innerHTML = "Время от начала";
	td = tr.insertCell();
	td.innerHTML = "Температура";
	
	td = tr.insertCell();
	td.innerHTML = "Длинна шага (сек)";
	td = tr.insertCell();
	td.innerHTML = "dT гр";

  var arr_len = Data[profile_row_id]['data'].length;
	
    for(var i = 0; i < Data[profile_row_id]['profile_size']; i++){
        var tr = tbl.insertRow();
        let el, td, num;
		
                td = tr.insertCell();
                td.innerHTML = i;
          
				// Time
				td = tr.insertCell();
                el = document.createElement('input');
				if (i < arr_len) {
			      el.value = Data[profile_row_id]['data'][i][0];
				}
				else {
				  el.value = 0;
				}
				
				el.setAttribute('type', 'text');
				el.setAttribute('name', `${table_name}-input-${i}`);
				el.setAttribute('id', `${table_name}-time-${i}`);
				//el.setAttribute('onchange', 'Calc("'+table_name+'", 2)');
				td.appendChild(el);
					  

				// Temp
				if (i == 0) {
					td = tr.insertCell();
					el = document.createElement('input');
					if (i < arr_len) {
			          el.value = Data[profile_row_id]['data'][i][1];
				    }
				    else {
				     el.value = 0;
				    }
					el.setAttribute('type', 'text');
					el.setAttribute('name', `${table_name}-input-${i}`);
					el.setAttribute('id', table_name+'-temp-start-0');
					el.setAttribute('onchange', 'Calc("'+table_name+'", 2)');
					td.appendChild(el);
				}
				else {
					td = tr.insertCell();
					if (i < arr_len) {
			          td.innerHTML = Data[profile_row_id]['data'][i][1];
				    }
				    else {
				     td.innerHTML = 0;
				    }
				}
				
				if (i == 0) {
				    td = tr.insertCell();
					td.innerHTML = '0';
					//
					td = tr.insertCell();
					td.innerHTML = '0';
				}
				else {
					td = tr.insertCell();
					el = document.createElement('input');
					el.value = 0;
					el.setAttribute('type', 'text');
					el.setAttribute('name', `${table_name}-input-${i}`);
					el.setAttribute('id', `${table_name}-length_sec-${i}`);
					el.setAttribute('onchange', 'Calc("'+table_name+'", 2)');
					td.appendChild(el);

					//
					td = tr.insertCell();
					el = document.createElement('input');
					el.value = 0;
					el.setAttribute('type', 'text');
					el.setAttribute('name', `${table_name}-input-${i}`);
					el.setAttribute('id', `${table_name}-dt-${i}`);
					el.setAttribute('onchange', 'Calc("'+table_name+'", 2)');
					td.appendChild(el);
               }

    }
    table.appendChild(tbl);
	Calc(table_name, 1);
 });

}
TableSettings();
TableCreate(0, "top");
TableCreate(1, "bottom");
TableCreate(2, "pcb");

function Calc(table_name, calc_type) {
	  var prev_time = 0;
	  var v_time;
	  var temp_start = parseInt(document.getElementById(table_name+"-temp-start-0").value);
		
	  var temp_top = [];
	  
	  var table = document.getElementById(table_name+"-calc");
		for (var i = 0, row; row = table.rows[i]; i++) {
		    
			if (i > 1) {
			    v_time = parseInt(document.getElementById(table_name+"-time-"+(i-1)).value);
			
			     if (calc_type == 1) {
				   document.getElementById(table_name+"-length_sec-"+(i-1)).value =  v_time - prev_time;	
				   document.getElementById(table_name+"-dt-"+(i-1)).value = parseInt(row.cells[2].innerHTML) - temp_start;		
				 }
				 else
				 {
				   //row.cells[1].innerHTML = prev_time + parseInt(document.getElementById(table_name+"-length_sec-"+(i-1)).value);	
				   document.getElementById(table_name+"-time-"+(i-1)).value = prev_time + parseInt(document.getElementById(table_name+"-length_sec-"+(i-1)).value);
				   row.cells[2].innerHTML = temp_start + parseInt(document.getElementById(table_name+"-dt-"+(i-1)).value);			
				 }
				 prev_time = v_time;//parseInt(row.cells[1].innerHTML);	
				 temp_start = parseInt(row.cells[2].innerHTML);		
				 
				 if (v_time > 0 && temp_start > 0) {
                   temp_top.push([ parseFloat(v_time), parseFloat(temp_start)]); 			
                 }				   
	        }
			
			if (i == 1) {
			  prev_time = 0;	
			}			
		}
       
	  if (table_name == "top")
        chart.series[0].setData(temp_top);
       if (table_name == "bottom")
        chart.series[1].setData(temp_top);	
      if (table_name == "pcb")
        chart.series[2].setData(temp_top);				

}

function SaveProfile() {
   var obj = {};
   obj.time_step_top = []; 
   obj.temperature_step_top = []; 
   obj.time_step_bottom = []; 
   obj.temperature_step_bottom = [];
   obj.time_step_pcb = []; 
   obj.temperature_step_pcb = [];
   obj.setings = {};
   
   var table_top = document.getElementById("top-calc");
   var table_bottom = document.getElementById("bottom-calc");
   var table_pcb = document.getElementById("pcb-calc");
   
	for (var i = 1, row; row = table_top.rows[i]; i++) {
		  //obj.time_step_top.push(row.cells[1].innerText);
      obj.time_step_top.push(parseInt(document.getElementById("top-time-"+(i-1)).value));
      if (i == 1) {
        obj.temperature_step_top.push(parseInt(document.getElementById("top-temp-start-0").value));
      }
		  else {
        obj.temperature_step_top.push(row.cells[2].innerText);
      }
  }
   for (var i = 1, row; row = table_bottom.rows[i]; i++) {   
	   //obj.time_step_bottom.push(row.cells[1].innerText);
	   obj.time_step_bottom.push(parseInt(document.getElementById("bottom-time-"+(i-1)).value));
       if (i == 1) {
        obj.temperature_step_bottom.push(parseInt(document.getElementById("bottom-temp-start-0").value));
      }
		  else {
		    obj.temperature_step_bottom.push(row.cells[2].innerText);
      }
  }
	for (var i = 1, row; row = table_pcb.rows[i]; i++) {
	  //obj.time_step_pcb.push(row.cells[1].innerText);
	  obj.time_step_pcb.push(parseInt(document.getElementById("pcb-time-"+(i-1)).value));
      if (i == 1) {
        obj.temperature_step_pcb.push(parseInt(document.getElementById("pcb-temp-start-0").value));
      }
		  else {
		    obj.temperature_step_pcb.push(row.cells[2].innerText);
      }
  }

    obj.setings["profile_steps"] = document.getElementById('profile_steps').value; 
	obj.setings["table_size"] = document.getElementById('table_size').value; 
	obj.setings["kp1"] = document.getElementById('kp1').value; 
	obj.setings["ki1"] = document.getElementById('ki1').value; 
	obj.setings["kd1"] = document.getElementById('kd1').value;
	obj.setings["kp2"] = document.getElementById('kp2').value; 
	obj.setings["ki2"] = document.getElementById('ki2').value; 
	obj.setings["kd2"] = document.getElementById('kd2').value; 
	obj.setings["kp3"] = document.getElementById('kp3').value; 
	obj.setings["ki3"] = document.getElementById('ki3').value; 
	obj.setings["kd3"] = document.getElementById('kd3').value; 
	obj.setings["max_correction_top"] = document.getElementById('max_correction_top').value; 
	obj.setings["max_correction_bottom"] = document.getElementById('max_correction_bottom').value; 
	obj.setings["max_pcb_delta"] = document.getElementById('max_pcb_delta').value;
	obj.setings["hold_lenght"] = document.getElementById('hold_lenght').value; 
	obj.setings["participation_rate_top"] = document.getElementById('participation_rate_top').value; 
    obj.setings["alias"] = document.getElementById('aliasprofile').value; 
	  
	//alert(JSON.stringify(obj));
	console.log(JSON.stringify(obj));
	
	$.ajax({
      type: "POST",
      url: "./SaveProfile",
      data: JSON.stringify(obj),
      contentType: "application/json",
      dataType: "json",
      success: function(data) {
         console.log("Data added!", data);
      }
	});
	 document.getElementById("info").innerHTML = 'Изменения успешно сохранены.';
	
}
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
          series: 
		  [
    {
      "name": "Температура Верх",
      "type": "line",
      "color": "#C74A20"
	},
	{
      "name": "Температура Низ",
      "type": "line",
      "color": "#7F32C7"
	},
	{
      "name": "Температура Платы",
      "type": "line",
      "color": "#699a32"
	}	
]

        });


 //Calc("top");
 //Calc("bottom");	
 //Calc("pcb");
 
	</script>
  </body>
</html>

)rawliteral";
#endif