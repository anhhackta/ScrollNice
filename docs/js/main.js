/* ═══════════════════════════════════════════════════════
   ScrollNice — Premium Interactions v2
   ═══════════════════════════════════════════════════════ */

/* ── Scroll Progress Bar ── */
const progressBar = document.getElementById('scroll-progress');
const updateProgress = () => {
    if (!progressBar) return;
    const max = document.documentElement.scrollHeight - window.innerHeight;
    progressBar.style.width = `${(window.scrollY / max) * 100}%`;
};

/* ── Nav Scroll-Aware ── */
const nav = document.querySelector('.nav');
const updateNav = () => {
    if (!nav) return;
    nav.classList.toggle('scrolled', window.scrollY > 50);
};

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

    // Close on link click
    mobileMenu.querySelectorAll('a').forEach(a => {
        a.addEventListener('click', () => {
            mobileMenu.classList.remove('open');
            ham.classList.remove('open');
        });
    });
}

/* ── Canvas Hero Particles ── */
function initParticles() {
    const canvas = document.getElementById('hero-canvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');

    let W = 0, H = 0, particles = [];

    const resize = () => {
        W = canvas.width = canvas.offsetWidth;
        H = canvas.height = canvas.offsetHeight;
    };

    class Particle {
        constructor() { this.reset(); }
        reset() {
            this.x = Math.random() * W;
            this.y = Math.random() * H;
            this.r = Math.random() * 1.6 + 0.4;
            this.vx = (Math.random() - .5) * .3;
            this.vy = (Math.random() - .5) * .3;
            this.a = Math.random() * .5 + .1;
            this.color = Math.random() > .5
                ? `rgba(59,130,246,${this.a})`
                : `rgba(139,92,246,${this.a})`;
        }
        draw() {
            ctx.beginPath();
            ctx.arc(this.x, this.y, this.r, 0, Math.PI * 2);
            ctx.fillStyle = this.color;
            ctx.fill();
        }
        update() {
            this.x += this.vx;
            this.y += this.vy;
            if (this.x < 0 || this.x > W || this.y < 0 || this.y > H) this.reset();
        }
    }

    const COUNT = Math.min(60, Math.floor(W * H / 14000));
    particles = Array.from({ length: COUNT }, () => new Particle());

    let raf;
    const animate = () => {
        ctx.clearRect(0, 0, W, H);
        // draw connection lines
        for (let i = 0; i < particles.length; i++) {
            for (let j = i + 1; j < particles.length; j++) {
                const dx = particles[i].x - particles[j].x;
                const dy = particles[i].y - particles[j].y;
                const dist = Math.sqrt(dx * dx + dy * dy);
                if (dist < 110) {
                    ctx.beginPath();
                    ctx.moveTo(particles[i].x, particles[i].y);
                    ctx.lineTo(particles[j].x, particles[j].y);
                    ctx.strokeStyle = `rgba(99,102,241,${.12 * (1 - dist / 110)})`;
                    ctx.lineWidth = .8;
                    ctx.stroke();
                }
            }
            particles[i].draw();
            particles[i].update();
        }
        raf = requestAnimationFrame(animate);
    };

    resize();
    window.addEventListener('resize', () => { resize(); particles.forEach(p => p.reset()); });
    animate();

    // Pause when off-screen
    const heroObs = new IntersectionObserver(entries => {
        if (entries[0].isIntersecting) { if (!raf) animate(); }
        else { cancelAnimationFrame(raf); raf = null; }
    });
    heroObs.observe(canvas);
}

/* ── Staggered Scroll Reveal ── */
function initReveal() {
    const cards = document.querySelectorAll('.feature-card, .mode-card, .dl-card, .donation-card, .hero-demo, .hero-stats');

    // Add stagger classes to groups
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
            const cx = rect.left + rect.width / 2;
            const cy = rect.top + rect.height / 2;
            const dx = (e.clientX - cx) / (rect.width / 2);
            const dy = (e.clientY - cy) / (rect.height / 2);
            card.style.transform = `perspective(600px) rotateY(${dx * 6}deg) rotateX(${-dy * 6}deg) translateY(-6px)`;
        });
        card.addEventListener('mouseleave', () => {
            card.style.transform = '';
        });
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
        if (phase === 0) {
            top.style.background = 'rgba(59,130,246,.25)';
            bot.style.background = 'transparent';
        } else if (phase === 1) {
            top.style.background = 'transparent';
            bot.style.background = 'rgba(139,92,246,.25)';
        } else {
            top.style.background = 'transparent';
            bot.style.background = 'transparent';
        }
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
    initParticles();
    initReveal();
    initTilt();
    initZoneDemo();
    initSmoothScroll();
});

window.addEventListener('scroll', () => {
    updateProgress();
    updateNav();
}, { passive: true });

// Initial call
updateNav();
updateProgress();
