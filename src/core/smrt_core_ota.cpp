/**
 * @file    smrt_core_ota.cpp
 * @brief   OTA update implementation — ArduinoOTA + HTTP firmware upload
 * @project HOMENODE
 * @version 0.2.0
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#ifndef UNIT_TEST
#include "smrt_core.h"

//-----------------------------------------------------------------------------
// External global objects (defined in smrt_core_http.cpp)
//-----------------------------------------------------------------------------
extern AsyncWebServer smrt_server;

//-----------------------------------------------------------------------------
// OTA upload HTML page (PROGMEM)
//-----------------------------------------------------------------------------
static const char smrt_ota_page[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="es">
<head>
    <title>HomeNode - OTA Update</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
        html { font-family: 'Segoe UI', Arial, sans-serif; text-align: center; }
        body { background: #E8EEF4; color: #2C3E50; min-height: 100vh; }
        .topnav { background: linear-gradient(135deg, #4A6FA5, #3A5A8C); padding: 18px 20px; }
        .topnav h1 { font-size: 1.5rem; color: #FFF; font-weight: 600; }
        .topnav .subtitle { font-size: 0.8rem; color: rgba(255,255,255,0.7); margin-top: 4px; }
        .content { padding: 20px 16px 40px; max-width: 520px; margin: 0 auto; }
        .card { background: #FFF; border-radius: 12px; box-shadow: 0 2px 8px rgba(0,0,0,0.08);
                padding: 24px; margin-bottom: 16px; }
        .card h2 { font-size: 0.85rem; font-weight: 700; color: #4A6FA5;
                    text-transform: uppercase; letter-spacing: 1.5px; margin-bottom: 14px; }
        .info { background: #F4F7FB; border-radius: 8px; padding: 12px; margin-bottom: 16px;
                font-size: 0.85rem; color: #555; text-align: left; line-height: 1.6; }
        input[type="file"] { margin: 12px 0; font-size: 0.95rem; }
        .btn { display: inline-block; padding: 14px 48px; font-size: 1.05rem; font-weight: 600;
               color: #FFF; border: none; border-radius: 8px; cursor: pointer;
               background: linear-gradient(135deg, #E8723A, #D45E28);
               box-shadow: 0 4px 12px rgba(232,114,58,0.35); transition: all 0.3s ease; }
        .btn:hover { background: linear-gradient(135deg, #D45E28, #B84D1E);
                     transform: translateY(-2px); box-shadow: 0 6px 20px rgba(232,114,58,0.45); }
        .btn:disabled { opacity: 0.5; cursor: not-allowed; transform: none !important; }
        .progress-container { display: none; margin-top: 16px; }
        .progress-bar { width: 100%%; height: 24px; background: #E0E6ED; border-radius: 12px;
                        overflow: hidden; }
        .progress-fill { height: 100%%; background: linear-gradient(90deg, #2ECC71, #27AE60);
                         border-radius: 12px; transition: width 0.3s ease; width: 0%%; }
        .progress-text { margin-top: 8px; font-size: 0.9rem; font-weight: 600; color: #555; }
        .result { margin-top: 16px; padding: 12px; border-radius: 8px; font-weight: 600;
                  font-size: 0.9rem; display: none; }
        .result.success { display: block; background: #E8F8F0; color: #27AE60; border: 1px solid #A3DFBF; }
        .result.error { display: block; background: #FDEDEC; color: #E74C3C; border: 1px solid #F1948A; }
        .back-link { display: inline-block; margin-top: 16px; color: #4A6FA5; font-size: 0.9rem;
                     text-decoration: none; font-weight: 600; }
        .back-link:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="topnav">
        <h1>HOMENODE</h1>
        <div class="subtitle">OTA Firmware Update</div>
    </div>
    <div class="content">
        <div class="card">
            <h2>Actualizar Firmware</h2>
            <div class="info">
                Selecciona el archivo <strong>.bin</strong> del firmware compilado.<br>
                Ruta habitual: <code>.pio/build/nodemcu-32s/firmware.bin</code><br>
                El dispositivo se reiniciara automaticamente tras la actualizacion.
            </div>
            <form id="otaForm">
                <input type="file" id="otaFile" accept=".bin">
                <br>
                <button type="submit" class="btn" id="btnUpload">Subir Firmware</button>
            </form>
            <div class="progress-container" id="progressContainer">
                <div class="progress-bar">
                    <div class="progress-fill" id="progressFill"></div>
                </div>
                <div class="progress-text" id="progressText">0%%</div>
            </div>
            <div class="result" id="result"></div>
            <a href="/" class="back-link">&#x2190; Volver al Panel Principal</a>
        </div>
    </div>
    <script>
        document.getElementById('otaForm').addEventListener('submit', function(e) {
            e.preventDefault();
            var file = document.getElementById('otaFile').files[0];
            if (!file) { showResult(false, 'Selecciona un archivo .bin'); return; }
            if (!file.name.endsWith('.bin')) { showResult(false, 'Solo archivos .bin'); return; }
            var formData = new FormData();
            formData.append('update', file);
            var xhr = new XMLHttpRequest();
            xhr.open('POST', '/update', true);
            document.getElementById('btnUpload').disabled = true;
            document.getElementById('progressContainer').style.display = 'block';
            document.getElementById('result').style.display = 'none';
            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    var pct = Math.round((e.loaded / e.total) * 100);
                    document.getElementById('progressFill').style.width = pct + '%%';
                    document.getElementById('progressText').textContent = pct + '%%';
                }
            });
            xhr.onload = function() {
                if (xhr.status === 200) {
                    showResult(true, 'Firmware actualizado. Reiniciando...');
                    setTimeout(function() { window.location.href = '/'; }, 5000);
                } else {
                    showResult(false, 'Error: ' + xhr.responseText);
                    document.getElementById('btnUpload').disabled = false;
                }
            };
            xhr.onerror = function() {
                showResult(false, 'Error de conexion');
                document.getElementById('btnUpload').disabled = false;
            };
            xhr.send(formData);
        });
        function showResult(success, msg) {
            var el = document.getElementById('result');
            el.textContent = msg;
            el.className = 'result ' + (success ? 'success' : 'error');
        }
    </script>
</body>
</html>
)rawliteral";

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

/**
 * @brief  Initializes ArduinoOTA service with hostname and progress callbacks.
 * @return void
 */
