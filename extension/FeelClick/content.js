// ═══════════════════════════════════════════════════════
// ScrollNice — Content Script (v1.1 — 3 Mode)
// Mode 1: Click/Hold L↑ R↓
// Mode 2: Click/Hold Top↑ Bottom↓ (zone split)
// Mode 3: Hover Top↑ Bottom↓ (no click needed)
// ═══════════════════════════════════════════════════════

let settings = {
    enabled: true,
    mode: '1',           // '1', '2', '3'
    scrollAmount: 300,   // px per click
    soundEnabled: true,
    zoneTop: '100px',
    zoneRight: '20px',
    zoneLeft: 'auto',
    zoneWidth: 130,
    zoneHeight: 190,
    locked: false,
    coverImage: ''
};

let scrollInterval = null;
let holdTimer = null;
let hoverScrollInterval = null;
let isDragging = false;
let isResizing = false;
let isHolding = false;
let dragOffset = { x: 0, y: 0 };
let resizeStart = { x: 0, y: 0, w: 0, h: 0 };

// ─── Web Audio Click Sound ───
let audioCtx = null;
function playClickSound() {
    if (!settings.soundEnabled) return;
    try {
        if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
        const osc = audioCtx.createOscillator();
        const gain = audioCtx.createGain();
        osc.connect(gain);
        gain.connect(audioCtx.destination);
        osc.type = 'sine';
        osc.frequency.setValueAtTime(700, audioCtx.currentTime);
        osc.frequency.exponentialRampToValueAtTime(350, audioCtx.currentTime + 0.09);
        gain.gain.setValueAtTime(0.12, audioCtx.currentTime);
        gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + 0.1);
        osc.start(audioCtx.currentTime);
        osc.stop(audioCtx.currentTime + 0.1);
    } catch (e) { /* silent */ }
}

// ─── Init ───
chrome.storage.local.get(null, (result) => {
    settings = { ...settings, ...result };
    if (settings.enabled) createZone();
});

chrome.runtime.onMessage.addListener((message) => {
    if (message.type === 'UPDATE_SETTINGS') {
        settings = { ...settings, ...message.payload };
        if (settings.enabled) {
            if (!document.getElementById('scrollnice-zone')) createZone();
            else updateZoneUI();
        } else {
            removeZone();
        }
    } else if (message.type === 'RESET_POSITION') {
        resetPosition();
    }
});

// ─── Create Zone ───
function createZone() {
    if (document.getElementById('scrollnice-zone')) return;

    const zone = document.createElement('div');
    zone.id = 'scrollnice-zone';
    zone.style.top = settings.zoneTop;
    zone.style.width = settings.zoneWidth + 'px';
    zone.style.height = settings.zoneHeight + 'px';

    if (settings.zoneRight !== 'auto' && settings.zoneLeft === 'auto') {
        zone.style.right = settings.zoneRight;
    } else {
        zone.style.left = settings.zoneLeft;
    }

    if (settings.locked) zone.classList.add('locked');
    if (settings.coverImage) {
        zone.style.backgroundImage = `url(${settings.coverImage})`;
        zone.classList.add('has-cover');
    }

    // Lock button
    const lockBtn = document.createElement('div');
    lockBtn.className = 'lock-btn';
    lockBtn.innerHTML = settings.locked ? '🔒' : '🔓';
    lockBtn.title = 'Khóa / Mở khóa vị trí';
    lockBtn.addEventListener('mousedown', (e) => e.stopPropagation());
    lockBtn.addEventListener('click', toggleLock);
    zone.appendChild(lockBtn);

    // Mode visuals
    updateZoneModeVisuals(zone);

    // Resize handle
    const resizeHandle = document.createElement('div');
    resizeHandle.className = 'resize-handle';
    resizeHandle.addEventListener('mousedown', startResize);
    zone.appendChild(resizeHandle);

    document.body.appendChild(zone);

    // Events
    zone.addEventListener('mousedown', onMouseDown);
    zone.addEventListener('mouseup', onMouseUp);
    zone.addEventListener('mousemove', onMouseMove);
    zone.addEventListener('mouseleave', onMouseLeave);
    zone.addEventListener('contextmenu', (e) => e.preventDefault());
}

