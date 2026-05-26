// Configure Chart.js Theme for dark mode
Chart.defaults.color = '#94a3b8';
Chart.defaults.font.family = "'Inter', sans-serif";

const ctx = document.getElementById('airChart').getContext('2d');

// Chart data datasets
const chartData = {
    labels: [], 
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
            borderColor: 'rgba(239, 68, 68, 0.5)', 
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
                labels: {
                    boxWidth: 12,
                    font: {
                        size: 11
                    },
                    padding: 8
                }
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
                    text: 'PM2.5 Concentration (ug/m3)'
                }
            },
            x: {
                grid: {
                    color: 'rgba(255, 255, 255, 0.05)'
                },
                ticks: {
                    maxTicksLimit: 6, // Tránh chồng chéo nhãn thời gian trên mobile
                    autoSkip: true
                }
            }
        }
    }
});

// === STATE MANAGEMENT ===
const dataHistory = [];
let activeTab = 'pm25'; // 'pm25', 'weather', 'gas'

// Firebase elements from index.html
const db = window.firebaseDB;
const dbRef = window.firebaseRef;
const onValue = window.firebaseOnValue;

const modeSwitch = document.getElementById('mode-switch');
const statusBadge = document.getElementById('status-badge');
const statusText = document.getElementById('status-text');

let demoInterval = null;
let firebaseListenersActive = false;

// 1. Listen to Demo/Realtime toggle switch
modeSwitch.addEventListener('change', (e) => {
    const isDemo = e.target.checked;
    if (isDemo) {
        statusBadge.className = 'status-badge mode-demo';
        statusText.textContent = 'Demo Mode';
        console.log("Switched to OFFLINE DEMO MODE");
        startSimulation();
    } else {
        statusBadge.className = 'status-badge mode-realtime';
        statusText.textContent = 'Realtime Mode';
        console.log("Switched to ONLINE REALTIME FIREBASE MODE");
        stopSimulation();
        setupFirebaseListeners();
    }
});

// Start default mode
if (modeSwitch.checked) {
    startSimulation();
} else {
    setupFirebaseListeners();
}

// 2. Offline simulation (Demo Mode)
function startSimulation() {
    if (demoInterval) clearInterval(demoInterval);
    
    let ema_pm25 = -1;
    let ema_variance = 0;
    const EMA_ALPHA = 0.2;
    const ANOMALY_THRESHOLD_Z = 3.0;
    let anomalyTimer = null;

    function runSimulationStep() {
        const t = Date.now() / 1000;
        
        // Generate raw PM2.5 with occasional spikes
        let rawPm25 = 80.0 + 70.0 * Math.sin(t * 0.1);
        let anomalyDetected = false;
        
        if (Math.random() < 0.05) { // 5% chance of spike
            rawPm25 += 400.0 + Math.random() * 200.0;
        }

        // Z-Score Anomaly Filter (EMA)
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
                filteredPm25 = ema_pm25; // Filter the spike
            } else {
                ema_pm25 = (EMA_ALPHA * rawPm25) + ((1 - EMA_ALPHA) * ema_pm25);
                ema_variance = (EMA_ALPHA * diff * diff) + ((1 - EMA_ALPHA) * ema_variance);
            }
        }

        const temp = 27.0 + 5.0 * Math.sin(t * 0.05);
        const hum = 60.0 + 20.0 * Math.cos(t * 0.07);
        const gas = 1.0 + 0.5 * Math.sin(t * 0.08);
        const predictedPm25 = 80.0 + 70.0 * Math.sin((t + 5) * 0.1);

        handleNewData(rawPm25, filteredPm25, predictedPm25, temp, hum, gas, anomalyDetected);
    }

    demoInterval = setInterval(runSimulationStep, 1000);
    runSimulationStep();
}

function stopSimulation() {
    if (demoInterval) {
        clearInterval(demoInterval);
        demoInterval = null;
    }
}

