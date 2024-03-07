#include <WiFi.h>
#include <WebServer.h>


#define BUTTON_BOTTOM_LINE_1 2
#define BUTTON_TOP_LINE_1 15
#define BUTTON_BOTTOM_LINE_2 16
#define BUTTON_TOP_LINE_2 4
#define BEEPER 18


const bool own_hotspot = true;
const char *ssid = "ahoi";
const char *password = "88104676";
WebServer server(80);

//race states
#define STATE_IDLE 1
#define STATE_ARRANGING 2
#define STATE_BEEPING 3
#define STATE_REACTION 4
#define STATE_RACING 5
#define STATE_SHOWTIME 6
int race_last_state = 0;
int race_state = STATE_IDLE;

bool bottom_line1_state = HIGH;  // initial state of the button
bool top_line1_state = HIGH;     // initial state of the button

bool falschstart = false;
unsigned long ARRANGING_last_press = 0;
unsigned long ARRANGING_count = 0;
unsigned long BEEPING_falschstart_threshold = 0;
unsigned long REACTION_release_timestamp = 0;
unsigned long RACE_top_press_timestamp = 0;
bool HAS_falshstart = false;
bool HAS_reaction = false;
bool HAS_climbtime = false;


void handle_bottom_line1_state_arrange() {
  bool currentState = digitalRead(BUTTON_BOTTOM_LINE_1);
  if (currentState != bottom_line1_state) {  // if changed state
    bottom_line1_state = currentState;
    if (bottom_line1_state == LOW) {  // if pressed
      ARRANGING_last_press = millis();
      Serial.println("Apressed");
    } else if (bottom_line1_state == HIGH) {  // if released
      ARRANGING_last_press = 0;
      Serial.println("Areleased");
    }
  }
}
void handle_bottom_line1_state_beeping() {
  bool currentState = digitalRead(BUTTON_BOTTOM_LINE_1);
  if (currentState != bottom_line1_state) {  // if changed state
    bottom_line1_state = currentState;
    if (bottom_line1_state == HIGH) {  // if released
      HAS_falshstart = true;
      falschstart = true;
      detachInterrupt(digitalPinToInterrupt(BUTTON_BOTTOM_LINE_1));
      Serial.println("Bfalschstart");
      race_state = STATE_SHOWTIME;
    }
  }
}
void handle_bottom_line1_state_reaction() {
  bool currentState = digitalRead(BUTTON_BOTTOM_LINE_1);
  if (currentState != bottom_line1_state) {  // if changed state
    bottom_line1_state = currentState;
    if (bottom_line1_state == HIGH) {  // if released

      REACTION_release_timestamp = millis();
      if (REACTION_release_timestamp - BEEPING_falschstart_threshold < 100) {
        HAS_falshstart = true;
        falschstart = true;
        Serial.println("Rfalschstart");
        detachInterrupt(digitalPinToInterrupt(BUTTON_BOTTOM_LINE_1));
        race_state = STATE_SHOWTIME;
      } else {
        HAS_reaction = true;
        Serial.println("R1: " + String(REACTION_release_timestamp - BEEPING_falschstart_threshold) + " ms");
        attachInterrupt(digitalPinToInterrupt(BUTTON_TOP_LINE_1), handle_top_line1_state_race, CHANGE);
        detachInterrupt(digitalPinToInterrupt(BUTTON_BOTTOM_LINE_1));
        race_state = STATE_RACING;
      }
    }
  }
}
void handle_top_line1_state_race() {
  bool currentState = digitalRead(BUTTON_TOP_LINE_1);
  if (currentState == LOW) {  // if pressed
    race_state = STATE_SHOWTIME;
    HAS_climbtime = true;
    RACE_top_press_timestamp = millis();
    Serial.println("Tpressed");
    detachInterrupt(digitalPinToInterrupt(BUTTON_TOP_LINE_1));
    Serial.println("C1: " + String(RACE_top_press_timestamp - REACTION_release_timestamp));
  }
}

