#ifndef CONFIG_HTML_H
#define CONFIG_HTML_H

// HTML armazenado na Flash - Layout Expandido / Air UI
static const char config_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Nexus Telemetry - Configurações</title>
  <style>
    :root {
      --bg-main: #0a0413;
      --bg-card: rgba(24, 12, 48, 0.7);
      --bg-card-hover: rgba(35, 18, 68, 0.85);
      --bg-input: rgba(15, 7, 32, 0.9);
      --border-card: rgba(255, 255, 255, 0.08);
      --text-primary: #f0f3f9;
      --text-secondary: #8b7ca8;
      --accent-cyan: #00f2fe;
      --accent-magenta: #ff007f;
      --accent-green: #00ff88;
      --accent-yellow: #ffcc00;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; }

    body {
      background: var(--bg-main);
      background-image: 
        radial-gradient(circle at 10%% 20%%, rgba(255, 0, 127, 0.05) 0%%, transparent 40%%),
        radial-gradient(circle at 90%% 80%%, rgba(0, 242, 254, 0.05) 0%%, transparent 40%%);
      color: var(--text-primary);
      min-height: 100vh;
      padding: 20px;
      display: flex;
      justify-content: center;
      align-items: flex-start;
    }

    .app-container {
      width: 100%%;
      max-width: 1000px;
      display: flex;
      flex-direction: column;
      gap: 20px;
    }

    /* Header & Branding */
    .header-card {
      background: var(--bg-card);
      backdrop-filter: blur(12px);
      border: 1px solid var(--border-card);
      border-radius: 16px;
      padding: 24px;
      display: flex;
      flex-direction: column;
      gap: 20px;
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.4);
    }

    .brand-section {
      display: flex;
      align-items: center;
      gap: 20px;
    }

    /* Logo sem marca azul e expandido */
    .brand-logo {
      width: 85px;
      height: 85px;
      display: flex;
      align-items: center;
      justify-content: center;
      background: transparent;
      padding: 0;
    }

    .brand-logo img {
      width: 100%%;
      height: 100%%;
      object-fit: contain;
      filter: drop-shadow(0 4px 8px rgba(0,0,0,0.5));
    }

    .brand-titles h1 {
      font-size: 22px;
      font-weight: 700;
      letter-spacing: 0.5px;
      background: linear-gradient(90deg, #fff, var(--text-secondary));
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }

    .brand-titles p {
      font-size: 13px;
      color: var(--accent-cyan);
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 1px;
    }

    /* Grid de Telemetria Detalhada Completa */
    .telemetry-full-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
      gap: 15px 30px;
      width: 100%%;
      background: rgba(15, 7, 32, 0.6);
      border: 1px solid var(--border-card);
      border-radius: 12px;
      padding: 18px 22px;
    }

    .telemetry-column {
      display: flex;
      flex-direction: column;
      gap: 10px;
    }

    .telemetry-line {
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 13px;
      border-bottom: 1px dashed rgba(255, 255, 255, 0.08);
      padding-bottom: 6px;
    }

    .telemetry-line:last-child {
      border-bottom: none;
    }

    .telemetry-line .label {
      color: var(--text-secondary);
      font-weight: 600;
    }

    .telemetry-line .value {
      color: var(--accent-cyan);
      font-weight: 700;
      font-family: monospace;
    }

    /* Cards Layout */
    .content-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
      gap: 20px;
    }

    .card {
      background: var(--bg-card);
      backdrop-filter: blur(12px);
      border: 1px solid var(--border-card);
      border-radius: 16px;
      padding: 24px;
      display: flex;
      flex-direction: column;
      gap: 20px;
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
      transition: border-color 0.3s ease;
    }

    .card:hover {
      border-color: rgba(255, 255, 255, 0.15);
    }

    .card-header {
      display: flex;
      align-items: center;
      gap: 10px;
      border-bottom: 1px solid var(--border-card);
      padding-bottom: 12px;
    }

    .card-header svg {
      width: 20px;
      height: 20px;
      fill: var(--accent-cyan);
    }

    .card-header h2 {
      font-size: 15px;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 1px;
      color: var(--text-primary);
    }

    /* Controls */
    .setting-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      background: var(--bg-input);
      border: 1px solid var(--border-card);
      padding: 14px 18px;
      border-radius: 12px;
    }

    .setting-info {
      display: flex;
      flex-direction: column;
      gap: 2px;
    }

    .setting-title {
      font-size: 14px;
      font-weight: 600;
      color: var(--text-primary);
    }

    .setting-desc {
      font-size: 11px;
      color: var(--text-secondary);
    }

    /* Toggle Switch */
    .switch {
      position: relative;
      display: inline-block;
      width: 52px;
      height: 28px;
    }

    .switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }

    .slider {
      position: absolute;
      cursor: pointer;
      top: 0; left: 0; right: 0; bottom: 0;
      background-color: #231244;
      transition: .3s cubic-bezier(0.4, 0, 0.2, 1);
      border-radius: 34px;
      border: 1px solid var(--border-card);
    }

    .slider:before {
      position: absolute;
      content: "";
      height: 20px;
      width: 20px;
      left: 3px;
      bottom: 3px;
      background-color: #fff;
      transition: .3s cubic-bezier(0.4, 0, 0.2, 1);
      border-radius: 50%%;
      box-shadow: 0 2px 4px rgba(0,0,0,0.4);
    }

    input:checked + .slider {
      background-color: var(--accent-cyan);
      box-shadow: 0 0 12px rgba(0, 242, 254, 0.4);
    }

    input:checked + .slider:before {
      transform: translateX(24px);
      background-color: #0a0413;
    }

    /* Forms */
    .form-group {
      display: flex;
      flex-direction: column;
      gap: 16px;
    }

    .input-wrapper {
      display: flex;
      align-items: center;
      justify-content: space-between;
      background: var(--bg-input);
      border: 1px solid var(--border-card);
      padding: 12px 16px;
      border-radius: 12px;
      transition: all 0.3s ease;
    }

    .input-wrapper:focus-within {
      border-color: var(--accent-cyan);
      box-shadow: 0 0 10px rgba(0, 242, 254, 0.2);
    }

    .input-label {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 13px;
      font-weight: 600;
    }

    .tooltip-icon {
      width: 16px;
      height: 16px;
      cursor: pointer;
      opacity: 0.6;
      transition: opacity 0.2s;
    }

    .tooltip-icon:hover { opacity: 1; }

    input[type="time"] {
      background: transparent;
      border: none;
      color: var(--accent-cyan);
      font-size: 15px;
      font-weight: 700;
      outline: none;
      cursor: pointer;
    }

    input[type="time"]::-webkit-calendar-picker-indicator {
      filter: invert(1) hue-rotate(180deg);
      cursor: pointer;
    }

    .btn-primary {
      background: linear-gradient(135deg, var(--accent-cyan), #00a8ff);
      color: #0a0413;
      border: none;
      padding: 14px;
      border-radius: 12px;
      font-size: 13px;
      font-weight: 800;
      text-transform: uppercase;
      letter-spacing: 1px;
      cursor: pointer;
      transition: all 0.3s ease;
      box-shadow: 0 4px 15px rgba(0, 242, 254, 0.3);
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
    }

    .btn-primary:hover {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(0, 242, 254, 0.5);
    }

    /* Toast Notification */
    #toast {
      visibility: hidden;
      min-width: 250px;
      background: var(--accent-green);
      color: #000;
      text-align: center;
      border-radius: 8px;
      padding: 12px 20px;
      position: fixed;
      z-index: 1000;
      right: 30px;
      bottom: 30px;
      font-weight: 700;
      font-size: 13px;
      box-shadow: 0 5px 15px rgba(0, 255, 136, 0.4);
      opacity: 0;
      transition: opacity 0.3s, bottom 0.3s;
    }

    #toast.show {
      visibility: visible;
      opacity: 1;
      bottom: 40px;
    }

    @media (max-width: 768px) {
      body { padding: 12px; }
      .brand-section { flex-direction: column; text-align: center; }
      .telemetry-full-grid { grid-template-columns: 1fr; }
      .content-grid { grid-template-columns: 1fr; }
    }
  </style>