function removeZone() {
    stopAll();
    const zone = document.getElementById('scrollnice-zone');
    if (zone) zone.remove();
}

function updateZoneUI() {
    const zone = document.getElementById('scrollnice-zone');
    if (!zone) return;
    zone.style.width = settings.zoneWidth + 'px';
    zone.style.height = settings.zoneHeight + 'px';

    if (settings.coverImage) {
        zone.style.backgroundImage = `url(${settings.coverImage})`;
        zone.classList.add('has-cover');
    } else {
        zone.style.backgroundImage = '';
        zone.classList.remove('has-cover');
    }

    // Rebuild mode visuals
    zone.querySelectorAll('.mode-visual, .split-line, .half-label').forEach(el => el.remove());
    updateZoneModeVisuals(zone);
}

function updateZoneModeVisuals(zone) {
    // Clear existing
    zone.querySelectorAll('.mode-visual, .split-line, .half-top, .half-bot, .mode-label').forEach(el => el.remove());

    const mode = String(settings.mode);

    if (mode === '1') {
        // Mode 1: show ▲ / ▼ symbols, L↑ R↓ label
        const topArrow = el('div', 'mode-visual', '▲');
        topArrow.style.cssText = 'position:absolute;top:28%;left:50%;transform:translate(-50%,-50%);font-size:22px;opacity:0.5;pointer-events:none;color:#fff';
        zone.appendChild(topArrow);

        const botArrow = el('div', 'mode-visual', '▼');
        botArrow.style.cssText = 'position:absolute;top:72%;left:50%;transform:translate(-50%,-50%);font-size:22px;opacity:0.5;pointer-events:none;color:#fff';
        zone.appendChild(botArrow);

        const lbl = el('div', 'mode-label', 'L↑  R↓');
        lbl.style.cssText = 'position:absolute;bottom:5px;left:0;right:0;text-align:center;font-size:10px;color:rgba(255,255,255,0.5);pointer-events:none;font-weight:600;letter-spacing:1px';
        zone.appendChild(lbl);

    } else if (mode === '2') {
        // Mode 2: split line + top/bottom labels
        const line = el('div', 'split-line');
        line.style.cssText = 'position:absolute;left:8%;right:8%;top:50%;height:1px;background:rgba(255,255,255,0.35);pointer-events:none';
        zone.appendChild(line);

        const topLbl = el('div', 'half-top', '▲ Lên');
        topLbl.style.cssText = 'position:absolute;top:22%;left:0;right:0;text-align:center;font-size:13px;font-weight:700;color:rgba(255,255,255,0.7);pointer-events:none';
        zone.appendChild(topLbl);

        const botLbl = el('div', 'half-bot', '▼ Xuống');
        botLbl.style.cssText = 'position:absolute;top:62%;left:0;right:0;text-align:center;font-size:13px;font-weight:700;color:rgba(255,255,255,0.7);pointer-events:none';
        zone.appendChild(botLbl);

    } else if (mode === '3') {
        // Mode 3: hover zones, colored halves + labels
        const topZone = el('div', 'half-top');
        topZone.style.cssText = 'position:absolute;top:4px;left:4px;right:4px;bottom:calc(50% + 1px);background:rgba(52,152,219,0.25);border-radius:12px 12px 0 0;pointer-events:none;display:flex;align-items:center;justify-content:center;';
        topZone.innerHTML = '<span style="font-size:13px;font-weight:700;color:rgba(255,255,255,0.8);">▲ Di chuột</span>';
        zone.appendChild(topZone);

        const botZone = el('div', 'half-bot');
        botZone.style.cssText = 'position:absolute;bottom:4px;left:4px;right:4px;top:calc(50% + 1px);background:rgba(220,80,80,0.25);border-radius:0 0 12px 12px;pointer-events:none;display:flex;align-items:center;justify-content:center;';
        botZone.innerHTML = '<span style="font-size:13px;font-weight:700;color:rgba(255,255,255,0.8);">▼ Di chuột</span>';
        zone.appendChild(botZone);

        const line = el('div', 'split-line');
        line.style.cssText = 'position:absolute;left:8%;right:8%;top:50%;height:1px;background:rgba(255,255,255,0.3);pointer-events:none;z-index:2';
        zone.appendChild(line);
    }
}

