// Cấu hình Chart.js Theme cho nền tối
Chart.defaults.color = '#94a3b8';
Chart.defaults.font.family = "'Inter', sans-serif";

const ctx = document.getElementById('airChart').getContext('2d');

// Dữ liệu biểu đồ
const chartData = {
    labels: [], // Thời gian
    datasets: [
        {
            label: 'Filtered PM2.5 (Input to AI)',
            data: [],
            borderColor: '#38bdf8',
            backgroundColor: 'rgba(56, 189, 248, 0.1)',
            borderWidth: 3,
            tension: 0.4,
            fill: true
        },
        {
            label: 'AI Forecast (1 hr ahead)',
            data: [],
            borderColor: '#a78bfa',
            borderDash: [5, 5],
            borderWidth: 2,
            tension: 0.4
        },
        {
            label: 'Raw Sensor Data (With Spikes)',
            data: [],
            borderColor: 'rgba(239, 68, 68, 0.5)', // Red
            borderWidth: 1,
            borderDash: [2, 2],
            tension: 0.1
        }
    ]
};

const airChart = new Chart(ctx, {
    type: 'line',
    data: chartData,
    options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
            legend: {
                position: 'top',
            },
            tooltip: {
                mode: 'index',
                intersect: false,
                backgroundColor: 'rgba(15, 23, 42, 0.9)',
                titleColor: '#fff',
                bodyColor: '#cbd5e1',
                borderColor: 'rgba(255,255,255,0.1)',
                borderWidth: 1
            }
        },
        scales: {
            y: {
                beginAtZero: true,
                grid: {
                    color: 'rgba(255, 255, 255, 0.05)'
                },
                title: {
                    display: true,
                    text: 'PM2.5 Concentration (µg/m³)'
                }
            },
            x: {
                grid: {
                    color: 'rgba(255, 255, 255, 0.05)'
                }
            }
        }
    }
});

// Anomaly Detection State (Z-Score Based, matching C++)
let ema_pm25 = -1;
let ema_variance = 0;
const EMA_ALPHA = 0.2;
const ANOMALY_THRESHOLD_Z = 3.0;
let anomalyTimer = null;

// Giả lập dữ liệu theo sóng hình sin (Giống hệt code sensor_manager.cpp trên ESP32)
// Mục đích: Thể hiện Demo Dashboard cho tới khi Firebase được ráp nối
function updateDashboard() {
    const t = Date.now() / 1000; // Thời gian tính bằng giây
    
    // Sinh data ảo (Raw)
    let rawPm25 = 80.0 + 70.0 * Math.sin(t * 0.1);
    
    // Sinh nhiễu bất thường: 5% tỷ lệ
    let isSpike = false;
    if (Math.random() < 0.05) {
        rawPm25 += 400 + Math.random() * 200;
        isSpike = true;
    }
    
    // --- BỘ LỌC ANOMALY DETECTION (Z-Score) ---
    let filteredPm25 = rawPm25;
    let anomalyDetected = false;
    
    if (ema_pm25 < 0) {
        ema_pm25 = rawPm25;
    } else {
        let diff = rawPm25 - ema_pm25;
        let stddev = Math.sqrt(ema_variance);
        if (stddev < 10.0) stddev = 10.0;
        
        let z_score = Math.abs(diff) / stddev;
        
        if (z_score > ANOMALY_THRESHOLD_Z) {
            anomalyDetected = true;
            filteredPm25 = ema_pm25; // Block the noise!
            
            // Show Alert UI
            const alertEl = document.getElementById('anomaly-alert');
            alertEl.classList.remove('hidden');
            if(anomalyTimer) clearTimeout(anomalyTimer);
            anomalyTimer = setTimeout(() => { alertEl.classList.add('hidden'); }, 3000);
            
        } else {
            // Update Moving Average normally
            ema_pm25 = (EMA_ALPHA * rawPm25) + ((1 - EMA_ALPHA) * ema_pm25);
            ema_variance = (EMA_ALPHA * diff * diff) + ((1 - EMA_ALPHA) * ema_variance);
        }
    }
    // ------------------------------------------

    const temp = 27.0 + 5.0 * Math.sin(t * 0.05);
    const hum = 60.0 + 20.0 * Math.cos(t * 0.07);
    const pres = 1010.0 + 5.0 * Math.sin(t * 0.02);
    
    // Sinh data dự báo (AI Forecast is based on filtered data, shifted in time)
    const predictedPm25 = 80.0 + 70.0 * Math.sin((t + 5) * 0.1); 

    // Update Text DOM
    document.getElementById('val-pm25').textContent = filteredPm25.toFixed(1);
    document.getElementById('val-temp').textContent = temp.toFixed(1);
    document.getElementById('val-hum').textContent = hum.toFixed(1);
    document.getElementById('val-pres').textContent = pres.toFixed(1);
    
    // Cập nhật thẻ AI Predict
    const predictEl = document.getElementById('val-predict');
    const statusEl = document.getElementById('ai-status');
    predictEl.textContent = predictedPm25.toFixed(1);
    
    // Đổi màu cảnh báo AI
    statusEl.className = 'ai-status';
    if(predictedPm25 < 50) {
        statusEl.textContent = 'Good Air Quality';
        statusEl.classList.add('status-good');
    } else if(predictedPm25 < 100) {
        statusEl.textContent = 'Moderate - Watch out';
        statusEl.classList.add('status-warn');
    } else {
        statusEl.textContent = 'Warning: Severe Pollution Expected!';
        statusEl.classList.add('status-bad');
    }

    // Update Chart
    const now = new Date();
    const timeString = now.getHours().toString().padStart(2, '0') + ':' + 
                       now.getMinutes().toString().padStart(2, '0') + ':' + 
                       now.getSeconds().toString().padStart(2, '0');
                       
    chartData.labels.push(timeString);
    chartData.datasets[0].data.push(filteredPm25);
    chartData.datasets[1].data.push(predictedPm25);
    chartData.datasets[2].data.push(rawPm25);
    
    // Giữ cho biểu đồ không bị quá dài (Giới hạn 30 điểm cho dễ nhìn gai nhiễu)
    if(chartData.labels.length > 30) {
        chartData.labels.shift();
        chartData.datasets[0].data.shift();
        chartData.datasets[1].data.shift();
        chartData.datasets[2].data.shift();
    }
    
    airChart.update('none'); // Update không có animation để mượt hơn
}

// Chạy vòng lặp cập nhật mỗi 1 giây (1000ms)
setInterval(updateDashboard, 1000);
updateDashboard(); // Gọi ngay lập tức lần đầu
