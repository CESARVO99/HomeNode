/**
 * @file    smrt_core_webui.h
 * @brief   Base HTML/CSS/JS web interface stored in PROGMEM
 * @project HOMENODE
 * @version 1.0.0
 *
 * Full dashboard with:
 *   - Connection status card
 *   - System info card (uptime, RSSI, SSID, clients, NTP time)
 *   - Authentication card (PIN-based WS auth)
 *   - Module cards: ENV, RLY, SEC, PLG, NRG, ACC
 *   - ENV alert config panel
 *   - ACC learn mode panel
 *   - Scheduler panel (8 cron tasks)
 *   - MQTT config panel
 *   - Webhook config panel
 *   - Backup/Restore panel
 *   - Timezone config
 *   - WiFi configuration card
 *   - Firmware update link
 */

#ifndef SMRT_CORE_WEBUI_H
#define SMRT_CORE_WEBUI_H

#ifndef UNIT_TEST

const char smrt_webui_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="es">
<head>
    <title>HomeNode - Dashboard</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" href="data:,">
    <style>
        *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
        html { font-family: 'Segoe UI', Arial, Helvetica, sans-serif; text-align: center; scroll-behavior: smooth; }
        body { margin: 0; background-color: #E8EEF4; color: #2C3E50; min-height: 100vh; }
        .topnav { background: linear-gradient(135deg, #4A6FA5, #3A5A8C); padding: 18px 20px;
                   box-shadow: 0 2px 12px rgba(74, 111, 165, 0.3); }
        .topnav h1 { font-size: 1.5rem; color: #FFFFFF; font-weight: 600; letter-spacing: 0.5px; }
        .topnav .subtitle { font-size: 0.8rem; color: rgba(255,255,255,0.7); margin-top: 4px; }
        .content { padding: 20px 16px 40px; max-width: 520px; margin: 0 auto; }
        .card { background-color: #FFFFFF; border-radius: 12px; box-shadow: 0 2px 8px rgba(0,0,0,0.08);
                padding: 20px; margin-bottom: 16px; transition: box-shadow 0.3s ease, transform 0.3s ease; }
        .card:hover { box-shadow: 0 6px 20px rgba(0,0,0,0.12); transform: translateY(-2px); }
        .card h2 { font-size: 0.85rem; font-weight: 700; color: #4A6FA5; text-transform: uppercase;
                    letter-spacing: 1.5px; margin-bottom: 14px; }
        .conn-status { display: flex; align-items: center; justify-content: center; gap: 10px; padding: 8px 0; }
        .conn-dot { width: 12px; height: 12px; border-radius: 50%%; background-color: #E74C3C; flex-shrink: 0; }
        .conn-dot.connected { background-color: #2ECC71; animation: pulse-green 2s infinite; }
        .conn-dot.reconnecting { background-color: #E8723A; animation: pulse-orange 1s infinite; }
        @keyframes pulse-green { 0%%,100%% { box-shadow: 0 0 0 0 rgba(46,204,113,0.5); }
                                 50%% { box-shadow: 0 0 0 8px rgba(46,204,113,0); } }
        @keyframes pulse-orange { 0%%,100%% { box-shadow: 0 0 0 0 rgba(232,114,58,0.5); }
                                  50%% { box-shadow: 0 0 0 8px rgba(232,114,58,0); } }
        .conn-text { font-size: 0.9rem; font-weight: 600; color: #7F8C8D; }
        .conn-text.connected { color: #2ECC71; }
        .conn-text.disconnected { color: #E74C3C; }
        .conn-text.reconnecting { color: #E8723A; }
        .conn-ip { font-size: 0.8rem; color: #7F8C8D; margin-top: 4px; }
        .sys-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; text-align: left; }
        .sys-item { background: #F4F7FB; border-radius: 8px; padding: 12px; }
        .sys-item .sys-label { font-size: 0.7rem; font-weight: 700; color: #7F8C8D;
                               text-transform: uppercase; letter-spacing: 1px; margin-bottom: 4px; }
        .sys-item .sys-value { font-size: 1rem; font-weight: 600; color: #2C3E50; }
        .rssi-bar-container { width: 100%%; height: 6px; background: #E0E6ED; border-radius: 3px;
                              margin-top: 6px; overflow: hidden; }
        .rssi-bar { height: 100%%; border-radius: 3px; transition: width 0.5s ease, background-color 0.5s ease;
                    background-color: #2ECC71; }
        .btn { display: inline-block; padding: 14px 48px; font-size: 1.05rem; font-weight: 600;
               text-align: center; outline: none; color: #FFFFFF; border: none; border-radius: 8px;
               cursor: pointer; transition: all 0.3s ease; user-select: none;
               -webkit-tap-highlight-color: transparent; letter-spacing: 0.5px; }
        .btn:disabled { opacity: 0.5; cursor: not-allowed; transform: none !important; box-shadow: none !important; }
        .btn-settings { background: transparent; color: #4A6FA5; border: 2px solid #4A6FA5;
                        padding: 10px 28px; font-size: 0.85rem; box-shadow: none; }
        .btn-settings:hover:not(:disabled) { background-color: #4A6FA5; color: #FFFFFF;
                                              transform: translateY(-2px); box-shadow: 0 4px 12px rgba(74,111,165,0.3); }
        .btn-secondary { background: linear-gradient(135deg, #5B8DB8, #4A7CA5);
                         box-shadow: 0 4px 12px rgba(91,141,184,0.3); padding: 12px 36px; font-size: 0.95rem; }
        .btn-secondary:hover:not(:disabled) { background: linear-gradient(135deg, #4A7CA5, #3A6B92);
                                               transform: translateY(-2px); box-shadow: 0 6px 20px rgba(91,141,184,0.4); }
        .panel { max-height: 0; overflow: hidden; transition: max-height 0.4s ease, opacity 0.3s ease; opacity: 0; }
        .panel.open { max-height: 2000px; opacity: 1; }
        .form-group { margin-bottom: 14px; text-align: left; }
        .form-group label { display: block; font-size: 0.75rem; font-weight: 700; color: #4A6FA5;
                            text-transform: uppercase; letter-spacing: 1px; margin-bottom: 6px; }
        .form-group input, .form-group select { width: 100%%; padding: 11px 14px; font-size: 0.95rem; border: 2px solid #DDE4ED;
                            border-radius: 8px; outline: none; transition: border-color 0.3s ease, box-shadow 0.3s ease;
                            background: #F8FAFB; color: #2C3E50; }
        .form-group input:focus, .form-group select:focus { border-color: #E8723A; box-shadow: 0 0 0 3px rgba(232,114,58,0.15); background: #FFFFFF; }
        .form-group input::placeholder { color: #B0BEC5; }
        .form-msg { font-size: 0.85rem; font-weight: 600; margin-top: 12px; padding: 8px 12px;
                    border-radius: 6px; display: none; }
        .form-msg.success { display: block; background: #E8F8F0; color: #27AE60; border: 1px solid #A3DFBF; }
        .form-msg.error { display: block; background: #FDEDEC; color: #E74C3C; border: 1px solid #F1948A; }
        .form-msg.info { display: block; background: #EBF5FB; color: #2980B9; border: 1px solid #AED6F1; }
        .footer { text-align: center; padding: 16px; font-size: 0.7rem; color: #95A5A6; }
        .st-on { color: #2ECC71; } .st-off { color: #E74C3C; }
        .st-warn { color: #E8723A; } .st-trig { color: #E74C3C; font-weight: 700; }
        .btn-on { background: #2ECC71; color: #fff; border: none; border-radius: 6px; padding: 8px 16px;
                  font-size: 0.8rem; font-weight: 600; cursor: pointer; transition: all 0.2s; }
        .btn-on:hover:not(:disabled) { background: #27AE60; }
        .btn-off { background: #E74C3C; color: #fff; border: none; border-radius: 6px; padding: 8px 16px;
                   font-size: 0.8rem; font-weight: 600; cursor: pointer; transition: all 0.2s; }
        .btn-off:hover:not(:disabled) { background: #C0392B; }
        .btn-sm { padding: 8px 16px; font-size: 0.8rem; }
        .btn-warn { background: #E8723A; color: #fff; border: none; border-radius: 6px; padding: 8px 16px;
                    font-size: 0.8rem; font-weight: 600; cursor: pointer; transition: all 0.2s; }
        .btn-warn:hover:not(:disabled) { background: #D35400; }
        .rly-row { display: flex; justify-content: space-between; align-items: center;
                   background: #F4F7FB; border-radius: 8px; padding: 10px 12px; margin-bottom: 8px; }
        .rly-row .sys-label { margin-bottom: 2px; }
        .sched-row { background: #F4F7FB; border-radius: 8px; padding: 10px 12px; margin-bottom: 8px;
                     display: flex; justify-content: space-between; align-items: center; text-align: left; }
        .sched-row .sched-info { flex: 1; }
        .sched-row .sched-name { font-weight: 600; font-size: 0.9rem; }
        .sched-row .sched-detail { font-size: 0.75rem; color: #7F8C8D; }
        .hook-row { background: #F4F7FB; border-radius: 8px; padding: 10px 12px; margin-bottom: 8px;
                    display: flex; justify-content: space-between; align-items: center; text-align: left; }
        .hook-row .hook-url { flex: 1; font-size: 0.8rem; word-break: break-all; }
        .tab-bar { display: flex; gap: 4px; margin-bottom: 12px; flex-wrap: wrap; }
        .tab-btn { flex: 1; min-width: 60px; padding: 8px 4px; font-size: 0.7rem; font-weight: 600;
                   background: #F4F7FB; border: 2px solid #DDE4ED; border-radius: 6px; cursor: pointer;
                   color: #7F8C8D; transition: all 0.2s; text-transform: uppercase; letter-spacing: 0.5px; }
        .tab-btn.active { background: #4A6FA5; color: #fff; border-color: #4A6FA5; }
        .tab-panel { display: none; }
        .tab-panel.active { display: block; }
        .inline-row { display: flex; gap: 8px; align-items: flex-end; }
        .inline-row .form-group { flex: 1; margin-bottom: 0; }
        .chk-row { display: flex; align-items: center; gap: 8px; margin-bottom: 6px; }
        .chk-row input[type="checkbox"] { width: 18px; height: 18px; accent-color: #4A6FA5; }
        .chk-row label { font-size: 0.8rem; font-weight: 600; color: #2C3E50; text-transform: none; letter-spacing: 0; }
        @media (max-width: 480px) {
            .topnav h1 { font-size: 1.2rem; }
            .content { padding: 14px 10px 30px; }
            .card { padding: 16px; }
            .sys-grid { grid-template-columns: 1fr; }
        }
    </style>
</head>
<body>
    <div class="topnav">
        <h1>HOMENODE</h1>
        <div class="subtitle">IoT Modular Platform</div>
    </div>
    <div class="content">
        <!-- Connection status -->
        <div class="card">
            <h2>Conexion</h2>
            <div class="conn-status">
                <div id="connDot" class="conn-dot"></div>
                <span id="connText" class="conn-text disconnected">Disconnected</span>
            </div>
            <div id="connIp" class="conn-ip">IP: ---</div>
        </div>
        <!-- AP mode banner -->
        <div id="apBanner" class="card" style="display:none;background:#FFF3E0;border:2px solid #E8723A">
            <h2 style="color:#E8723A">MODO AP — Configura WiFi</h2>
            <div style="font-size:0.85rem;color:#5D4037;margin-top:4px;">
                No se pudo conectar a la red WiFi. Configura nuevas credenciales abajo.
            </div>
        </div>
        <!-- System info -->
        <div class="card">
            <h2>Sistema</h2>
            <div class="sys-grid">
                <div class="sys-item">
                    <div class="sys-label">UPTIME</div>
                    <div id="sysUptime" class="sys-value">--:--:--</div>
                </div>
                <div class="sys-item">
                    <div class="sys-label">WIFI RSSI</div>
                    <div id="sysRssi" class="sys-value">-- dBm</div>
                    <div class="rssi-bar-container">
                        <div id="rssiBar" class="rssi-bar" style="width: 0%%;"></div>
                    </div>
                </div>
                <div class="sys-item">
                    <div class="sys-label">RED WIFI</div>
                    <div id="sysSsid" class="sys-value">---</div>
                </div>
                <div class="sys-item">
                    <div class="sys-label">CLIENTES WS</div>
                    <div id="sysClients" class="sys-value">0</div>
                </div>
                <div class="sys-item">
                    <div class="sys-label">HORA NTP</div>
                    <div id="sysTime" class="sys-value">--:--:--</div>
                </div>
                <div class="sys-item">
                    <div class="sys-label">NTP</div>
                    <div id="sysNtp" class="sys-value st-off">No sync</div>
                </div>
            </div>
        </div>
        <!-- Node Identity -->
        <div class="card">
            <h2>Nodo</h2>
            <div class="sys-grid">
                <div class="sys-item"><div class="sys-label">NODE ID</div><div id="nodeId" class="sys-value" style="font-size:0.75rem;">---</div></div>
                <div class="sys-item"><div class="sys-label">NOMBRE</div><div id="nodeName" class="sys-value">---</div></div>
                <div class="sys-item"><div class="sys-label">HABITACION</div><div id="nodeRoom" class="sys-value">---</div></div>
                <div class="sys-item"><div class="sys-label">MODULOS</div><div id="nodeMods" class="sys-value">---</div></div>
            </div>
            <button class="wr btn-settings btn-sm" style="margin-top:10px;" onclick="togglePanel('nodeForm')">Configurar Nodo</button>
            <div id="nodeForm" class="panel" style="margin-top:12px;">
                <div class="form-group"><label>Nombre del nodo</label><input type="text" id="cfgNodeName" placeholder="Ej: Sensores Salon" maxlength="32"></div>
                <div class="form-group"><label>Habitacion</label><input type="text" id="cfgNodeRoom" placeholder="Ej: Salon" maxlength="32"></div>
                <button class="wr btn-secondary btn-sm" onclick="saveNode()">Guardar</button>
                <div id="nodeMsg" class="form-msg"></div>
            </div>
        </div>
        <!-- Authentication -->
        <div class="card" id="cardAuth">
            <h2>Autenticacion</h2>
            <div id="authOk" style="display:none">
                <div class="conn-status">
                    <div class="conn-dot connected"></div>
                    <span class="conn-text connected">Autenticado</span>
                </div>
            </div>
            <div id="authNo">
                <div class="form-group">
                    <label for="authPin">PIN</label>
                    <input type="password" id="authPin" placeholder="PIN de acceso" maxlength="8">
                </div>
                <button class="btn btn-secondary" style="padding:10px 32px;font-size:0.9rem;" onclick="doAuth()">Autenticar</button>
                <div id="authMsg" class="form-msg"></div>
            </div>
        </div>
        <!-- Module cards -->
        <div id="moduleCards">
            <!-- ENV module -->
            <div class="card" id="cardEnv" style="display:none">
                <h2>Ambiente</h2>
                <div class="sys-grid">
                    <div class="sys-item">
                        <div class="sys-label">TEMPERATURA</div>
                        <div id="envTemp" class="sys-value" style="font-size:1.4rem;">--.- &deg;C</div>
                    </div>
                    <div class="sys-item">
                        <div class="sys-label">HUMEDAD</div>
                        <div id="envHum" class="sys-value" style="font-size:1.4rem;">--.- %%</div>
                    </div>
                </div>
                <div id="envAlertBanner" style="display:none;color:#E74C3C;font-weight:700;margin-top:8px;font-size:0.8rem;">&#9888; ALERTA AMBIENTAL</div>
                <div id="envStatus" style="font-size:0.75rem;color:#7F8C8D;margin-top:10px;">Sensor: ---</div>
                <button class="wr btn-settings btn-sm" style="margin-top:10px;" onclick="togglePanel('envAlertPanel')">Alertas</button>
                <div id="envAlertPanel" class="panel" style="margin-top:12px;">
                    <div class="inline-row" style="margin-bottom:10px;">
                        <div class="form-group"><label>Temp Max</label><input type="number" id="envTHi" step="0.5" value="35"></div>
                        <div class="form-group"><label>Temp Min</label><input type="number" id="envTLo" step="0.5" value="10"></div>
                    </div>
                    <div class="inline-row" style="margin-bottom:10px;">
                        <div class="form-group"><label>Hum Max</label><input type="number" id="envHHi" step="1" value="80"></div>
                        <div class="form-group"><label>Hum Min</label><input type="number" id="envHLo" step="1" value="20"></div>
                    </div>
                    <div class="chk-row">
                        <input type="checkbox" id="envAlertEn" checked>
                        <label for="envAlertEn">Alertas habilitadas</label>
                    </div>
                    <button class="wr btn-secondary btn-sm" onclick="saveEnvAlert()">Guardar Alertas</button>
                    <div id="envAlertMsg" class="form-msg"></div>
                </div>
            </div>
            <!-- RLY module -->
            <div class="card" id="cardRly" style="display:none">
                <h2>Reles</h2>
                <div id="rlyList"></div>
            </div>
            <!-- SEC module -->
            <div class="card" id="cardSec" style="display:none">
                <h2>Seguridad</h2>
                <div id="secState" class="sys-value" style="font-size:1.2rem;margin-bottom:12px;">---</div>
                <div class="sys-grid">
                    <div class="sys-item"><div class="sys-label">PIR</div><div id="secPir" class="sys-value">--</div></div>
                    <div class="sys-item"><div class="sys-label">PUERTA</div><div id="secReed" class="sys-value">--</div></div>
                    <div class="sys-item"><div class="sys-label">VIBRACION</div><div id="secVibr" class="sys-value">--</div></div>
                    <div class="sys-item"><div class="sys-label">EVENTOS</div><div id="secEvt" class="sys-value">0</div></div>
                </div>
                <div style="margin-top:12px;">
                    <button class="wr btn-on btn-sm" onclick="wsSend({cmd:'sec_arm'})">Armar</button>
                    <button class="wr btn-off btn-sm" style="margin-left:8px;" onclick="wsSend({cmd:'sec_disarm'})">Desarmar</button>
                </div>
            </div>
            <!-- PLG module -->
            <div class="card" id="cardPlg" style="display:none">
                <h2>Enchufe</h2>
                <div style="margin-bottom:12px;">
                    <button id="plgBtn" class="wr btn-off" onclick="wsSend({cmd:'plg_toggle'})">OFF</button>
                </div>
                <div class="sys-grid">
                    <div class="sys-item"><div class="sys-label">VOLTAJE</div><div id="plgV" class="sys-value">-- V</div></div>
                    <div class="sys-item"><div class="sys-label">CORRIENTE</div><div id="plgI" class="sys-value">-- A</div></div>
                    <div class="sys-item"><div class="sys-label">POTENCIA</div><div id="plgW" class="sys-value">-- W</div></div>
                    <div class="sys-item"><div class="sys-label">ENERGIA</div><div id="plgE" class="sys-value">-- Wh</div></div>
                </div>
                <div id="plgOL" style="display:none;color:#E74C3C;font-weight:700;margin-top:8px;font-size:0.8rem;">&#9888; SOBRECARGA</div>
            </div>
            <!-- NRG module -->
            <div class="card" id="cardNrg" style="display:none">
                <h2>Energia</h2>
                <div id="nrgCh"></div>
                <div style="margin-top:8px;font-size:0.75rem;color:#7F8C8D;">
                    Alerta: <span id="nrgAlert">--</span> W
                </div>
            </div>
            <!-- ACC module -->
            <div class="card" id="cardAcc" style="display:none">
                <h2>Acceso</h2>
                <div style="margin-bottom:12px;">
                    <button id="accBtn" class="wr btn-off" onclick="wsSend({cmd:'acc_toggle'})">Cerrado</button>
                </div>
                <div class="sys-grid">
                    <div class="sys-item"><div class="sys-label">UIDS</div><div id="accUids" class="sys-value">0</div></div>
                    <div class="sys-item"><div class="sys-label">EVENTOS</div><div id="accEvts" class="sys-value">0</div></div>
                    <div class="sys-item"><div class="sys-label">PULSO</div><div id="accPulse" class="sys-value">-- ms</div></div>
                    <div class="sys-item"><div class="sys-label">ULTIMO</div><div id="accLast" class="sys-value" style="font-size:0.8rem;">---</div></div>
                </div>
                <div id="accLearnBanner" style="display:none;background:#FFF3E0;border:1px solid #E8723A;border-radius:6px;padding:8px;margin-top:10px;font-size:0.8rem;color:#E8723A;font-weight:600;">
                    Modo aprendizaje activo — Acerca tarjeta NFC
                </div>
                <div style="margin-top:12px;">
                    <button class="wr btn-warn btn-sm" onclick="wsSend({cmd:'acc_learn'})">Aprender UID</button>
                    <button class="wr btn-settings btn-sm" style="margin-left:8px;" onclick="wsSend({cmd:'acc_learn_cancel'})">Cancelar</button>
                </div>
            </div>
        </div>

        <!-- Scheduler -->
        <div class="card" id="cardSched" style="display:none">
            <h2>Programador</h2>
            <div id="schedStatus" style="font-size:0.8rem;color:#7F8C8D;margin-bottom:10px;">Tareas activas: 0</div>
            <div id="schedList"></div>
            <button class="wr btn-settings btn-sm" onclick="togglePanel('schedForm')">Nueva Tarea</button>
            <div id="schedForm" class="panel" style="margin-top:12px;">
                <div class="form-group"><label>Nombre</label><input type="text" id="schedName" placeholder="Ej: Luz noche" maxlength="15"></div>
                <div class="inline-row" style="margin-bottom:10px;">
                    <div class="form-group"><label>Hora</label><input type="number" id="schedHour" min="0" max="23" value="0"></div>
                    <div class="form-group"><label>Minuto</label><input type="number" id="schedMin" min="0" max="59" value="0"></div>
                </div>
                <div class="form-group"><label>Accion (comando)</label><input type="text" id="schedAction" placeholder="Ej: rly_toggle 0" maxlength="31"></div>
                <div style="text-align:left;margin-bottom:10px;">
                    <div class="sys-label" style="margin-bottom:6px;">DIAS</div>
                    <div class="chk-row"><input type="checkbox" id="schD0" checked><label for="schD0">Dom</label>
                        <input type="checkbox" id="schD1" checked><label for="schD1">Lun</label>
                        <input type="checkbox" id="schD2" checked><label for="schD2">Mar</label>
                        <input type="checkbox" id="schD3" checked><label for="schD3">Mie</label>
                        <input type="checkbox" id="schD4" checked><label for="schD4">Jue</label>
                        <input type="checkbox" id="schD5" checked><label for="schD5">Vie</label>
                        <input type="checkbox" id="schD6" checked><label for="schD6">Sab</label>
                    </div>
                </div>
                <div class="form-group"><label>Slot (0-7)</label><input type="number" id="schedIdx" min="0" max="7" value="0"></div>
                <button class="wr btn-secondary btn-sm" onclick="saveSched()">Guardar Tarea</button>
                <div id="schedMsg" class="form-msg"></div>
            </div>
        </div>

        <!-- MQTT config -->
        <div class="card" id="cardMqtt" style="display:none">
            <h2>MQTT</h2>
            <div class="sys-grid" style="margin-bottom:12px;">
                <div class="sys-item"><div class="sys-label">ESTADO</div><div id="mqttState" class="sys-value st-off">---</div></div>
                <div class="sys-item"><div class="sys-label">SERVIDOR</div><div id="mqttSrv" class="sys-value" style="font-size:0.8rem;">---</div></div>
            </div>
            <button class="wr btn-settings btn-sm" onclick="togglePanel('mqttForm')">Configurar</button>
            <div id="mqttForm" class="panel" style="margin-top:12px;">
                <div class="form-group"><label>Servidor</label><input type="text" id="mqttServer" placeholder="mqtt.ejemplo.com" maxlength="63"></div>
                <div class="form-group"><label>Puerto</label><input type="number" id="mqttPort" value="1883" min="1" max="65535"></div>
                <div class="form-group"><label>Usuario</label><input type="text" id="mqttUser" placeholder="(opcional)" maxlength="31"></div>
                <div class="form-group"><label>Password</label><input type="password" id="mqttPass" placeholder="(opcional)" maxlength="31"></div>
                <div class="chk-row" style="margin-bottom:12px;">
                    <input type="checkbox" id="mqttEn">
                    <label for="mqttEn">MQTT habilitado</label>
                </div>
                <button class="wr btn-secondary btn-sm" onclick="saveMqtt()">Guardar MQTT</button>
                <div id="mqttMsg" class="form-msg"></div>
            </div>
        </div>

        <!-- Webhook config -->
        <div class="card" id="cardWebhook" style="display:none">
            <h2>Webhooks</h2>
            <div id="hookList"></div>
            <button class="wr btn-settings btn-sm" onclick="togglePanel('hookForm')">Nuevo Webhook</button>
            <div id="hookForm" class="panel" style="margin-top:12px;">
                <div class="form-group"><label>Slot (0-3)</label><input type="number" id="hookIdx" min="0" max="3" value="0"></div>
                <div class="form-group"><label>URL</label><input type="text" id="hookUrl" placeholder="https://ejemplo.com/hook" maxlength="127"></div>
                <div style="text-align:left;margin-bottom:10px;">
                    <div class="sys-label" style="margin-bottom:6px;">EVENTOS</div>
                    <div class="chk-row"><input type="checkbox" id="hkE0" checked><label for="hkE0">Seguridad</label></div>
                    <div class="chk-row"><input type="checkbox" id="hkE1" checked><label for="hkE1">Acceso</label></div>
                    <div class="chk-row"><input type="checkbox" id="hkE2" checked><label for="hkE2">Energia</label></div>
                    <div class="chk-row"><input type="checkbox" id="hkE3" checked><label for="hkE3">Ambiente</label></div>
                    <div class="chk-row"><input type="checkbox" id="hkE4" checked><label for="hkE4">Reles</label></div>
                </div>
                <button class="wr btn-secondary btn-sm" onclick="saveHook()">Guardar Webhook</button>
                <div id="hookMsg" class="form-msg"></div>
            </div>
        </div>

        <!-- Backup/Restore -->
        <div class="card" id="cardBackup" style="display:none">
            <h2>Backup / Restaurar</h2>
            <div style="margin-bottom:10px;">
                <button class="wr btn-settings btn-sm" onclick="wsSend({cmd:'cfg_export'})">Exportar Config</button>
                <button class="wr btn-warn btn-sm" style="margin-left:8px;" onclick="doImport()">Importar Config</button>
            </div>
            <div id="backupData" class="panel" style="margin-top:8px;">
                <div class="form-group">
                    <label>JSON de configuracion</label>
                    <textarea id="cfgJson" rows="6" style="width:100%%;padding:10px;font-size:0.8rem;border:2px solid #DDE4ED;border-radius:8px;font-family:monospace;resize:vertical;background:#F8FAFB;color:#2C3E50;"></textarea>
                </div>
            </div>
            <div id="backupMsg" class="form-msg"></div>
        </div>

        <!-- Timezone -->
        <div class="card" id="cardTz" style="display:none">
            <h2>Zona Horaria</h2>
            <div class="inline-row" style="margin-bottom:10px;">
                <div class="form-group"><label>GMT Offset (s)</label><input type="number" id="tzGmt" value="3600"></div>
                <div class="form-group"><label>DST Offset (s)</label><input type="number" id="tzDst" value="0"></div>
            </div>
            <button class="wr btn-secondary btn-sm" onclick="saveTz()">Guardar Timezone</button>
            <div id="tzMsg" class="form-msg"></div>
        </div>

        <!-- WiFi config -->
        <div class="card">
            <h2>Configuracion WiFi</h2>
            <button id="btnShowWifi" class="btn btn-settings" onclick="togglePanel('wifiPanel')">Abrir Ajustes</button>
            <div id="wifiPanel" class="panel">
                <div style="margin-top: 16px;">
                    <div class="form-group">
                        <label for="inputPin">PIN de Acceso</label>
                        <input type="password" id="inputPin" placeholder="Introduce PIN" maxlength="8">
                    </div>
                    <div class="form-group">
                        <label for="inputSsid">SSID</label>
                        <input type="text" id="inputSsid" placeholder="Nombre de la red WiFi" maxlength="32">
                    </div>
                    <div class="form-group">
                        <label for="inputPass">Password</label>
                        <input type="password" id="inputPass" placeholder="Contrasena WiFi" maxlength="64">
                    </div>
                    <button id="btnSaveWifi" class="btn btn-secondary" onclick="saveWifiCredentials()">Guardar Credenciales</button>
                    <div id="wifiMsg" class="form-msg"></div>
                </div>
            </div>
        </div>
        <!-- Maintenance -->
        <div class="card">
            <h2>Mantenimiento</h2>
            <a href="/update" class="btn btn-settings" style="text-decoration:none;">Actualizar Firmware</a>
        </div>
    </div>
    <div class="footer" id="appFooter">HOMENODE &middot; IoT Modular Platform</div>
    <script>
        var gateway = 'ws://' + window.location.hostname + '/ws';
        var websocket = null;
        var wsConnected = false;
        var isAuth = false;
        var localUptime = 0;
        var uptimeInterval = null;
        var importPending = false;
        window.addEventListener('load', function() { initWebSocket(); });

        /* WebSocket lifecycle */
        function initWebSocket() {
            updateConnectionUI('reconnecting');
            websocket = new WebSocket(gateway);
            websocket.onopen = onOpen;
            websocket.onclose = onClose;
            websocket.onerror = onError;
            websocket.onmessage = onMessage;
        }
        function onOpen(event) {
            wsConnected = true;
            updateConnectionUI('connected');
            wsSend({cmd: 'status'});
        }
        function onClose(event) {
            wsConnected = false;
            updateConnectionUI('disconnected');
            setAuth(false);
            setTimeout(initWebSocket, 2000);
        }
        function onError(event) { wsConnected = false; updateConnectionUI('disconnected'); }

        /* Main message handler */
        function onMessage(event) {
            try {
                var d = JSON.parse(event.data);
                /* Core telemetry */
                if (d.rssi !== undefined) updateRssiUI(d.rssi);
                if (d.uptime !== undefined) { localUptime = d.uptime; updateUptimeUI(localUptime); startUptimeCounter(); }
                if (d.ip) document.getElementById('connIp').textContent = 'IP: ' + d.ip;
                if (d.clients !== undefined) document.getElementById('sysClients').textContent = d.clients;
                if (d.ssid) document.getElementById('sysSsid').textContent = d.ssid;
                if (d.ap_mode !== undefined) document.getElementById('apBanner').style.display = d.ap_mode ? '' : 'none';
                if (d.version) document.getElementById('appFooter').textContent = 'HOMENODE \u00B7 IoT Modular Platform \u00B7 v' + d.version;

                /* Node identity */
                if (d.node_id) document.getElementById('nodeId').textContent = d.node_id;
                if (d.name !== undefined) document.getElementById('nodeName').textContent = d.name || '(sin nombre)';
                if (d.room !== undefined) document.getElementById('nodeRoom').textContent = d.room || '(sin asignar)';

                /* Show service cards on first telemetry */
                if (d.version) {
                    showEl('cardSched'); showEl('cardMqtt'); showEl('cardWebhook');
                    showEl('cardBackup'); showEl('cardTz');
                    if (!window._svcInit) { window._svcInit = true; requestServiceStatus(); }
                }

                /* Auth response */
                if (d.auth_result !== undefined) {
                    setAuth(d.auth_result);
                    if (!d.auth_result) showMsg('authMsg', false, d.auth_msg || 'Error');
                    else document.getElementById('authPin').value = '';
                }
                /* WiFi response */
                if (d.wifi_result !== undefined) showWifiMsg(d.wifi_result, d.wifi_msg || '');

                /* Scheduler responses */
                if (d.type === 'sched_list') renderSchedList(d.tasks);
                if (d.type === 'sched_status') {
                    document.getElementById('schedStatus').textContent = 'Tareas activas: ' + d.active;
                    document.getElementById('sysTime').textContent = d.time || '--:--:--';
                    var ntp = document.getElementById('sysNtp');
                    if (d.ntp_synced) { ntp.textContent = 'Synced'; ntp.className = 'sys-value st-on'; }
                    else { ntp.textContent = 'No sync'; ntp.className = 'sys-value st-off'; }
                }
                if (d.sched_result !== undefined) showMsg('schedMsg', d.sched_result, d.sched_msg);
                if (d.sched_result) wsSend({cmd:'sched_list'});

                /* MQTT responses */
                if (d.type === 'mqtt_status') {
                    var ms = document.getElementById('mqttState');
                    if (d.connected) { ms.textContent = 'Conectado'; ms.className = 'sys-value st-on'; }
                    else if (d.enabled) { ms.textContent = 'Desconectado'; ms.className = 'sys-value st-warn'; }
                    else { ms.textContent = 'Deshabilitado'; ms.className = 'sys-value st-off'; }
                    document.getElementById('mqttSrv').textContent = d.server || '---';
                    document.getElementById('mqttServer').value = d.server || '';
                    document.getElementById('mqttPort').value = d.port || 1883;
                    document.getElementById('mqttEn').checked = !!d.enabled;
                }
                if (d.mqtt_result !== undefined) showMsg('mqttMsg', d.mqtt_result, d.mqtt_msg);

                /* Webhook responses */
                if (d.type === 'webhook_list') renderHookList(d.webhooks);
                if (d.webhook_result !== undefined) {
                    showMsg('hookMsg', d.webhook_result, d.webhook_msg);
                    if (d.webhook_result) wsSend({cmd:'webhook_list'});
                }

                /* Backup responses */
                if (d.type === 'cfg_export') {
                    document.getElementById('cfgJson').value = JSON.stringify(d.config, null, 2);
                    var bp = document.getElementById('backupData');
                    if (!bp.classList.contains('open')) bp.classList.add('open');
                    showMsg('backupMsg', true, 'Configuracion exportada');
                }
                if (d.backup_result !== undefined) showMsg('backupMsg', d.backup_result, d.backup_msg);
                if (d.time_result !== undefined) showMsg('tzMsg', d.time_result, d.time_msg);

                /* Node config responses */
                if (d.node_result !== undefined) showMsg('nodeMsg', d.node_result, d.node_msg);
                if (d.type === 'node_status') {
                    document.getElementById('nodeId').textContent = d.node_id || '---';
                    document.getElementById('nodeName').textContent = d.name || '(sin nombre)';
                    document.getElementById('nodeRoom').textContent = d.room || '(sin asignar)';
                    document.getElementById('nodeMods').textContent = d.modules_str || '---';
                }

                /* ENV alert config response */
                if (d.type === 'env_alert_config') {
                    document.getElementById('envTHi').value = d.temp_hi;
                    document.getElementById('envTLo').value = d.temp_lo;
                    document.getElementById('envHHi').value = d.hum_hi;
                    document.getElementById('envHLo').value = d.hum_lo;
                    document.getElementById('envAlertEn').checked = d.enabled;
                }

                /* ACC learn mode response */
                if (d.type === 'acc_learn') {
                    document.getElementById('accLearnBanner').style.display = d.active ? '' : 'none';
                }

                /* ENV module */
                if (d.modules && d.modules.env) {
                    var env = d.modules.env;
                    document.getElementById('cardEnv').style.display = '';
                    if (env.temperature !== undefined)
                        document.getElementById('envTemp').textContent = env.temperature.toFixed(1) + ' \u00B0C';
                    if (env.humidity !== undefined)
                        document.getElementById('envHum').textContent = env.humidity.toFixed(1) + ' %%';
                    if (env.ok !== undefined)
                        document.getElementById('envStatus').textContent = env.ok ? 'Sensor: OK' : 'Sensor: Error';
                    if (env.alert_active !== undefined)
                        document.getElementById('envAlertBanner').style.display = env.alert_active ? '' : 'none';
                }
                /* RLY module */
                if (d.modules && d.modules.rly) {
                    var r = d.modules.rly;
                    document.getElementById('cardRly').style.display = '';
                    var h = '';
                    for (var i = 0; i < r.count; i++) {
                        var on = r.states[i];
                        var nm = r.names[i] || ('Rele '+(i+1));
                        h += '<div class="rly-row"><div><div class="sys-label">'+nm+'</div>'
                           + '<div class="sys-value '+(on?'st-on':'st-off')+'">'+(on?'ON':'OFF')+'</div></div>'
                           + '<div><button class="wr '+(on?'btn-on':'btn-off')+'"'
                           + ' onclick="wsSend({cmd:\'rly_toggle\',index:'+i+'})">'+(on?'ON':'OFF')+'</button>'
                           + ' <button class="wr btn-settings btn-sm"'
                           + ' onclick="wsSend({cmd:\'rly_pulse\',index:'+i+'})">Pulso</button></div></div>';
                    }
                    document.getElementById('rlyList').innerHTML = h;
                    reapplyAuth();
                }
                /* SEC module */
                if (d.modules && d.modules.sec) {
                    var s = d.modules.sec;
                    document.getElementById('cardSec').style.display = '';
                    var st = document.getElementById('secState');
                    var sm = {disarmed:['DESARMADO','st-on'], armed:['ARMADO','st-warn'],
                              triggered:['ALARMA','st-trig'], entry_delay:['ENTRADA...','st-warn'],
                              exit_delay:['SALIDA...','st-warn']};
                    var si = sm[s.state] || [s.state,''];
                    st.textContent = si[0]; st.className = 'sys-value ' + si[1];
                    document.getElementById('secPir').textContent = s.pir ? 'Activo' : 'Normal';
                    document.getElementById('secReed').textContent = s.reed ? 'Abierta' : 'Cerrada';
                    document.getElementById('secVibr').textContent = (s.vibration !== undefined) ? (s.vibration ? 'Activo' : 'Normal') : '--';
                    document.getElementById('secEvt').textContent = s.events;
                }
                /* PLG module */
                if (d.modules && d.modules.plg) {
                    var p = d.modules.plg;
                    document.getElementById('cardPlg').style.display = '';
                    var btn = document.getElementById('plgBtn');
                    btn.textContent = p.state ? 'ON' : 'OFF';
                    btn.className = 'wr ' + (p.state ? 'btn-on' : 'btn-off');
                    document.getElementById('plgV').textContent = p.voltage.toFixed(1) + ' V';
                    document.getElementById('plgI').textContent = p.current.toFixed(2) + ' A';
                    document.getElementById('plgW').textContent = p.power.toFixed(1) + ' W';
                    document.getElementById('plgE').textContent = p.energy.toFixed(2) + ' Wh';
                    document.getElementById('plgOL').style.display = (p.current >= p.overload) ? '' : 'none';
                }
                /* NRG module */
                if (d.modules && d.modules.nrg) {
                    var n = d.modules.nrg;
                    document.getElementById('cardNrg').style.display = '';
                    document.getElementById('nrgAlert').textContent = n.alert.toFixed(0);
                    var nh = '';
                    if (n.ch) {
                        for (var ci = 0; ci < n.ch.length; ci++) {
                            var c = n.ch[ci];
                            nh += '<div style="margin-bottom:10px;"><div class="sys-label" style="margin-bottom:6px;">CANAL '+(ci+1)+'</div>'
                               + '<div class="sys-grid">'
                               + '<div class="sys-item"><div class="sys-label">V</div><div class="sys-value">'+c.v.toFixed(1)+'</div></div>'
                               + '<div class="sys-item"><div class="sys-label">A</div><div class="sys-value">'+c.i.toFixed(2)+'</div></div>'
                               + '<div class="sys-item"><div class="sys-label">W</div><div class="sys-value">'+c.w.toFixed(1)+'</div></div>'
                               + '<div class="sys-item"><div class="sys-label">kWh</div><div class="sys-value">'+c.kwh.toFixed(2)+'</div></div>'
                               + '</div></div>';
                        }
                    }
                    document.getElementById('nrgCh').innerHTML = nh;
                }
                /* ACC module */
                if (d.modules && d.modules.acc) {
                    var a = d.modules.acc;
                    document.getElementById('cardAcc').style.display = '';
                    var ab = document.getElementById('accBtn');
                    ab.textContent = a.locked ? 'Cerrado' : 'Abierto';
                    ab.className = 'wr ' + (a.locked ? 'btn-off' : 'btn-on');
                    document.getElementById('accUids').textContent = a.uids;
                    document.getElementById('accEvts').textContent = a.events;
                    document.getElementById('accPulse').textContent = a.pulse_ms + ' ms';
                    document.getElementById('accLast').textContent = a.last_event || '---';
                    if (a.learn_mode !== undefined)
                        document.getElementById('accLearnBanner').style.display = a.learn_mode ? '' : 'none';
                }
            } catch (e) {}
        }

        /* Send JSON via WS */
        function wsSend(obj) {
            if (websocket && websocket.readyState === WebSocket.OPEN) websocket.send(JSON.stringify(obj));
        }

        /* Request service status after connect */
        function requestServiceStatus() {
            wsSend({cmd:'node_status'});
            wsSend({cmd:'sched_list'}); wsSend({cmd:'sched_status'});
            wsSend({cmd:'mqtt_status'}); wsSend({cmd:'webhook_list'});
            wsSend({cmd:'env_get_alert'});
        }

        /* Auth functions */
        function doAuth() {
            var pin = document.getElementById('authPin').value.trim();
            if (!pin) { showMsg('authMsg', false, 'Introduce el PIN'); return; }
            wsSend({cmd:'auth', pin: pin});
        }
        function setAuth(ok) {
            isAuth = ok;
            document.getElementById('authOk').style.display = ok ? '' : 'none';
            document.getElementById('authNo').style.display = ok ? 'none' : '';
            reapplyAuth();
        }
        function reapplyAuth() {
            var btns = document.querySelectorAll('.wr');
            for (var i = 0; i < btns.length; i++) btns[i].disabled = !isAuth;
        }

        /* Generic helpers */
        function showEl(id) { document.getElementById(id).style.display = ''; }
        function showMsg(id, ok, msg) {
            var el = document.getElementById(id);
            el.textContent = msg; el.className = 'form-msg ' + (ok ? 'success' : 'error');
            setTimeout(function() { el.className = 'form-msg'; el.textContent = ''; }, 5000);
        }
        function togglePanel(id) {
            document.getElementById(id).classList.toggle('open');
        }

        /* Connection UI */
        function updateConnectionUI(status) {
            var dot = document.getElementById('connDot');
            var text = document.getElementById('connText');
            dot.className = 'conn-dot ' + status;
            if (status === 'connected') { text.textContent = 'Connected'; text.className = 'conn-text connected'; }
            else if (status === 'reconnecting') { text.textContent = 'Reconnecting...'; text.className = 'conn-text reconnecting'; }
            else { text.textContent = 'Disconnected'; text.className = 'conn-text disconnected';
                   document.getElementById('connIp').textContent = 'IP: ---'; }
        }
        function updateRssiUI(rssi) {
            document.getElementById('sysRssi').textContent = rssi + ' dBm';
            var pct = Math.min(100, Math.max(0, ((rssi + 90) / 60) * 100));
            var bar = document.getElementById('rssiBar');
            bar.style.width = pct + '%%';
            if (pct > 60) bar.style.backgroundColor = '#2ECC71';
            else if (pct > 30) bar.style.backgroundColor = '#F39C12';
            else bar.style.backgroundColor = '#E74C3C';
        }
        function startUptimeCounter() {
            if (uptimeInterval) clearInterval(uptimeInterval);
            uptimeInterval = setInterval(function() { localUptime += 1000; updateUptimeUI(localUptime); }, 1000);
        }
        function updateUptimeUI(ms) {
            var totalSec = Math.floor(ms / 1000);
            var h = Math.floor(totalSec / 3600);
            var m = Math.floor((totalSec %% 3600) / 60);
            var s = totalSec %% 60;
            document.getElementById('sysUptime').textContent = pad(h) + ':' + pad(m) + ':' + pad(s);
        }
        function pad(n) { return n < 10 ? '0' + n : '' + n; }

        /* WiFi panel */
        function saveWifiCredentials() {
            var pin = document.getElementById('inputPin').value.trim();
            var ssid = document.getElementById('inputSsid').value.trim();
            var pass = document.getElementById('inputPass').value;
            if (!pin) { showWifiMsg(false, 'Introduce el PIN de acceso'); return; }
            if (!ssid) { showWifiMsg(false, 'Introduce el nombre de la red (SSID)'); return; }
            if (pass.length < 8 && pass.length > 0) { showWifiMsg(false, 'La contrasena debe tener al menos 8 caracteres'); return; }
            wsSend({cmd: 'wifi', ssid: ssid, pass: pass, pin: pin});
        }
        function showWifiMsg(success, msg) {
            var el = document.getElementById('wifiMsg');
            el.textContent = success ? (msg || 'Credenciales guardadas. Reiniciando...') : (msg || 'Error al guardar');
            el.className = 'form-msg ' + (success ? 'success' : 'error');
            if (success) { document.getElementById('inputPin').value = '';
                          document.getElementById('inputSsid').value = '';
                          document.getElementById('inputPass').value = ''; }
        }

        /* ENV alerts */
        function saveEnvAlert() {
            wsSend({cmd:'env_set_alert',
                temp_hi: parseFloat(document.getElementById('envTHi').value),
                temp_lo: parseFloat(document.getElementById('envTLo').value),
                hum_hi: parseFloat(document.getElementById('envHHi').value),
                hum_lo: parseFloat(document.getElementById('envHLo').value),
                enabled: document.getElementById('envAlertEn').checked
            });
            showMsg('envAlertMsg', true, 'Alertas guardadas');
        }

        /* Scheduler */
        var dayNames = ['D','L','M','X','J','V','S'];
        function renderSchedList(tasks) {
            var html = '';
            for (var i = 0; i < tasks.length; i++) {
                var t = tasks[i];
                if (!t.enabled && !t.action) continue;
                var days = '';
                for (var d = 0; d < 7; d++) days += (t.days & (1 << d)) ? dayNames[d] : '-';
                var cls = t.enabled ? 'st-on' : 'st-off';
                html += '<div class="sched-row"><div class="sched-info">'
                     + '<div class="sched-name '+ cls +'">' + (t.name || 'Tarea '+i) + '</div>'
                     + '<div class="sched-detail">' + pad(t.hour)+':'+pad(t.minute) + ' ['+days+'] &rarr; ' + t.action + '</div>'
                     + '</div><div>'
                     + '<button class="wr btn-off btn-sm" onclick="wsSend({cmd:\'sched_delete\',index:'+i+'})">X</button>'
                     + '</div></div>';
            }
            document.getElementById('schedList').innerHTML = html || '<div style="font-size:0.8rem;color:#7F8C8D;margin-bottom:8px;">Sin tareas programadas</div>';
            reapplyAuth();
        }
        function saveSched() {
            var days = 0;
            for (var i = 0; i < 7; i++) if (document.getElementById('schD'+i).checked) days |= (1 << i);
            wsSend({cmd:'sched_set',
                index: parseInt(document.getElementById('schedIdx').value),
                name: document.getElementById('schedName').value.trim() || 'Tarea',
                hour: parseInt(document.getElementById('schedHour').value),
                minute: parseInt(document.getElementById('schedMin').value),
                days: days,
                action: document.getElementById('schedAction').value.trim(),
                enabled: 1
            });
        }

        /* MQTT */
        function saveMqtt() {
            var server = document.getElementById('mqttServer').value.trim();
            var port = parseInt(document.getElementById('mqttPort').value);
            if (!server) { showMsg('mqttMsg', false, 'Introduce servidor'); return; }
            wsSend({cmd:'mqtt_config', server: server, port: port,
                user: document.getElementById('mqttUser').value.trim(),
                pass: document.getElementById('mqttPass').value
            });
            var en = document.getElementById('mqttEn').checked;
            wsSend({cmd:'mqtt_enable', enabled: en});
        }

        /* Webhooks */
        function renderHookList(hooks) {
            var html = '';
            for (var i = 0; i < hooks.length; i++) {
                if (!hooks[i].active) continue;
                html += '<div class="hook-row"><div class="hook-url">'
                     + '<strong>#'+i+'</strong> ' + hooks[i].url
                     + '</div><div>'
                     + '<button class="wr btn-settings btn-sm" onclick="wsSend({cmd:\'webhook_test\',index:'+i+'})">Test</button> '
                     + '<button class="wr btn-off btn-sm" onclick="wsSend({cmd:\'webhook_delete\',index:'+i+'})">X</button>'
                     + '</div></div>';
            }
            document.getElementById('hookList').innerHTML = html || '<div style="font-size:0.8rem;color:#7F8C8D;margin-bottom:8px;">Sin webhooks configurados</div>';
            reapplyAuth();
        }
        function saveHook() {
            var url = document.getElementById('hookUrl').value.trim();
            if (!url) { showMsg('hookMsg', false, 'Introduce URL'); return; }
            var events = 0;
            /* SEC=0x01|0x02|0x04, ACC=0x08|0x10, NRG=0x20, ENV=0x40, RLY=0x80 */
            if (document.getElementById('hkE0').checked) events |= 0x07;
            if (document.getElementById('hkE1').checked) events |= 0x18;
            if (document.getElementById('hkE2').checked) events |= 0x20;
            if (document.getElementById('hkE3').checked) events |= 0x40;
            if (document.getElementById('hkE4').checked) events |= 0x80;
            wsSend({cmd:'webhook_set',
                index: parseInt(document.getElementById('hookIdx').value),
                url: url, events: events
            });
        }

        /* Backup/Restore */
        function doImport() {
            var json = document.getElementById('cfgJson').value.trim();
            if (!json) {
                var bp = document.getElementById('backupData');
                if (!bp.classList.contains('open')) bp.classList.add('open');
                showMsg('backupMsg', false, 'Pega el JSON de configuracion y pulsa Importar de nuevo');
                return;
            }
            try { var cfg = JSON.parse(json); } catch(e) { showMsg('backupMsg', false, 'JSON invalido'); return; }
            if (!importPending) {
                importPending = true;
                showMsg('backupMsg', false, 'Pulsa Importar de nuevo en 10s para confirmar');
                setTimeout(function() { importPending = false; }, 10000);
                wsSend({cmd:'cfg_import', config: cfg});
                return;
            }
            importPending = false;
            wsSend({cmd:'cfg_import', config: cfg});
        }

        /* Timezone */
        function saveTz() {
            wsSend({cmd:'time_set_tz',
                gmt_offset: parseInt(document.getElementById('tzGmt').value),
                dst_offset: parseInt(document.getElementById('tzDst').value)
            });
        }

        /* Node config */
        function saveNode() {
            var name = document.getElementById('cfgNodeName').value.trim();
            var room = document.getElementById('cfgNodeRoom').value.trim();
            if (name) wsSend({cmd:'node_set_name', name: name});
            if (room) wsSend({cmd:'node_set_room', room: room});
            if (!name && !room) showMsg('nodeMsg', false, 'Introduce nombre o habitacion');
            else showMsg('nodeMsg', true, 'Guardado');
            setTimeout(function() { wsSend({cmd:'node_status'}); }, 500);
        }
    </script>
</body>
</html>
)rawliteral";

#endif // UNIT_TEST

#endif // SMRT_CORE_WEBUI_H