function el(tag, cls = '', text = '') {
    const e = document.createElement(tag);
    if (cls) e.className = cls;
    if (text) e.textContent = text;
    return e;
}

// ─── Lock ───
function toggleLock() {
    settings.locked = !settings.locked;
    chrome.storage.local.set({ locked: settings.locked });
    const zone = document.getElementById('scrollnice-zone');
    const lockBtn = zone ? zone.querySelector('.lock-btn') : null;
    zone?.classList.toggle('locked', settings.locked);
    if (lockBtn) lockBtn.innerHTML = settings.locked ? '🔒' : '🔓';
}

// ─── Drag ───
function startDrag(e) {
    if (settings.locked || isResizing) return;
    if (e.button !== 0) return;
    isDragging = true;
    const zone = document.getElementById('scrollnice-zone');
    const rect = zone.getBoundingClientRect();
    dragOffset.x = e.clientX - rect.left;
    dragOffset.y = e.clientY - rect.top;
    document.addEventListener('mousemove', onDragGlobal);
    document.addEventListener('mouseup', stopDrag);
}

function onDragGlobal(e) {
    if (!isDragging) return;
    const zone = document.getElementById('scrollnice-zone');
    const newTop = Math.max(0, Math.min(e.clientY - dragOffset.y, window.innerHeight - zone.offsetHeight));
    const newLeft = Math.max(0, Math.min(e.clientX - dragOffset.x, window.innerWidth - zone.offsetWidth));
    zone.style.top = newTop + 'px';
    zone.style.left = newLeft + 'px';
    zone.style.right = 'auto';
}

function stopDrag() {
    if (!isDragging) return;
    isDragging = false;
    document.removeEventListener('mousemove', onDragGlobal);
    document.removeEventListener('mouseup', stopDrag);
    const zone = document.getElementById('scrollnice-zone');
    if (zone) {
        settings.zoneTop = zone.style.top;
        settings.zoneLeft = zone.style.left;
        settings.zoneRight = 'auto';
        chrome.storage.local.set({ zoneTop: settings.zoneTop, zoneLeft: settings.zoneLeft, zoneRight: 'auto' });
    }
}

// ─── Resize ───
function startResize(e) {
    e.stopPropagation(); e.preventDefault();
    isResizing = true;
    const zone = document.getElementById('scrollnice-zone');
    resizeStart = { x: e.clientX, y: e.clientY, w: zone.offsetWidth, h: zone.offsetHeight };
    document.addEventListener('mousemove', onResizeGlobal);
    document.addEventListener('mouseup', stopResize);
}

function onResizeGlobal(e) {
    if (!isResizing) return;
    const zone = document.getElementById('scrollnice-zone');
    const nw = Math.max(60, resizeStart.w + (e.clientX - resizeStart.x));
    const nh = Math.max(60, resizeStart.h + (e.clientY - resizeStart.y));
    zone.style.width = nw + 'px';
    zone.style.height = nh + 'px';
}

function stopResize() {
    isResizing = false;
    document.removeEventListener('mousemove', onResizeGlobal);
    document.removeEventListener('mouseup', stopResize);
    const zone = document.getElementById('scrollnice-zone');
    if (zone) {
        settings.zoneWidth = zone.offsetWidth;
        settings.zoneHeight = zone.offsetHeight;
        chrome.storage.local.set({ zoneWidth: settings.zoneWidth, zoneHeight: settings.zoneHeight });
    }
}

