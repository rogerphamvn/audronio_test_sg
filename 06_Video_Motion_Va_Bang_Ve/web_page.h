#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Khung Ảnh, Video & Bảng Viết ESP32</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; touch-action: manipulation; }
    body { background: #0b1120; color: #f8fafc; padding: 12px; display: flex; flex-direction: column; align-items: center; min-height: 100vh; }
    .card { background: #1e293b; border-radius: 16px; padding: 16px; width: 100%; max-width: 400px; box-shadow: 0 10px 30px rgba(0,0,0,0.6); border: 1px solid #334155; }
    h1 { font-size: 18px; font-weight: 700; text-align: center; color: #38bdf8; margin-bottom: 2px; }
    p.sub { font-size: 12px; text-align: center; color: #94a3b8; margin-bottom: 12px; }
    
    .tabs { display: grid; grid-template-columns: repeat(4, 1fr); gap: 4px; margin-bottom: 14px; background: #0f172a; padding: 4px; border-radius: 10px; }
    .tab-btn { padding: 9px 2px; font-size: 11px; font-weight: 600; border: none; border-radius: 7px; background: transparent; color: #94a3b8; cursor: pointer; text-align: center; transition: 0.2s; white-space: nowrap; }
    .tab-btn.active { background: #3b82f6; color: white; box-shadow: 0 2px 8px rgba(59,130,246,0.4); }
    
    .tab-pane { display: none; }
    .tab-pane.active { display: block; }

    .upload-btn-wrapper { position: relative; overflow: hidden; display: block; margin-bottom: 10px; }
    .btn-pick { background: #2563eb; color: white; padding: 11px 14px; border-radius: 10px; font-size: 13px; font-weight: 600; text-align: center; display: block; cursor: pointer; border: none; width: 100%; }
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

    .btn-send { background: linear-gradient(135deg, #10b981, #059669); color: white; padding: 13px; border-radius: 10px; font-size: 14px; font-weight: 700; width: 100%; border: none; cursor: pointer; box-shadow: 0 4px 15px rgba(16, 185, 129, 0.4); margin-bottom: 8px; }
    .btn-send:disabled { background: #64748b; cursor: not-allowed; box-shadow: none; }

    .btn-action { background: #0284c7; color: white; border: none; padding: 11px; border-radius: 9px; font-size: 13px; font-weight: 600; cursor: pointer; width: 100%; margin-top: 8px; text-align: center; }
    .btn-action:hover { background: #0369a1; }

    .form-group { margin-bottom: 13px; }
    .form-group label { display: block; font-size: 13px; font-weight: 600; color: #38bdf8; margin-bottom: 6px; }
    .form-group select { width: 100%; padding: 10px; background: #0f172a; color: #f8fafc; border: 1px solid #334155; border-radius: 8px; font-size: 13px; outline: none; }
    .tip-box { background: #0f172a; border-left: 3px solid #38bdf8; padding: 10px; border-radius: 6px; font-size: 12px; color: #94a3b8; line-height: 1.4; margin-top: 10px; }

    .progress-bar { width: 100%; height: 6px; background: #334155; border-radius: 3px; overflow: hidden; margin-top: 10px; display: none; }
    .progress-fill { width: 0%; height: 100%; background: #10b981; transition: width 0.2s; }
    .status-msg { font-size: 12px; text-align: center; margin-top: 10px; color: #38bdf8; min-height: 18px; }

    .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-top: 8px; }
    .btn-effect { background: #1e1b4b; border: 1px solid #6366f1; color: #c7d2fe; padding: 10px; border-radius: 8px; font-size: 12px; font-weight: 600; cursor: pointer; }
    .btn-effect:hover { background: #312e81; }
  </style>
</head>
<body>
  <div class="card">
    <h1>Khung Ảnh, Video & Bảng Viết</h1>
    <p class="sub">Hỗ trợ ảnh tĩnh, video motion & viết vẽ bằng bút cảm ứng</p>

    <!-- TABS -->
    <div class="tabs">
      <button class="tab-btn active" id="tabBtn1" onclick="switchTab('main')">🖼️ Ảnh</button>
      <button class="tab-btn" id="tabBtn2" onclick="switchTab('motion')">🎬 Video</button>
      <button class="tab-btn" id="tabBtn3" onclick="switchTab('draw')">✍️ Viết Vẽ</button>
      <button class="tab-btn" id="tabBtn4" onclick="switchTab('standby')">🕒 Chờ</button>
    </div>

    <!-- TAB 1: ẢNH CHÍNH -->
    <div class="tab-pane active" id="paneMain">
      <div class="upload-btn-wrapper">
        <button class="btn-pick">📸 Chọn ảnh từ điện thoại</button>
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

    <!-- TAB 2: VIDEO & MOTION GRAPHIC -->
    <div class="tab-pane" id="paneMotion">
      <div class="form-group">
        <label>🎬 Tải Video / Motion Graphic (GIF):</label>
        <div class="upload-btn-wrapper">
          <button class="btn-pick" style="background:#8b5cf6;">🎞️ Chọn file GIF / Video ngắn</button>
          <input type="file" id="fileInputGif" accept=".gif,video/*">
        </div>
        <div id="gifPreviewBox" style="display:none; text-align:center; margin:10px 0;">
          <img id="gifPreview" style="max-width:200px; max-height:260px; border-radius:8px; border:1px solid #475569;">
        </div>
        <button class="btn-send" id="sendBtnGif" style="display:none;" onclick="uploadGif()">🚀 Gửi Video/GIF lên ESP32</button>
      </div>

      <div class="form-group" style="margin-top:16px; border-top:1px solid #334155; padding-top:12px;">
        <label>✨ Hiệu Ứng Motion Graphic Có Sẵn:</label>
        <div class="grid-2">
          <button class="btn-effect" onclick="setEffect('matrix')">🟢 Mưa Số Matrix</button>
          <button class="btn-effect" onclick="setEffect('starfield')">🌌 Vũ Trụ Ngàn Sao</button>
          <button class="btn-effect" onclick="setEffect('wave')">🌈 Sóng Neon Sống Động</button>
          <button class="btn-effect" onclick="setEffect('file')">▶️ Phát Video Đã Tải</button>
        </div>
      </div>
      <div class="tip-box">
        💡 <b>Mẹo Video:</b> ESP32 phát mượt mà các file GIF động, sticker chuyển động, anime và motion graphic lặp vô tận (loop)!
      </div>
    </div>

    <!-- TAB 3: BẢNG VIẾT TAY TRÊN ESP32 -->
    <div class="tab-pane" id="paneDraw">
      <div class="tip-box" style="margin-top:0;">
        ✍️ <b>Bút Cảm Ứng Trên ESP32:</b><br>
        Màn hình CYD hỗ trợ cảm ứng lực. Bạn có thể dùng <b>bút cảm ứng (Stylus), móng tay hoặc đầu bút nhựa</b> để viết vẽ trực tiếp lên màn hình!
      </div>
      
      <button class="btn-action" style="background:#10b981; margin-top:14px;" onclick="openWhiteboard()">🖌️ MỞ BẢNG VIẾT TAY TRÊN ESP32</button>
      <button class="btn-action" style="background:#0284c7; margin-top:8px;" onclick="viewSavedNote()">📝 XEM LẠI GHI CHÚ ĐÃ LƯU</button>
      <button class="btn-action" style="background:#ef4444; margin-top:8px;" onclick="clearSavedNote()">🗑️ XOÁ GHI CHÚ VIẾT TAY</button>
    </div>

    <!-- TAB 4: CÀI ĐẶT MÀN HÌNH CHỜ (AOD) -->
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
        <label>✨ Kiểu Màn Hình Chờ (Standby Mode):</label>
        <select id="cfgMode">
          <option value="clock">🕒 Đồng hồ số AOD (Giờ + Ngày)</option>
          <option value="motion">🎬 Motion Graphic (Matrix / Starfield / GIF)</option>
          <option value="note">✍️ Ghi chú viết tay bằng bút</option>
          <option value="photo">🖼️ Ảnh chờ phụ riêng</option>
          <option value="sleep">🌙 Tắt màn hình (Chạm là sáng)</option>
        </select>
      </div>

      <button class="btn-send" style="display:block; margin-top:14px;" onclick="saveConfig()">💾 Lưu Cài Đặt Standby</button>
      <button class="btn-sec" style="width:100%; margin-top:6px;" onclick="toggleColorOrder()">🎨 Đổi hệ màu RGB / BGR</button>
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
    let selectedGifBlob = null;
    const TARGET_W = 240;
    const TARGET_H = 320;
    const statusMsg = document.getElementById('statusMsg');

    const switchTab = (tab) => {
      const tabs = ['main', 'motion', 'draw', 'standby'];
      tabs.forEach((t, i) => {
        document.getElementById('tabBtn' + (i+1)).classList.toggle('active', t === tab);
        document.getElementById('pane' + t.charAt(0).toUpperCase() + t.slice(1)).classList.toggle('active', t === tab);
      });
    };

    // Uploader ảnh chính
    const canvas = document.getElementById('canvasMain');
    const ctx = canvas.getContext('2d');
    const s = state.main;

    document.getElementById('fileInputMain').addEventListener('change', (e) => {
      const file = e.target.files[0];
      if (!file) return;
      const reader = new FileReader();
      reader.onload = (evt) => {
        s.img.onload = () => {
          s.rot = 0; s.zoom = 1.0; s.panX = 0; s.panY = 0;
          document.getElementById('phMain').style.display = 'none';
          canvas.style.display = 'block';
          document.getElementById('controlsMain').style.display = 'block';
          document.getElementById('sendBtnMain').style.display = 'block';
          drawMain();
        };
        s.img.src = evt.target.result;
      };
      reader.readAsDataURL(file);
    });

    const onStart = (cx, cy) => { s.isDrag = true; s.sX = cx - s.panX; s.sY = cy - s.panY; };
    const onMove  = (cx, cy) => { if (!s.isDrag) return; s.panX = cx - s.sX; s.panY = cy - s.sY; drawMain(); };
    const onEnd   = () => { s.isDrag = false; };

    canvas.addEventListener('mousedown', (e) => onStart(e.clientX, e.clientY));
    window.addEventListener('mousemove', (e) => onMove(e.clientX, e.clientY));
    window.addEventListener('mouseup', onEnd);
    canvas.addEventListener('touchstart', (e) => { if (e.touches.length === 1) onStart(e.touches[0].clientX, e.touches[0].clientY); });
    window.addEventListener('touchmove', (e) => { if (e.touches.length === 1) onMove(e.touches[0].clientX, e.touches[0].clientY); });
    window.addEventListener('touchend', onEnd);

    const drawMain = () => {
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

    const updateZoom = (key, val) => { s.zoom = val / 10; drawMain(); };
    const rotateImg  = (key) => { s.rot = (s.rot + 90) % 360; s.panX = 0; s.panY = 0; drawMain(); };
    const toggleFit  = (key) => { s.fit = (s.fit === 'cover') ? 'contain' : 'cover'; drawMain(); };

    const uploadImg = (key) => {
      const sendBtn = document.getElementById('sendBtnMain');
      const pBar = document.getElementById('progressBar');
      const pFill = document.getElementById('progressFill');

      sendBtn.disabled = true;
      pBar.style.display = 'block';
      pFill.style.width = '20%';
      statusMsg.innerText = 'Đang gửi ảnh lên ESP32...';

      canvas.toBlob((blob) => {
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
      }, 'image/jpeg', 0.9);
    };

    // Chọn & Upload GIF/Video
    document.getElementById('fileInputGif').addEventListener('change', (e) => {
      const file = e.target.files[0];
      if (!file) return;
      selectedGifBlob = file;

      const previewBox = document.getElementById('gifPreviewBox');
      const previewImg = document.getElementById('gifPreview');
      const sendBtnGif = document.getElementById('sendBtnGif');

      if (file.type.includes('gif')) {
        previewImg.src = URL.createObjectURL(file);
        previewBox.style.display = 'block';
        sendBtnGif.style.display = 'block';
        statusMsg.style.color = '#38bdf8';
        statusMsg.innerText = 'Đã chọn GIF (' + Math.round(file.size/1024) + ' KB)';
      } else {
        // File video khác: gửi trực tiếp
        previewBox.style.display = 'none';
        sendBtnGif.style.display = 'block';
        statusMsg.style.color = '#38bdf8';
        statusMsg.innerText = 'Đã chọn file: ' + file.name + ' (' + Math.round(file.size/1024) + ' KB)';
      }
    });

    const uploadGif = () => {
      if (!selectedGifBlob) return;
      const sendBtn = document.getElementById('sendBtnGif');
      const pBar = document.getElementById('progressBar');
      const pFill = document.getElementById('progressFill');

      sendBtn.disabled = true;
      pBar.style.display = 'block';
      pFill.style.width = '10%';
      statusMsg.innerText = 'Đang tải file lên ESP32...';

      const formData = new FormData();
      formData.append('file', selectedGifBlob, 'motion.gif');
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/upload_gif', true);
      xhr.upload.onprogress = (e) => {
        if (e.lengthComputable) pFill.style.width = Math.round((e.loaded / e.total) * 100) + '%';
      };
      xhr.onload = () => {
        sendBtn.disabled = false;
        if (xhr.status === 200) {
          pFill.style.width = '100%';
          statusMsg.style.color = '#10b981';
          statusMsg.innerText = '✅ Đã tải xong! ESP32 đang phát motion graphic!';
        } else {
          statusMsg.style.color = '#ef4444';
          statusMsg.innerText = '❌ Lỗi tải: ' + xhr.status;
        }
      };
      xhr.onerror = () => {
        sendBtn.disabled = false;
        statusMsg.style.color = '#ef4444';
        statusMsg.innerText = '❌ Lỗi kết nối tới ESP32!';
      };
      xhr.send(formData);
    };

    const setEffect = (eff) => {
      fetch('/set_effect?eff=' + eff)
        .then(r => r.text())
        .then(res => {
          statusMsg.style.color = '#10b981';
          statusMsg.innerText = '✅ ' + res;
        });
    };

    const openWhiteboard = () => {
      fetch('/whiteboard')
        .then(r => r.text())
        .then(res => {
          statusMsg.style.color = '#10b981';
          statusMsg.innerText = '✅ Đã mở bảng vẽ trên màn hình ESP32! Hãy dùng bút viết.';
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

    const saveConfig = () => {
      const timeout = document.getElementById('cfgTimeout').value;
      const mode = document.getElementById('cfgMode').value;
      statusMsg.innerText = 'Đang lưu cấu hình...';
      fetch('/save_config?timeout=' + timeout + '&mode=' + mode)
        .then(r => r.text())
        .then(res => {
          statusMsg.style.color = '#10b981';
          statusMsg.innerText = '✅ ' + res;
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

    window.addEventListener('DOMContentLoaded', () => {
      const epoch = Math.floor(Date.now() / 1000);
      fetch('/sync_time?epoch=' + epoch).catch(() => {});
      fetch('/get_config')
        .then(r => r.json())
        .then(cfg => {
          if (cfg.timeout !== undefined) document.getElementById('cfgTimeout').value = cfg.timeout;
          if (cfg.mode) document.getElementById('cfgMode').value = cfg.mode;
        }).catch(() => {});
    });
  </script>
</body>
</html>
)rawliteral";

#endif
