/**
 * ScrollNice docs-i18n (for sub-pages)
 * Shares localStorage keys with main i18n.js
 */
const DOCS_STRINGS = {
    vi: {
        nav_home: 'Trang chủ', nav_docs: 'Hướng dẫn',
        sidebar_overview: 'Tổng quan', sidebar_install: 'Cài đặt',
        sidebar_quickstart: 'Bắt đầu nhanh', sidebar_modes: 'Chế độ lăn',
        sidebar_settings: 'Cài đặt chi tiết', sidebar_troubleshoot: 'Xử lý lỗi',
        sidebar_privacy: 'Quyền riêng tư', sidebar_support: '❤️ Ủng hộ',
        footer_home: 'Trang chủ',
    },
    en: {
        nav_home: 'Home', nav_docs: 'Docs',
        sidebar_overview: 'Overview', sidebar_install: 'Installation',
        sidebar_quickstart: 'Quick Start', sidebar_modes: 'Scroll Modes',
        sidebar_settings: 'Settings', sidebar_troubleshoot: 'Troubleshooting',
        sidebar_privacy: 'Privacy', sidebar_support: '❤️ Support',
        footer_home: 'Home',
    }
};

const LANG_META = {
    vi: { flag: '🇻🇳', label: 'VI' },
    en: { flag: '🇺🇸', label: 'EN' },
};

async function detectLang() {
    const saved = localStorage.getItem('sn_lang');
    if (saved) return saved;
    try {
        const res = await fetch('https://ipapi.co/json/', { signal: AbortSignal.timeout(3000) });
        const data = await res.json();
        return data.country_code === 'VN' ? 'vi' : 'en';
    } catch {
        return navigator.language?.startsWith('vi') ? 'vi' : 'en';
    }
}

function applyDocsLang(lang) {
    const t = DOCS_STRINGS[lang] || DOCS_STRINGS.en;
    document.documentElement.lang = lang;
    localStorage.setItem('sn_lang', lang);
    document.querySelectorAll('[data-i18n]').forEach(el => {
        const key = el.dataset.i18n;
        if (t[key] !== undefined) el.innerHTML = t[key];
    });
    // Update dropdown triggers
    const meta = LANG_META[lang];
    const flagCode = lang === 'vi' ? 'vn' : 'us';
    const flagSrc = `https://flagcdn.com/18x13/${flagCode}.png`;
    document.querySelectorAll('.lang-trigger-flag').forEach(el => {
        if (el.tagName === 'IMG') { el.src = flagSrc; el.alt = meta.label; }
        else el.textContent = meta.label;
    });
    document.querySelectorAll('.lang-trigger-label').forEach(el => { el.textContent = meta.label; });
    document.querySelectorAll('.lang-option').forEach(opt => {
        const active = opt.dataset.lang === lang;
        opt.classList.toggle('active', active);
        const check = opt.querySelector('.check');
        if (check) check.style.display = active ? '' : 'none';
    });
}

function initDocsDropdowns() {
    document.querySelectorAll('.lang-dropdown').forEach(dropdown => {
        const trigger = dropdown.querySelector('.lang-trigger');
        if (!trigger) return;
        trigger.addEventListener('click', e => {
            e.stopPropagation();
            document.querySelectorAll('.lang-dropdown.open').forEach(d => { if (d !== dropdown) d.classList.remove('open'); });
            dropdown.classList.toggle('open');
        });
        dropdown.querySelectorAll('.lang-option').forEach(opt => {
            opt.addEventListener('click', () => { applyDocsLang(opt.dataset.lang); dropdown.classList.remove('open'); });
        });
    });
    document.addEventListener('click', () => {
        document.querySelectorAll('.lang-dropdown.open').forEach(d => d.classList.remove('open'));
    });
}

function initDocsTheme() {
    const saved = localStorage.getItem('sn_theme') || 'dark';
    document.documentElement.dataset.theme = saved;
    document.querySelectorAll('.theme-btn').forEach(btn => {
        btn.textContent = saved === 'dark' ? '☀️' : '🌙';
        btn.addEventListener('click', () => {
            const cur = document.documentElement.dataset.theme || 'dark';
            const next = cur === 'dark' ? 'light' : 'dark';
            document.documentElement.dataset.theme = next;
            localStorage.setItem('sn_theme', next);
            btn.textContent = next === 'dark' ? '☀️' : '🌙';
        });
    });
}

document.addEventListener('DOMContentLoaded', async () => {
    initDocsTheme();
    initDocsDropdowns();
    const lang = await detectLang();
    applyDocsLang(lang);
});
