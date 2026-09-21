#ifndef INDEX_HTML_H
#define INDEX_HTML_H

// HTML armazenado na Flash
static const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR" data-theme="dark">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Dashboard de Telemetria</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    @import url("https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined:opsz,wght,FILL,GRAD@20..48,100..700,0..1,-50..200");
    @import url('https://fonts.googleapis.com/css2?family=Vast+Shadow&display=swap');
    :root {
      --bg-main: #0c0517;
      --bg-sidebar: #120724;
      --bg-card: #180c30;
      --border-card: #28144d;
      --text-primary: #ffffff;
      --text-muted: #8b7ca8;
      --accent-cyan: #00f2fe;
      --accent-magenta: #ff007f;
      --accent-green: #00ff88;
      --accent-yellow: #ffcc00;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; }
    body { background: var(--bg-main); color: var(--text-primary); display: flex; min-height: 100vh; overflow-x: hidden; }

    .main-wrapper { flex: 1; display: flex; flex-direction: column; }
    .topbar { height: 60px; background: var(--bg-sidebar); border-bottom: 1px solid var(--border-card); display: flex; align-items: center; justify-content: space-between; padding: 0 25px; }
    .brand { font-size: 20px; font-weight: bold; color: var(--accent-cyan); display: flex; align-items: center; gap: 10px; }
    .topbar-right { display: flex; align-items: center; gap: 15px; }
    nav .icon {
        color: #cf0844;
        font-weight: bold;
        font-size: 30px;
        position: relative;
        top: 15px;
    }
    .select-host {
      background: var(--bg-card);
      color: var(--text-primary);
      border: 1px solid var(--border-card);
      padding: 6px 14px;
      border-radius: 8px;
      font-size: 13px;
      font-weight: 600;
      outline: none;
      cursor: pointer;
      transition: all 0.3s ease;
    }
    .select-host:hover, .select-host:focus { border-color: var(--accent-cyan); }

    .status { 
      font-size: 12px; 
      font-weight: bold; 
      padding: 4px 12px; 
      border-radius: 20px; 
      color: var(--accent-green); 
      border: 1px solid var(--accent-green); 
      background: rgba(0, 255, 136, 0.15); 
      transition: all 0.3s ease; 
      display: inline-block;
    }

    .content { padding: 25px; display: flex; flex-direction: column; gap: 20px; overflow-y: auto; }
    .breadcrumb { font-size: 12px; color: var(--text-muted); margin-bottom: 5px; }

    .card { background: var(--bg-card); border: 1px solid var(--border-card); border-radius: 12px; padding: 18px; position: relative; box-shadow: 0 4px 20px rgba(0,0,0,0.3); }
    .card-title { font-size: 12px; color: var(--text-muted); text-transform: uppercase; font-weight: 700; margin-bottom: 10px; white-space: nowrap; }

    /* CARD SISTEMA OPERACIONAL */
    .row-os { width: 100%; }
    .card-os { width: 100%; overflow: hidden; position: relative; min-height: 95px; }
    .os-bg-logo { 
      position: absolute; 
      right: 15px; 
      bottom: -10px; 
      width: 120px; 
      height: 120px; 
      opacity: 0.15; 
      background-size: contain; 
      background-repeat: no-repeat; 
      background-position: center right; 
      pointer-events: none; 
      transition: background-image 0.4s ease, opacity 0.4s ease; 
    }
    .os-content { position: relative; z-index: 2; }
    .os-name { font-size: 22px; font-weight: bold; color: var(--text-primary); white-space: nowrap; overflow: hidden; text-overflow: ellipsis; margin-bottom: 4px; }
    .os-details { white-space: nowrap; overflow: hidden; text-overflow: ellipsis; font-size: 13px; color: var(--text-muted); }

    /* GAUGES CIRCULARES */
    .row-top { display: grid; grid-template-columns: repeat(auto-fit, minmax(160px, 1fr)); gap: 15px; width: 100%; }
    .gauge-container { 
      position: relative; 
      width: 100px; 
      height: 100px; 
      margin: 0 auto; 
      display: flex; 
      align-items: center; 
      justify-content: center; 
    }
    .gauge-value { 
      position: absolute; 
      top: 50%; 
      left: 50%; 
      transform: translate(-50%, -50%); 
      font-size: 16px; 
      font-weight: bold; 
      line-height: 1; 
      pointer-events: none; 
      margin: 0; 
      padding: 0;
    }
    .stat-label { text-align: center; font-size: 12px; color: var(--accent-cyan); font-weight: bold; margin-top: 8px; white-space: nowrap; }

    /* MÉTRICAS EMPILHADAS */
    .row-bottom { width: 100%; }
    .card-metrics { width: 100%; display: flex; flex-direction: column; gap: 15px; }
    .metrics-stack { display: flex; flex-direction: column; gap: 15px; }
    .metric-item { width: 100%; display: flex; flex-direction: column; }
    .metric-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 6px; }
    .metric-label { font-size: 13px; font-weight: 600; color: var(--text-primary); }
    .spectrum-bar { height: 10px; border-radius: 5px; background: #251342; overflow: hidden; position: relative; }
    .spectrum-fill { height: 100%; width: 0%; background: linear-gradient(90deg, var(--accent-cyan), var(--accent-magenta)); transition: width 0.5s ease; }
    .sub-text { font-size: 11px; color: var(--text-muted); margin-top: 5px; display: block; }

    .metrics-footer { display: flex; justify-content: space-between; align-items: flex-end; border-top: 1px solid var(--border-card); padding-top: 10px; margin-top: 5px; }

    .host-section {
      display: flex;
      flex-direction: column;
      gap: 20px;
      padding-bottom: 30px;
      border-bottom: 1px dashed var(--border-card);
    }
    .host-section:last-child {
      border-bottom: none;
      padding-bottom: 0;
    }
  </style>
</head>
<body>
  <div class="main-wrapper">
    <header class="topbar">
      <div class="brand"><span>≡</span> Dashboard</div>
      <div class="topbar-right">
        <select id="hostSelect" class="select-host" onchange="updateHostVisibility()">
          <option value="ALL">Todos os Hosts</option>
        </select>
        <div class="conteiner_right">
            <div class="data_hora">
                <p id="current_hour">--:--:--</p>
                <p id="current_date">--/--/----</p>
            </div>
        </div>
        <span id="status" class="status">Conectando...</span>
      </div>
    </header>

    <main class="content">
      <div class="breadcrumb">Dashboard / Home</div>
      <div id="dashboard-container" style="display: flex; flex-direction: column; gap: 30px;">
        <!-- Os blocos de cada Host serão renderizados dinamicamente aqui -->
      </div>
    </main>
  </div>

  <script>
    const gaugeCharts = {};
    const hostsMap = {};

    function getSafeKey(key) {
      return String(key).replace(/[^a-zA-Z0-9_-]/g, '_');
    }

    function createGauge(elementId, color) {
      return new Chart(document.getElementById(elementId), {
        type: 'doughnut',
        data: { datasets: [{ data: [0, 100], backgroundColor: [color, '#251342'], borderWidth: 0 }] },
        options: { 
          cutout: '80%', 
          responsive: true, 
          maintainAspectRatio: false, 
          aspectRatio: 1,
          plugins: { legend: { display: false }, tooltip: { enabled: false } } 
        }
      });
    }

    function updateGauge(chart, value) {
      if (!chart) return;
      chart.data.datasets[0].data = [value, Math.max(0, 100 - value)];
      chart.update();
    }

    function formatDynamicBytes(value, inputUnit = 'MB') {
      if (value === undefined || value === null || value <= 0) return '--';
      let bytes = value;
      if (inputUnit === 'MB') bytes = value * 1024 * 1024;
      else if (inputUnit === 'GB') bytes = value * 1024 * 1024 * 1024;
      else if (inputUnit === 'KB') bytes = value * 1024;

      const units = ['B', 'KB', 'MB', 'GB', 'TB'];
      let i = 0;
      while (bytes >= 1024 && i < units.length - 1) {
        bytes /= 1024;
        i++;
      }
      return `${bytes.toFixed(bytes % 1 === 0 ? 0 : 1)} ${units[i]}`;
    }

    function addOptionToSelect(hostName) {
      const select = document.getElementById('hostSelect');
      const exists = Array.from(select.options).some(opt => opt.value === hostName);
      if (!exists) {
        const option = document.createElement('option');
        option.value = hostName;
        option.textContent = hostName;
        select.appendChild(option);
      }
    }

    function updateHostVisibility() {
      const select = document.getElementById('hostSelect');
      const selectedValue = select.value;

      Object.keys(hostsMap).forEach(key => {
        const safeKey = getSafeKey(key);
        const el = document.getElementById(`host-section-${safeKey}`);
        if (el) {
          if (selectedValue === 'ALL' || selectedValue === key) {
            el.style.display = 'flex';
          } else {
            el.style.display = 'none';
          }
        }
      });
    }

    function createHostDOM(hostName, safeKey) {
      const container = document.createElement('div');
      container.className = 'host-section';
      container.id = `host-section-${safeKey}`;

      container.innerHTML = `
        <div class="row-os">
          <div class="card card-os">
            <div class="os-bg-logo" id="os-bg-logo-${safeKey}"></div>
            <div class="os-content">
              <div class="card-title" style="color: var(--accent-yellow);">SISTEMA OPERACIONAL</div>
              <div class="os-name" id="os-name-short-${safeKey}">--</div>
              <p class="sub-text os-details" id="os-details-${safeKey}" style="margin-top:0;">Aguardando telemetria...</p>
            </div>
          </div>
        </div>

        <div class="row-top">
          <div class="card">
            <div class="gauge-container">
              <canvas id="gaugeCpu-${safeKey}"></canvas>
              <div class="gauge-value" id="cpu-value-${safeKey}">0%</div>
            </div>
            <div class="stat-label">CARGA CPU</div>
          </div>

          <div class="card">
            <div class="gauge-container">
              <canvas id="gaugeRam-${safeKey}"></canvas>
              <div class="gauge-value" id="ram-value-${safeKey}">0%</div>
            </div>
            <div class="stat-label">MEMÓRIA RAM</div>
          </div>

          <div class="card">
            <div class="gauge-container">
              <canvas id="gaugeDisk-${safeKey}"></canvas>
              <div class="gauge-value" id="disk-value-${safeKey}">0%</div>
            </div>
            <div class="stat-label">DISCO AMBIENTE</div>
          </div>

          <div class="card">
            <div class="gauge-container">
              <canvas id="gaugeTemp-${safeKey}"></canvas>
              <div class="gauge-value" id="temp-value-${safeKey}">0°C</div>
            </div>
            <div class="stat-label" style="color: #FF0055;">TEMPERATURA</div>
          </div>
        </div>

        <div class="row-bottom">
          <div class="card card-metrics">
            <div class="card-title" style="color: var(--accent-magenta);">MÉTRICAS EM TEMPO REAL</div>
            
            <div class="metrics-stack">
              <div class="metric-item">
                <div class="metric-header">
                  <span class="metric-label">Uso de Memória RAM</span>
                  <span class="sub-text" id="ram-sub-${safeKey}">0%</span>
                </div>
                <div class="spectrum-bar">
                  <div class="spectrum-fill" id="ram-bar-${safeKey}"></div>
                </div>
                <span class="sub-text" id="ram-total-label-${safeKey}" style="color: var(--accent-cyan); font-weight: 600; text-align: right; display: block;">Total: --</span>
              </div>

              <div class="metric-item">
                <div class="metric-header">
                  <span class="metric-label">Uso do Disco</span>
                  <span class="sub-text" id="disk-sub-${safeKey}">0%</span>
                </div>
                <div class="spectrum-bar">
                  <div class="spectrum-fill" id="disk-bar-${safeKey}" style="background: linear-gradient(90deg, var(--accent-green), var(--accent-yellow));"></div>
                </div>
                <span class="sub-text" id="disk-total-label-${safeKey}" style="color: var(--accent-cyan); font-weight: 600; text-align: right; display: block;">Total: --</span>
              </div>
            </div>

            <div class="metrics-footer">
              <span class="sub-text" id="host-label-${safeKey}" style="margin-top:0;">Host: -- | IP: -- | MAC: --</span>
              <div style="display: flex; flex-direction: column; align-items: flex-end;">
                <span class="sub-text" id="datetime-label-${safeKey}" style="margin-top:0;">Data: --</span>
                <span class="sub-text" id="uptime-label-${safeKey}" style="margin-top:0;">Uptime: --</span>
              </div>
            </div>
            <div style="margin-top: 5px;">
              <span class="sub-text" id="id-label-${safeKey}" style="margin-top:0;">ID: --</span>
            </div>
          </div>
        </div>
      `;

      document.getElementById('dashboard-container').appendChild(container);

      gaugeCharts[safeKey] = {
        cpuGauge: createGauge(`gaugeCpu-${safeKey}`, '#00f2fe'),
        ramGauge: createGauge(`gaugeRam-${safeKey}`, '#ff007f'),
        diskGauge: createGauge(`gaugeDisk-${safeKey}`, '#BD00FF'),
        tempGauge: createGauge(`gaugeTemp-${safeKey}`, '#FF0055')
      };
    }

    function atualizarDashboardItem(data) {
      if (!data) return;

      const hostName = data.server || data.host || data.id || 'Desconhecido';
      const safeKey = getSafeKey(hostName);

      if (!hostsMap[hostName]) {
        hostsMap[hostName] = true;
        createHostDOM(hostName, safeKey);
        addOptionToSelect(hostName);
      }

      // CPU
      const rawCpu = data.cpu_load !== undefined ? data.cpu_load : 
                    (data.metricas ? data.metricas.cpu_load_1m : 0);
      const cpuVal = Math.min(Math.round((rawCpu * 100) / 4), 100);

      // RAM
      const ramVal = data.ram_pct !== undefined ? Math.round(data.ram_pct) : 
                    (data.metricas && data.metricas.memoria ? Math.round(data.metricas.memoria.percentual) : 0);
                    
      // Disco
      const diskVal = data.disk_pct !== undefined ? Math.round(data.disk_pct) : 
                    (data.metricas && data.metricas.disco ? Math.round(data.metricas.disco.uso_percentual) : 0);

      // Temperatura
      const rawTemp = data.temp !== undefined ? data.temp : 
                     (data.metricas && data.metricas.cpu_temp !== undefined ? data.metricas.cpu_temp : 0);
      const tempVal = typeof rawTemp === 'number' ? Math.round(rawTemp) : 0;

      // Gauges
      const elCpu = document.getElementById(`cpu-value-${safeKey}`);
      const elRam = document.getElementById(`ram-value-${safeKey}`);
      const elDisk = document.getElementById(`disk-value-${safeKey}`);
      const elTemp = document.getElementById(`temp-value-${safeKey}`);
      
      if (elCpu) elCpu.textContent = cpuVal + '%';
      if (elRam) elRam.textContent = ramVal + '%';
      if (elDisk) elDisk.textContent = diskVal + '%';
      if (elTemp) elTemp.textContent = tempVal + '°C';

      const charts = gaugeCharts[safeKey];
      if (charts) {
        updateGauge(charts.cpuGauge, cpuVal);
        updateGauge(charts.ramGauge, ramVal);
        updateGauge(charts.diskGauge, diskVal);
        updateGauge(charts.tempGauge, Math.min(tempVal, 100));
      }

      // Sistema Operacional & Logo Background
      const osName = data.os_name || (data.sistema_operacional ? data.sistema_operacional.nome : '--');
      const osKernel = data.kernel || (data.sistema_operacional ? data.sistema_operacional.kernel : '--');
      const logoUrl = data.logo_url || (data.sistema_operacional ? data.sistema_operacional.logo_url : '');

      const elOsName = document.getElementById(`os-name-short-${safeKey}`);
      const elOsDetails = document.getElementById(`os-details-${safeKey}`);
      const elOsBgLogo = document.getElementById(`os-bg-logo-${safeKey}`);

      if (elOsName) elOsName.textContent = osName;
      if (elOsDetails) elOsDetails.textContent = 'Kernel: ' + osKernel;
      
      if (elOsBgLogo) {
        if (logoUrl && logoUrl.length > 0) {
          elOsBgLogo.style.backgroundImage = `url('${logoUrl}')`;
          elOsBgLogo.style.opacity = '0.15';
        } else {
          elOsBgLogo.style.backgroundImage = 'none';
        }
      }

      // Barras de Progresso e Sub-Textos
      const host = data.server || data.host || '--';
      const uptime = data.uptime || '--';
      const datetime = data.datetime || data.timestamp || '--';
      const ip = data.ip || '--';
      const mac = data.mac || '--';
      const id = data.id || '--';

      const elRamBar = document.getElementById(`ram-bar-${safeKey}`);
      const elDiskBar = document.getElementById(`disk-bar-${safeKey}`);
      const elRamSub = document.getElementById(`ram-sub-${safeKey}`);
      const elDiskSub = document.getElementById(`disk-sub-${safeKey}`);
      const elRamTotalLabel = document.getElementById(`ram-total-label-${safeKey}`);
      const elDiskTotalLabel = document.getElementById(`disk-total-label-${safeKey}`);
      const elHost = document.getElementById(`host-label-${safeKey}`);
      const elDatetime = document.getElementById(`datetime-label-${safeKey}`);
      const elUptime = document.getElementById(`uptime-label-${safeKey}`);
      const elId = document.getElementById(`id-label-${safeKey}`);

      if (elRamBar) elRamBar.style.width = ramVal + '%';
      if (elDiskBar) elDiskBar.style.width = diskVal + '%';

      if (elRamSub) elRamSub.textContent = ramVal + '%';
      if (elDiskSub) elDiskSub.textContent = diskVal + '%';

      const ramTotalMb = data.ram_total_mb !== undefined ? data.ram_total_mb :
                         (data.metricas && data.metricas.memoria && data.metricas.memoria.total_mb ? data.metricas.memoria.total_mb : 0);
      if (elRamTotalLabel) elRamTotalLabel.textContent = 'Total: ' + formatDynamicBytes(ramTotalMb, 'MB');

      const diskTotalGb = data.disk_total_gb !== undefined ? data.disk_total_gb : 
                          (data.metricas && data.metricas.disco && data.metricas.disco.total_gb !== undefined ? data.metricas.disco.total_gb : 0);
      if (elDiskTotalLabel) elDiskTotalLabel.textContent = 'Total: ' + formatDynamicBytes(diskTotalGb, 'GB');

      if (elHost) elHost.textContent = 'Host: ' + host + ' | IP: ' + ip + ' | MAC: ' + mac;
      if (elDatetime) elDatetime.textContent = 'Data: ' + datetime;
      if (elUptime) elUptime.textContent = 'Uptime: ' + uptime;
      if (elId) elId.textContent = 'ID: ' + id;

      updateHostVisibility();
    }

    function atualizarDashboard(data) {
      if (Array.isArray(data)) {
        // 1. Mapeia os nomes/IDs de todos os hosts presentes no JSON atual
        const hostsNoJson = new Set(
          data.map(item => item.server || item.host || item.id || 'Desconhecido')
        );

        // 2. Remove os cards dos hosts que não estão mais no JSON
        Object.keys(hostsMap).forEach(hostName => {
          if (!hostsNoJson.has(hostName)) {
            const safeKey = getSafeKey(hostName);
            const el = document.getElementById(`host-section-${safeKey}`);
            if (el) el.remove(); // Remove o card do DOM

            // Limpa o registro das variáveis globais
            delete hostsMap[hostName];
            delete gaugeCharts[safeKey];

            // Opcional: remove a opção do <select>
            const select = document.getElementById('hostSelect');
            Array.from(select.options).forEach(opt => {
              if (opt.value === hostName) opt.remove();
            });
          }
        });

        // 3. Atualiza ou cria os hosts ativos
        data.forEach(item => atualizarDashboardItem(item));
      } else if (data) {
        atualizarDashboardItem(data);
      }
    }

    function initWebSocket() {
      const gateway = `ws://${window.location.host}/ws`;
      websocket = new WebSocket(gateway);

      websocket.onopen = () => { 
        const statusEl = document.getElementById('status');
        if (statusEl) {
          statusEl.textContent = 'Online'; 
          statusEl.style.color = 'var(--accent-green)';
          statusEl.style.borderColor = 'var(--accent-green)';
          statusEl.style.backgroundColor = 'rgba(0, 255, 136, 0.15)';
        }
      };

      websocket.onclose = () => { 
        const statusEl = document.getElementById('status');
        if (statusEl) {
          statusEl.textContent = 'Conectando...'; 
          statusEl.style.color = 'var(--accent-yellow)';
          statusEl.style.borderColor = 'var(--accent-yellow)';
          statusEl.style.backgroundColor = 'rgba(255, 204, 0, 0.15)';
        }
        setTimeout(initWebSocket, 2000); 
      };

      websocket.onerror = (error) => {
        console.error("Erro no WebSocket:", error);
      };

      websocket.onmessage = (event) => {
        try {
          const data = JSON.parse(event.data);
          atualizarDashboard(data);
        } catch (e) {
          console.error("Erro de parse no JSON recebido:", e);
        }
      };
    }

    window.addEventListener('load', initWebSocket);

    // RELÓGIO (DATE & HOUR)
    const currentHourElement = document.getElementById('current_hour');
    const currentDateElement = document.getElementById('current_date');

    async function updateDateTime() {
        try {
            const response = await fetch('/datetime');
            if (response.ok) {
                const data = await response.json();
                currentHourElement.textContent = data.time;
                currentDateElement.textContent = data.date;
            } else {
                currentHourElement.textContent = "--:--:--";
                currentDateElement.textContent = "--/--/----";
            }
        } catch (error) {
            console.error("Erro de rede ao buscar data/hora:", error);
            currentHourElement.textContent = "";
            currentDateElement.innerHTML = '<span class="material-symbols-outlined icon">wifi_off</span>';
        }
    }
    setInterval(updateDateTime, 1000); 
    updateDateTime();

  </script>
</body>
</html>
)rawliteral";

#endif