// 3. Firebase listeners (Realtime Mode)
function setupFirebaseListeners() {
    if (firebaseListenersActive) return;
    if (!db || !dbRef || !onValue) {
        console.error("Firebase SDK not loaded properly!");
        return;
    }

    console.log("Firebase is initialized. Waiting for data...");
    const sensorRef = dbRef(db, 'sensor');
    const aiRef = dbRef(db, 'ai');

    const queryFn = window.firebaseQuery;
    const limitToLastFn = window.firebaseLimitToLast;
    const getFn = window.firebaseGet;

    // Load last 30 historical records from Firebase first
    if (queryFn && limitToLastFn && getFn) {
        const historyRef = queryFn(dbRef(db, 'history'), limitToLastFn(30));
        getFn(historyRef).then((snapshot) => {
            if (modeSwitch.checked) return; // Switched back to demo mode
            if (snapshot.exists()) {
                dataHistory.length = 0; // Clear simulated data
                const historyData = snapshot.val();
                
                // Sort keys by timestamp
                const sortedRecords = Object.values(historyData).sort((a, b) => a.timestamp - b.timestamp);
                
                sortedRecords.forEach(r => {
                    const date = new Date(r.timestamp);
                    const timeString = date.getHours().toString().padStart(2, '0') + ':' + 
                                       date.getMinutes().toString().padStart(2, '0') + ':' + 
                                       date.getSeconds().toString().padStart(2, '0');
                                       
                    dataHistory.push({
                        time: timeString,
                        raw_pm25: r.raw_pm25 || 0,
                        filtered_pm25: r.filtered_pm25 || 0,
                        predicted_pm25: r.predicted_pm25 || 0,
                        temp: r.temp || 25,
                        hum: r.hum || 50,
                        gas: r.gas || 1.0
                    });
                });
                
                repopulateChartData();
                updateTableLog();
            }
        }).catch(err => {
            console.error("Error loading Firebase history: ", err);
        });
    }

    let firebaseTemp = 25, firebaseHum = 50, firebaseGas = 1.0;
    let firebaseRawPm = 0, firebaseFilteredPm = 0, firebasePredictedPm = 0;

    onValue(sensorRef, (snapshot) => {
        if (modeSwitch.checked) return;
        const data = snapshot.val();
        if (data) {
            firebaseTemp = data.temp || 25;
            firebaseHum = data.hum || 50;
            firebaseGas = data.gas || 1.0;
            firebaseRawPm = data.raw_pm25 || 0;
        }
    });

    onValue(aiRef, (snapshot) => {
        if (modeSwitch.checked) return;
        const data = snapshot.val();
        if (data) {
            firebaseFilteredPm = data.filtered_pm25 || 0;
            firebasePredictedPm = data.predicted_pm25 || 0;

            // Detect anomaly based on raw vs filtered
            let anomaly = false;
            if (firebaseRawPm && firebaseFilteredPm && Math.abs(firebaseRawPm - firebaseFilteredPm) > 100) {
                anomaly = true;
            }

            handleNewData(firebaseRawPm, firebaseFilteredPm, firebasePredictedPm, firebaseTemp, firebaseHum, firebaseGas, anomaly);
        }
    });

    firebaseListenersActive = true;
}