void setup() {
  //pin configuration
  pinMode(BEEPER, OUTPUT);
  pinMode(BUTTON_BOTTOM_LINE_1, INPUT_PULLUP);
  pinMode(BUTTON_TOP_LINE_1, INPUT_PULLUP);

  //serial
  gpio_install_isr_service(0); // Install GPIO ISR service
  Serial.begin(9600);

  //wifi setup
  if (own_hotspot) {
    Serial.println("Setting up host");
    WiFi.softAP(ssid, password);
    Serial.print("Host opened at ");
    Serial.print(ssid);
    Serial.print(" with ");
    Serial.println(password);
  } else {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }


  //server endpoints
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", mainPage());
  });

  server.on("/variables", HTTP_GET, []() {
    String variables = "";
    if (HAS_falshstart) variables += String(falschstart) + "\n";
    else variables += "-1\n";
    if (HAS_climbtime) variables += String(RACE_top_press_timestamp - REACTION_release_timestamp) + "\n";
    else variables += "-1\n";
    if (HAS_reaction) variables += String(REACTION_release_timestamp - BEEPING_falschstart_threshold) + "\n";
    else variables += "-1\n";
    variables += String(race_state);
    server.send(200, "text/plain", variables);
  });
  // server.on("/font.ttf", HTTP_GET, []() {
  //   server.sendHeader("Content-Disposition", "attachment; filename=font.ttf");
  //   server.sendHeader("Content-Type", "font/ttf");
  //   server.sendHeader("Connection", "close");
  //   server.send(200, "application/octet-stream", (const char *)KodeMono_Medium20pt7bBitmaps, sizeof(KodeMono_Medium20pt7bBitmaps));  // Serve font data
  // });
  server.on("/action", HTTP_POST, []() {
    if (server.hasArg("action")) {
      String action = server.arg("action");
      if (action == "remove_person_1") {
      } else if (action == "add_person_1") {
      } else if (action == "remove_person_2") {
      } else if (action == "add_person_2") {
      } else if (action == "fall_1") {
      } else if (action == "fall_2") {
      } else if (action == "start") {
        if (race_state == STATE_IDLE) {
          Serial.println("UI reqested action " + action);
          race_state = STATE_ARRANGING;
          detachInterrupt(digitalPinToInterrupt(BUTTON_BOTTOM_LINE_1));
          attachInterrupt(digitalPinToInterrupt(BUTTON_BOTTOM_LINE_1), handle_bottom_line1_state_arrange, CHANGE);
        }
      } else if (action == "stop") {
      } else if (action == "reset") {
        Serial.println("UI reqested action " + action);
        race_state = STATE_IDLE;
        detachInterrupt(digitalPinToInterrupt(BUTTON_BOTTOM_LINE_1));
        detachInterrupt(digitalPinToInterrupt(BUTTON_TOP_LINE_1));
        ARRANGING_last_press = 0;
        ARRANGING_count = 0;
        BEEPING_falschstart_threshold = 0;
        REACTION_release_timestamp = 0;
        RACE_top_press_timestamp = 0;
        falschstart = false;
        HAS_climbtime = false;
        HAS_falshstart = false;
        HAS_reaction = false;
      }
    } else {
      server.send(400, "text/plain", "Bad Request: button_presssed parameter missing");
    }
  });


  server.begin();
}
void start_count_delayed() {
  Serial.println("beeping");
  digitalWrite(BEEPER, LOW);
  delay(1000);  //1s
  digitalWrite(BEEPER, HIGH);
  delay(200);  //1.2s
  digitalWrite(BEEPER, LOW);
  delay(800);  //2s
  digitalWrite(BEEPER, HIGH);
  delay(200);  //2.2s
  digitalWrite(BEEPER, LOW);
  delay(800);  //3s
  digitalWrite(BEEPER, HIGH);
  if (!falschstart) {
    race_state = STATE_REACTION;
    BEEPING_falschstart_threshold = millis();
    detachInterrupt(digitalPinToInterrupt(BUTTON_BOTTOM_LINE_1));
    attachInterrupt(digitalPinToInterrupt(BUTTON_BOTTOM_LINE_1), handle_bottom_line1_state_reaction, CHANGE);
  }
  delay(100);  //3.1s
  digitalWrite(BEEPER, LOW);
}

