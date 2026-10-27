#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Khung Ảnh & Màn Hình Chờ ESP32</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; touch-action: manipulation; }
    body { background: #0b1120; color: #f8fafc; padding: 12px; display: flex; flex-direction: column; align-items: center; min-height: 100vh; }
    .card { background: #1e293b; border-radius: 16px; padding: 16px; width: 100%; max-width: 400px; box-shadow: 0 10px 30px rgba(0,0,0,0.6); border: 1px solid #334155; }
    h1 { font-size: 19px; font-weight: 700; text-align: center; color: #38bdf8; margin-bottom: 2px; }
    p.sub { font-size: 12px; text-align: center; color: #94a3b8; margin-bottom: 12px; }
    
    .tabs { display: flex; gap: 6px; margin-bottom: 14px; background: #0f172a; padding: 4px; border-radius: 10px; }
    .tab-btn { flex: 1; padding: 10px 4px; font-size: 13px; font-weight: 600; border: none; border-radius: 8px; background: transparent; color: #94a3b8; cursor: pointer; text-align: center; transition: 0.2s; }
    .tab-btn.active { background: #3b82f6; color: white; box-shadow: 0 2px 8px rgba(59,130,246,0.4); }
    
    .tab-pane { display: none; }
    .tab-pane.active { display: block; }

    .upload-btn-wrapper { position: relative; overflow: hidden; display: block; margin-bottom: 10px; }
    .btn-pick { background: #2563eb; color: white; padding: 12px 16px; border-radius: 10px; font-size: 14px; font-weight: 600; text-align: center; display: block; cursor: pointer; border: none; width: 100%; }
    .upload-btn-wrapper input[type=file] { position: absolute; left: 0; top: 0; opacity: 0; width: 100%; height: 100%; cursor: pointer; }

    .preview-box { width: 240px; height: 320px; margin: 0 auto 12px auto; border: 2px dashed #475569; border-radius: 10px; overflow: hidden; background: #000; position: relative; display: flex; align-items: center; justify-content: center; touch-action: none; }
    canvas { width: 100%; height: 100%; object-fit: contain; cursor: grab; }
    canvas:active { cursor: grabbing; }
    .placeholder { color: #64748b; font-size: 13px; text-align: center; padding: 20px; }

    .slider-row { display: flex; align-items: center; justify-content: space-between; margin-bottom: 10px; font-size: 13px; color: #cbd5e1; }
    .slider-row input[type=range] { flex: 1; margin-left: 10px; }

    .controls { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-bottom: 12px; }
    .btn-sec { background: #334155; color: #e2e8f0; border: none; padding: 9px; border-radius: 8px; font-size: 12px; font-weight: 500; cursor: pointer; text-align: center; }
    .btn-sec:active { background: #475569; }

    .btn-send { background: linear-gradient(135deg, #10b981, #059669); color: white; padding: 14px; border-radius: 10px; font-size: 15px; font-weight: 700; width: 100%; border: none; cursor: pointer; box-shadow: 0 4px 15px rgba(16, 185, 129, 0.4); display: none; margin-bottom: 8px; }
    .btn-send:disabled { background: #64748b; cursor: not-allowed; box-shadow: none; }

    .btn-save-cfg { background: linear-gradient(135deg, #3b82f6, #1d4ed8); color: white; padding: 13px; border-radius: 10px; font-size: 14px; font-weight: 700; width: 100%; border: none; cursor: pointer; box-shadow: 0 4px 15px rgba(59, 130, 246, 0.4); margin-top: 14px; }
    .btn-color { background: #4f46e5; color: white; border: none; padding: 10px; border-radius: 8px; font-size: 12px; font-weight: 600; cursor: pointer; width: 100%; margin-top: 8px; }

    .form-group { margin-bottom: 14px; }
    .form-group label { display: block; font-size: 13px; font-weight: 600; color: #38bdf8; margin-bottom: 6px; }
    .form-group select { width: 100%; padding: 10px; background: #0f172a; color: #f8fafc; border: 1px solid #334155; border-radius: 8px; font-size: 13px; outline: none; }
    .tip-box { background: #0f172a; border-left: 3px solid #38bdf8; padding: 10px; border-radius: 6px; font-size: 12px; color: #94a3b8; line-height: 1.4; margin-top: 10px; }

    .progress-bar { width: 100%; height: 6px; background: #334155; border-radius: 3px; overflow: hidden; margin-top: 10px; display: none; }
    .progress-fill { width: 0%; height: 100%; background: #10b981; transition: width 0.2s; }
    .status-msg { font-size: 12px; text-align: center; margin-top: 10px; color: #38bdf8; min-height: 18px; }
  </style>
</head>
<body>
  <div class="card">
    <h1>Khung Ảnh & Màn Hình Chờ</h1>
    <p class="sub">Tự động vào màn hình chờ sau 30s không chạm</p>

    <!-- TAB HEADERS -->
    <div class="tabs">
      <button class="tab-btn active" id="tabBtn1" onclick="switchTab('main')">🖼️ Ảnh Chính</button>
      <button class="tab-btn" id="tabBtn2" onclick="switchTab('standby')">🕒 Màn Hình Chờ</button>
    </div>

    <!-- TAB 1: ẢNH CHÍNH -->
    <div class="tab-pane active" id="paneMain">
      <div class="upload-btn-wrapper">
        <button class="btn-pick">📸 Chọn ảnh chính từ điện thoại</button>
        <input type="file" id="fileInputMain" accept="image/*">
      </div>

      <div class="preview-box">
        <div class="placeholder" id="phMain">Chưa chọn ảnh chính</div>
        <canvas id="canvasMain" width="240" height="320" style="display:none;"></canvas>
      </div>

      <div id="controlsMain" style="display:none;">
        <div class="slider-row">
          <span>Thu phóng:</span>
          <input type="range" id="zoomMain" min="10" max="30" value="10" oninput="updateZoom('main', this.value)">
        </div>
        <div class="controls">
          <button class="btn-sec" onclick="rotateImg('main')">🔄 Xoay 90°</button>
          <button class="btn-sec" onclick="toggleFit('main')">📐 Đổi Fit (Lấp/Vừa)</button>
        </div>
        <button class="btn-send" id="sendBtnMain" onclick="uploadImg('main')">🚀 Gửi ảnh chính lên màn hình</button>
      </div>
    </div>

    <!-- TAB 2: CÀI ĐẶT MÀN HÌNH CHỜ (AOD) -->
    <div class="tab-pane" id="paneStandby">
      <div class="form-group">
        <label>⏱️ Thời gian chờ tự tắt / vào Standby:</label>
        <select id="cfgTimeout">
          <option value="10">10 giây (Test nhanh)</option>
          <option value="15">15 giây</option>
          <option value="30" selected>30 giây (Mặc định)</option>
          <option value="60">60 giây (1 phút)</option>
          <option value="0">Tắt (Luôn hiện ảnh chính)</option>
        </select>
      </div>

      <div class="form-group">
        <label>✨ Kiểu Màn Hình Chờ (Standby Mode):</label>
        <select id="cfgMode" onchange="onModeChange()">
          <option value="clock">🕒 Đồng hồ số AOD (Hiển thị giờ + ngày)</option>
          <option value="photo">🖼️ Ảnh chờ phụ (Screensaver riêng)</option>
          <option value="sleep">🌙 Tắt màn hình (Tiết kiệm pin, chạm là bật)</option>
        </select>
      </div>

      <!-- KHU VỰC CÀI ẢNH CHỜ PHỤ (KHI CHỌN PHOTO) -->
      <div id="standbyPhotoBox" style="display:none; margin-top:12px; border-top:1px solid #334155; padding-top:12px;">
        <p style="font-size:12px; color:#38bdf8; font-weight:600; margin-bottom:8px;">Chọn ảnh hiển thị khi ở chế độ chờ:</p>
        <div class="upload-btn-wrapper">
          <button class="btn-pick" style="background:#0891b2;">🖼️ Chọn ảnh chờ phụ</button>
          <input type="file" id="fileInputSb" accept="image/*">
        </div>
        <div class="preview-box">
          <div class="placeholder" id="phSb">Chưa chọn ảnh chờ</div>
          <canvas id="canvasSb" width="240" height="320" style="display:none;"></canvas>
        </div>
        <div id="controlsSb" style="display:none;">
          <div class="slider-row">
            <span>Thu phóng:</span>
            <input type="range" id="zoomSb" min="10" max="30" value="10" oninput="updateZoom('sb', this.value)">
          </div>
          <div class="controls">
            <button class="btn-sec" onclick="rotateImg('sb')">🔄 Xoay 90°</button>
            <button class="btn-sec" onclick="toggleFit('sb')">📐 Đổi Fit (Lấp/Vừa)</button>
          </div>
          <button class="btn-send" id="sendBtnSb" onclick="uploadImg('sb')">🚀 Gửi ảnh chờ lên ESP32</button>
        </div>
      </div>

      <div class="tip-box">
        💡 <b>Cách hoạt động:</b><br>
        - Khi không có người chạm trong 30s, ESP32 sẽ tự chuyển sang màn hình chờ.<br>
        - Chạm vào màn hình bất kỳ lúc nào để quay lại ảnh chính!
      </div>

      <button class="btn-save-cfg" onclick="saveConfig()">💾 Lưu Cài Đặt Màn Hình Chờ</button>
    </div>

    <!-- THANH TIẾN TRÌNH & THÔNG BÁO -->
    <div class="progress-bar" id="progressBar">
      <div class="progress-fill" id="progressFill"></div>
    </div>
    <div class="status-msg" id="statusMsg"></div>

    <button class="btn-color" onclick="toggleColorOrder()">🎨 Đổi hệ màu RGB / BGR</button>
  </div>

  <script>
    const state = {
      main: { img: new Image(), rot: 0, fit: 'cover', zoom: 1.0, panX: 0, panY: 0, isDrag: false, sX: 0, sY: 0 },
      sb:   { img: new Image(), rot: 0, fit: 'cover', zoom: 1.0, panX: 0, panY: 0, isDrag: false, sX: 0, sY: 0 }
    };

    const TARGET_W = 240;
    const TARGET_H = 320;
    const statusMsg = document.getElementById('statusMsg');

    const switchTab = (tab) => {
      document.getElementById('tabBtn1').classList.toggle('active', tab === 'main');
      document.getElementById('tabBtn2').classList.toggle('active', tab === 'standby');
      document.getElementById('paneMain').classList.toggle('active', tab === 'main');
      document.getElementById('paneStandby').classList.toggle('active', tab === 'standby');
    };

    const onModeChange = () => {
      const mode = document.getElementById('cfgMode').value;
      document.getElementById('standbyPhotoBox').style.display = (mode === 'photo') ? 'block' : 'none';
    };

    const setupUploader = (key, inputId, phId, canvasId, ctrlId, sendId) => {
      const s = state[key];
      const canvas = document.getElementById(canvasId);
      const ctx = canvas.getContext('2d');

      document.getElementById(inputId).addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = (evt) => {
          s.img.onload = () => {
            s.rot = 0; s.zoom = 1.0; s.panX = 0; s.panY = 0;
            document.getElementById(phId).style.display = 'none';
            canvas.style.display = 'block';
            document.getElementById(ctrlId).style.display = 'block';
            document.getElementById(sendId).style.display = 'block';
            draw(key);
          };
          s.img.src = evt.target.result;
        };
        reader.readAsDataURL(file);
      });

      const onStart = (cx, cy) => { s.isDrag = true; s.sX = cx - s.panX; s.sY = cy - s.panY; };
      const onMove  = (cx, cy) => { if (!s.isDrag) return; s.panX = cx - s.sX; s.panY = cy - s.sY; draw(key); };
      const onEnd   = () => { s.isDrag = false; };

      canvas.addEventListener('mousedown', (e) => onStart(e.clientX, e.clientY));
      window.addEventListener('mousemove', (e) => onMove(e.clientX, e.clientY));
      window.addEventListener('mouseup', onEnd);

      canvas.addEventListener('touchstart', (e) => { if (e.touches.length === 1) onStart(e.touches[0].clientX, e.touches[0].clientY); });
      window.addEventListener('touchmove', (e) => { if (e.touches.length === 1) onMove(e.touches[0].clientX, e.touches[0].clientY); });
      window.addEventListener('touchend', onEnd);
    };

    const draw = (key) => {
      const s = state[key];
      const canvas = document.getElementById(key === 'main' ? 'canvasMain' : 'canvasSb');
      const ctx = canvas.getContext('2d');

      ctx.fillStyle = '#000000';
      ctx.fillRect(0, 0, TARGET_W, TARGET_H);

      ctx.save();
      ctx.translate(TARGET_W / 2 + s.panX, TARGET_H / 2 + s.panY);
      ctx.rotate((s.rot * Math.PI) / 180);

      let w = s.img.width;
      let h = s.img.height;
      if (s.rot === 90 || s.rot === 270) { const tmp = w; w = h; h = tmp; }

      let baseScale = (s.fit === 'cover') ? Math.max(TARGET_W / w, TARGET_H / h) : Math.min(TARGET_W / w, TARGET_H / h);
      const finalScale = baseScale * s.zoom;

      ctx.drawImage(s.img, -s.img.width * finalScale / 2, -s.img.height * finalScale / 2, s.img.width * finalScale, s.img.height * finalScale);
      ctx.restore();
    };

    const updateZoom = (key, val) => { state[key].zoom = val / 10; draw(key); };
    const rotateImg  = (key) => { state[key].rot = (state[key].rot + 90) % 360; state[key].panX = 0; state[key].panY = 0; draw(key); };
    const toggleFit  = (key) => { state[key].fit = (state[key].fit === 'cover') ? 'contain' : 'cover'; draw(key); };

    const uploadImg = (key) => {
      const isMain = (key === 'main');
      const sendBtn = document.getElementById(isMain ? 'sendBtnMain' : 'sendBtnSb');
      const canvas = document.getElementById(isMain ? 'canvasMain' : 'canvasSb');
      const endpoint = isMain ? '/upload' : '/upload_standby';

      const pBar = document.getElementById('progressBar');
      const pFill = document.getElementById('progressFill');

      sendBtn.disabled = true;
      pBar.style.display = 'block';
      pFill.style.width = '20%';
      statusMsg.innerText = isMain ? 'Đang gửi ảnh chính...' : 'Đang gửi ảnh chờ...';

      canvas.toBlob((blob) => {
        const formData = new FormData();
        formData.append('image', blob, isMain ? 'photo.jpg' : 'standby.jpg');

        const xhr = new XMLHttpRequest();
        xhr.open('POST', endpoint, true);

        xhr.upload.onprogress = (e) => {
          if (e.lengthComputable) {
            let p = Math.round((e.loaded / e.total) * 100);
            pFill.style.width = p + '%';
          }
        };

        xhr.onload = () => {
          sendBtn.disabled = false;
          if (xhr.status === 200) {
            pFill.style.width = '100%';
            statusMsg.style.color = '#10b981';
            statusMsg.innerText = isMain ? '✅ Đã tải xong ảnh chính!' : '✅ Đã tải xong ảnh chờ!';
          } else {
            statusMsg.style.color = '#ef4444';
            statusMsg.innerText = '❌ Lỗi gửi ảnh: ' + xhr.status;
          }
        };

        xhr.onerror = () => {
          sendBtn.disabled = false;
          statusMsg.style.color = '#ef4444';
          statusMsg.innerText = '❌ Mất kết nối tới ESP32!';
        };

        xhr.send(formData);
      }, 'image/jpeg', 0.9);
    };

    const saveConfig = () => {
      const timeout = parseInt(document.getElementById('cfgTimeout').value, 10);
      const mode = document.getElementById('cfgMode').value;
      statusMsg.style.color = '#38bdf8';
      statusMsg.innerText = 'Đang lưu cấu hình...';

      fetch('/save_config?timeout=' + timeout + '&mode=' + mode)
        .then(r => r.text())
        .then(res => {
          statusMsg.style.color = '#10b981';
          statusMsg.innerText = '✅ ' + res;
        })
        .catch(() => {
          statusMsg.style.color = '#ef4444';
          statusMsg.innerText = '❌ Lỗi khi lưu cấu hình!';
        });
    };

    const toggleColorOrder = () => {
      fetch('/toggle_color')
        .then(r => r.text())
        .then(txt => {
          statusMsg.style.color = '#38bdf8';
          statusMsg.innerText = 'Hệ màu: ' + txt;
        });
    };

    // Khởi tạo uploader cho cả 2 tab
    setupUploader('main', 'fileInputMain', 'phMain', 'canvasMain', 'controlsMain', 'sendBtnMain');
    setupUploader('sb', 'fileInputSb', 'phSb', 'canvasSb', 'controlsSb', 'sendBtnSb');

    // Tự động đồng bộ thời gian từ điện thoại và đọc cấu hình hiện tại
    window.addEventListener('DOMContentLoaded', () => {
      const epoch = Math.floor(Date.now() / 1000);
      fetch('/sync_time?epoch=' + epoch).catch(() => {});

      fetch('/get_config')
        .then(r => r.json())
        .then(cfg => {
          if (cfg.timeout !== undefined) document.getElementById('cfgTimeout').value = cfg.timeout;
          if (cfg.mode) {
            document.getElementById('cfgMode').value = cfg.mode;
            onModeChange();
          }
        })
        .catch(() => {});
    });
  </script>
</body>
</html>
)rawliteral";

#endif
