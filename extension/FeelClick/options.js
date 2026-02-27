document.addEventListener('DOMContentLoaded', () => {
    const els = {
        enabled: document.getElementById('opt-enabled'),
        mode: document.querySelectorAll('input[name="opt-mode"]'),
        amount: document.getElementById('opt-amount'),
        amountVal: document.getElementById('opt-amount-val'),
        width: document.getElementById('opt-width'),
        height: document.getElementById('opt-height'),
        locked: document.getElementById('opt-locked'),
        sound: document.getElementById('opt-sound'),
        reset: document.getElementById('opt-reset'),
        save: document.getElementById('opt-save'),
        status: document.getElementById('status-msg')
    };

    // Load
    chrome.storage.local.get(null, (r) => {
        if (r.enabled !== undefined) els.enabled.checked = r.enabled;
        if (r.mode) {
            const radio = document.querySelector(`input[name="opt-mode"][value="${r.mode}"]`);
            if (radio) radio.checked = true;
        }
        if (r.scrollAmount) { els.amount.value = r.scrollAmount; els.amountVal.textContent = r.scrollAmount + 'px'; }
        if (r.zoneWidth) els.width.value = r.zoneWidth;
        if (r.zoneHeight) els.height.value = r.zoneHeight;
        if (r.locked !== undefined) els.locked.checked = r.locked;
        if (r.soundEnabled !== undefined) els.sound.checked = r.soundEnabled;
    });

    // Slider live update
    els.amount.addEventListener('input', () => {
        els.amountVal.textContent = els.amount.value + 'px';
    });

    // Save
    els.save.addEventListener('click', () => {
        const mode = document.querySelector('input[name="opt-mode"]:checked').value;
        const data = {
            enabled: els.enabled.checked,
            mode: mode,
            scrollAmount: parseInt(els.amount.value),
            zoneWidth: parseInt(els.width.value),
            zoneHeight: parseInt(els.height.value),
            locked: els.locked.checked,
            soundEnabled: els.sound.checked
        };
        chrome.storage.local.set(data, () => {
            showStatus('✅ Settings saved!');
            // Update active tabs
            chrome.tabs.query({}, (tabs) => {
                tabs.forEach(tab => {
                    chrome.tabs.sendMessage(tab.id, { type: 'UPDATE_SETTINGS', payload: data }).catch(() => { });
                });
            });
        });
    });

    // Reset
    els.reset.addEventListener('click', () => {
        const defaults = {
            enabled: true, mode: '1', scrollAmount: 300,
            zoneWidth: 130, zoneHeight: 190, locked: false, soundEnabled: true,
            zoneTop: '100px', zoneRight: '20px'
        };
        chrome.storage.local.set(defaults, () => {
            // Reload page with defaults
            location.reload();
        });
    });

    function showStatus(msg) {
        els.status.textContent = msg;
        els.status.classList.add('show');
        setTimeout(() => els.status.classList.remove('show'), 2000);
    }
});
