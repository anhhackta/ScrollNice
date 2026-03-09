/* ═══════════════════════════════════════════════════════
   ScrollNice — main.js v3
   Removed canvas particles (orbit SVG handles ambient visuals).
   Added RAF-throttled scroll listener for progress bar + nav.
   ═══════════════════════════════════════════════════════ */

/* ── Scroll Progress Bar + Nav (RAF-throttled) ── */
const progressBar = document.getElementById('scroll-progress');
const nav = document.querySelector('.nav');
let ticking = false;

function onScroll() {
    if (!ticking) {
        requestAnimationFrame(() => {
            // Progress bar
            if (progressBar) {
                const max = document.documentElement.scrollHeight - window.innerHeight;
                progressBar.style.width = max > 0 ? `${(window.scrollY / max) * 100}%` : '0%';
            }
            // Scroll-aware nav
            if (nav) nav.classList.toggle('scrolled', window.scrollY > 50);
            ticking = false;
        });
        ticking = true;
    }
}

window.addEventListener('scroll', onScroll, { passive: true });
onScroll(); // init

/* ── Mobile Hamburger ── */
function initHamburger() {
    const ham = document.getElementById('hamburger');
    const mobileMenu = document.getElementById('nav-mobile');
    if (!ham || !mobileMenu) return;

    ham.addEventListener('click', () => {
        const open = mobileMenu.classList.toggle('open');
        ham.classList.toggle('open', open);
        ham.setAttribute('aria-expanded', open);
    });

    mobileMenu.querySelectorAll('a').forEach(a => {
        a.addEventListener('click', () => {
            mobileMenu.classList.remove('open');
            ham.classList.remove('open');
        });
    });
}

/* ── Staggered Scroll Reveal ── */
function initReveal() {
    document.querySelectorAll('.feature-grid, .mode-grid, .dl-cards, .donation-cards').forEach(grid => {
        [...grid.children].forEach((child, i) => {
            child.classList.add('reveal', `stagger-${i + 1}`);
        });
    });

    document.querySelectorAll('.hero-demo, .hero-stats').forEach(el => {
        el.classList.add('reveal');
    });

    const obs = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                entry.target.classList.add('visible');
                obs.unobserve(entry.target);
            }
        });
    }, { threshold: 0.08, rootMargin: '0px 0px -40px 0px' });

    document.querySelectorAll('.reveal').forEach(el => obs.observe(el));
}

/* ── 3D Mouse Tilt on Mode Cards ── */
function initTilt() {
    document.querySelectorAll('.mode-card').forEach(card => {
        card.addEventListener('mousemove', e => {
            const rect = card.getBoundingClientRect();
            const dx = (e.clientX - rect.left - rect.width / 2) / (rect.width / 2);
            const dy = (e.clientY - rect.top - rect.height / 2) / (rect.height / 2);
            card.style.transform = `perspective(600px) rotateY(${dx * 6}deg) rotateX(${-dy * 6}deg) translateY(-6px)`;
        });
        card.addEventListener('mouseleave', () => { card.style.transform = ''; });
    });
}

/* ── Zone Demo Animation ── */
function initZoneDemo() {
    const top = document.querySelector('.zone-demo-top');
    const bot = document.querySelector('.zone-demo-bot');
    if (!top || !bot) return;

    let phase = 0;
    setInterval(() => {
        phase = (phase + 1) % 3;
        top.style.background = phase === 0 ? 'rgba(59,130,246,.25)' : 'transparent';
        bot.style.background = phase === 1 ? 'rgba(139,92,246,.25)' : 'transparent';
    }, 1200);
}

/* ── Smooth Anchor Scroll ── */
function initSmoothScroll() {
    document.querySelectorAll('a[href^="#"]').forEach(a => {
        a.addEventListener('click', e => {
            const target = document.querySelector(a.getAttribute('href'));
            if (target) {
                e.preventDefault();
                target.scrollIntoView({ behavior: 'smooth', block: 'start' });
            }
        });
    });
}

/* ── Init ── */
document.addEventListener('DOMContentLoaded', () => {
    initHamburger();
    initReveal();
    initTilt();
    initZoneDemo();
    initSmoothScroll();
});