// 4. Unified Data Handler
function handleNewData(rawPm, filteredPm, predictedPm, temp, hum, gas, anomaly) {
    const now = new Date();
    const timeString = now.getHours().toString().padStart(2, '0') + ':' + 
                       now.getMinutes().toString().padStart(2, '0') + ':' + 
                       now.getSeconds().toString().padStart(2, '0');
                       
    const record = {
        time: timeString,
        raw_pm25: rawPm,
        filtered_pm25: filteredPm,
        predicted_pm25: predictedPm,
        temp: temp,
        hum: hum,
        gas: gas
    };

    dataHistory.push(record);
    if (dataHistory.length > 30) {
        dataHistory.shift();
    }

    // A. Update DOM texts
    document.getElementById('val-pm25').textContent = filteredPm.toFixed(1);
    document.getElementById('val-temp').textContent = temp.toFixed(1);
    document.getElementById('val-hum').textContent = hum.toFixed(1);
    document.getElementById('val-pres').textContent = gas.toFixed(2); // ID "val-pres" reused for Gas V
    document.getElementById('val-predict').textContent = predictedPm.toFixed(1);

    // B. Anomaly Alert Badge
    const alertEl = document.getElementById('anomaly-alert');
    if (anomaly) {
        alertEl.classList.remove('hidden');
    } else if (!modeSwitch.checked) {
        alertEl.classList.add('hidden');
    }

    // C. Update SVG Radial Gauges
    updateGauge('gauge-pm25-path', filteredPm, 150);
    updateGauge('gauge-temp-path', temp, 50);
    updateGauge('gauge-hum-path', hum, 100);
    updateGauge('gauge-pres-path', gas, 5.0);

    // D. Update AI forecast color status
    const statusEl = document.getElementById('ai-status');
    statusEl.className = 'ai-status';
    if (predictedPm < 30) {
        statusEl.textContent = 'Good Air Quality';
        statusEl.classList.add('status-good');
    } else if (predictedPm < 80) {
        statusEl.textContent = 'Moderate - Watch out';
        statusEl.classList.add('status-warn');
    } else {
        statusEl.textContent = 'Warning: Severe Pollution!';
        statusEl.classList.add('status-bad');
    }

    // E. Redraw chart
    repopulateChartData();

    // F. Update History Log Table
    updateTableLog();
}

// 5. Update circular progress SVG
function updateGauge(elementId, value, maxVal) {
    const path = document.getElementById(elementId);
    if (!path) return;
    let percent = value / maxVal;
    if (percent > 1) percent = 1;
    if (percent < 0) percent = 0;
    const offset = 314 - (percent * 314);
    path.style.strokeDashoffset = offset;
}

// 6. Navigation and update for Chart Tabs
const tabs = {
    pm25: document.getElementById('tab-pm25'),
    weather: document.getElementById('tab-weather'),
    gas: document.getElementById('tab-gas')
};

function switchChartTab(tabId) {
    activeTab = tabId;
    
    Object.keys(tabs).forEach(k => {
        if (tabs[k]) tabs[k].classList.remove('active');
    });
    if (tabs[tabId]) tabs[tabId].classList.add('active');

    // Reconfigure Chart.js datasets
    if (tabId === 'pm25') {
        airChart.data.datasets[0].label = 'Filtered PM2.5 (Input to AI)';
        airChart.data.datasets[0].borderColor = '#38bdf8';
        airChart.data.datasets[0].backgroundColor = 'rgba(56, 189, 248, 0.1)';
        airChart.data.datasets[0].hidden = false;

        airChart.data.datasets[1].label = 'AI Forecast (1 hr ahead)';
        airChart.data.datasets[1].borderColor = '#a78bfa';
        airChart.data.datasets[1].borderDash = [5, 5];
        airChart.data.datasets[1].hidden = false;

        airChart.data.datasets[2].label = 'Raw Sensor Data (With Spikes)';
        airChart.data.datasets[2].borderColor = 'rgba(239, 68, 68, 0.5)';
        airChart.data.datasets[2].hidden = false;
        
        airChart.options.scales.y.title.text = 'PM2.5 Concentration (ug/m3)';
    } else if (tabId === 'weather') {
        airChart.data.datasets[0].label = 'Temperature (C)';
        airChart.data.datasets[0].borderColor = '#fb7185';
        airChart.data.datasets[0].backgroundColor = 'rgba(251, 113, 133, 0.1)';
        airChart.data.datasets[0].hidden = false;

        airChart.data.datasets[1].label = 'Humidity (%)';
        airChart.data.datasets[1].borderColor = '#38bdf8';
        airChart.data.datasets[1].borderDash = [];
        airChart.data.datasets[1].hidden = false;

        airChart.data.datasets[2].hidden = true;
        
        airChart.options.scales.y.title.text = 'Weather Measurements';
    } else if (tabId === 'gas') {
        airChart.data.datasets[0].label = 'Gas Voltage (MQ135)';
        airChart.data.datasets[0].borderColor = '#fbbf24';
        airChart.data.datasets[0].backgroundColor = 'rgba(251, 191, 36, 0.1)';
        airChart.data.datasets[0].hidden = false;

        airChart.data.datasets[1].hidden = true;
        airChart.data.datasets[2].hidden = true;
        
        airChart.options.scales.y.title.text = 'Voltage (V)';
    }

    repopulateChartData();
}