void smrt_ota_init(void) {
    ArduinoOTA.setHostname(SMRT_OTA_HOSTNAME);
    ArduinoOTA.setPort(SMRT_OTA_PORT);

    if (strlen(SMRT_OTA_PASSWORD) > 0) {
        ArduinoOTA.setPassword(SMRT_OTA_PASSWORD);
    }

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
        Serial.println("OTA Start: updating " + type);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nOTA End: update complete, rebooting...");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();
    Serial.println("ArduinoOTA initialized on port " + String(SMRT_OTA_PORT));
}

/**
 * @brief  Registers the HTTP OTA upload endpoint on the async web server.
 * @return void
 */
void smrt_ota_web_init(void) {
    // Serve OTA upload page
    smrt_server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", smrt_ota_page);
    });

    // Handle firmware upload
    smrt_server.on("/update", HTTP_POST,
        // Response callback (called after upload completes)
        [](AsyncWebServerRequest *request) {
            bool success = !Update.hasError();
            AsyncWebServerResponse *response = request->beginResponse(
                success ? 200 : 500,
                "text/plain",
                success ? "OK - Rebooting..." : "FAIL"
            );
            response->addHeader("Connection", "close");
            request->send(response);
            if (success) {
                delay(1000);
                ESP.restart();
            }
        },
        // Upload handler callback (called for each chunk)
        [](AsyncWebServerRequest *request, const String& filename,
           size_t index, uint8_t *data, size_t len, bool final) {
            if (index == 0) {
                Serial.printf("OTA Web: Receiving %s (%u bytes)\n",
                              filename.c_str(), request->contentLength());
                if (!Update.begin(request->contentLength(), U_FLASH)) {
                    Update.printError(Serial);
                }
            }
            if (Update.isRunning()) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                }
            }
            if (final) {
                if (Update.end(true)) {
                    Serial.printf("OTA Web: Update success (%u bytes)\n", index + len);
                } else {
                    Update.printError(Serial);
                }
            }
        }
    );
}

#endif // UNIT_TEST
