#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <StreamUtils.h>

const uint64_t CURRENT_DATA_VERSION = 2;
const int SETTING_DATA_ADDRESS = 0;

EepromStream eepromStream(0, SPI_FLASH_SEC_SIZE);
DynamicJsonDocument settings(SPI_FLASH_SEC_SIZE);

String getSerializedSettingData() {
  String serializedSettingData;
  serializeJson(settings, serializedSettingData);
  return serializedSettingData;
}

void saveSettings() {
  serializeJson(settings, eepromStream);
  EEPROM.commit();
}

void resetSettings() {
  settings["dataVersion"]        = CURRENT_DATA_VERSION;

  settings["apSSID"]             = "ESP8266"; // 32
  settings["apPassword"]         = "12345678"; // 63
  settings["apAuth"]               = false;
  settings["apHidden"]             = false;
  settings["apChannel"]            = 1;
  settings["apMaxConnections"]     = 4;
  settings["apBeaconInterval"]     = 100;

  settings["apLocalIP"][0]       = 192;
  settings["apLocalIP"][1]       = 168;
  settings["apLocalIP"][2]       = 4;
  settings["apLocalIP"][3]       = 1;

  settings["apGateway"][0]       = 192;
  settings["apGateway"][1]       = 168;
  settings["apGateway"][2]       = 4;
  settings["apGateway"][3]       = 1;

  settings["apSubnet"][0]        = 255;
  settings["apSubnet"][1]        = 255;
  settings["apSubnet"][2]        = 255;
  settings["apSubnet"][3]        = 0;

  settings["managerUser"]        = "manager"; // 20
  settings["managerPassword"]    = "12345678"; // 128
  settings["managerPort"]        = 8000;

  settings["captivePortalIP"][0] = 192;
  settings["captivePortalIP"][1] = 168;
  settings["captivePortalIP"][2] = 4;
  settings["captivePortalIP"][3] = 100;

  settings["captivePortalPort"]  = 8000;

  saveSettings();
}

void setLedBrightness(const float &brightness = 0) {
  digitalWrite(LED_BUILTIN, 1 - brightness);
}