if (tabs.pm25) tabs.pm25.addEventListener('click', () => switchChartTab('pm25'));
if (tabs.weather) tabs.weather.addEventListener('click', () => switchChartTab('weather'));
if (tabs.gas) tabs.gas.addEventListener('click', () => switchChartTab('gas'));

function repopulateChartData() {
    airChart.data.labels = dataHistory.map(r => r.time);
    
    if (activeTab === 'pm25') {
        airChart.data.datasets[0].data = dataHistory.map(r => r.filtered_pm25);
        airChart.data.datasets[1].data = dataHistory.map(r => r.predicted_pm25);
        airChart.data.datasets[2].data = dataHistory.map(r => r.raw_pm25);
    } else if (activeTab === 'weather') {
        airChart.data.datasets[0].data = dataHistory.map(r => r.temp);
        airChart.data.datasets[1].data = dataHistory.map(r => r.hum);
    } else if (activeTab === 'gas') {
        airChart.data.datasets[0].data = dataHistory.map(r => r.gas);
    }
    airChart.update('none');
}

// 7. Update History Log Table
function updateTableLog() {
    const tbody = document.getElementById('history-table-body');
    if (!tbody) return;
    
    tbody.innerHTML = '';
    const lastTen = [...dataHistory].reverse().slice(0, 10);
    
    if (lastTen.length === 0) {
        tbody.innerHTML = `<tr><td colspan="8" style="text-align: center; color: var(--text-secondary); padding: 1.5rem;">Waiting for incoming data stream...</td></tr>`;
        return;
    }

    lastTen.forEach(r => {
        let statusClass = 'status-good';
        let statusText = 'Good';
        if (r.predicted_pm25 >= 80) {
            statusClass = 'status-bad';
            statusText = 'Unhealthy';
        } else if (r.predicted_pm25 >= 30) {
            statusClass = 'status-warn';
            statusText = 'Moderate';
        }

        const row = document.createElement('tr');
        row.innerHTML = `
            <td>${r.time}</td>
            <td>${r.raw_pm25.toFixed(1)}</td>
            <td>${r.filtered_pm25.toFixed(1)}</td>
            <td>${r.predicted_pm25.toFixed(1)}</td>
            <td>${r.temp.toFixed(1)}</td>
            <td>${r.hum.toFixed(0)}</td>
            <td>${r.gas.toFixed(2)}</td>
            <td><span class="status-pill ${statusClass}">${statusText}</span></td>
        `;
        tbody.appendChild(row);
    });
}

// 8. CSV Export
function exportHistoryToCSV() {
    if (dataHistory.length === 0) {
        alert("No data available to export yet!");
        return;
    }

    let csvContent = "data:text/csv;charset=utf-8,";
    csvContent += "Timestamp,Raw PM2.5 (ug/m3),Filtered PM2.5 (ug/m3),AI Forecast 1h (ug/m3),Temp (C),Hum (%),Gas Voltage (V)\n";

    dataHistory.forEach(r => {
        const row = [
            r.time,
            r.raw_pm25.toFixed(2),
            r.filtered_pm25.toFixed(2),
            r.predicted_pm25.toFixed(2),
            r.temp.toFixed(2),
            r.hum.toFixed(2),
            r.gas.toFixed(2)
        ].join(",");
        csvContent += row + "\n";
    });

    const encodedUri = encodeURI(csvContent);
    const link = document.createElement("a");
    link.setAttribute("href", encodedUri);
    
    const now = new Date();
    const dateStr = now.getFullYear() + 
                    (now.getMonth() + 1).toString().padStart(2, '0') + 
                    now.getDate().toString().padStart(2, '0') + "_" + 
                    now.getHours().toString().padStart(2, '0') + 
                    now.getMinutes().toString().padStart(2, '0');
    
    link.setAttribute("download", `AQI_AIoT_Report_${dateStr}.csv`);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
}

const btnCsv = document.getElementById('btn-csv-export');
if (btnCsv) btnCsv.addEventListener('click', exportHistoryToCSV);