</head>
<body>

  <div class="app-container">
    <!-- Header Principal -->
    <header class="header-card">
      <div class="brand-section">
        <div class="brand-logo">
          <img src="https://cdn.iconscout.com/icon/free/png-256/free-espressif-logo-icon-svg-download-png-2285012.png?f=webp" alt="ESP32 Logo" />
        </div>
        <div class="brand-titles">
          <h1>PAINEL NEXUS</h1>
          <p>Módulo de Telemetria & Configuração</p>
        </div>
      </div>

      <!-- Telemetria Explicita com TODAS as linhas do topo -->
      <div class="telemetry-full-grid">
        <div class="telemetry-column">
          <div class="telemetry-line">
            <span class="label">Módulo:</span>
            <span class="value">%MODULE_VALUE%</span>
          </div>
          <div class="telemetry-line">
            <span class="label">SSID:</span>
            <span class="value">%SSID_VALUE%</span>
          </div>
          <div class="telemetry-line">
            <span class="label">IP Address:</span>
            <span class="value">%IP_VALUE%</span>
          </div>
          <div class="telemetry-line">
            <span class="label">MAC Address:</span>
            <span class="value">%MAC_VALUE%</span>
          </div>
        </div>

        <div class="telemetry-column">
          <div class="telemetry-line">
            <span class="label">Total RAM Free:</span>
            <span class="value" style="color: var(--accent-green);">%TOTAL_RAM_VALUE%</span>
          </div>
          <div class="telemetry-line">
            <span class="label">Flash Size:</span>
            <span class="value">%FLASH_SIZE_VALUE%</span>
          </div>
          <div class="telemetry-line">
            <span class="label">Menor RAM Free Heap Register:</span>
            <span class="value" style="color: var(--accent-yellow);">%MENOR_RAM_SIZE_VALUE%</span>
          </div>
          <div class="telemetry-line">
            <span class="label">Sketch Size:</span>
            <span class="value">%SKETCH_SIZE_VALUE%</span>
          </div>
        </div>
      </div>
    </header>

    <!-- Seções de Configuração -->
    <div class="content-grid">
      
    <!-- Card: Servidor Web -->
    <section class="card">
    <div class="card-header">
        <svg viewBox="0 0 24 24"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-1 17.93c-3.95-.49-7-3.85-7-7.93 0-.62.08-1.21.21-1.79L9 15v1c0 1.1.9 2 2 2v1.93zm6.9-2.54c-.26-.81-1-1.39-1.9-1.39h-1v-3c0-.55-.45-1-1-1H8v-2h2c.55 0 1-.45 1-1V7h2c1.1 0 2-.9 2-2v-.41c2.93 1.19 5 4.06 5 7.41 0 2.08-.8 3.97-2.1 5.39z"/></svg>
        <h2>Servidor Web</h2>
    </div>

    <div class="setting-row">
        <div class="setting-info">
        <span class="setting-title">Servidor HTTP Local</span>
        <span class="setting-desc">Ativa ou suspende a interface web do ESP32</span>
        </div>
        <label class="switch">
        <!-- O marcador %WEBSERVER_STATE% injetará 'checked' se o servidor estiver ligado -->
        <input id="switch-webserver" type="checkbox" %WEBSERVER_STATE%>
        <span class="slider"></span>
        </label>
    </div>
    </section>

    <!-- Card: Agenda Wakeon (OLED) -->
    <section class="card">
    <div class="card-header">
        <svg viewBox="0 0 24 24" style="fill: var(--accent-magenta);"><path d="M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm.5-13H11v6l5.25 3.15.75-1.23-4.5-2.67z"/></svg>
        <h2 style="color: var(--accent-magenta);">Agendamento Display OLED</h2>
    </div>

    <form id="wakeonHtmlForm" class="form-group" onsubmit="salvarConfiguracoes(event)">
        <div class="input-wrapper">
        <label class="input-label" for="web_wake_up_time">
            <span>WakeOn (Despertar)</span>
            <svg class="tooltip-icon" viewBox="0 0 24 24" fill="var(--text-secondary)" title="Define o horário em que o display OLED sairá do modo de suspensão automaticamente."><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z"/></svg>
        </label>
        <!-- Preenchido dinamicamente via %WAKE_TIME_VALUE% -->
        <input type="time" id="web_wake_up_time" value="%WAKE_TIME_VALUE%" required>
        </div>

        <div class="input-wrapper">
        <label class="input-label" for="web_sleep_time">
            <span>Sleep (Desligar)</span>
            <svg class="tooltip-icon" viewBox="0 0 24 24" fill="var(--text-secondary)" title="Define o horário em que o display OLED entrará em modo de economia/suspensão profunda."><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z"/></svg>
        </label>
        <!-- Preenchido dinamicamente via %SLEEP_TIME_VALUE% -->
        <input type="time" id="web_sleep_time" value="%SLEEP_TIME_VALUE%" required>
        </div>

        <button type="submit" class="btn-primary">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><path d="M17 3H5c-1.11 0-2 .9-2 2v14c0 1.1.89 2 2 2h14c1.1 0 2-.9 2-2V7l-4-4zm-5 16c-1.66 0-3-1.34-3-3s1.34-3 3-3 3 1.34 3 3-1.34 3-3 3zm3-10H5V5h10v4z"/></svg>
        Salvar Parâmetros
        </button>
    </form>
    </section>
    </div>
  </div>

  <div id="toast">Configurações salvas com sucesso!</div>

  <script>
    function showToast(mensagem, cor = 'var(--accent-green)') {
      const toast = document.getElementById("toast");
      toast.innerText = mensagem;
      toast.style.background = cor;
      toast.classList.add("show");
      setTimeout(() => { toast.classList.remove("show"); }, 3000);
    }

    document.addEventListener("DOMContentLoaded", () => {
    // 1. Escuta a alteração direta do Switch do Servidor
    const switchWeb = document.getElementById("switch-webserver");
    if (switchWeb) {
        switchWeb.addEventListener("change", async (e) => {
        try {
            await fetch("/api/config", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ webserver: e.target.checked })
            });
        } catch (err) {
            console.error("Erro ao alterar estado do servidor:", err);
        }
        });
    }
    });

    // 2. Manipula o envio do Formulário de Agendamento (Botão "Salvar Parâmetros")
    async function salvarConfiguracoes(event) {
    event.preventDefault();
    
    const wakeTime = document.getElementById("web_wake_up_time").value;
    const sleepTime = document.getElementById("web_sleep_time").value;

    try {
        const response = await fetch("/api/config", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
            wake_time: wakeTime,
            sleep_time: sleepTime
        })
        });
        
        if (response.ok) {
        alert("Parâmetros do Display salvos com sucesso!");
        }
    } catch (err) {
        console.error("Erro ao salvar agendamento:", err);
    }
    }
  </script>
</body>
</html>
)rawliteral";

#endif // CONFIG_HTML_H