const char* htmlContent = R"=====(<!DOCTYPE html>
<html>
	<head>
		<meta charset="UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<title>ESP Manager</title>
		<style>
			* {
				margin: 0;
				padding: 0;
			}

			body {
				background-color: rgb(50, 50, 50);
				font-family: Arial;
				color: white
			}

			.bar-button {
				height: 25px;
				padding-left: 10px;
				padding-right: 10px;
				border-width: 1px;
				border-style: solid;
				border-radius: 2.5px;
				background-color: rgb(35, 35, 35);
				font-weight: bold;
			}

			.bar-button:disabled {
				filter: brightness(0.4);
			}

			.bar-button:hover:enabled {
				filter: brightness(1.5);
			}

			.top-bar {
				display: flex;
				justify-content: space-between;
				align-items: center;
				position: fixed;
				height: 35px;
				width: 100%;
				background-color: rgb(40, 40, 40);
			}

			.top-bar > h2 {
				margin-left: 10px;
				color: rgb(100, 100, 100);
			}

			#logout-button {
				margin-right: 5px;
				color: rgb(255, 0, 0);
				border-color: rgb(255, 0, 0);
			}

			#logout-button:active:enabled {
				background-color: rgb(255, 0, 0);
				color: white;
			}

			.container {
				position: fixed;
				margin-top: 35px;
				width: 100%;
				height: calc(100% - 70px);
				overflow: auto;
			}

			#setting-form {
				margin-top: 2.5px;
				margin-bottom: 10px;
				margin-left: 10px;
				margin-right: 10px;
			}

			#setting-form > fieldset {
				padding-top: 2.5px;
				padding-left: 10px;
				padding-right: 10px;
				margin-bottom: 5px;
				color: rgb(200, 200, 200);
				border: 1px solid rgb(100, 100, 100);
				border-radius: 2.5px;
			}

			#setting-form > fieldset > legend {
				margin-left: 20px;
				white-space: pre;
				border-left: 5px solid transparent;
				border-right: 5px solid transparent;
			}

			#setting-form > fieldset > div {
				padding-top: 5px;
				padding-bottom: 5px;
				padding-left: 10px;
				padding-right: 10px;
				margin-bottom: 10px;
				background-color: rgb(40, 40, 40);
				border-radius: 2.5px;
			}

			#setting-form > fieldset > div > h3 {
				margin-bottom: 2.5px;
			}

			#setting-form > fieldset > div > input {
				width: calc(100% - 20px);
				height: 25px;
				padding-left: 7.5px;
				padding-right: 7.5px;
				margin-bottom: 5px;
				color: white;
				background-color: rgb(60, 60, 60);
				border: 1px solid rgb(150, 150, 150);
				border-radius: 2.5px;
				outline: none;
			}

			#ap-password-confirm-input, #manager-password-confirm-input {
				display: none;
			}

			#setting-form > fieldset > div > label {
				display: flex;
				align-items: center;
				margin-bottom: 5px;
				font-size: small;
			}

			#setting-form > fieldset > div > label > input {
				margin-right: 2.5px;
			}

			#setting-form > fieldset > div > select {
				width: 100%;
				height: 25px;
				padding-left: 7.5px;
				padding-right: 7.5px;
				margin-bottom: 5px;
				color: white;
				background-color: rgb(60, 60, 60);
				border-top: none;
				border-bottom: none;
				border-left: none;
				border-right: 7.5px solid transparent;
				border-radius: 2.5px;
				outline: 1px solid rgb(150, 150, 150);
			}

			#setting-form > fieldset > div > p {
				font-size: small;
			}

			.bottom-bar {
				display: flex;
				justify-content: space-between;
				align-items: center;
				position: fixed;
				height: 35px;
				width: 100%;
				bottom: 0;
				background-color: rgb(40, 40, 40);
			}

			#reboot-button {
				margin-left: 5px;
				color: rgb(255, 0, 0);
				border-color: rgb(255, 0, 0);
			}

			#reboot-button:active:enabled {
				background-color: rgb(255, 0, 0);
				color: white;
			}

			.bottom-bar > div {
				display: flex;
				margin-right: 5px;
			}

			#reset-button {
				margin-right: 5px;
				color: rgb(255, 0, 0);
				border-color: rgb(255, 0, 0);
			}

			#reset-button:active:enabled {
				background-color: rgb(255, 0, 0);
				color: white;
			}

			#save-button {
				color: rgb(0, 255, 0);
				border-color: rgb(0, 255, 0);
			}

			#save-button:active:enabled {
				background-color: rgb(0, 255, 0);
				color: white;
			}
		</style>
	</head>
	<body>
		<div class="top-bar">
			<h2>ESP Manager</h2>
			<button id="logout-button" class="bar-button">Logout</button>
		</div>
		<div class="container">
			<form id="setting-form">
				<fieldset>
					<legend>Access Point</legend>
					<div>
						<h3>SSID</h3>
						<input type="text" minlength="1" maxlength="32" placeholder="Enter the SSID for your Wi-Fi network" id="ap-ssid-input" required />
						<p>SSID is the name of a Wi-Fi network. It helps your device identify and connect to a specific wireless network among others nearby.</p>
					</div>
					<div>
						<h3>Password</h3>
						<input type="password" minlength="8" maxlength="63" placeholder="Enter your Wi-Fi password" id="ap-password-input" required />
						<input type="password" minlength="8" maxlength="63" placeholder="Re-enter your Wi-Fi password" id="ap-password-confirm-input" />
						<label>
							<input type="checkbox" id="ap-show-password-checkbox" />
							Show password
						</label>
						<p>The password that the user needs to provide (if authentication is enabled) in order to connect to the Wi-Fi network.</p>
					</div>
					<div>
						<h3>Authentication</h3>
						<select id="ap-auth-select">
							<option value="disabled">Disabled</option>
							<option value="enabled">Enabled</option>
						</select>
						<p>Other devices will be required to provide the correct password to connect to the Wi-Fi.</p>
					</div>
					<div>
						<h3>Hidden</h3>
						<select id="ap-hidden-select">
							<option value="disabled">Disabled</option>
							<option value="enabled">Enabled</option>
						</select>
						<p>The access point will not be visible on other devices during the scanning process, and users must manually input the network information to connect.</p>
					</div>
					<div>
						<h3>Channel</h3>
						<input type="number" min="1" max="14" placeholder="Enter the Wi-Fi channel number" id="ap-channel-input" required />
						<p>A Wi-Fi channel is a designated frequency range used for wireless communication. It helps transmit signals for Wi-Fi devices and prevents interference with other networks.</p>
					</div>
					<div>
						<h3>Max Connections</h3>
						<input type="number" min="1" placeholder="Enter the maximum number of allowed connections" id="ap-max-connections-input" required />
						<p>Ensure optimal performance and bandwidth allocation by configuring the maximum number of devices that can simultaneously connect to the wireless network.</p>
					</div>
					<div>
						<h3>Beacon Interval</h3>
						<input type="number" placeholder="Enter the Beacon Interval in milliseconds" id="ap-beacon-interval-input" required />
						<p>Delay between each beacon frames being send to other nearby devices, beacon frames are small packets of information that contain essential network details, such as the network name (SSID), security settings, and other parameters. These frames help Wi-Fi devices discover and connect to the network</p>
					</div>
					<div>
						<h3>Local IP Address</h3>
						<input type="text" placeholder="Enter the local IP address for your ESP" pattern="^(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])$" id="ap-local-ip-input" required />
						<p>Local IP address of the ESP board in this network, this IP address will also be used for hosting the manager web server and redirection server.</p>
					</div>
					<div>
						<h3>Gateway IP Address</h3>
						<input type="text" placeholder="Enter the Gateway IP address" pattern="^(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])$" id="ap-gateway-input" required />
						<p>The gateway IP address of this Wi-Fi network.</p>
					</div>
					<div>
						<h3>Subnet Mask</h3>
						<input type="text" placeholder="Enter the Subnet Mask" pattern="^(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])$" id="ap-subnet-mask-input" required />
						<p>Network subnet mask of the Wi-Fi network. Subnet mask is used to divide an IP address into network and host portions, allowing devices within a network to communicate directly with each other.</p>
					</div>
				</fieldset>
				<fieldset>
					<legend>Manager</legend>
					<div>
						<h3>User</h3>
						<input type="text" placeholder="Enter the username for the manager account" id="manager-user-input" maxlength="20" />
						<p>The username of the manager account which user will need to provide when logging into this panel.</p>
					</div>
					<div>
						<h3>Password</h3>
						<input type="password" placeholder="Enter the password for the manager account" maxlength="128" id="manager-password-input" />
						<input type="password" placeholder="Re-enter the password for the manager account" maxlength="128" id="manager-password-confirm-input" />
						<label>
							<input type="checkbox" id="manager-show-password-checkbox" />
							Show password
						</label>
						<p>The password of the manager account which user will need to provide when logging into this panel.</p>
					</div>
					<div>
						<h3>Server Port</h3>
						<input type="number" min="0" max="65535" placeholder="Enter the port number for the manager server" id="manager-port-input" required />
						<p>The port number where the manager server should be listening for incoming requests. The IP address for this server is the save as local IP address.</p>
					</div>
				</fieldset>
				<fieldset>
					<legend>Captive Portal</legend>
					<div>
						<h3>Server IP Address</h3>
						<input type="text" placeholder="Enter the IP address for the Captive Portal" pattern="^(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])$" id="cp-ip-input" required />
						<p>The IP address of your captive portal server. All devices will be direct to this server when they're connected to this access point.</p>
					</div>
					<div>
						<h3>Server Port</h3>
						<input type="number" min="0" max="65535" placeholder="Enter the port number for the Captive Portal" id="cp-port-input" required />
						<p>The port number of your captive portal server. All devices will be direct to this server when they're connected to this access point.</p>
					</div>
				</fieldset>
			</form>
		</div>
		<div class="bottom-bar">
			<button id="reboot-button" class="bar-button">Reboot</button>
			<div>
				<button id="reset-button" class="bar-button">Reset</button>
				<button id="save-button" class="bar-button" form="setting-form">Save</button>
			</div>
		</div>
		<script>
			const logoutButton = document.getElementById("logout-button");
			const rebootButton = document.getElementById("reboot-button");
			const resetButton = document.getElementById("reset-button");
			const saveButton = document.getElementById("save-button");

			const apSSIDInput = document.getElementById("ap-ssid-input");
			const apPasswordInput = document.getElementById("ap-password-input");
			const apAuthSelect = document.getElementById("ap-auth-select");
			const apHiddenSelect = document.getElementById("ap-hidden-select");
			const apChannelInput = document.getElementById("ap-channel-input");
			const apMaxConnectionsInput = document.getElementById("ap-max-connections-input");
			const apBeaconIntervalInput = document.getElementById("ap-beacon-interval-input");
			const apLocalIPInput = document.getElementById("ap-local-ip-input");
			const apGatewayInput = document.getElementById("ap-gateway-input");
			const apSubnetMaskInput = document.getElementById("ap-subnet-mask-input");
			const managerUserInput = document.getElementById("manager-user-input");
			const managerPasswordInput = document.getElementById("manager-password-input");
			const managerPortInput = document.getElementById("manager-port-input");
			const cpIPInput = document.getElementById("cp-ip-input");
			const cpPortInput = document.getElementById("cp-port-input");

			const apPasswordConfirmInput = document.getElementById("ap-password-confirm-input");
			const managerPasswordConfirmInput = document.getElementById("manager-password-confirm-input");

			const apShowPasswordCheckbox = document.getElementById("ap-show-password-checkbox");
			const managerShowPasswordCheckbox = document.getElementById("manager-show-password-checkbox");

			function setActionButtonDisabled(state) {
				logoutButton.disabled = state;
				rebootButton.disabled = state;
				resetButton.disabled = state;
				saveButton.disabled = state;
			}

			logoutButton.addEventListener("click", mouseEvent => {
				if (confirm("Are you sure that you want to logout?")) {
					setActionButtonDisabled(true);
					fetch("/", {headers: {Authorization: "Basic "}}).then(response => {
						const statusCode = response.status;
						if (statusCode === 401) {
							document.body.style.display = "none";
							location.reload();
						} else
							throw new Error(`Unexpected response status code: ${statusCode}`);
					}).catch(err => {
						console.error(err);
						setActionButtonDisabled(false);
						alert("Failed to logout due to an unexpected error.");
					});
				}
			});

			rebootButton.addEventListener("click", mouseEvent => {
				if (confirm("Are you sure that you want to reboot your ESP?")) {
					setActionButtonDisabled(true);
					fetch("/api?type=reboot", {method: "POST"}).then(response => {
						const statusCode = response.status;
						if (statusCode === 204)
							location.reload();
						else
							throw new Error(`Unexpected response status code: ${statusCode}`);
					}).catch(err => {
						console.error(err);
						setActionButtonDisabled(false);
						alert("Failed to reboot due to an unexpected error.");
					});
				}
			});

			resetButton.addEventListener("click", mouseEvent => {
				if (confirm("Are you sure that you want to reset all settings?")) {
					setActionButtonDisabled(true);
					fetch("/api?type=settings", {method: "DELETE"}).then(response => {
						const statusCode = response.status;
						if (statusCode === 204) {
							alert("For some changes to take effect, you will need to reboot your ESP.");
							location.reload();
						} else
							throw new Error(`Unexpected response status code: ${statusCode}`);
					}).catch(err => {
						console.error(err);
						setActionButtonDisabled(false);
						alert("Failed to reset all settings due to an unexpected error.");
					});
				}
			});

			document.getElementById("setting-form").addEventListener("submit", submitEvent => {
				submitEvent.preventDefault();
				if (!saveButton.disabled) {
					setActionButtonDisabled(true);
					fetch("/api?type=settings", {
						method: "POST",
						headers: {
							"Content-Type": "application/json"
						},
						body: JSON.stringify({
							apSSID: apSSIDInput.value,
							apPassword: apPasswordInput.value,
							apAuth: apAuthSelect.value === "enabled",
							apHidden: apHiddenSelect.value === "enabled",
							apChannel: Number(apChannelInput.value),
							apMaxConnections: Number(apMaxConnectionsInput.value),
							apBeaconInterval: Number(apBeaconIntervalInput.value),
							apLocalIP: apLocalIPInput.value.split(".").map(octetValue => Number(octetValue)),
							apGateway: apGatewayInput.value.split(".").map(octetValue => Number(octetValue)),
							apSubnet: apSubnetMaskInput.value.split(".").map(octetValue => Number(octetValue)),

							managerUser: managerUserInput.value,
							managerPassword: managerPasswordInput.value,
							managerPort: Number(managerPortInput.value),

							captivePortalIP: cpIPInput.value.split(".").map(octetValue => Number(octetValue)),
							captivePortalPort: Number(cpPortInput.value)
						})
					}).then(response => {
						const statusCode = response.status;
						if (statusCode === 204) {
							alert("For some changes to take effect, you will need to reboot your ESP.");
							location.reload();
						} else
							throw new Error(`Unexpected response status code: ${statusCode}`);
					}).catch(err => {
						console.error(err);
						setActionButtonDisabled(false);
						alert("Failed to save the settings due to an unexpected error.");
					})
				}
			});

			function createConfirmPattern(originalString) {
				return (new RegExp(`^${originalString.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")}$`)).source;
			}

			let currentAPPassword = apPasswordInput.value;

			apPasswordInput.addEventListener("input", inputEvent => {
				const newPassword = apPasswordInput.value;
				const passwordChanged = newPassword !== currentAPPassword;
				apPasswordConfirmInput.pattern = createConfirmPattern(newPassword);
				apPasswordConfirmInput.required = passwordChanged;
				if (passwordChanged && apPasswordConfirmInput.style.display === "none")
					apPasswordConfirmInput.value = "";
				apPasswordConfirmInput.style.display = passwordChanged ? "block" : "none";
			});

			let currentManagerPassword = managerPasswordInput.value;

			managerPasswordInput.addEventListener("input", inputEvent => {
				const newPassword = managerPasswordInput.value;
				const passwordChanged = newPassword !== currentManagerPassword;
				managerPasswordConfirmInput.pattern = createConfirmPattern(newPassword);
				managerPasswordConfirmInput.required = passwordChanged;
				if (passwordChanged && managerPasswordConfirmInput.style.display === "none")
					managerPasswordConfirmInput.value = "";
				managerPasswordConfirmInput.style.display = passwordChanged ? "block" : "none";
			});

			apShowPasswordCheckbox.addEventListener("click", mouseEvent => {
				const newType = apShowPasswordCheckbox.checked ? "text" : "password";
				apPasswordInput.type = newType;
				apPasswordConfirmInput.type = newType;
			});

			managerShowPasswordCheckbox.addEventListener("click", mouseEvent => {
				const newType = managerShowPasswordCheckbox.checked ? "text" : "password";
				managerPasswordInput.type = newType;
				managerPasswordConfirmInput.type = newType;
			});

			fetch("/api?type=settings").then(response => {
				const statusCode = response.status;
				if (statusCode === 200) {
					response.json().then(data => {
						apSSIDInput.value = data.apSSID;
						currentAPPassword = data.apPassword;
						apPasswordInput.value = data.apPassword;
						apAuthSelect.value = data.apAuth ? "enabled" : "disabled";
						apHiddenSelect.value = data.apHidden ? "enabled" : "disabled";
						apChannelInput.value = data.apChannel;
						apMaxConnectionsInput.value = data.apMaxConnections;
						apBeaconIntervalInput.value = data.apBeaconInterval;
						apLocalIPInput.value = data.apLocalIP.join(".");
						apGatewayInput.value = data.apGateway.join(".");
						apSubnetMaskInput.value = data.apSubnet.join(".");
						
						managerUserInput.value = data.managerUser;
						currentManagerPassword = data.managerPassword;
						managerPasswordInput.value = data.managerPassword;
						managerPortInput.value = data.managerPort;

						cpIPInput.value = data.captivePortalIP.join(".");
						cpPortInput.value = data.captivePortalPort;
					});
				} else
					throw new Error(`Unexpected response status code: ${statusCode}`);
			}).catch(err => {
				console.error(err);
				if (confirm("An unexpected error has occurred while loading this page, do you wish to reload the page?"))
					location.reload();
			});
		</script>
	</body>
