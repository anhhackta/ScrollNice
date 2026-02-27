// ═══════════════════════════════════════════════════════
// ScrollNice — Content Script (v1.0)
// ═══════════════════════════════════════════════════════

let settings = {
    enabled: true,
    mode: '1',           // 1-5
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
let isDragging = false;
let isResizing = false;
let dragOffset = { x: 0, y: 0 };
let resizeStart = { x: 0, y: 0, w: 0, h: 0 };

// ─── Audio Context for Click Sound ───
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
        osc.frequency.setValueAtTime(800, audioCtx.currentTime);
        osc.frequency.exponentialRampToValueAtTime(400, audioCtx.currentTime + 0.08);
        gain.gain.setValueAtTime(0.15, audioCtx.currentTime);
        gain.gain.exponentialRampToValueAtTime(0.001, audioCtx.currentTime + 0.1);
        osc.start(audioCtx.currentTime);
        osc.stop(audioCtx.currentTime + 0.1);
    } catch (e) { /* silent fail */ }
}

// ─── Initialize ───
chrome.storage.local.get(null, (result) => {
    settings = { ...settings, ...result };
    if (settings.enabled) createZone();
});

chrome.runtime.onMessage.addListener((message) => {
    if (message.type === 'UPDATE_SETTINGS') {
        settings = { ...settings, ...message.payload };
        if (settings.enabled) {
            if (!document.getElementById('scrollnice-zone')) {
                createZone();
            } else {
                updateZoneUI();
            }
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

    if (settings.zoneRight !== 'auto') {
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
    lockBtn.title = 'Toggle Lock';
    lockBtn.addEventListener('mousedown', (e) => e.stopPropagation());
    lockBtn.addEventListener('click', toggleLock);
    zone.appendChild(lockBtn);

    // Mode indicator
    const modeIndicator = document.createElement('div');
    modeIndicator.className = 'mode-indicator';
    modeIndicator.innerText = getModeLabel(settings.mode);
    zone.appendChild(modeIndicator);

    // Resize handle
    const resizeHandle = document.createElement('div');
    resizeHandle.className = 'resize-handle';
    resizeHandle.addEventListener('mousedown', startResize);
    zone.appendChild(resizeHandle);

    // Split line & half labels
    updateSplitVisuals(zone);

    document.body.appendChild(zone);

    // Events
    zone.addEventListener('mousedown', startDrag);
    zone.addEventListener('contextmenu', (e) => e.preventDefault());
    zone.addEventListener('mousedown', handleMouseDown);
    zone.addEventListener('mouseup', handleMouseUp);
    zone.addEventListener('mouseleave', stopContinuousScroll);
}

function removeZone() {
    const zone = document.getElementById('scrollnice-zone');
    if (zone) zone.remove();
}

function updateZoneUI() {
    const zone = document.getElementById('scrollnice-zone');
    if (!zone) return;

    const indicator = zone.querySelector('.mode-indicator');
    if (indicator) indicator.innerText = getModeLabel(settings.mode);

    zone.style.width = settings.zoneWidth + 'px';
    zone.style.height = settings.zoneHeight + 'px';

    if (settings.coverImage) {
        zone.style.backgroundImage = `url(${settings.coverImage})`;
        zone.classList.add('has-cover');
    } else {
        zone.style.backgroundImage = '';
        zone.classList.remove('has-cover');
    }

    // Update split visuals
    zone.querySelectorAll('.split-line, .half-label').forEach(el => el.remove());
    updateSplitVisuals(zone);
}

function updateSplitVisuals(zone) {
    const mode = settings.mode;

    if (mode === '4') {
        // Split Top/Bottom
        const line = document.createElement('div');
        line.className = 'split-line horizontal';
        zone.appendChild(line);

        const topLabel = document.createElement('div');
        topLabel.className = 'half-label top-half';
        topLabel.innerText = '▲';
        zone.appendChild(topLabel);

        const botLabel = document.createElement('div');
        botLabel.className = 'half-label bottom-half';
        botLabel.innerText = '▼';
        zone.appendChild(botLabel);
    } else if (mode === '3') {
        // Split Left/Right
        const line = document.createElement('div');
        line.className = 'split-line vertical';
        zone.appendChild(line);

        const leftLabel = document.createElement('div');
        leftLabel.className = 'half-label left-half';
        leftLabel.innerText = '▲';
        zone.appendChild(leftLabel);

        const rightLabel = document.createElement('div');
        rightLabel.className = 'half-label right-half';
        rightLabel.innerText = '▼';
        zone.appendChild(rightLabel);
    }
}

function getModeLabel(mode) {
    const labels = {
        '1': 'L↑ R↓',
        '2': 'L↓ R↑',
        '3': 'Split L/R',
        '4': 'Split T/B',
        '5': 'Hold'
    };
    return labels[mode] || `Mode ${mode}`;
}

// ─── Lock ───
function toggleLock() {
    settings.locked = !settings.locked;
    chrome.storage.local.set({ locked: settings.locked });
    const zone = document.getElementById('scrollnice-zone');
    const lockBtn = zone.querySelector('.lock-btn');
    zone.classList.toggle('locked', settings.locked);
    lockBtn.innerHTML = settings.locked ? '🔒' : '🔓';
}

// ─── Drag ───
function startDrag(e) {
    if (settings.locked || isResizing) return;
    if (e.button !== 0 || e.target.closest('.lock-btn') || e.target.closest('.resize-handle')) return;

    isDragging = true;
    const zone = document.getElementById('scrollnice-zone');
    const rect = zone.getBoundingClientRect();
    dragOffset.x = e.clientX - rect.left;
    dragOffset.y = e.clientY - rect.top;

    document.addEventListener('mousemove', onDrag);
    document.addEventListener('mouseup', stopDrag);
}

function onDrag(e) {
    if (!isDragging) return;
    const zone = document.getElementById('scrollnice-zone');
    let newTop = Math.max(0, Math.min(e.clientY - dragOffset.y, window.innerHeight - zone.offsetHeight));
    let newLeft = Math.max(0, Math.min(e.clientX - dragOffset.x, window.innerWidth - zone.offsetWidth));
    zone.style.top = `${newTop}px`;
    zone.style.left = `${newLeft}px`;
    zone.style.right = 'auto';
}

function stopDrag() {
    if (!isDragging) return;
    isDragging = false;
    document.removeEventListener('mousemove', onDrag);
    document.removeEventListener('mouseup', stopDrag);

    const zone = document.getElementById('scrollnice-zone');
    settings.zoneTop = zone.style.top;
    settings.zoneLeft = zone.style.left;
    settings.zoneRight = 'auto';
    chrome.storage.local.set({
        zoneTop: settings.zoneTop,
        zoneLeft: settings.zoneLeft,
        zoneRight: 'auto'
    });
}

// ─── Resize ───
function startResize(e) {
    e.stopPropagation();
    e.preventDefault();
    isResizing = true;
    const zone = document.getElementById('scrollnice-zone');
    resizeStart = {
        x: e.clientX,
        y: e.clientY,
        w: zone.offsetWidth,
        h: zone.offsetHeight
    };
    document.addEventListener('mousemove', onResize);
    document.addEventListener('mouseup', stopResize);
}

function onResize(e) {
    if (!isResizing) return;
    const zone = document.getElementById('scrollnice-zone');
    let nw = Math.max(60, resizeStart.w + (e.clientX - resizeStart.x));
    let nh = Math.max(60, resizeStart.h + (e.clientY - resizeStart.y));
    zone.style.width = `${nw}px`;
    zone.style.height = `${nh}px`;
}

function stopResize() {
    isResizing = false;
    document.removeEventListener('mousemove', onResize);
    document.removeEventListener('mouseup', stopResize);

    const zone = document.getElementById('scrollnice-zone');
    settings.zoneWidth = zone.offsetWidth;
    settings.zoneHeight = zone.offsetHeight;
    chrome.storage.local.set({
        zoneWidth: settings.zoneWidth,
        zoneHeight: settings.zoneHeight
    });
}

// ─── Reset Position ───
function resetPosition() {
    settings.zoneTop = '100px';
    settings.zoneRight = '20px';
    settings.zoneLeft = 'auto';
    settings.zoneWidth = 130;
    settings.zoneHeight = 190;
    chrome.storage.local.set({
        zoneTop: '100px',
        zoneRight: '20px',
        zoneWidth: 130,
        zoneHeight: 190
    });
    const zone = document.getElementById('scrollnice-zone');
    if (zone) {
        zone.style.top = '100px';
        zone.style.right = '20px';
        zone.style.left = 'auto';
        zone.style.width = '130px';
        zone.style.height = '190px';
    }
}

// ─── Ripple Effect ───
function createRipple(zone, e) {
    const rect = zone.getBoundingClientRect();
    const ripple = document.createElement('div');
    ripple.className = 'ripple';
    ripple.style.left = (e.clientX - rect.left) + 'px';
    ripple.style.top = (e.clientY - rect.top) + 'px';
    zone.appendChild(ripple);
    setTimeout(() => ripple.remove(), 400);
}

// ─── Scroll Arrow Animation ───
function showScrollArrow(zone, direction) {
    const arrow = document.createElement('div');
    arrow.className = 'scroll-arrow ' + (direction > 0 ? 'up' : 'down');
    arrow.innerText = direction > 0 ? '▲' : '▼';
    zone.appendChild(arrow);
    setTimeout(() => arrow.remove(), 500);
}

// ─── Scrolling Logic ───
function handleMouseDown(e) {
    if (isDragging || isResizing) return;
    if (e.target.closest('.lock-btn') || e.target.closest('.resize-handle')) return;
    e.preventDefault();

    const mode = settings.mode;
    const zone = document.getElementById('scrollnice-zone');
    const rect = zone.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;
    const width = rect.width;
    const height = rect.height;

    // Ripple effect
    createRipple(zone, e);

    if (mode === '5') {
        // Continuous Scroll
        if (e.button === 0) {
            startContinuousScroll(settings.scrollAmount / 30); // scale speed
            showScrollArrow(zone, -1);
        } else if (e.button === 2) {
            startContinuousScroll(-settings.scrollAmount / 30);
            showScrollArrow(zone, 1);
        }
        playClickSound();
        return;
    }

    let scrollAmount = settings.scrollAmount;
    let direction = 0;

    if (mode === '1') {
        direction = (e.button === 0) ? -1 : 1;
    } else if (mode === '2') {
        direction = (e.button === 0) ? 1 : -1;
    } else if (mode === '3') {
        direction = (x < width / 2) ? -1 : 1;
    } else if (mode === '4') {
        direction = (y < height / 2) ? -1 : 1;
    }

    if (direction !== 0) {
        window.scrollBy({ top: direction * scrollAmount, behavior: 'smooth' });
        showScrollArrow(zone, -direction);
        playClickSound();
    }
}

function handleMouseUp(e) {
    if (settings.mode === '5') {
        stopContinuousScroll();
    }
}

function startContinuousScroll(speed) {
    if (scrollInterval) clearInterval(scrollInterval);
    scrollInterval = setInterval(() => {
        window.scrollBy(0, speed);
    }, 16);
}

function stopContinuousScroll() {
    if (scrollInterval) {
        clearInterval(scrollInterval);
        scrollInterval = null;
    }
}
