#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>CYD Photo Frame</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background: #0f172a; color: #f8fafc; padding: 16px; display: flex; flex-direction: column; align-items: center; min-height: 100vh; }
    .card { background: #1e293b; border-radius: 16px; padding: 20px; width: 100%; max-width: 400px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); border: 1px solid #334155; }
    h1 { font-size: 20px; font-weight: 700; text-align: center; color: #38bdf8; margin-bottom: 6px; }
    p.sub { font-size: 13px; text-align: center; color: #94a3b8; margin-bottom: 20px; }
    
    .upload-btn-wrapper { position: relative; overflow: hidden; display: block; margin-bottom: 16px; }
    .btn-pick { background: #3b82f6; color: white; padding: 14px 20px; border-radius: 12px; font-size: 15px; font-weight: 600; text-align: center; display: block; cursor: pointer; border: none; width: 100%; }
    .upload-btn-wrapper input[type=file] { position: absolute; left: 0; top: 0; opacity: 0; width: 100%; height: 100%; cursor: pointer; }

    .preview-box { width: 240px; height: 320px; margin: 0 auto 16px auto; border: 2px dashed #475569; border-radius: 12px; overflow: hidden; background: #000; position: relative; display: flex; align-items: center; justify-content: center; }
    canvas { width: 100%; height: 100%; object-fit: contain; }
    .placeholder { color: #64748b; font-size: 13px; text-align: center; padding: 20px; }

    .controls { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 16px; }
    .btn-sec { background: #334155; color: #e2e8f0; border: none; padding: 10px; border-radius: 8px; font-size: 13px; font-weight: 500; cursor: pointer; text-align: center; }
    .btn-sec:active { background: #475569; }

    .btn-send { background: linear-gradient(135deg, #10b981, #059669); color: white; padding: 15px; border-radius: 12px; font-size: 16px; font-weight: 700; width: 100%; border: none; cursor: pointer; box-shadow: 0 4px 15px rgba(16, 185, 129, 0.4); display: none; margin-bottom: 10px; }
    .btn-send:disabled { background: #64748b; cursor: not-allowed; box-shadow: none; }

    .btn-color { background: #6366f1; color: white; border: none; padding: 10px; border-radius: 8px; font-size: 13px; font-weight: 600; cursor: pointer; width: 100%; margin-top: 10px; }
    .btn-color:active { background: #4f46e5; }

    .progress-bar { width: 100%; height: 6px; background: #334155; border-radius: 3px; overflow: hidden; margin-top: 14px; display: none; }
    .progress-fill { width: 0%; height: 100%; background: #10b981; transition: width 0.2s; }
    .status-msg { font-size: 13px; text-align: center; margin-top: 12px; color: #38bdf8; min-height: 20px; }
  </style>
</head>
<body>
  <div class="card">
    <h1>Khung Ảnh ESP32 CYD</h1>
    <p class="sub">Gửi ảnh trực tiếp từ điện thoại lên màn hình 2.8"</p>

    <div class="upload-btn-wrapper">
      <button class="btn-pick">📸 Chọn ảnh từ thư viện / Chụp ảnh</button>
      <input type="file" id="fileInput" accept="image/*">
    </div>

    <div class="preview-box" id="previewContainer">
      <div class="placeholder" id="placeholder">Chưa chọn ảnh</div>
      <canvas id="cropCanvas" width="240" height="320" style="display:none;"></canvas>
    </div>

    <div class="controls" id="controlBox" style="display:none;">
      <button class="btn-sec" onclick="rotateImage()">🔄 Xoay ảnh 90°</button>
      <button class="btn-sec" onclick="toggleFit()">📐 Đổi chế độ Fit</button>
    </div>

    <button class="btn-send" id="sendBtn" onclick="uploadImage()">🚀 Gửi ảnh lên màn hình</button>

    <div class="progress-bar" id="progressBar">
      <div class="progress-fill" id="progressFill"></div>
    </div>
    <div class="status-msg" id="statusMsg"></div>

    <button class="btn-color" onclick="toggleColorOrder()">🎨 Đổi hệ màu RGB / BGR trực tiếp trên màn hình</button>
  </div>

  <script>
    let rawImg = new Image();
    let rotation = 0;
    let fitMode = 'cover';
    const canvas = document.getElementById('cropCanvas');
    const ctx = canvas.getContext('2d');
    const TARGET_W = 240;
    const TARGET_H = 320;
    const statusMsg = document.getElementById('statusMsg');

    document.getElementById('fileInput').addEventListener('change', (e) => {
      const file = e.target.files[0];
      if (!file) return;

      const reader = new FileReader();
      reader.onload = (evt) => {
        rawImg.onload = () => {
          rotation = 0;
          document.getElementById('placeholder').style.display = 'none';
          canvas.style.display = 'block';
          document.getElementById('controlBox').style.display = 'grid';
          document.getElementById('sendBtn').style.display = 'block';
          drawCanvas();
        };
        rawImg.src = evt.target.result;
      };
      reader.readAsDataURL(file);
    });

    const rotateImage = () => {
      rotation = (rotation + 90) % 360;
      drawCanvas();
    };

    const toggleFit = () => {
      fitMode = (fitMode === 'cover') ? 'contain' : 'cover';
      drawCanvas();
    };

    const drawCanvas = () => {
      ctx.fillStyle = '#000000';
      ctx.fillRect(0, 0, TARGET_W, TARGET_H);

      ctx.save();
      ctx.translate(TARGET_W / 2, TARGET_H / 2);
      ctx.rotate((rotation * Math.PI) / 180);

      let w = rawImg.width;
      let h = rawImg.height;
      if (rotation === 90 || rotation === 270) {
        const tmp = w; w = h; h = tmp;
      }

      let scale;
      if (fitMode === 'cover') {
        scale = Math.max(TARGET_W / w, TARGET_H / h);
      } else {
        scale = Math.min(TARGET_W / w, TARGET_H / h);
      }

      ctx.drawImage(rawImg, -rawImg.width * scale / 2, -rawImg.height * scale / 2, rawImg.width * scale, rawImg.height * scale);
      ctx.restore();
    };

    const uploadImage = () => {
      const sendBtn = document.getElementById('sendBtn');
      const pBar = document.getElementById('progressBar');
      const pFill = document.getElementById('progressFill');

      sendBtn.disabled = true;
      pBar.style.display = 'block';
      pFill.style.width = '20%';
      statusMsg.innerText = 'Đang xử lý và gửi ảnh...';

      canvas.toBlob((blob) => {
        const formData = new FormData();
        formData.append('image', blob, 'photo.jpg');

        const xhr = new XMLHttpRequest();
        xhr.open('POST', '/upload', true);

        xhr.upload.onprogress = (e) => {
          if (e.lengthComputable) {
            let percent = Math.round((e.loaded / e.total) * 100);
            pFill.style.width = percent + '%';
          }
        };

        xhr.onload = () => {
          sendBtn.disabled = false;
          if (xhr.status === 200) {
            pFill.style.width = '100%';
            statusMsg.style.color = '#10b981';
            statusMsg.innerText = '✅ Ảnh đã hiển thị trên màn hình ESP32!';
          } else {
            statusMsg.style.color = '#ef4444';
            statusMsg.innerText = '❌ Lỗi khi gửi ảnh: ' + xhr.status;
          }
        };

        xhr.onerror = () => {
          sendBtn.disabled = false;
          statusMsg.style.color = '#ef4444';
          statusMsg.innerText = '❌ Không thể kết nối tới ESP32!';
        };

        xhr.send(formData);
      }, 'image/jpeg', 0.9);
    };

    const toggleColorOrder = () => {
      fetch('/toggle_color')
        .then(r => r.text())
        .then(txt => {
          statusMsg.style.color = '#38bdf8';
          statusMsg.innerText = 'Đã chuyển sang hệ màu: ' + txt;
        });
    };
  </script>
</body>
</html>
)rawliteral";

#endif
