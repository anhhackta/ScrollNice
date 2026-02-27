document.addEventListener('DOMContentLoaded', () => {
    const enableBox = document.getElementById('enable-ext');
    const modeRadios = document.querySelectorAll('input[name="mode"]');
    const scrollSlider = document.getElementById('scroll-amount');
    const scrollVal = document.getElementById('scroll-amount-val');
    const soundToggle = document.getElementById('sound-toggle');
    const resetBtn = document.getElementById('reset-position');
    const optionsBtn = document.getElementById('open-options');

    // Load
    chrome.storage.local.get(['enabled', 'mode', 'scrollAmount', 'soundEnabled'], (r) => {
        if (r.enabled !== undefined) enableBox.checked = r.enabled;
        if (r.mode) {
            const radio = document.querySelector(`input[name="mode"][value="${r.mode}"]`);
            if (radio) radio.checked = true;
        }
        if (r.scrollAmount) {
            scrollSlider.value = r.scrollAmount;
            scrollVal.textContent = r.scrollAmount + 'px';
        }
        if (r.soundEnabled !== undefined) soundToggle.checked = r.soundEnabled;
    });

    // Enable toggle
    enableBox.addEventListener('change', () => {
        const enabled = enableBox.checked;
        chrome.storage.local.set({ enabled }, () => {
            sendMsg({ type: 'UPDATE_SETTINGS', payload: { enabled } });
        });
    });

    // Mode
    modeRadios.forEach(radio => {
        radio.addEventListener('change', () => {
            const mode = document.querySelector('input[name="mode"]:checked').value;
            chrome.storage.local.set({ mode }, () => {
                sendMsg({ type: 'UPDATE_SETTINGS', payload: { mode } });
            });
        });
    });

    // Sensitivity
    scrollSlider.addEventListener('input', () => {
        const val = parseInt(scrollSlider.value);
        scrollVal.textContent = val + 'px';
        chrome.storage.local.set({ scrollAmount: val }, () => {
            sendMsg({ type: 'UPDATE_SETTINGS', payload: { scrollAmount: val } });
        });
    });

    // Sound
    soundToggle.addEventListener('change', () => {
        const soundEnabled = soundToggle.checked;
        chrome.storage.local.set({ soundEnabled }, () => {
            sendMsg({ type: 'UPDATE_SETTINGS', payload: { soundEnabled } });
        });
    });

    // Reset
    resetBtn.addEventListener('click', () => {
        sendMsg({ type: 'RESET_POSITION' });
    });

    // Options
    optionsBtn.addEventListener('click', () => {
        chrome.runtime.openOptionsPage();
    });

    function sendMsg(message) {
        chrome.tabs.query({ active: true, currentWindow: true }, (tabs) => {
            if (tabs[0]) chrome.tabs.sendMessage(tabs[0].id, message);
        });
    }
});
