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

// === CẬP NHẬT: KẾT NỐI FIREBASE HOẶC CHẠY DEMO OFFLINE ===
const DEMO_MODE = true; // CHUYỂN THÀNH true để chạy demo giả lập offline, false để kết nối Firebase thật

// Biến từ index.html
const db = window.firebaseDB;
const dbRef = window.firebaseRef;
const onValue = window.firebaseOnValue;

if (DEMO_MODE) {
    console.log("Running in OFFLINE DEMO MODE...");
    setupDemoSimulation();
} else if (db && dbRef && onValue) {
    console.log("Firebase is initialized. Waiting for data...");
    
    // Lắng nghe dữ liệu realtime từ Firebase
    const sensorRef = dbRef(db, 'sensor');
    const aiRef = dbRef(db, 'ai');

    onValue(sensorRef, (snapshot) => {
        const data = snapshot.val();
        if (data) {
            // Hiển thị text
            document.getElementById('val-temp').textContent = data.temp ? data.temp.toFixed(1) : '--';
            document.getElementById('val-hum').textContent = data.hum ? data.hum.toFixed(1) : '--';
            document.getElementById('val-pres').textContent = data.pres ? data.pres.toFixed(1) : '--';
            
            // Xử lý Anomaly Alert (Nếu raw khác quá xa filtered)
            // (Chỉ mang tính chất demo UI, logic thật đã xử lý trên ESP32)
            if (window.lastFilteredPm25 && Math.abs(data.raw_pm25 - window.lastFilteredPm25) > 100) {
                const alertEl = document.getElementById('anomaly-alert');
                alertEl.classList.remove('hidden');
                setTimeout(() => { alertEl.classList.add('hidden'); }, 3000);
            }

            window.lastRawPm25 = data.raw_pm25;
        }
    });

    onValue(aiRef, (snapshot) => {
        const data = snapshot.val();
        if (data) {
            document.getElementById('val-pm25').textContent = data.filtered_pm25 ? data.filtered_pm25.toFixed(1) : '--';
            document.getElementById('val-predict').textContent = data.predicted_pm25 ? data.predicted_pm25.toFixed(1) : '--';
            
            window.lastFilteredPm25 = data.filtered_pm25;
            window.lastPredictedPm25 = data.predicted_pm25;

            // Đổi màu cảnh báo
            const statusEl = document.getElementById('ai-status');
            const p = data.predicted_pm25;
            if (p < 50) {
                statusEl.textContent = 'Good';
                statusEl.style.color = '#34d399';
            } else if (p < 100) {
                statusEl.textContent = 'Moderate';
                statusEl.style.color = '#fbbf24';
            } else {
                statusEl.textContent = 'Unhealthy';
                statusEl.style.color = '#ef4444';
            }

            // Cập nhật biểu đồ (Mỗi khi có data mới từ AI)
            updateChartWithData(window.lastFilteredPm25, window.lastPredictedPm25, window.lastRawPm25);
        }
    });

} else {
    console.error("Firebase SDK not loaded properly! Running fallback demo...");
    setupDemoSimulation();
}

// --- HÀM GIẢ LẬP ĐỂ QUICK DEMO DASHBOARD OFFLINE ---
function setupDemoSimulation() {
    let ema_pm25 = -1;
    let ema_variance = 0;
    const EMA_ALPHA = 0.2;
    const ANOMALY_THRESHOLD_Z = 3.0;
    let anomalyTimer = null;

    function runSimulationStep() {
        const t = Date.now() / 1000;
        
        // 1. Tạo bụi thô (Raw PM2.5) kèm gai nhiễu ngẫu nhiên
        let rawPm25 = 80.0 + 70.0 * Math.sin(t * 0.1);
        let anomalyDetected = false;
        
        if (Math.random() < 0.05) { // 5% tỷ lệ xảy ra gai nhiễu
            rawPm25 += 400.0 + Math.random() * 200.0;
        }

        // 2. Chạy bộ lọc Z-Score (Y hệt C++ trên ESP32)
        let filteredPm25 = rawPm25;
        if (ema_pm25 < 0) {
            ema_pm25 = rawPm25;
        } else {
            let diff = rawPm25 - ema_pm25;
            let stddev = Math.sqrt(ema_variance);
            if (stddev < 10.0) stddev = 10.0;
            
            let z_score = Math.abs(diff) / stddev;
            if (z_score > ANOMALY_THRESHOLD_Z) {
                anomalyDetected = true;
                filteredPm25 = ema_pm25; // Chặn nhiễu!
                
                // Hiển thị badge cảnh báo
                const alertEl = document.getElementById('anomaly-alert');
                alertEl.classList.remove('hidden');
                if (anomalyTimer) clearTimeout(anomalyTimer);
                anomalyTimer = setTimeout(() => { alertEl.classList.add('hidden'); }, 3000);
            } else {
                ema_pm25 = (EMA_ALPHA * rawPm25) + ((1 - EMA_ALPHA) * ema_pm25);
                ema_variance = (EMA_ALPHA * diff * diff) + ((1 - EMA_ALPHA) * ema_variance);
            }
        }

        // 3. Giả lập các chỉ số môi trường khác
        const temp = 27.0 + 5.0 * Math.sin(t * 0.05);
        const hum = 60.0 + 20.0 * Math.cos(t * 0.07);
        const pres = 1010.0 + 5.0 * Math.sin(t * 0.02);
        
        // 4. Dự báo AI (Dựa trên bụi đã lọc và tịnh tiến thời gian)
        const predictedPm25 = 80.0 + 70.0 * Math.sin((t + 5) * 0.1);

        // 5. Cập nhật DOM Text
        document.getElementById('val-temp').textContent = temp.toFixed(1);
        document.getElementById('val-hum').textContent = hum.toFixed(1);
        document.getElementById('val-pres').textContent = pres.toFixed(1);
        document.getElementById('val-pm25').textContent = filteredPm25.toFixed(1);
        document.getElementById('val-predict').textContent = predictedPm25.toFixed(1);

        // 6. Đổi màu trạng thái AI
        const statusEl = document.getElementById('ai-status');
        if (predictedPm25 < 50) {
            statusEl.textContent = 'Good';
            statusEl.style.color = '#34d399';
        } else if (predictedPm25 < 100) {
            statusEl.textContent = 'Moderate';
            statusEl.style.color = '#fbbf24';
        } else {
            statusEl.textContent = 'Unhealthy';
            statusEl.style.color = '#ef4444';
        }

        // 7. Đẩy lên biểu đồ
        updateChartWithData(filteredPm25, predictedPm25, rawPm25);
    }

    // Chạy vòng lặp mỗi 1 giây
    setInterval(runSimulationStep, 1000);
    runSimulationStep();
}

function updateChartWithData(filtered, predicted, raw) {
    if (filtered === undefined || predicted === undefined || raw === undefined) return;

    const now = new Date();
    const timeString = now.getHours().toString().padStart(2, '0') + ':' + 
                       now.getMinutes().toString().padStart(2, '0') + ':' + 
                       now.getSeconds().toString().padStart(2, '0');
                       
    chartData.labels.push(timeString);
    chartData.datasets[0].data.push(filtered);
    chartData.datasets[1].data.push(predicted);
    chartData.datasets[2].data.push(raw);
    
    // Giữ cho biểu đồ không bị quá dài (Giới hạn 30 điểm)
    if(chartData.labels.length > 30) {
        chartData.labels.shift();
        chartData.datasets[0].data.shift();
        chartData.datasets[1].data.shift();
        chartData.datasets[2].data.shift();
    }
    
    airChart.update('none'); // Update không có animation để mượt hơn
}
