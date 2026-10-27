#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Điện Thoại Thu Nhỏ ESP32</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; touch-action: manipulation; -webkit-tap-highlight-color: transparent; }
    body { background: #0b1120; color: #f8fafc; padding: 10px; display: flex; flex-direction: column; align-items: center; min-height: 100vh; }
    .card { background: #1e293b; border-radius: 16px; padding: 14px; width: 100%; max-width: 400px; box-shadow: 0 10px 30px rgba(0,0,0,0.6); border: 1px solid #334155; }
    h1 { font-size: 17px; font-weight: 700; text-align: center; color: #38bdf8; margin-bottom: 2px; }
    p.sub { font-size: 11px; text-align: center; color: #94a3b8; margin-bottom: 10px; }
    
    .tabs { display: grid; grid-template-columns: repeat(4, 1fr); gap: 4px; margin-bottom: 12px; background: #0f172a; padding: 4px; border-radius: 10px; }
    .tab-btn { padding: 8px 2px; font-size: 11px; font-weight: 600; border: none; border-radius: 7px; background: transparent; color: #94a3b8; cursor: pointer; text-align: center; transition: 0.2s; white-space: nowrap; }
    .tab-btn.active { background: #3b82f6; color: white; box-shadow: 0 2px 8px rgba(59,130,246,0.4); }
    
    .tab-pane { display: none; }
    .tab-pane.active { display: block; }

    .upload-btn-wrapper { position: relative; overflow: hidden; display: block; margin-bottom: 8px; }
    .btn-pick { background: #2563eb; color: white; padding: 10px 12px; border-radius: 10px; font-size: 13px; font-weight: 600; text-align: center; display: block; cursor: pointer; border: none; width: 100%; }
    .upload-btn-wrapper input[type=file] { position: absolute; left: 0; top: 0; opacity: 0; width: 100%; height: 100%; cursor: pointer; }

    /* Khung Preview chuẩn tỷ lệ màn hình ESP32 (240x320) không bao giờ bị tràn */
    .preview-box { width: 210px; height: 280px; margin: 0 auto 10px auto; border: 2px dashed #475569; border-radius: 10px; overflow: hidden; background: #000; position: relative; display: flex; align-items: center; justify-content: center; touch-action: none; }
    canvas { width: 100%; height: 100%; object-fit: contain; }
    .placeholder { color: #64748b; font-size: 12px; text-align: center; padding: 16px; }

    .slider-row { display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px; font-size: 12px; color: #cbd5e1; }
    .slider-row input[type=range] { flex: 1; margin-left: 10px; }

    .controls { display: grid; grid-template-columns: 1fr 1fr; gap: 6px; margin-bottom: 10px; }
    .btn-sec { background: #334155; color: #e2e8f0; border: none; padding: 8px 6px; border-radius: 8px; font-size: 11px; font-weight: 600; cursor: pointer; text-align: center; }
    .btn-sec:active { background: #475569; }

    .btn-send { background: linear-gradient(135deg, #10b981, #059669); color: white; padding: 12px; border-radius: 10px; font-size: 13px; font-weight: 700; width: 100%; border: none; cursor: pointer; box-shadow: 0 4px 15px rgba(16, 185, 129, 0.4); margin-bottom: 6px; }
    .btn-send:disabled { background: #64748b; cursor: not-allowed; box-shadow: none; }

    .btn-action { background: #0284c7; color: white; border: none; padding: 10px; border-radius: 8px; font-size: 12px; font-weight: 600; cursor: pointer; width: 100%; margin-top: 6px; text-align: center; }
    .btn-action:hover { background: #0369a1; }

    .form-group { margin-bottom: 10px; }
    .form-group label { display: block; font-size: 12px; font-weight: 600; color: #38bdf8; margin-bottom: 4px; }
    .form-group select, .form-group input[type=text], .form-group input[type=number] { width: 100%; padding: 8px 10px; background: #0f172a; color: #f8fafc; border: 1px solid #334155; border-radius: 8px; font-size: 12px; outline: none; }
    .tip-box { background: #0f172a; border-left: 3px solid #38bdf8; padding: 8px; border-radius: 6px; font-size: 11px; color: #94a3b8; line-height: 1.4; margin-top: 8px; }

    .progress-bar { width: 100%; height: 6px; background: #334155; border-radius: 3px; overflow: hidden; margin-top: 8px; display: none; }
    .progress-fill { width: 0%; height: 100%; background: #10b981; transition: width 0.2s; }
    .status-msg { font-size: 11px; text-align: center; margin-top: 8px; color: #38bdf8; min-height: 16px; }

    .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 6px; margin-top: 6px; }
    .btn-effect { background: #1e1b4b; border: 1px solid #6366f1; color: #c7d2fe; padding: 8px 4px; border-radius: 8px; font-size: 11px; font-weight: 600; cursor: pointer; text-align: center; }
    .btn-effect:hover { background: #312e81; }
    .badge { display: inline-block; background: #3b82f6; color: white; padding: 1px 5px; border-radius: 4px; font-size: 10px; margin-left: 2px; }
  </style>
</head>
<body>
  <div class="card">
    <h1>Điện Thoại Thu Nhỏ ESP32</h1>
    <p class="sub">Khung ảnh, Video Motion, Bảng viết & Smartwatch thời tiết</p>

    <!-- TABS -->
    <div class="tabs">
      <button class="tab-btn active" id="tabBtn1" onclick="switchTab('main')">🖼️ Ảnh</button>
      <button class="tab-btn" id="tabBtn2" onclick="switchTab('motion')">🎬 Video</button>
      <button class="tab-btn" id="tabBtn3" onclick="switchTab('draw')">✍️ Viết Vẽ</button>
      <button class="tab-btn" id="tabBtn4" onclick="switchTab('standby')">🕒 Chờ & Khí</button>
    </div>

    <!-- TAB 1: ẢNH CHÍNH -->
    <div class="tab-pane active" id="paneMain">
      <div class="upload-btn-wrapper">
        <button class="btn-pick">📷 Chọn ảnh từ thư viện / Chụp ảnh</button>
        <input type="file" id="fileInputMain" accept="image/*">
      </div>

      <div class="preview-box">
        <div class="placeholder" id="phMain">Chưa chọn ảnh</div>
        <canvas id="canvasMain" width="240" height="320" style="display:none;"></canvas>
      </div>

      <div id="controlsMain" style="display:none;">
        <div class="slider-row">
          <span>Thu phóng:</span>
          <input type="range" id="zoomMain" min="10" max="30" value="10" oninput="updateZoom('main', this.value)">
        </div>
        <div class="controls">
          <button class="btn-sec" onclick="rotateImg('main')">🔄 Xoay 90°</button>
          <button class="btn-sec" onclick="toggleFit('main')">📐 Đổi Fit</button>
        </div>
        <button class="btn-send" id="sendBtnMain" onclick="uploadImg('main')">🚀 Gửi ảnh lên màn hình</button>
      </div>
    </div>

    <!-- TAB 2: VIDEO & MOTION GRAPHIC (ĐÃ FIX TRÀN KHUNG & NÚT XOAY) -->
    <div class="tab-pane" id="paneMotion">
      <div class="form-group">
        <label>🎬 Tải Video từ điện thoại / File GIF:</label>
        <div class="upload-btn-wrapper">
          <button class="btn-pick" style="background:#8b5cf6;">🎞️ Chọn Video (.mp4 / .mov) hoặc GIF</button>
          <input type="file" id="fileInputVid" accept="video/*,.gif">
        </div>

        <!-- Khung Preview chuẩn Canvas 240x320: Không bao giờ bị tràn layout trên iOS Safari -->
        <div class="preview-box" id="vidPreviewBox">
          <div class="placeholder" id="phVid">Chưa chọn video/GIF</div>
          <canvas id="canvasVid" width="240" height="320" style="display:none;"></canvas>
          <img id="gifPreview" style="display:none; max-width:100%; max-height:100%; object-fit:contain;">
        </div>

        <!-- Video ẩn dùng giải mã và vẽ khung hình sang Canvas -->
        <video id="hiddenVid" playsinline muted loop style="display:none;"></video>

        <div id="controlsVid" style="display:none;">
          <div class="controls">
            <button class="btn-sec" onclick="rotateVideo()">🔄 Xoay: <span class="badge" id="vidRotBadge">0°</span></button>
            <button class="btn-sec" onclick="toggleVidFit()">📐 Fit: <span class="badge" id="vidFitBadge">Lấp đầy</span></button>
          </div>
          <button class="btn-send" id="sendBtnVid" onclick="processAndUploadVideo()">🚀 Chuyển đổi & Gửi Video lên ESP32</button>
        </div>
      </div>

      <!-- CÁC HIỆU ỨNG MOTION GRAPHIC SẴN CÓ -->
      <div class="form-group" style="margin-top:12px; border-top:1px solid #334155; padding-top:10px;">
        <label>✨ Hiệu Ứng Motion Graphic Sẵn Có (Bấm là chạy ngay):</label>
        <div class="grid-2">
          <button class="btn-effect" onclick="setEffect('matrix')">🟢 Mưa Số Matrix</button>
          <button class="btn-effect" onclick="setEffect('starfield')">🌌 Vũ Trụ 3D</button>
          <button class="btn-effect" onclick="setEffect('wave')">🌈 Sóng Âm Neon</button>
          <button class="btn-effect" onclick="setEffect('mjpeg')">▶️ Video Vừa Tải</button>
        </div>
      </div>
      <div class="tip-box">
        💡 <b>Mẹo Video:</b> Nếu video quay ngang (16:9), bạn bấm nút <b>🔄 Xoay 90°</b> để xoay dọc vừa khít toàn màn hình 240x320 của ESP32! Video phát lặp liên tục, xuyên suốt không bị chớp giật.
      </div>
    </div>

    <!-- TAB 3: BẢNG VIẾT TAY TRÊN ESP32 -->
    <div class="tab-pane" id="paneDraw">
      <div class="tip-box" style="margin-top:0;">
        ✍️ <b>Bút Cảm Ứng Trực Tiếp:</b><br>
        Bạn có thể dùng <b>bút cảm ứng (Stylus), móng tay hoặc đầu bút nhựa</b> để viết vẽ trực tiếp lên màn hình! Khi đổi màu bút trên thanh công cụ sẽ <b>không bị xoá mất nét vẽ</b>.
      </div>
      
      <button class="btn-action" style="background:#10b981; margin-top:12px;" onclick="openWhiteboard()">🖌️ MỞ BẢNG VIẾT TAY TRÊN ESP32</button>
      <button class="btn-action" style="background:#0284c7; margin-top:6px;" onclick="viewSavedNote()">📑 XEM LẠI GHI CHÚ ĐÃ LƯU</button>
      <button class="btn-action" style="background:#ef4444; margin-top:6px;" onclick="clearSavedNote()">🗑️ XOÁ GHI CHÚ VIẾT TAY</button>
    </div>

    <!-- TAB 4: MÀN HÌNH CHỜ SMARTWATCH & THỜI TIẾT -->
    <div class="tab-pane" id="paneStandby">
      <div class="form-group">
        <label>⏱️ Thời gian tự vào chế độ chờ:</label>
        <select id="cfgTimeout">
          <option value="10">10 giây (Test nhanh)</option>
          <option value="15">15 giây</option>
          <option value="30" selected>30 giây (Mặc định)</option>
          <option value="60">60 giây (1 phút)</option>
          <option value="0">Tắt (Luôn sáng màn hình chính)</option>
        </select>
      </div>

      <div class="form-group">
        <label>✨ Kiểu Màn Hình Chờ (Standby):</label>
        <select id="cfgMode">
          <option value="watch" selected>⌚ Mặt Đồng Hồ Smartwatch Kim & Thời Tiết</option>
          <option value="motion">🎬 Video Motion / Matrix Rain</option>
          <option value="photo">🖼️ Ảnh tĩnh kỷ niệm</option>
          <option value="sleep">🌙 Tắt màn hình (Chạm là sáng)</option>
        </select>
      </div>

      <div class="form-group" style="border-top:1px solid #334155; padding-top:10px;">
        <label>🌦️ Thời tiết & Lượng mưa cho Smartwatch:</label>
        <p style="font-size:11px; color:#94a3b8; margin-bottom:6px;">ESP32 tự động lấy thời tiết thực tế từ Open-Meteo & đồng bộ chuẩn qua điện thoại.</p>
        <div class="grid-2">
          <input type="number" id="inpTemp" placeholder="Nhiệt độ (°C)" value="28">
          <input type="number" id="inpRain" placeholder="Lượng mưa (mm)" step="0.1" value="0.0">
        </div>
        <button class="btn-action" style="background:#059669; margin-top:6px;" onclick="syncWeather()">🌤️ Đồng Bộ Thời Tiết Sang ESP32</button>
      </div>

      <button class="btn-send" style="display:block; margin-top:12px;" onclick="saveConfig()">💾 Lưu Cài Đặt Standby</button>
      <button class="btn-sec" style="width:100%; margin-top:6px;" onclick="toggleColorOrder()">🎨 Đổi hệ màu RGB / BGR</button>
      <button class="btn-action" style="background:#4f46e5; margin-top:6px;" onclick="showWatchNow()">⌚ Xem Ngay Mặt Đồng Hồ Smartwatch</button>
    </div>

    <!-- TIẾN TRÌNH & THÔNG BÁO -->
    <div class="progress-bar" id="progressBar">
      <div class="progress-fill" id="progressFill"></div>
    </div>
    <div class="status-msg" id="statusMsg"></div>
  </div>

  <script>
    const state = {
      main: { img: new Image(), rot: 0, fit: 'cover', zoom: 1.0, panX: 0, panY: 0, isDrag: false, sX: 0, sY: 0 }
    };
    let selectedVideoFile = null;
    let videoRot = 0;
    let videoFit = 'cover';
    let vidAnimId = null;

    const TARGET_W = 240;
    const TARGET_H = 320;
    const statusMsg = document.getElementById('statusMsg');
    const pBar = document.getElementById('progressBar');
    const pFill = document.getElementById('progressFill');

    const switchTab = (tab) => {
      const tabs = ['main', 'motion', 'draw', 'standby'];
      tabs.forEach((t, i) => {
        document.getElementById('tabBtn' + (i+1)).classList.toggle('active', t === tab);
        document.getElementById('pane' + t.charAt(0).toUpperCase() + t.slice(1)).classList.toggle('active', t === tab);
      });
    };

    // --- TAB 1: ẢNH CHÍNH ---
    const canvasMain = document.getElementById('canvasMain');
    const ctxMain = canvasMain.getContext('2d');
    const s = state.main;

    document.getElementById('fileInputMain').addEventListener('change', (e) => {
      const file = e.target.files[0];
      if (!file) return;
      const reader = new FileReader();
      reader.onload = (evt) => {
        s.img.onload = () => {
          s.rot = 0; s.zoom = 1.0; s.panX = 0; s.panY = 0;
          document.getElementById('phMain').style.display = 'none';
          canvasMain.style.display = 'block';
          document.getElementById('controlsMain').style.display = 'block';
          drawMain();
        };
        s.img.src = evt.target.result;
      };
      reader.readAsDataURL(file);
    });

    const onStart = (cx, cy) => { s.isDrag = true; s.sX = cx - s.panX; s.sY = cy - s.panY; };
    const onMove  = (cx, cy) => { if (!s.isDrag) return; s.panX = cx - s.sX; s.panY = cy - s.sY; drawMain(); };
    const onEnd   = () => { s.isDrag = false; };

    canvasMain.addEventListener('mousedown', (e) => onStart(e.clientX, e.clientY));
    window.addEventListener('mousemove', (e) => onMove(e.clientX, e.clientY));
    window.addEventListener('mouseup', onEnd);
    canvasMain.addEventListener('touchstart', (e) => { if (e.touches.length === 1) onStart(e.touches[0].clientX, e.touches[0].clientY); });
    window.addEventListener('touchmove', (e) => { if (e.touches.length === 1) onMove(e.touches[0].clientX, e.touches[0].clientY); });
    window.addEventListener('touchend', onEnd);

    const drawMain = () => {
      ctxMain.fillStyle = '#000000';
      ctxMain.fillRect(0, 0, TARGET_W, TARGET_H);
      ctxMain.save();
      ctxMain.translate(TARGET_W / 2 + s.panX, TARGET_H / 2 + s.panY);
      ctxMain.rotate((s.rot * Math.PI) / 180);

      let w = s.img.width;
      let h = s.img.height;
      if (s.rot === 90 || s.rot === 270) { const tmp = w; w = h; h = tmp; }
      let baseScale = (s.fit === 'cover') ? Math.max(TARGET_W / w, TARGET_H / h) : Math.min(TARGET_W / w, TARGET_H / h);
      const finalScale = baseScale * s.zoom;

      ctxMain.drawImage(s.img, -s.img.width * finalScale / 2, -s.img.height * finalScale / 2, s.img.width * finalScale, s.img.height * finalScale);
      ctxMain.restore();
    };

    const updateZoom = (key, val) => { s.zoom = val / 10; drawMain(); };
    const rotateImg  = (key) => { s.rot = (s.rot + 90) % 360; s.panX = 0; s.panY = 0; drawMain(); };
    const toggleFit  = (key) => { s.fit = (s.fit === 'cover') ? 'contain' : 'cover'; drawMain(); };

    const uploadImg = (key) => {
      const sendBtn = document.getElementById('sendBtnMain');
      sendBtn.disabled = true;
      pBar.style.display = 'block';
      pFill.style.width = '20%';
      statusMsg.innerText = 'Đang gửi ảnh lên ESP32...';

      canvasMain.toBlob((blob) => {
        const formData = new FormData();
        formData.append('image', blob, 'photo.jpg');
        const xhr = new XMLHttpRequest();
        xhr.open('POST', '/upload', true);
        xhr.upload.onprogress = (e) => {
          if (e.lengthComputable) pFill.style.width = Math.round((e.loaded / e.total) * 100) + '%';
        };
        xhr.onload = () => {
          sendBtn.disabled = false;
          if (xhr.status === 200) {
            pFill.style.width = '100%';
            statusMsg.style.color = '#10b981';
            statusMsg.innerText = '✅ Ảnh đã hiển thị trên màn hình!';
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
      }, 'image/jpeg', 0.88);
    };

    // --- TAB 2: VIDEO CANVAS PREVIEW & CONVERTER ---
    const canvasVid = document.getElementById('canvasVid');
    const ctxVid = canvasVid.getContext('2d');
    const hiddenVid = document.getElementById('hiddenVid');
    const gifPreview = document.getElementById('gifPreview');
    const phVid = document.getElementById('phVid');
    const controlsVid = document.getElementById('controlsVid');

    const renderVidPreviewLoop = () => {
      if (!hiddenVid || hiddenVid.paused || hiddenVid.ended) return;
      ctxVid.fillStyle = '#000000';
      ctxVid.fillRect(0, 0, TARGET_W, TARGET_H);

      ctxVid.save();
      ctxVid.translate(TARGET_W / 2, TARGET_H / 2);
      ctxVid.rotate((videoRot * Math.PI) / 180);

      let vw = hiddenVid.videoWidth || 240;
      let vh = hiddenVid.videoHeight || 320;
      let effW = (videoRot === 90 || videoRot === 270) ? vh : vw;
      let effH = (videoRot === 90 || videoRot === 270) ? vw : vh;

      let scale = (videoFit === 'cover') ? Math.max(TARGET_W / effW, TARGET_H / effH) : Math.min(TARGET_W / effW, TARGET_H / effH);
      let dw = vw * scale;
      let dh = vh * scale;

      ctxVid.drawImage(hiddenVid, -dw / 2, -dh / 2, dw, dh);
      ctxVid.restore();

      vidAnimId = requestAnimationFrame(renderVidPreviewLoop);
    };

    const rotateVideo = () => {
      videoRot = (videoRot + 90) % 360;
      document.getElementById('vidRotBadge').innerText = videoRot + '°';
      if (gifPreview.style.display !== 'none') {
        gifPreview.style.transform = 'rotate(' + videoRot + 'deg)';
      }
    };

    const toggleVidFit = () => {
      videoFit = (videoFit === 'cover') ? 'contain' : 'cover';
      document.getElementById('vidFitBadge').innerText = (videoFit === 'cover') ? 'Lấp đầy' : 'Vừa vặn';
    };

    document.getElementById('fileInputVid').addEventListener('change', (e) => {
      const file = e.target.files[0];
      if (!file) return;
      selectedVideoFile = file;
      videoRot = 0;
      videoFit = 'cover';
      document.getElementById('vidRotBadge').innerText = '0°';
      document.getElementById('vidFitBadge').innerText = 'Lấp đầy';

      if (vidAnimId) cancelAnimationFrame(vidAnimId);
      phVid.style.display = 'none';
      controlsVid.style.display = 'block';

      if (file.type.includes('gif') || file.name.toLowerCase().endsWith('.gif')) {
        canvasVid.style.display = 'none';
        hiddenVid.pause();
        gifPreview.style.display = 'block';
        gifPreview.src = URL.createObjectURL(file);
        statusMsg.style.color = '#38bdf8';
        statusMsg.innerText = 'Đã chọn ảnh GIF (' + Math.round(file.size/1024) + ' KB)';
      } else {
        gifPreview.style.display = 'none';
        canvasVid.style.display = 'block';
        hiddenVid.src = URL.createObjectURL(file);
        hiddenVid.play().then(() => {
          renderVidPreviewLoop();
        }).catch(() => {
          hiddenVid.play();
          renderVidPreviewLoop();
        });
        statusMsg.style.color = '#38bdf8';
        statusMsg.innerText = 'Đã chọn Video: ' + file.name + ' (' + Math.round(file.size/1024) + ' KB)';
      }
    });

    const processAndUploadVideo = () => {
      if (!selectedVideoFile) return;
      const sendBtn = document.getElementById('sendBtnVid');
      sendBtn.disabled = true;
      pBar.style.display = 'block';
      pFill.style.width = '10%';

      // 1. Trường hợp File GIF
      if (selectedVideoFile.type.includes('gif') || selectedVideoFile.name.toLowerCase().endsWith('.gif')) {
        statusMsg.innerText = 'Đang gửi file GIF lên ESP32...';
        const formData = new FormData();
        formData.append('file', selectedVideoFile, 'motion.gif');
        const xhr = new XMLHttpRequest();
        xhr.open('POST', '/upload_gif', true);
        xhr.upload.onprogress = (e) => {
          if (e.lengthComputable) pFill.style.width = Math.round((e.loaded / e.total) * 100) + '%';
        };
        xhr.onload = () => {
          sendBtn.disabled = false;
          statusMsg.style.color = '#10b981';
          statusMsg.innerText = '✅ Đã tải xong! ESP32 đang phát GIF mượt mà!';
        };
        xhr.send(formData);
        return;
      }

      // 2. Trường hợp Video MP4/MOV: Trích xuất frame chất lượng cao chuẩn 240x320
      statusMsg.innerText = 'Đang chuẩn bị khung hình video...';
      const video = document.createElement('video');
      video.muted = true;
      video.playsInline = true;
      video.src = URL.createObjectURL(selectedVideoFile);

      video.onloadedmetadata = () => {
        const duration = Math.min(video.duration || 3, 4.0);
        const fps = 12; // 12 khung hình/giây siêu mượt
        const totalFrames = Math.max(12, Math.floor(duration * fps));
        const tempCanvas = document.createElement('canvas');
        tempCanvas.width = 240;
        tempCanvas.height = 320;
        const tempCtx = tempCanvas.getContext('2d');
        const frameBlobs = [];
        let cur = 0;

        const captureFrame = () => {
          if (cur >= totalFrames) {
            statusMsg.innerText = 'Đang nạp ' + frameBlobs.length + ' khung hình lên ESP32...';
            pFill.style.width = '75%';
            const mjpegBlob = new Blob(frameBlobs, { type: 'video/x-motion-jpeg' });
            
            const formData = new FormData();
            formData.append('file', mjpegBlob, 'motion.mjpg');
            const xhr = new XMLHttpRequest();
            xhr.open('POST', '/upload_mjpeg', true);
            xhr.upload.onprogress = (e) => {
              if (e.lengthComputable) pFill.style.width = 75 + Math.round((e.loaded / e.total) * 25) + '%';
            };
            xhr.onload = () => {
              sendBtn.disabled = false;
              if (xhr.status === 200) {
                pFill.style.width = '100%';
                statusMsg.style.color = '#10b981';
                statusMsg.innerText = '✅ Video phát liên tục, mượt mà trên ESP32!';
              } else {
                statusMsg.style.color = '#ef4444';
                statusMsg.innerText = '❌ Lỗi gửi MJPEG: ' + xhr.status;
              }
            };
            xhr.onerror = () => {
              sendBtn.disabled = false;
              statusMsg.style.color = '#ef4444';
              statusMsg.innerText = '❌ Mất kết nối tới ESP32!';
            };
            xhr.send(formData);
            return;
          }

          video.currentTime = (cur / fps);
        };

        video.onseeked = () => {
          tempCtx.fillStyle = '#000';
          tempCtx.fillRect(0, 0, 240, 320);

          tempCtx.save();
          tempCtx.translate(120, 160);
          tempCtx.rotate((videoRot * Math.PI) / 180);

          let vw = video.videoWidth || 240;
          let vh = video.videoHeight || 320;
          let effW = (videoRot === 90 || videoRot === 270) ? vh : vw;
          let effH = (videoRot === 90 || videoRot === 270) ? vw : vh;

          let scale = (videoFit === 'cover') ? Math.max(240 / effW, 320 / effH) : Math.min(240 / effW, 320 / effH);
          let drawW = vw * scale;
          let drawH = vh * scale;

          tempCtx.drawImage(video, -drawW / 2, -drawH / 2, drawW, drawH);
          tempCtx.restore();

          tempCanvas.toBlob((blob) => {
            frameBlobs.push(blob);
            cur++;
            pFill.style.width = Math.round(10 + (cur / totalFrames) * 65) + '%';
            statusMsg.innerText = 'Đang tối ưu frame ' + cur + '/' + totalFrames + '...';
            captureFrame();
          }, 'image/jpeg', 0.60);
        };

        video.currentTime = 0;
      };

      video.onerror = () => {
        sendBtn.disabled = false;
        statusMsg.style.color = '#ef4444';
        statusMsg.innerText = '❌ Lỗi đọc video!';
      };
    };

    // --- HIỆU ỨNG MOTION GRAPHIC ---
    const setEffect = (eff) => {
      fetch('/set_effect?eff=' + eff)
        .then(r => r.text())
        .then(res => {
          statusMsg.style.color = '#10b981';
          statusMsg.innerText = '✅ ' + res;
        });
    };

    // --- BẢNG VIẾT TAY ---
    const openWhiteboard = () => {
      fetch('/whiteboard')
        .then(r => r.text())
        .then(res => {
          statusMsg.style.color = '#10b981';
          statusMsg.innerText = '✅ Đã mở bảng vẽ trên ESP32! Dùng bút viết nhé.';
        });
    };

    const viewSavedNote = () => {
      fetch('/view_note')
        .then(r => r.text())
        .then(res => {
          statusMsg.style.color = '#10b981';
          statusMsg.innerText = '✅ ' + res;
        });
    };

    const clearSavedNote = () => {
      if (confirm('Bạn có chắc muốn xoá ghi chú viết tay này?')) {
        fetch('/clear_note')
          .then(r => r.text())
          .then(res => {
            statusMsg.style.color = '#ef4444';
            statusMsg.innerText = '🗑️ ' + res;
          });
      }
    };

    // --- STANDBY & THỜI TIẾT ---
    const syncWeather = () => {
      const temp = document.getElementById('inpTemp').value;
      const rain = document.getElementById('inpRain').value;
      fetch('/sync_weather?temp=' + temp + '&rain=' + rain)
        .then(r => r.text())
        .then(res => {
          statusMsg.style.color = '#10b981';
          statusMsg.innerText = '✅ Đã đồng bộ: ' + temp + '°C, ' + rain + 'mm lên Smartwatch!';
        });
    };

    const saveConfig = () => {
      const timeout = document.getElementById('cfgTimeout').value;
      const mode = document.getElementById('cfgMode').value;
      fetch('/save_config?timeout=' + timeout + '&mode=' + mode)
        .then(r => r.text())
        .then(res => {
          statusMsg.style.color = '#10b981';
          statusMsg.innerText = '✅ Đã lưu cài đặt chế độ chờ!';
        });
    };

    const toggleColorOrder = () => {
      fetch('/toggle_color')
        .then(r => r.text())
        .then(res => {
          statusMsg.style.color = '#10b981';
          statusMsg.innerText = '🎨 Hệ màu hiện tại: ' + res;
        });
    };

    const showWatchNow = () => {
      fetch('/show_watch')
        .then(r => r.text())
        .then(res => {
          statusMsg.style.color = '#10b981';
          statusMsg.innerText = '✅ Đã mở mặt đồng hồ Smartwatch!';
        });
    };

    // Tự động đồng bộ giờ từ điện thoại khi kết nối
    window.onload = () => {
      const epoch = Math.floor(Date.now() / 1000);
      fetch('/sync_time?epoch=' + epoch).catch(()=>{});
    };
  </script>
</body>
</html>
)rawliteral";

#endif