</html>)=====";

void setup() {
  Serial.begin(9600);
  Serial.println("Starting...");
  Serial.println("Current data version: " + CURRENT_DATA_VERSION);
  Serial.println("Setting data address: " + SETTING_DATA_ADDRESS);
  Serial.println("Setting data size & EEPROM size: " + SPI_FLASH_SEC_SIZE);
  EEPROM.begin(SPI_FLASH_SEC_SIZE);
  pinMode(LED_BUILTIN, OUTPUT);
  setLedBrightness(LOW);

  DeserializationError deserializationError = deserializeJson(settings, eepromStream);
  Serial.println("Deserialized data from EEPROM: " + getSerializedSettingData());
  Serial.println("Validating setting data...");
  if (deserializationError != DeserializationError::Ok) {
    Serial.println("Resetting setting data due to error: " + String(deserializationError.c_str()));
    resetSettings();
  } else if (settings["dataVersion"].as<uint64_t>() != CURRENT_DATA_VERSION) {
    Serial.println("Resetting setting data due to data version not matched");
    resetSettings();
  } else
    Serial.println("Setting data is valid");

  Serial.println("Current setting data: " + getSerializedSettingData());

  const IPAddress apLocalIP(
    settings["apLocalIP"][0].as<uint8_t>(),
    settings["apLocalIP"][1].as<uint8_t>(),
    settings["apLocalIP"][2].as<uint8_t>(),
    settings["apLocalIP"][3].as<uint8_t>()
  );

  Serial.println("Setting up access point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(
    apLocalIP,
    IPAddress(settings["apGateway"][0].as<uint8_t>(), settings["apGateway"][1].as<uint8_t>(), settings["apGateway"][2].as<uint8_t>(), settings["apGateway"][3].as<uint8_t>()),
    IPAddress(settings["apSubnet"][0].as<uint8_t>(), settings["apSubnet"][1].as<uint8_t>(), settings["apSubnet"][2].as<uint8_t>(), settings["apSubnet"][3].as<uint8_t>())
  );
  WiFi.softAP(
    settings["apSSID"].as<const char*>(),
    settings["apAuth"].as<bool>() ? settings["apPassword"].as<const char*>() : 0,
    settings["apChannel"].as<int>(),
    settings["apHidden"].as<bool>(),
    settings["apMaxConnections"].as<int>(),
    settings["apBeaconInterval"].as<int>()
  );
  Serial.println("Access point is now ready for connections");

  Serial.println("Setting up manager web server...");
  AsyncWebServer managerWebServer(settings["managerPort"].as<uint16_t>());

  managerWebServer.on("/api", HTTP_ANY, [](AsyncWebServerRequest *request) {
    if (request->authenticate(settings["managerUser"].as<const char*>(), settings["managerPassword"].as<const char*>()))
      if (request->hasParam("type")) {
        const String apiType = request->getParam("type")->value();
        if (apiType.equals("settings"))
          switch (request->method()) {
            case HTTP_GET:
              request->send(200, "application/json", getSerializedSettingData());
              break;
            case HTTP_POST:
              break;
            case HTTP_DELETE:
              resetSettings();
              request->send(204);
              break;
            default:
              request->send(405);
          }
        else if (apiType.equals("reboot"))
          if (request->method() == HTTP_POST) {
            request->send(204);
            ESP.restart();
          } else
            request->send(405);
        else
          request->send(400);
      } else
        request->send(400);
    else
      request->requestAuthentication();
  }, NULL, [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (request->url() == "/api" && request->hasParam("type") && request->getParam("type")->value() == "settings" && request->method() == HTTP_POST)
      if (request->getHeader("Content-Type")->value().equals("application/json")) {
        DeserializationError bodyDeserializationError = deserializeJson(settings, data);
        settings["dataVersion"] = CURRENT_DATA_VERSION;
        saveSettings();
        if (bodyDeserializationError == DeserializationError::Ok)
          request->send(204);
        else if (bodyDeserializationError == DeserializationError::NoMemory)
          request->send(507);
        else
          request->send(400);
      } else
        request->send(400);
  });

  managerWebServer.onNotFound([](AsyncWebServerRequest *request) {
    if (request->authenticate(settings["managerUser"].as<const char*>(), settings["managerPassword"].as<const char*>()))
      if (request->method() == HTTP_GET) {
        request->send_P(200, "text/html", htmlContent);
      } else
        request->send(405);
    else
      request->requestAuthentication();
  });

  managerWebServer.begin();
  Serial.println("Manager web server is now ready for requests");

  Serial.println("Setting up a redirection server...");
  AsyncWebServer redirectionWebServer(80);

  redirectionWebServer.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("http://" + IPAddress(
      settings["captivePortalIP"][0].as<uint8_t>(),
      settings["captivePortalIP"][1].as<uint8_t>(),
      settings["captivePortalIP"][2].as<uint8_t>(),
      settings["captivePortalIP"][3].as<uint8_t>()
    ).toString() + ":" + settings["captivePortalPort"].as<uint16_t>());
    setLedBrightness(HIGH);
    delay(10);
    setLedBrightness(LOW);
  });

  redirectionWebServer.begin();
  Serial.println("Redirection server is now ready for requests");

  Serial.println("Setting up DNS server...");
  DNSServer dnsServer;
  dnsServer.start(53, "*", apLocalIP);
  Serial.println("DNS server is now ready");
  Serial.println("Everything ready");

  while (true) {
    dnsServer.processNextRequest();
  }
}

void loop() {}