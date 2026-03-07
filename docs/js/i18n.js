/**
 * ScrollNice i18n System v2
 * - IP geolocation auto-detect
 * - Glass dropdown language switcher (flag + name + chevron)
 * - Dark/Light theme toggle
 */

const TRANSLATIONS = {
    vi: {
        nav_features: 'Tính năng', nav_modes: 'Chế độ', nav_download: 'Tải về',
        nav_docs: 'Hướng dẫn', nav_changelog: 'Changelog', nav_support: 'Ủng hộ',
        nav_cta: '⬇ Tải về miễn phí',
        hero_badge: '✨ Miễn phí & Mã nguồn mở',
        hero_h1: 'Lăn Chuột Bằng <span class="gradient">Click</span>,<br>Không Cần Wheel',
        hero_p: 'Zone nổi always-on-top — click, giữ, hoặc chỉ di chuột để lăn trang.<br>Hoàn hảo khi wheel hỏng, dùng một tay, hoặc muốn trải nghiệm cuộn trang mới.',
        hero_btn_win: '⬇ Tải về Windows', hero_btn_ext: '🌐 Chrome Extension',
        features_title: 'Tại sao dùng ScrollNice?',
        feat1_title: 'Click / Giữ / Di chuột để lăn', feat1_p: 'Zone nổi phản hồi ngay — 3 chế độ linh hoạt, không cần wheel.',
        feat2_title: 'Siêu nhẹ', feat2_p: '≤500KB. CPU ≈ 0% idle. Không telemetry, không quảng cáo.',
        feat3_title: 'Tùy chỉnh', feat3_p: 'Resize, di chuyển, chỉnh độ trong suốt, đặt ảnh bìa, điều chỉnh độ nhạy.',
        feat4_title: 'Âm thanh click', feat4_p: 'Phản hồi âm thanh mỗi lần click. Dùng âm tích hợp hoặc tự upload file WAV.',
        feat5_title: 'Phím tắt', feat5_p: 'Ctrl+Alt+S bật/tắt. Ctrl+Alt+E chỉnh sửa. Hoàn toàn tuỳ chỉnh.',
        feat6_title: 'Chặn Wheel', feat6_p: 'Tùy chọn chặn sự kiện wheel vật lý — hoàn hảo cho wheel hỏng.',
        modes_title: '3 Chế Độ Lăn', modes_sub: 'Chọn chế độ phù hợp với thói quen của bạn',
        mode1_title: 'Mode 1: Click / Giữ',
        mode1_p: '<strong>Click trái</strong> = lăn lên ↑ &nbsp; <strong>Click phải</strong> = lăn xuống ↓<br><em>Giữ chuột → lăn liên tục, tốc độ tăng dần</em>',
        mode2_title: 'Mode 2: Chia Trên / Dưới',
        mode2_p: 'Zone chia 2 nửa.<br><strong>Click/Giữ nửa trên</strong> = lăn lên ↑<br><strong>Click/Giữ nửa dưới</strong> = lăn xuống ↓',
        mode3_title: 'Mode 3: Di Chuột Tự Động',
        mode3_p: '<strong>Không cần click!</strong><br>Di chuột vào nửa trên = tự lăn lên ↑<br>Di chuột vào nửa dưới = tự lăn xuống ↓',
        download_title: 'Tải về ScrollNice', download_sub: 'Miễn phí mãi mãi. Không cần đăng ký.',
        dl_win_title: '🖥️ Ứng dụng Windows', dl_win_p: 'File .exe portable — hoạt động với mọi ứng dụng trên hệ thống. Dưới 500KB.',
        dl_win_btn: 'Tải về (.zip)',
        dl_ext_title: '🌐 Chrome Extension', dl_ext_p: 'Phiên bản trình duyệt — load unpacked không cần cài đặt chính thức.',
        dl_ext_btn: 'Xem trên GitHub',
        donation_title: '❤️ Ủng hộ dự án',
        donation_sub: 'ScrollNice hoàn toàn miễn phí. Nếu tiện ích giúp ích cho bạn, hãy cân nhắc ủng hộ để dự án tiếp tục phát triển!',
        donation_kofi: 'Ko-fi', donation_paypal: 'PayPal',
        footer_text: 'ScrollNice — Made with ❤️ | Không telemetry · Không quảng cáo · 100% miễn phí',
        supporters_label: '🤝 Đồng hành cùng dự án',
    },
    en: {
        nav_features: 'Features', nav_modes: 'Modes', nav_download: 'Download',
        nav_docs: 'Docs', nav_changelog: 'Changelog', nav_support: 'Support',
        nav_cta: '⬇ Download Free',
        hero_badge: '✨ Free & Open Source',
        hero_h1: 'Scroll by <span class="gradient">Clicking</span>,<br>No Wheel Needed',
        hero_p: 'A floating always-on-top zone — click, hold, or just hover to scroll.<br>Perfect for broken wheels, one-hand workflows, or anyone who wants a better scroll experience.',
        hero_btn_win: '⬇ Download for Windows', hero_btn_ext: '🌐 Chrome Extension',
        features_title: 'Why ScrollNice?',
        feat1_title: 'Click / Hold / Hover to Scroll', feat1_p: 'Floating zone responds instantly — 3 flexible modes, no wheel required.',
        feat2_title: 'Lightweight', feat2_p: '≤500KB. CPU ≈ 0% idle. No telemetry, no ads, no bloat.',
        feat3_title: 'Customizable', feat3_p: 'Resize, reposition, adjust opacity, set cover image, tune sensitivity.',
        feat4_title: 'Click Sounds', feat4_p: 'Audio feedback on every click. Use built-in sounds or load your own WAV file.',
        feat5_title: 'Hotkeys', feat5_p: 'Ctrl+Alt+S toggle. Ctrl+Alt+E edit mode. Fully configurable.',
        feat6_title: 'Wheel Block', feat6_p: 'Optionally block physical wheel events — perfect for broken scroll wheels.',
        modes_title: '3 Scroll Modes', modes_sub: 'Choose the mode that fits your workflow',
        mode1_title: 'Mode 1: Click / Hold',
        mode1_p: '<strong>Left click</strong> = scroll up ↑ &nbsp; <strong>Right click</strong> = scroll down ↓<br><em>Hold mouse button for continuous scroll with acceleration</em>',
        mode2_title: 'Mode 2: Top / Bottom Split',
        mode2_p: 'Zone splits in half.<br><strong>Click/Hold top half</strong> = scroll up ↑<br><strong>Click/Hold bottom half</strong> = scroll down ↓',
        mode3_title: 'Mode 3: Hover Auto-Scroll',
        mode3_p: '<strong>No click needed!</strong><br>Hover top half = auto scroll up ↑<br>Hover bottom half = auto scroll down ↓',
        download_title: 'Get ScrollNice', download_sub: 'Free forever. No sign-up required.',
        dl_win_title: '🖥️ Windows App', dl_win_p: 'Portable .exe — works system-wide with any app. Under 500KB.',
        dl_win_btn: 'Download (.zip)',
        dl_ext_title: '🌐 Chrome Extension', dl_ext_p: 'Browser version — load as unpacked extension, no official install needed.',
        dl_ext_btn: 'View on GitHub',
        donation_title: '❤️ Support the Project',
        donation_sub: 'ScrollNice is completely free. If it helps you, consider buying a coffee to keep development going!',
        donation_kofi: 'Ko-fi', donation_paypal: 'PayPal',
        footer_text: 'ScrollNice — Made with ❤️ | No telemetry · No ads · 100% free',
        supporters_label: '🤝 Project Supporters',
    }
};

