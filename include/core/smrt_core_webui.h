/**
 * @file    smrt_core_webui.h
 * @brief   Base HTML/CSS/JS web interface stored in PROGMEM
 * @project HOMENODE
 * @version 0.6.0
 *
 * Full dashboard with:
 *   - Connection status card
 *   - System info card (uptime, RSSI, SSID, clients)
 *   - Authentication card (PIN-based WS auth)
 *   - Module cards: ENV, RLY, SEC, PLG, NRG, ACC
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
        .wifi-panel { max-height: 0; overflow: hidden; transition: max-height 0.4s ease, opacity 0.3s ease; opacity: 0; }
        .wifi-panel.open { max-height: 500px; opacity: 1; }
        .form-group { margin-bottom: 14px; text-align: left; }
        .form-group label { display: block; font-size: 0.75rem; font-weight: 700; color: #4A6FA5;
                            text-transform: uppercase; letter-spacing: 1px; margin-bottom: 6px; }
        .form-group input { width: 100%%; padding: 11px 14px; font-size: 0.95rem; border: 2px solid #DDE4ED;
                            border-radius: 8px; outline: none; transition: border-color 0.3s ease, box-shadow 0.3s ease;
                            background: #F8FAFB; color: #2C3E50; }
        .form-group input:focus { border-color: #E8723A; box-shadow: 0 0 0 3px rgba(232,114,58,0.15); background: #FFFFFF; }
        .form-group input::placeholder { color: #B0BEC5; }
        .form-msg { font-size: 0.85rem; font-weight: 600; margin-top: 12px; padding: 8px 12px;
                    border-radius: 6px; display: none; }
        .form-msg.success { display: block; background: #E8F8F0; color: #27AE60; border: 1px solid #A3DFBF; }
        .form-msg.error { display: block; background: #FDEDEC; color: #E74C3C; border: 1px solid #F1948A; }
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
        .rly-row { display: flex; justify-content: space-between; align-items: center;
                   background: #F4F7FB; border-radius: 8px; padding: 10px 12px; margin-bottom: 8px; }
        .rly-row .sys-label { margin-bottom: 2px; }
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
                <div id="envStatus" style="font-size:0.75rem;color:#7F8C8D;margin-top:10px;">Sensor: ---</div>
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
            </div>
        </div>
        <!-- WiFi config -->
        <div class="card">
            <h2>Configuracion WiFi</h2>
            <button id="btnShowWifi" class="btn btn-settings" onclick="toggleWifiPanel()">Abrir Ajustes</button>
            <div id="wifiPanel" class="wifi-panel">
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
    <div class="footer">HOMENODE &middot; IoT Modular Platform &middot; v0.5.0</div>
    <script>
        var gateway = 'ws://' + window.location.hostname + '/ws';
        var websocket = null;
        var wsConnected = false;
        var isAuth = false;
        var localUptime = 0;
        var uptimeInterval = null;
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
                /* Auth response */
                if (d.auth_result !== undefined) {
                    setAuth(d.auth_result);
                    if (!d.auth_result) showAuthMsg(false, d.auth_msg || 'Error');
                    else document.getElementById('authPin').value = '';
                }
                /* WiFi response */
                if (d.wifi_result !== undefined) showWifiMsg(d.wifi_result, d.wifi_msg || '');
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
                }
            } catch (e) {}
        }

        /* Send JSON via WS */
        function wsSend(obj) {
            if (websocket && websocket.readyState === WebSocket.OPEN) websocket.send(JSON.stringify(obj));
        }

        /* Auth functions */
        function doAuth() {
            var pin = document.getElementById('authPin').value.trim();
            if (!pin) { showAuthMsg(false, 'Introduce el PIN'); return; }
            wsSend({cmd:'auth', pin: pin});
        }
        function showAuthMsg(ok, msg) {
            var el = document.getElementById('authMsg');
            el.textContent = msg;
            el.className = 'form-msg ' + (ok ? 'success' : 'error');
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
        function toggleWifiPanel() {
            var panel = document.getElementById('wifiPanel');
            var btn = document.getElementById('btnShowWifi');
            panel.classList.toggle('open');
            if (panel.classList.contains('open')) btn.textContent = 'Cerrar Ajustes';
            else { btn.textContent = 'Abrir Ajustes'; hideWifiMsg(); }
        }
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
        function hideWifiMsg() {
            var el = document.getElementById('wifiMsg');
            el.className = 'form-msg'; el.textContent = '';
        }
    </script>
</body>
</html>
)rawliteral";

#endif // UNIT_TEST

#endif // SMRT_CORE_WEBUI_H
