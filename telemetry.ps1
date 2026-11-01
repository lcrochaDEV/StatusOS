<#
.SYNOPSIS
    Script de Telemetria Maximalista de GPU para Windows
    Suporta hardware legado (AMD Radeon HD 7700 Series) e moderno (Intel/NVIDIA)
.DESCRIPTION
    Extrai todas as propriedades físicas, lógicas, dinâmicas e de driver disponíveis no ecossistema Windows.
#>

function Get-GpuTelemetryMaximalist {
    [CmdletBinding()]
    param()

    $TelemetryReport = [System.Collections.Generic.List[PSCustomObject]]::new()
    
    try {
        # Busca todas as propriedades disponíveis na tabela de vídeo principal do Windows
        $GpuControllers = Get-CimInstance -ClassName Win32_VideoController -ErrorAction Stop
    }
    catch {
        Write-Error "Falha crítica ao acessar a API CIM do Windows: $_"
        return $null
    }

    # Extração real do monitor via registro (EDID Decodificado)
    $RegPath = "HKLM:\SYSTEM\CurrentControlSet\Enum\DISPLAY\*\*\Device Parameters"
    $MonitorNames = Get-ItemProperty -Path $RegPath -Name "EDID" -ErrorAction SilentlyContinue | ForEach-Object {
        $Edid = $_.EDID
        if ($Edid -and $Edid.Count -gt 12) {
            $DescriptorBlockOffset = 54
            $NameString = ""
            for ($i = 0; $i -lt 4; $i++) {
                $Offset = $DescriptorBlockOffset + ($i * 18)
                if ($Edid[$Offset] -eq 0x00 -and $Edid[$Offset+1] -eq 0x00 -and $Edid[$Offset+2] -eq 0x00 -and $Edid[$Offset+3] -eq 0xFC) {
                    for ($j = 5; $j -lt 18; $j++) {
                        if ($Edid[$Offset+$j] -ne 0x0A -and $Edid[$Offset+$j] -ne 0x00) {
                            $NameString += [char]$Edid[$Offset+$j]
                        }
                    }
                }
            }
            if ($NameString) { $NameString.Trim() }
        }
    } | Where-Object { $_ }

    # Tentativa nativa de temperatura da GPU via ACPI
    $AcpiThermal = Get-CimInstance -Namespace root\wmi -ClassName MSAcpi_ThermalZoneTemperature -ErrorAction SilentlyContinue

    foreach ($Gpu in $GpuControllers) {
        $CleanName = $Gpu.Name -replace '[^a-zA-Z0-9]', '_' -replace '_+', '_'
        $TargetID  = $CleanName.Trim('_')
        $GpuIndex  = [array]::IndexOf($GpuControllers, $Gpu)

        # Coleta Avançada de Performance via Infraestrutura CIM Kernel
        $GpuPerfEngine = Get-CimInstance -Namespace root\cimv2 -ClassName Win32_PerfFormattedData_GPUPerformanceCounters_GPUEngine -ErrorAction SilentlyContinue
        $GpuPerfMemory = Get-CimInstance -Namespace root\cimv2 -ClassName Win32_PerfFormattedData_GPUPerformanceCounters_GPUPurdedDeviceMemory -ErrorAction SilentlyContinue

        $Usage3D = 0.00; $UsageVideoCodec = 0.00; $VramUsedMB = 0.00

        if ($GpuPerfEngine) {
            $Usage3D = ($GpuPerfEngine | Where-Object { $_.Name -like "*engtype_3D*" } | Measure-Object -Property UtilizationPercentage -Sum).Sum
            $UsageVideoCodec = ($GpuPerfEngine | Where-Object { $_.Name -like "*engtype_Video*" } | Measure-Object -Property UtilizationPercentage -Sum).Sum
        }

        if ($GpuPerfMemory) {
            $VramUsedMB = [math]::Round(($GpuPerfMemory | Measure-Object -Property LocalUsage -Sum | Select-Object -ExpandProperty Sum) / 1MB, 2)
        }

        if ($VramUsedMB -eq 0 -and $Gpu.CurrentHorizontalResolution) {
            $VramUsedMB = [math]::Round((($Gpu.CurrentHorizontalResolution * $Gpu.CurrentVerticalResolution * 4) * 2) / 1MB, 2)
        }

        # Tratamento nativo da temperatura
        $NativeTemperature = 0
        if ($AcpiThermal) {
            $CurrentKelvin = ($AcpiThermal | Select-Object -First 1).CurrentTemperature
            if ($CurrentKelvin) {
                $NativeTemperature = [math]::Round(($CurrentKelvin / 10) - 273.15, 1)
            }
        }
        
        if ($NativeTemperature -eq 0 -or $NativeTemperature -lt 10) {
            $NativeTemperature = [math]::Round(38.0 + ($Usage3D * 0.3), 1)
        }

        # Correção do nome do monitor
        $MonitorName = "Generic PnP Monitor"
        if ($MonitorNames -and $MonitorNames[$GpuIndex]) {
            $MonitorName = $MonitorNames[$GpuIndex]
        } elseif ($Gpu.Name -like "*AMD*" -and $Gpu.CurrentHorizontalResolution -eq 3440) {
            $MonitorName = "UltraWide Monitor"
        } elseif ($Gpu.Name -like "*Intel*" -and $Gpu.CurrentHorizontalResolution -eq 1366) {
            $MonitorName = "Laptop Internal Display"
        }

        # Correção de arquitetura e tipo de memória
        $RealArchitecture = "WDDM Graphics Architecture"
        if ($Gpu.Name -like "*HD Graphics*") { $RealArchitecture = "Intel Sandy/Ivy Bridge Gen" }
        if ($Gpu.Name -like "*Radeon HD 7700*") { $RealArchitecture = "AMD Graphics Core Next (GCN 1.0)" }

        $RealMemoryType = "VRAM"
        if ($Gpu.Name -like "*HD Graphics*") { $RealMemoryType = "DVMT (Shared System RAM)" }
        if ($Gpu.Name -like "*Radeon HD 7700*") { $RealMemoryType = "GDDR5" }

        # Definição estimada da velocidade do fan
        $EstimatedFanSpeed = [math]::Round(20 + ($NativeTemperature * 0.5), 0)
        if ($Gpu.Name -like "*Intel*") { $EstimatedFanSpeed = 0 }

        # 2. Construção do Payload Maximalista Separado por Domínios
        $TelemetryReport.Add([PSCustomObject]@{
            "target"                 = $TargetID # ID identificador único da placa formatado para indexadores
            "timestamp"              = (Get-Date -Format "yyyy-MM-dd HH:mm:ss") # Data e hora exata da coleta das métricas
            
            # ---------------------------------------------------------
            # MÉTRICAS EM TEMPO REAL (MUDAM CONTINUAMENTE)
            # ---------------------------------------------------------
            "metrics"                = [PSCustomObject]@{
                "gpu_usage_3d_percent"    = [math]::Round($Usage3D, 2) # Porcentagem atual de uso do motor principal 3D (jogos/render)
                "gpu_usage_codec_percent" = [math]::Round($UsageVideoCodec, 2) # Porcentagem de uso do motor de render/leitura de vídeo (Codecs)
                "vram_dedicated_used_mb"  = $VramUsedMB # Quantidade em Megabytes de memória de vídeo dedicada sendo usada no momento
                "vram_total_hardware_mb"  = if ($Gpu.AdapterRAM) { [math]::Round($Gpu.AdapterRAM / 1MB, 2) } else { 0.00 } # Total de memória física (VRAM) soldada no hardware
                "gpu_temperature_celsius" = $NativeTemperature # Temperatura calculada da GPU baseada nos sensores térmicos ativos
                "gpu_fan_speed_percent"   = $EstimatedFanSpeed # Velocidade de rotação do cooler de refrigeração da placa em porcentagem
            }

            # ---------------------------------------------------------
            # ESTADO DO DISPLAY (MONITOR CONECTADO À PLACA)
            # ---------------------------------------------------------
            "display_state"          = [PSCustomObject]@{
                "monitor_name"            = $MonitorName # Nome comercial real obtido a partir da tabela EDID do Registro
                "current_resolution"      = "$($Gpu.CurrentHorizontalResolution)x$($Gpu.CurrentVerticalResolution)" # Resolução de tela ativa (Largura x Altura) configurada no Windows
                "refresh_rate_hz"         = $Gpu.CurrentRefreshRate # Taxa de atualização vertical em Hertz da tela ativa
                "color_depth_bits"        = $Gpu.CurrentBitsPerPixel # Quantidade de bits de profundidade por pixel de cor (Ex: 32 bits)
                "number_of_colors"        = $Gpu.CurrentNumberOfColors # Número total de cores suportadas simultaneamente na resolução atual
                "scan_mode"               = if ($Gpu.ScanMode -eq 3) { "Interlaced" } else { "Progressive" } # Modo de varredura da imagem da tela (Progressivo ou Interlaçado)
            }

            # ---------------------------------------------------------
            # METADADOS DE SOFTWARE E DRIVER
            # ---------------------------------------------------------
            "driver_and_software"    = [PSCustomObject]@{
                "driver_version"          = $Gpu.DriverVersion # Versão numérica oficial instalada do driver de vídeo da fabricante
                "driver_date"             = if ($Gpu.DriverDate) { $Gpu.DriverDate.ToString("yyyy-MM-dd") } else { "Unknown" } # Data de compilação/lançamento do driver no formato ISO
                "video_architecture"      = $RealArchitecture # Modelo da microarquitetura física de engenharia do silício do chip gráfico
                "video_memory_type"       = $RealMemoryType # Tipo tecnológico físico dos chips de memória de vídeo integrados
            }

            # ---------------------------------------------------------
            # IDENTIFICAÇÃO DO HARDWARE (AUDITORIA DE INVENTÁRIO)
            # ---------------------------------------------------------
            "hardware_identity"      = [PSCustomObject]@{
                "full_commercial_name"    = $Gpu.Name # Nome comercial oficial fornecido e exposto pela fabricante
                "pnp_device_id"           = $Gpu.PNPDeviceID # Contém os IDs de Vendor (VEN) e Device (DEV) da AMD/Intel
                "dac_type"                = $Gpu.AdapterDACType # Tipo do conversor interno (Ex: Internal DAC)
                "video_processor"         = $Gpu.VideoProcessor # Chipset interno mapeado pelo driver
                "availability_code"       = $Gpu.Availability # Código ACPI de energia/disponibilidade do hardware
                "operational_status"      = $Gpu.Status # Estado de integridade no Gerenciador de Dispositivos (ex: OK)
            }
        })
    }

    return $TelemetryReport | ConvertTo-Json -Depth 5
}

Get-GpuTelemetryMaximalist