// Lang metadata for dropdown
const LANG_META = {
    vi: { flag: '🇻🇳', name: 'Tiếng Việt', label: 'VI' },
    en: { flag: '🇺🇸', name: 'English', label: 'EN' },
};

// ─── Language Detection ───────────────────────────────
async function detectLanguage() {
    const saved = localStorage.getItem('sn_lang');
    if (saved && TRANSLATIONS[saved]) return saved;
    try {
        const res = await fetch('https://ipapi.co/json/', { signal: AbortSignal.timeout(3000) });
        const data = await res.json();
        return data.country_code === 'VN' ? 'vi' : 'en';
    } catch {
        return navigator.language?.startsWith('vi') ? 'vi' : 'en';
    }
}

// ─── Apply Translation ────────────────────────────────
function applyLang(lang) {
    const t = TRANSLATIONS[lang] || TRANSLATIONS.en;
    document.documentElement.lang = lang;
    localStorage.setItem('sn_lang', lang);

    document.querySelectorAll('[data-i18n]').forEach(el => {
        const key = el.dataset.i18n;
        if (t[key] !== undefined) el.innerHTML = t[key];
    });

    // Update ALL lang triggers on the page
    updateDropdownUI(lang);
}

// ─── Update Dropdown Display ────────────────────────────
function updateDropdownUI(lang) {
    const meta = LANG_META[lang];
    document.querySelectorAll('.lang-trigger-flag').forEach(el => { el.textContent = meta.flag; });
    document.querySelectorAll('.lang-trigger-label').forEach(el => { el.textContent = meta.label; });

    // Update active state on options
    document.querySelectorAll('.lang-option').forEach(opt => {
        const isActive = opt.dataset.lang === lang;
        opt.classList.toggle('active', isActive);
        const check = opt.querySelector('.check');
        if (check) check.style.display = isActive ? '' : 'none';
    });
}

// ─── Dropdown Init ────────────────────────────────────
function initLangDropdowns() {
    document.querySelectorAll('.lang-dropdown').forEach(dropdown => {
        const trigger = dropdown.querySelector('.lang-trigger');
        const menu = dropdown.querySelector('.lang-menu');
        if (!trigger) return;

        trigger.addEventListener('click', (e) => {
            e.stopPropagation();
            // Close any other open dropdowns
            document.querySelectorAll('.lang-dropdown.open').forEach(d => {
                if (d !== dropdown) d.classList.remove('open');
            });
            dropdown.classList.toggle('open');
        });

        dropdown.querySelectorAll('.lang-option').forEach(opt => {
            opt.addEventListener('click', () => {
                applyLang(opt.dataset.lang);
                dropdown.classList.remove('open');
            });
        });
    });

    // Close on outside click
    document.addEventListener('click', () => {
        document.querySelectorAll('.lang-dropdown.open').forEach(d => d.classList.remove('open'));
    });
}

// ─── Theme ────────────────────────────────────────────
function initTheme() {
    const saved = localStorage.getItem('sn_theme') || 'dark';
    applyTheme(saved);

    document.querySelectorAll('.theme-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const current = document.documentElement.dataset.theme || 'dark';
            applyTheme(current === 'dark' ? 'light' : 'dark');
        });
    });
}

function applyTheme(theme) {
    document.documentElement.dataset.theme = theme;
    localStorage.setItem('sn_theme', theme);
    document.querySelectorAll('.theme-btn').forEach(btn => {
        btn.textContent = theme === 'dark' ? '☀️' : '🌙';
        btn.title = theme === 'dark' ? 'Switch to Light Mode' : 'Switch to Dark Mode';
    });
}

// ─── Init ─────────────────────────────────────────────
document.addEventListener('DOMContentLoaded', async () => {
    initTheme();
    initLangDropdowns();
    const lang = await detectLanguage();
    applyLang(lang);
});
