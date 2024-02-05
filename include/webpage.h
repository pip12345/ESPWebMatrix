#ifndef __WEBPAGE_H__
#define __WEBPAGE_H__

const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML><html>
    <head>
      <title>LED Matrix Control Panel</title>
      <style>
      
       body {
          font: normal 15px Verdana;
          color: black;
          text-align: center;
          background-color: rgb(255, 255, 255);
       }
      
       .header {
          font-family: "Arial", sans-serif;
          padding: 10px;
          text-align: center;
          background: #4CAF50;
          color: white;
          font-size: 11px;
        }
      
        .p1 {
          font-family: "Arial", sans-serif;
          text-align: left;
        }
        
        .button {
          border: none;
          color: white;
          padding: 15px 32px;
          text-decoration: none;
          display: inline-block;
          font-size: 16px;
          margin: 4px 2px;
          cursor: pointer;
          border-radius: 15px;
          text-align: center;
        }
        .button1 {background-color: #4CAF50;}
        .button2 {background-color: rgb(22, 133, 49);}
        .button3 {background-color: #4CAF50;}
        .button4 {background-color: #fc4646;}
    
        button:focus{background-color:rgb(14, 78, 30);}

        input[type=text] {
          width: 50%;
          padding: 8px 10px;
          margin: 8px 0;
          box-sizing: border-box;
          border: solid #555;
          border-radius: 4px;
        }

        input[type=text2] {
          width: 20%;
          padding: 8px 10px;
          margin: 8px 0;
          box-sizing: border-box;
          border: solid #555;
          border-radius: 4px;
        }
        
        input[type=submit] {
          width: 20%;
          padding: 8px 10px;
          margin: 8px 0;
          box-sizing: border-box;
          border: solid #555;
          border-radius: 4px;
        }
        
        .slidecontainer {
          width: 100%;
        }
    
        .slider {
          -webkit-appearance: none;
          width: 100%;
          height: 25px;
          background: #d3d3d3;
          outline: none;
          opacity: 0.7;
        }
    
        .slider::-moz-range-thumb {
          width: 25px;
          height: 25px;
          background: #4CAF50;
          cursor: pointer;
        }

        .smallheader {
          font-family: "Verdana", serif;
          padding: 0px;
          text-align: center;
          background: #4CAF50;
          color: rgb(255, 255, 255);
          font-size: 20px;
        }

        .description {
          font-size: 10px;
          text-align: center;
        }

        .collapsible {
          background-color: #5c5c5c;
          color: white;
          cursor: pointer;
          padding: 5px;
          width: 100%;
          border: none;
          text-align: center;
          outline: none;
          font-size: 15px;
        }

        .active, .collapsible:hover {
          background-color: rgb(202, 37, 37);
        }

        .content {
          padding: 0 18px;
          max-height: 0;
          overflow: hidden;
          transition: max-height 0.2s ease-out;
          background-color: #f1f1f1;
        }
        
        .content2 {
          padding: 18px;
          background-color: #f1f1f1;
        }
      </style>
      <script>
        function updateSlider(element) {
          var sliderValue = document.getElementById("sliderValue").value;
          document.getElementById("textSliderValue").innerHTML = sliderValue;
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/slider?scroll_value="+sliderValue, true);
          xhr.send();
        }

        function ButtonRequestDebug() {
          var xhttp = new XMLHttpRequest();
          xhttp.open("GET", "/ButtonRequestDebug", true);
          xhttp.send();
          setTimeout(function(){location.reload()}, 50); // Delay so ESP can catch the request before reload
        }

        function ButtonRequestScrollSingle() {
          var xhttp = new XMLHttpRequest();
          xhttp.open("GET", "/ButtonRequestScrollSingle", true);
          xhttp.send();
          setTimeout(function(){location.reload()}, 50);
        }

        function ButtonRequestScrollRGB() {
          var xhttp = new XMLHttpRequest();
          xhttp.open("GET", "/ButtonRequestScrollRGB", true);
          xhttp.send();
          setTimeout(function(){location.reload()}, 50);
        }

        function ButtonRequestFlash() {
          var xhttp = new XMLHttpRequest();
          xhttp.open("GET", "/ButtonRequestFlash", true);
          xhttp.send();
          setTimeout(function(){location.reload()}, 50);
        }

        function ButtonRequestSaveSettings() {
          var xhttp = new XMLHttpRequest();
          xhttp.open("GET", "/ButtonRequestSaveSettings", true);
          xhttp.send();
          setTimeout(function(){location.reload()}, 50);
        }

        function ButtonRequestLoadSettings() {
          var xhttp = new XMLHttpRequest();
          xhttp.open("GET", "/ButtonRequestLoadSettings", true);
          xhttp.send();
          setTimeout(function(){location.reload()}, 50);
        }

        function ButtonRequestBattery() {
          var xhttp = new XMLHttpRequest();
          xhttp.open("GET", "/ButtonRequestBattery", true);
          xhttp.send();
          setTimeout(function(){location.reload()}, 50);
        }

      </script>
      <meta name="viewport" content="width=device-width, initial-scale=1">
    </head>
    
    
    <body>
      <div class="header">
          <h1>LED Matrix Control Panel</h1>
      </div> 
  
      <div class="slidecontainer">
        <p><span hidden id="textSliderValue">%SLIDERVALUE%</span></p> <!-- Hack om slidervalue uit te kunnen lezen --> 
          <input type="range" min="0" max="100" value="%SLIDERVALUE%" class="slider" id="sliderValue" oninput="this.nextElementSibling.value = this.value" onchange="updateSlider(this)">
        Brightness <output></output>%
      </div><br>
    
      <div class="smallheader">
        <button class="button button2" type="button" onclick="ButtonRequestScrollSingle();">Select Scroll Single Color</button>
      </div>
      <div class="content2">
        <p class="description">Program will scroll the text with the selected RGB color</p>
        Display Text
        <form action="/get">
            <input type="text" name="input_text">
            <input type="submit" value="Submit">
        </form><br>
    
        Scroll Speed (letters per second) <br>
        <form action="/get">
          <input type="text" name="input_speed">
          <input type="submit" value="Submit">
        </form><br>

        <form action="/get">
          Color <br>
          R
          <input type="text2" name="input_red">
          G
          <input type="text2" name="input_green">
          B
          <input type="text2" name="input_blue">
          &nbsp&nbsp&nbsp
          <input type="submit" value="Submit">
        </form><br>
      </div>

      <div class="smallheader">
        <button class="button button2" type="button" onclick="ButtonRequestScrollRGB();">Select Scroll RGB Colors</button>
      </div>
      <div class="content2">
        <p class="description">Program will scroll the text alternating between red, green and blue</p>

        Display Text
        <form action="/get">
            <input type="text" name="input_text">
            <input type="submit" value="Submit">
        </form><br>

        Scroll Speed (letters per second) <br>
        <form action="/get">
          <input type="text" name="input_speed">
          <input type="submit" value="Submit">
        </form><br>
      </div>
        
      <div class="smallheader">
        <button class="button button2" type="button" onclick="ButtonRequestFlash();">Select Flash</button>
      </div>
      <div class="content2">
        <p class="description">Program will flash between two different texts with the selected RGB color</p>

        Display Text 1
        <form action="/get">
            <input type="text" name="input_text">
            <input type="submit" value="Submit">
        </form><br>

        Display Text 2
        <form action="/get">
            <input type="text" name="input_text2">
            <input type="submit" value="Submit">
        </form><br>

        Flash Speed (seconds between flash) <br>
        <form action="/get">
          <input type="text" name="input_speed">
          <input type="submit" value="Submit">
        </form><br>
        
        <form action="/get">
          Color <br>
          R
          <input type="text2" name="input_red">
          G
          <input type="text2" name="input_green">
          B
          <input type="text2" name="input_blue">
          &nbsp&nbsp&nbsp
          <input type="submit" value="Submit">
        </form><br>
      </div>
        
      <button class="button button3" type="button" onclick="ButtonRequestSaveSettings();">Save Settings</button>
      <button class="button button3" type="button" onclick="ButtonRequestLoadSettings();">Load Settings</button><br>
      <button class="button button3" type="button" onclick="ButtonRequestBattery();">Show Battery Status</button>
  
      <button class="collapsible">Network Settings</button>
      <div class="content">
        Change wifi password<br>
        Requires reboot to apply!<br>
        Enter current and new password
        <form action="/get">
            Current: <input type="text" name="input_password_old"><br>
            New:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<input type="text" name="input_password_new"><br>
            <input type="submit" value="Change">
        </form><br>
        <button class="button button1" type="button" onclick="ButtonRequestDebug();">Toggle WHITE BACKLIGHT DEBUG</button><br>
        <small> Warning, may use a lot of power </small>
      </div>
      <footer> <small>Copyright &copy; 2021, ledpoint. All rights reserved.</small> </footer> 
      <script>
        // Remember scroll position after page reload
        document.addEventListener("DOMContentLoaded", function(event) { 
            var scrollpos = localStorage.getItem('scrollpos');
            if (scrollpos) window.scrollTo(0, scrollpos);
        });

        window.onbeforeunload = function(e) {
            localStorage.setItem('scrollpos', window.scrollY);
        };

        // Collapsible
        var coll = document.getElementsByClassName("collapsible");
        var i;

        for (i = 0; i < coll.length; i++) {
          coll[i].addEventListener("click", function() {
            this.classList.toggle("active");
            var content = this.nextElementSibling;
            if (content.style.maxHeight){
              content.style.maxHeight = null;
            } else {
              content.style.maxHeight = content.scrollHeight + "px";
            }
          });
        }
      </script>
    </body>
    </html>
)rawliteral";
#endif // __WEBPAGE_H__