// ─── Reset ───
function resetPosition() {
    const zone = document.getElementById('scrollnice-zone');
    if (zone) {
        zone.style.top = '100px';
        zone.style.right = '20px';
        zone.style.left = 'auto';
        zone.style.width = '130px';
        zone.style.height = '190px';
    }
    const d = { zoneTop: '100px', zoneRight: '20px', zoneLeft: 'auto', zoneWidth: 130, zoneHeight: 190 };
    Object.assign(settings, d);
    chrome.storage.local.set(d);
}

// ─── Ripple ───
function createRipple(zone, e) {
    const rect = zone.getBoundingClientRect();
    const ripple = document.createElement('div');
    ripple.className = 'ripple';
    ripple.style.left = (e.clientX - rect.left) + 'px';
    ripple.style.top = (e.clientY - rect.top) + 'px';
    zone.appendChild(ripple);
    setTimeout(() => ripple.remove(), 400);
}

// ─── Scroll utilities ───
function scrollPage(direction) {
    // direction: 1=up, -1=down
    window.scrollBy({ top: -direction * settings.scrollAmount, behavior: 'smooth' });
}

function startHoldScroll(direction) {
    if (scrollInterval) clearInterval(scrollInterval);
    const baseSpeed = 6;
    let speed = baseSpeed;
    let holdTime = 0;
    scrollInterval = setInterval(() => {
        holdTime += 0.016;
        speed = Math.min(baseSpeed + holdTime * 3, 30);
        window.scrollBy(0, -direction * speed);
    }, 16);
}

function stopAll() {
    if (scrollInterval) { clearInterval(scrollInterval); scrollInterval = null; }
    if (hoverScrollInterval) { clearInterval(hoverScrollInterval); hoverScrollInterval = null; }
    isHolding = false;
}

// ─── Event handlers ───
function onMouseDown(e) {
    const zone = document.getElementById('scrollnice-zone');
    const mode = String(settings.mode);

    // Lock btn & resize handle bypass
    if (e.target.closest('.lock-btn') || e.target.closest('.resize-handle')) return;

    // Drag: any non-locked left-click starts drag potential
    if (e.button === 0 && !settings.locked) {
        startDrag(e);
    }

    // Mode 3: no click interaction
    if (mode === '3') return;

    e.preventDefault();

    const rect = zone.getBoundingClientRect();
    const y = e.clientY - rect.top;
    const h = rect.height;

    let direction = 0;
    if (mode === '1') {
        direction = (e.button === 0) ? 1 : -1;
    } else if (mode === '2') {
        direction = (y < h / 2) ? 1 : -1;
    }

    if (direction !== 0) {
        createRipple(zone, e);
        playClickSound();
        scrollPage(direction);
        // Start hold scroll after 250ms delay
        isHolding = true;
        if (scrollInterval) clearInterval(scrollInterval);
        holdTimer = setTimeout(() => {
            if (isHolding) startHoldScroll(direction);
        }, 250);
    }
}

function onMouseUp(e) {
    if (holdTimer) { clearTimeout(holdTimer); holdTimer = null; }
    stopAll();
}

function onMouseMove(e) {
    const mode = String(settings.mode);
    if (mode !== '3') return;

    const zone = document.getElementById('scrollnice-zone');
    if (!zone) return;
    const rect = zone.getBoundingClientRect();
    const y = e.clientY - rect.top;
    const h = rect.height;
    const dir = (y < h / 2) ? 1 : -1;

    // Change hover scroll direction
    if (hoverScrollInterval) {
        clearInterval(hoverScrollInterval);
        hoverScrollInterval = null;
    }
    hoverScrollInterval = setInterval(() => {
        window.scrollBy(0, -dir * 6);
    }, 16);
}

function onMouseLeave(e) {
    // Stop hover scroll when leaving zone (Mode 3)
    if (hoverScrollInterval) { clearInterval(hoverScrollInterval); hoverScrollInterval = null; }
    if (holdTimer) { clearTimeout(holdTimer); holdTimer = null; }
    stopAll();
}
