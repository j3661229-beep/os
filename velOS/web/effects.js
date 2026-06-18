/* ==============================================================================
 * effects.js — Code Rain, Confetti, & Theme Toggle
 * ============================================================================== */

(function() {

    // ==========================================
    // 1. CODE RAIN (Matrix-style Shunya keywords)
    // ==========================================
    var canvas = document.getElementById('code-rain');
    if (canvas) {
        var ctx = canvas.getContext('2d');
        var keywords = [
            'aarambh', 'khatam', 'mahavir', 'rakho', 'kaam', 'wapas',
            'agar', 'warna', 'ghuma', 'jab_tak', 'mila', 'bas',
            'dikha', 'badal', 'sahi', 'galat', 'se', 'tak',
            'let', 'const', 'func', 'return', 'if', 'else',
            'loop', 'while', 'match', 'end', 'show', '0', '1'
        ];

        function resizeRain() {
            canvas.width = window.innerWidth;
            canvas.height = window.innerHeight;
        }
        resizeRain();
        window.addEventListener('resize', resizeRain);

        var fontSize = 14;
        var columns = Math.floor(canvas.width / (fontSize * 4));
        var drops = [];
        for (var i = 0; i < columns; i++) {
            drops[i] = Math.random() * -100;
        }

        function drawRain() {
            ctx.fillStyle = 'rgba(5, 8, 16, 0.06)';
            ctx.fillRect(0, 0, canvas.width, canvas.height);

            ctx.font = fontSize + 'px JetBrains Mono, monospace';

            for (var i = 0; i < drops.length; i++) {
                var word = keywords[Math.floor(Math.random() * keywords.length)];
                var x = i * (fontSize * 4);
                var y = drops[i] * fontSize;

                // Random color between purple and cyan
                var colors = [
                    'rgba(139, 92, 246, 0.7)',
                    'rgba(34, 211, 238, 0.5)',
                    'rgba(52, 211, 153, 0.5)',
                    'rgba(244, 114, 182, 0.4)',
                    'rgba(255, 153, 51, 0.4)'
                ];
                ctx.fillStyle = colors[Math.floor(Math.random() * colors.length)];
                ctx.fillText(word, x, y);

                if (y > canvas.height && Math.random() > 0.98) {
                    drops[i] = 0;
                }
                drops[i] += 0.4;
            }
            requestAnimationFrame(drawRain);
        }
        drawRain();
    }

    // ==========================================
    // 2. CONFETTI EXPLOSION
    // ==========================================
    var confettiCanvas = document.getElementById('confetti-canvas');
    var confettiCtx = confettiCanvas ? confettiCanvas.getContext('2d') : null;
    var confettiParticles = [];
    var confettiActive = false;

    function resizeConfetti() {
        if (confettiCanvas) {
            confettiCanvas.width = window.innerWidth;
            confettiCanvas.height = window.innerHeight;
        }
    }
    resizeConfetti();
    window.addEventListener('resize', resizeConfetti);

    function createConfetti(x, y) {
        var colors = ['#8b5cf6', '#22d3ee', '#34d399', '#f472b6', '#fbbf24', '#fb923c', '#ff9933', '#138808'];
        for (var i = 0; i < 80; i++) {
            confettiParticles.push({
                x: x,
                y: y,
                vx: (Math.random() - 0.5) * 16,
                vy: (Math.random() - 0.7) * 14,
                color: colors[Math.floor(Math.random() * colors.length)],
                size: Math.random() * 8 + 3,
                rotation: Math.random() * 360,
                rotSpeed: (Math.random() - 0.5) * 10,
                gravity: 0.3,
                opacity: 1,
                shape: Math.random() > 0.5 ? 'rect' : 'circle'
            });
        }
        if (!confettiActive) {
            confettiActive = true;
            animateConfetti();
        }
    }

    function animateConfetti() {
        if (!confettiCtx) return;
        confettiCtx.clearRect(0, 0, confettiCanvas.width, confettiCanvas.height);

        for (var i = confettiParticles.length - 1; i >= 0; i--) {
            var p = confettiParticles[i];
            p.x += p.vx;
            p.y += p.vy;
            p.vy += p.gravity;
            p.vx *= 0.99;
            p.rotation += p.rotSpeed;
            p.opacity -= 0.008;

            if (p.opacity <= 0) {
                confettiParticles.splice(i, 1);
                continue;
            }

            confettiCtx.save();
            confettiCtx.translate(p.x, p.y);
            confettiCtx.rotate(p.rotation * Math.PI / 180);
            confettiCtx.globalAlpha = p.opacity;
            confettiCtx.fillStyle = p.color;

            if (p.shape === 'rect') {
                confettiCtx.fillRect(-p.size / 2, -p.size / 4, p.size, p.size / 2);
            } else {
                confettiCtx.beginPath();
                confettiCtx.arc(0, 0, p.size / 2, 0, Math.PI * 2);
                confettiCtx.fill();
            }
            confettiCtx.restore();
        }

        if (confettiParticles.length > 0) {
            requestAnimationFrame(animateConfetti);
        } else {
            confettiActive = false;
            confettiCtx.clearRect(0, 0, confettiCanvas.width, confettiCanvas.height);
        }
    }

    // Expose globally so app.js can trigger it
    window.fireConfetti = function() {
        // Fire from multiple positions for a fuller effect
        createConfetti(window.innerWidth * 0.3, window.innerHeight * 0.3);
        createConfetti(window.innerWidth * 0.7, window.innerHeight * 0.3);
        setTimeout(function() {
            createConfetti(window.innerWidth * 0.5, window.innerHeight * 0.2);
        }, 150);
    };

    // ==========================================
    // 3. DARK/LIGHT THEME TOGGLE
    // ==========================================
    var toggleBtn = document.getElementById('theme-toggle');
    var isDark = localStorage.getItem('shunya-theme') !== 'light';

    function applyTheme() {
        if (isDark) {
            document.body.classList.remove('light-mode');
            if (toggleBtn) toggleBtn.textContent = '🌙';
        } else {
            document.body.classList.add('light-mode');
            if (toggleBtn) toggleBtn.textContent = '☀️';
        }
    }
    applyTheme();

    if (toggleBtn) {
        toggleBtn.addEventListener('click', function() {
            isDark = !isDark;
            localStorage.setItem('shunya-theme', isDark ? 'dark' : 'light');
            applyTheme();
        });
    }

})();