void loop() {

  if (race_state != race_last_state) {
    race_last_state = race_state;
    Serial.print("STATUS = ");
    Serial.println(String(race_state));
    if (race_state == STATE_BEEPING) {
      start_count_delayed();
    }
  }
  if (race_state == STATE_ARRANGING) {
    delay(100);
    if (LOW == digitalRead(BUTTON_BOTTOM_LINE_1)) {
      ARRANGING_count = millis();
      if (ARRANGING_count - ARRANGING_last_press >= 3000) {
        race_state = STATE_BEEPING;

        Serial.println("arranged: "+String(ARRANGING_count - ARRANGING_last_press));
        detachInterrupt(digitalPinToInterrupt(BUTTON_BOTTOM_LINE_1));
        attachInterrupt(digitalPinToInterrupt(BUTTON_BOTTOM_LINE_1), handle_bottom_line1_state_beeping, CHANGE);
        server.handleClient();
      }
    }
  } else {
    server.handleClient();
  }
}
String mainPage() {
  String page = "";
  page += "<!DOCTYPE html>\n";
  page += "<html lang='de'>\n";

  page += "<head>\n";
  page += "    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
  page += "    <link rel='stylesheet'\n";
  page += "        href='https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined:opsz,wght,FILL,GRAD@20..48,100..700,0..1,-50..200' />\n";
  page += "    <title>Timer</title>\n";
  page += "</head>\n";

  page += "<body class='default-scheme'>\n";
  page += "    <nav class='timer-wrapers'>\n";
  page += "        <nav class='timer-wrapper' id='timer-wrapper-1'>\n";
  page += "            <button class='line-removal-button material-symbols-outlined' id='line-removal-button-1'\n";
  page += "                onclick='reportAction(`remove_person_1`)'>-</button>\n";
  page += "            <nav class='times-wrapper' id='times-wrapper-1'>\n";
  page += "                <nav class='timer-time' id='timer-time-1'>0.000</nav>\n";
  page += "                <nav class='reaction-time' id='reaction-time-1'>Reaction: 0.150ms</nav>\n";
  page += "            </nav>\n";
  page += "            <button class='fall-button' id='button-fall-1' onclick='reportAction(`fall_1`)'>fall</button>\n";
  page += "            <button class='line-addition-button material-symbols-outlined' id='line-addition-button-1'\n";
  page += "                onclick='reportAction(`add_person_1`)'>+</button>\n";
  page += "        </nav>\n";
  page += "        <nav class='timer-wrapper' id='timer-wrapper-2'>\n";
  page += "            <button class='line-removal-button material-symbols-outlined' id='line-removal-button-2'\n";
  page += "                onclick='reportAction(`remove_person_2`)'>-</button>\n";
  page += "            <nav class='times-wrapper' id='times-wrapper-2'>\n";
  page += "                <nav class='timer-time' id='timer-time-2'>0.000</nav>\n";
  page += "                <nav class='reaction-time' id='reaction-time-2'>Reaction: 0.150ms</nav>\n";
  page += "            </nav>\n";
  page += "            <button class='fall-button' id='button-fall-2' onclick='reportAction(`fall_2`)'>fall</button>\n";
  page += "            <button class='line-addition-button material-symbols-outlined' id='line-addition-button-2'\n";
  page += "                onclick='reportAction(`add_person_2`)'>+</button>\n";
  page += "        </nav>\n";
  page += "    </nav>\n";
  page += "    <nav class='controls-wrapper'>\n";
  page += "        <nav class='progress' id='main_progress'></nav>\n";
  page += "        <button class='main-button' id='main-button' onclick='reportAction(`start`)'>START</button>\n";
  page += "    </nav>\n";
  page += "</body>\n";
  page += "<style>\n";
  page += "    @import url('https://fonts.googleapis.com/css2?family=Kode+Mono:wght@400..700&display=swap');\n";
  page += "    @media (prefers-color-scheme: dark) {\n";
  page += "        .default-scheme {\n";
  page += "            --main-color: #333333;\n";
  page += "            --help-color: #D65A31;\n";
  page += "            --back-color: #222222;\n";
  page += "            --tone-color: #ffffff;\n";
  page += "        }\n";
  page += "    }\n";

  page += "    @media (prefers-color-scheme: light) {\n";
  page += "        .default-scheme {\n";
  page += "            --main-color: #f8efcc;\n";
  page += "            --help-color: #ee6a0f;\n";
  page += "            --back-color: #589ca8;\n";
  page += "            --tone-color: #042850;\n";
  page += "        }\n";
  page += "    }\n";

  page += "    .dark-scheme {\n";
  page += "        --main-color: #333333;\n";
  page += "        --help-color: #D65A31;\n";
  page += "        --back-color: #222222;\n";
  page += "        --tone-color: #ffffff;\n";
  page += "    }\n";

  page += "    .light-scheme {\n";
  page += "        --main-color: #f8efcc;\n";
  page += "        --help-color: #ee6a0f;\n";
  page += "        --back-color: #589ca8;\n";
  page += "        --tone-color: #042850;\n";
  page += "    }\n";

  page += "    * {\n";
  page += "        padding: 0;\n";
  page += "        margin: 0;\n";
  page += "        color: var(--tone-color);\n";
  page += "        --border-width: 3px;\n";
  page += "        user-select: none;\n";
  page += "        font-family: 'Kode Mono', monospace;\n";
  page += "        transition: opacity 0.25s, background-color 0.5s, color 0.5s, border 0.5s, box-shadow 0.5s, background-image 0.5s;\n";
  page += "    }\n";

  page += "    body {\n";
  page += "        background-color: var(--back-color);\n";
  page += "        display: flex;\n";
  page += "        justify-content: space-evenly;\n";
  page += "        flex-direction: column;\n";
  page += "        align-items: center;\n";
  page += "        width: 100vw;\n";
  page += "        height: 100vh;\n";
  page += "        overflow: hidden;\n";
  page += "        background-size: calc(100vh * 0.048381743) calc(100vh * 0.048381743);\n";
  page += "        background-image:\n";
  page += "            linear-gradient(to right, var(--main-color) 2px, transparent 1px),\n";
  page += "            linear-gradient(to bottom, var(--main-color) 2px, transparent 1px);\n";
  page += "    }\n";

  page += "    .timer-wrapers {\n";
  page += "        width: calc(95% + var(--border-width) + var(--border-width));\n";
  page += "        height: 70%;\n";
  page += "        display: flex;\n";
  page += "        justify-content: space-between;\n";
  page += "        flex-direction: row;\n";
  page += "    }\n";

  page += "    .controls-wrapper {\n";
  page += "        width: 95%;\n";
  page += "        height: 15%;\n";
  page += "        border: var(--tone-color) solid var(--border-width);\n";
  page += "        border-radius: 20px;\n";
  page += "        background-color: var(--main-color);\n";
  page += "        box-shadow: 5px 5px var(--tone-color);\n";
  page += "    }\n";

  page += "    .timer-wrapper {\n";
  page += "        width: 48.5%;\n";
  page += "        height: 100%;\n";
  page += "        border: var(--tone-color) solid var(--border-width);\n";
  page += "        border-radius: 20px;\n";
  page += "        background-color: var(--main-color);\n";
  page += "        box-shadow: 5px 5px var(--tone-color);\n";
  page += "        display: flex;\n";
  page += "        flex-direction: column;\n";
  page += "        justify-content: space-between;\n";
  page += "        transition: width 0.5s, opacity 0.5s, background-color 0.5s, border 0.5s, box-shadow 0.5s;\n";
  page += "    }\n";

  page += "    .line-removal-button {\n";
  page += "        width: 12.5%;\n";
  page += "        height: 15%;\n";
  page += "        margin-left: 87.5%;\n";
  page += "        border: 0;\n";
  page += "        background-color: transparent;\n";
  page += "        font-size: 48px;\n";
  page += "        border-style: dashed;\n";
  page += "        border-color: var(--tone-color);\n";
  page += "        border-bottom-width: 3px;\n";
  page += "        border-left-width: 3px;\n";
  page += "    }\n";

  page += "    .line-addition-button {\n";
  page += "        width: 100%;\n";
  page += "        height: 100%;\n";
  page += "        border: 0;\n";
  page += "        background-color: transparent;\n";
  page += "        font-size: 48px;\n";
  page += "        display: none;\n";
  page += "    }\n";

  page += "    .material-symbols-outlined {\n";
  page += "        font-variation-settings:\n";
  page += "            'opsz' 48,\n";
  page += "            'FILL' 0,\n";
  page += "            'wght' 400,\n";
  page += "            'GRAD' 0\n";
  page += "    }\n";

  page += "    .times-wrapper {\n";
  page += "        height: 50%;\n";
  page += "        display: flex;\n";
  page += "        flex-direction: column;\n";
  page += "        align-items: center;\n";
  page += "        overflow: hidden;\n";
  page += "        z-index: 1;\n";
  page += "    }\n";

  page += "    .fall-button {\n";
  page += "        height: 10%;\n";
  page += "        border: 0;\n";
  page += "        border-bottom-right-radius: 16px;\n";
  page += "        border-bottom-left-radius: 16px;\n";
  page += "        background-color: var(--help-color);\n";
  page += "        border-style: dashed;\n";
  page += "        border-color: var(--tone-color);\n";
  page += "        border-top-width: 3px;\n";
  page += "        font-size: 2.5vh;\n";
  page += "        position: relative;\n";
  page += "        transition: opacity 0.5s, height 0.5s, border 0.5s, color 0.5s;\n";
  page += "    }\n";

  page += "    .timer-time {\n";
  page += "        font-size: 20vh;\n";
  page += "    }\n";

  page += "    .main-button {\n";
  page += "        width: 100%;\n";
  page += "        height: 100%;\n";
  page += "        border-radius: 16px;\n";
  page += "        background-color: transparent;\n";
  page += "        border: 0;\n";
  page += "        font-size: 5vh;\n";
  page += "        position: relative;\n";
  page += "        top: -100%;\n";
  page += "    }\n";

  page += "    .progress {\n";
  page += "        width: 100%;\n";
  page += "        height: 100%;\n";
  page += "        border-radius: 16px;\n";
  page += "        background-color: var(--help-color);\n";
  page += "        transition: width 3s linear;\n";
  page += "    }\n";
  page += "</style>\n";
  page += "<script>\n";
  page += "    const STATE_IDLE = 1\n";
  page += "    const STATE_ARRANGING = 2\n";
  page += "    const STATE_BEEPING = 3\n";
  page += "    const STATE_REACTION = 4\n";
  page += "    const STATE_RACING = 5\n";
  page += "    const STATE_SHOWTIME = 6\n";
  page += "    const states =['STATE_IDLE','STATE_ARRANGING','STATE_BEEPING','STATE_REACTION','STATE_RACING','STATE_SHOWTIME']\n";
  page += "    var race_state = 1;\n";
  page += "    var last_race_state = 1;\n";
  page += "    var interval;\n";
  page += "    var requestInProgress = false;\n";
  page += "    var timer1milliseconds = 0;\n";
  page += "    var timer_interval;\n";
  page += "    var HAS_climbtime = false;\n";
  page += "    var HAS_reaction = false;\n";
  page += "    function timerticker() {\n";
  page += "        timer1milliseconds += 100;\n";
  page += "        document.getElementById('timer-time-1').innerText = `${(timer1milliseconds/1000.0).toFixed(3)}s`;\n";
  page += "    }\n";
  page += "    function makeRequest() {\n";
  page += "        if (!requestInProgress) {\n";
  page += "            requestInProgress = true;\n";
  page += "            var xhttp = new XMLHttpRequest();\n";
  page += "            xhttp.onreadystatechange = function () {\n";
  page += "                if (this.readyState == 4) {\n";
  page += "                    requestInProgress = false; // Reset request status\n";
  page += "                    if (this.status == 200) {\n";
  page += "                        var variables = this.responseText.split('\\n');\n";
  page += "                        var falschstart = variables[0];\n";
  page += "                        var climbtime = variables[1];\n";
  page += "                        var reaction = variables[2];\n";
  page += "                        if (climbtime != '-1' && !HAS_climbtime) {\n";
  page += "                            document.getElementById('timer-time-1').innerText = `${(climbtime - 0) / 1000.0}s`;\n";
  page += "                            HAS_climbtime = true;\n";
  page += "                        }\n";
  page += "                        document.getElementById('timer-time-2').innerText = 'untracked';\n";
  page += "                        if (reaction != '-1' && !HAS_reaction) {\n";
  page += "                            document.getElementById('reaction-time-1').innerText = `Reaction: ${reaction}ms`;\n";
  page += "                            HAS_reaction = true;\n";
  page += "                        }\n";
  page += "                        document.getElementById('reaction-time-2').innerText = 'untracked';\n";
  page += "                        var race_state = parseInt(variables[3]); // Parse as integer\n";
  page += "                        if (race_state != last_race_state) {\n";
  page += "                            console.info('CHANGED STATE TO ' + race_state);\n";
  page += "                            last_race_state = race_state; // Update last_race_state\n";
  page += "                            document.title = states[race_state - 1];\n";
  page += "                            if (race_state == STATE_RACING) {\n";
  page += "                                timer_interval = setInterval(timerticker, 100);\n";
  page += "                            } else if (race_state == STATE_SHOWTIME) {\n";
  page += "                                clearInterval(timer_interval);\n";
  page += "                            }\n";
  page += "                        }\n";
  page += "                    } else {\n";
  page += "                        console.error('Request failed with status: ' + this.status);\n";
  page += "                    }\n";
  page += "                }\n";
  page += "            };\n";
  page += "            xhttp.open('GET', '/variables', true);\n";
  page += "            xhttp.send();\n";
  page += "        }\n";
  page += "    }\n";
  page += "    interval = setInterval(makeRequest, 1000);\n";
  page += "    function reportAction(action) {\n";
  page += "        console.info(action);\n";
  page += "        var xhttp = new XMLHttpRequest();\n";
  page += "        xhttp.open('POST', '/action', true);\n";
  page += "        xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');\n";
  page += "        if (action == 'remove_person_1') {\n";
  page += "            document.getElementById('timer-wrapper-1').style.width = '5%'\n";
  page += "            document.getElementById('timer-wrapper-1').style.opacity = '0.75'\n";
  page += "            document.getElementById('timer-wrapper-2').style.width = '92.5%'\n";
  page += "            document.getElementById('timer-wrapper-2').style.opacity = '1'\n";
  page += "            document.getElementById('line-addition-button-1').style.display = 'block'\n";
  page += "            document.getElementById('line-removal-button-1').style.display = 'none'\n";
  page += "            document.getElementById('line-removal-button-2').style.opacity = '0'\n";
  page += "            document.getElementById('times-wrapper-1').style.display = 'none'\n";
  page += "            document.getElementById('button-fall-1').style.display = 'none'\n";
  page += "        }\n";
  page += "        else if (action == 'add_person_1') {\n";
  page += "            document.getElementById('timer-wrapper-1').style.width = '48.5%'\n";
  page += "            document.getElementById('timer-wrapper-1').style.opacity = '1'\n";
  page += "            document.getElementById('timer-wrapper-2').style.width = '48.5%'\n";
  page += "            document.getElementById('timer-wrapper-2').style.opacity = '1'\n";
  page += "            document.getElementById('line-addition-button-1').style.display = 'none'\n";
  page += "            document.getElementById('line-removal-button-1').style.display = 'block'\n";
  page += "            document.getElementById('line-removal-button-2').style.opacity = '1'\n";
  page += "            document.getElementById('times-wrapper-1').style.display = 'flex'\n";
  page += "            document.getElementById('button-fall-1').style.display = 'block'\n";
  page += "        }\n";
  page += "        else if (action == 'remove_person_2') {\n";
  page += "            document.getElementById('timer-wrapper-2').style.width = '5%'\n";
  page += "            document.getElementById('timer-wrapper-2').style.opacity = '0.75'\n";
  page += "            document.getElementById('timer-wrapper-1').style.width = '92.5%'\n";
  page += "            document.getElementById('timer-wrapper-1').style.opacity = '1'\n";
  page += "            document.getElementById('line-addition-button-2').style.display = 'block'\n";
  page += "            document.getElementById('line-removal-button-2').style.display = 'none'\n";
  page += "            document.getElementById('line-removal-button-1').style.opacity = '0'\n";
  page += "            document.getElementById('times-wrapper-2').style.display = 'none'\n";
  page += "            document.getElementById('button-fall-2').style.display = 'none'\n";
  page += "        }\n";
  page += "        else if (action == 'add_person_2') {\n";
  page += "            document.getElementById('timer-wrapper-1').style.width = '48.5%'\n";
  page += "            document.getElementById('timer-wrapper-1').style.opacity = '1'\n";
  page += "            document.getElementById('timer-wrapper-2').style.width = '48.5%'\n";
  page += "            document.getElementById('timer-wrapper-2').style.opacity = '1'\n";
  page += "            document.getElementById('line-addition-button-2').style.display = 'none'\n";
  page += "            document.getElementById('line-removal-button-2').style.display = 'block'\n";
  page += "            document.getElementById('line-removal-button-1').style.opacity = '1'\n";
  page += "            document.getElementById('times-wrapper-2').style.display = 'flex'\n";
  page += "            document.getElementById('button-fall-2').style.display = 'block'\n";
  page += "        }\n";
  page += "        else if (action == 'fall_1') {\n";
  page += "            document.getElementById('timer-wrapper-1').style.backgroundColor = 'var(--help-color)';\n";
  page += "            document.getElementById('button-fall-1').style.opacity = '0'\n";
  page += "            document.getElementById('line-removal-button-1').style.opacity = '0'\n";
  page += "            document.getElementById('timer-time-1').innerText = 'FALL';\n";
  page += "        }\n";
  page += "        else if (action == 'fall_2') {\n";
  page += "            document.getElementById('timer-wrapper-2').style.backgroundColor = 'var(--help-color)';\n";
  page += "            document.getElementById('button-fall-2').style.opacity = '0'\n";
  page += "            document.getElementById('line-removal-button-2').style.opacity = '0'\n";
  page += "            document.getElementById('timer-time-2').innerText = 'FALL';\n";
  page += "        }\n";
  page += "        else if (action == 'start') {\n";
  page += "            document.getElementById('main-button').onclick = function () {\n";
  page += "                reportAction('stop');\n";
  page += "            };\n";
  page += "            document.getElementById('main-button').innerText = 'STOP'\n";
  page += "        }\n";
  page += "        else if (action == 'stop') {\n";
  page += "            document.getElementById('main-button').onclick = function () {\n";
  page += "                reportAction('reset');\n";
  page += "            };\n";
  page += "            document.getElementById('main-button').innerText = 'RESET'\n";
  page += "        }\n";
  page += "        else if (action == 'reset') {\n";
  page += "            document.getElementById('main-button').onclick = function () {\n";
  page += "                reportAction('start');\n";
  page += "            };\n";
  page += "            document.getElementById('main-button').innerText = 'START'\n";
  page += "            document.getElementById('line-removal-button-1').style.opacity = '1'\n";
  page += "            document.getElementById('line-removal-button-2').style.opacity = '1'\n";
  page += "            document.getElementById('line-removal-button-1').style.display = 'block'\n";
  page += "            document.getElementById('line-removal-button-2').style.display = 'block'\n";
  page += "            document.getElementById('button-fall-1').style.opacity = '1'\n";
  page += "            document.getElementById('button-fall-2').style.opacity = '1'\n";
  page += "            document.getElementById('timer-wrapper-1').style.backgroundColor = 'var(--main-color)';\n";
  page += "            document.getElementById('timer-wrapper-2').style.backgroundColor = 'var(--main-color)';\n";
  page += "            document.getElementById('timer-time-1').innerText = '0.000';\n";
  page += "            document.getElementById('timer-time-2').innerText = '0.000';\n";
  page += "            document.getElementById('timer-wrapper-1').style.width = '48.5%'\n";
  page += "            document.getElementById('timer-wrapper-2').style.width = '48.5%'\n";
  page += "            document.getElementById('line-addition-button-1').style.display = 'none'\n";
  page += "            document.getElementById('line-addition-button-2').style.display = 'none'\n";
  page += "            document.getElementById('times-wrapper-1').style.display = 'flex'\n";
  page += "            document.getElementById('times-wrapper-2').style.display = 'flex'\n";
  page += "            document.getElementById('timer-wrapper-1').style.opacity = '1'\n";
  page += "            document.getElementById('timer-wrapper-2').style.opacity = '1'\n";
  page += "            document.getElementById('button-fall-1').style.display = 'block'\n";
  page += "            document.getElementById('button-fall-2').style.display = 'block'\n";
  page += "        }\n";
  page += "        xhttp.send('action=' + action);\n";
  page += "    }\n";
  page += "    function toggleScheme() {\n";
  page += "        if (document.getElementsByTagName('body')[0].classList == 'default-scheme' || document.getElementsByTagName('body')[0].classList == 'light-scheme') document.getElementsByTagName('body')[0].classList = 'dark-scheme';\n";
  page += "        else if (document.getElementsByTagName('body')[0].classList == 'dark-scheme') document.getElementsByTagName('body')[0].classList = 'light-scheme';\n";
  page += "    }\n";
  page += "</script>\n";

  page += "</html>\n";



  return page;
}