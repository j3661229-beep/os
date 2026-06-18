/* ==============================================================================
 * app.js — Vel Playground Controller
 * MUST load before vel.js
 * ============================================================================== */

var outputBuffer = '';
var isReady = false;

var Module = {
    locateFile: function(path, prefix) {
        if (path.endsWith('.wasm')) {
            return prefix + path + '?v=' + Date.now();
        }
        return prefix + path;
    },
    print: function(text) {
        outputBuffer += text + '\n';
        renderOutput();
    },
    printErr: function(text) {
        outputBuffer += text + '\n';
        renderOutput();
    },
    onRuntimeInitialized: function() {
        isReady = true;
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', onWasmReady);
        } else {
            onWasmReady();
        }
    }
};

function onWasmReady() {
    var btn = document.getElementById('btn-run');
    var label = document.getElementById('run-label');
    var icon = document.getElementById('run-icon');
    var dot = document.getElementById('status-dot');
    var st = document.getElementById('status-text');
    if (btn) btn.disabled = false;
    if (label) label.textContent = 'Run';
    if (icon) icon.textContent = '▶';
    if (dot) dot.classList.add('ready');
    if (st) st.textContent = 'Ready';
}

function renderOutput() {
    var el = document.getElementById('output');
    if (!el) return;
    var lines = outputBuffer.split('\n');
    var html = '';
    for (var i = 0; i < lines.length; i++) {
        var line = lines[i];
        var escaped = escapeHtml(line);
        if (line.indexOf('[ERR]') === 0 || line.indexOf('[Parse Error]') === 0) {
            html += '<span class="out-err">' + escaped + '</span>\n';
        } else {
            html += escaped + '\n';
        }
    }
    el.innerHTML = html;
    var body = document.getElementById('output-body');
    if (body) body.scrollTop = body.scrollHeight;
}

function escapeHtml(t) {
    var d = document.createElement('div');
    d.textContent = t;
    return d.innerHTML;
}

document.addEventListener('DOMContentLoaded', function() {
    var editor = document.getElementById('code-editor');
    var output = document.getElementById('output');
    var btnRun = document.getElementById('btn-run');
    var btnReset = document.getElementById('btn-reset');
    var btnClear = document.getElementById('btn-clear');
    var lineNums = document.getElementById('line-numbers');
    var dropdown = document.getElementById('examples-dropdown');

    if (isReady) onWasmReady();

    // Run
    btnRun.addEventListener('click', function() {
        var code = editor.value;
        if (!code.trim()) return;
        outputBuffer = '';
        output.innerHTML = '';
        try {
            Module.ccall('run_vel_code', null, ['string'], [code]);
        } catch (e) {
            outputBuffer += '[System Error] ' + e.message + '\n';
            renderOutput();
        }
        if (outputBuffer.trim() === '') {
            output.innerHTML = '<span class="out-ok">✓ Script executed (no output)</span>';
        }
    });

    // Reset
    btnReset.addEventListener('click', function() {
        outputBuffer = '';
        output.innerHTML = '';
        try { Module.ccall('reset_vel_env', null, [], []); }
        catch (e) { output.innerHTML = '<span class="out-err">Reset failed</span>'; }
    });

    // Clear
    btnClear.addEventListener('click', function() {
        outputBuffer = '';
        output.innerHTML = '<span class="dim">-- Output cleared</span>';
    });

    // ==========================================
    // TRANSPILER — BIG GRID VERSION
    // ==========================================
    var btnTranslate = document.getElementById('btn-translate');
    var translateGrid = document.getElementById('translate-grid');
    var codePython = document.getElementById('code-python');
    var codeC = document.getElementById('code-c');
    var codeCpp = document.getElementById('code-cpp');
    var codeJava = document.getElementById('code-java');

    // Big Translate button
    btnTranslate.addEventListener('click', function() {
        var code = editor.value;
        if (!code.trim()) return;

        // Show the grid with animation
        translateGrid.style.display = 'grid';

        // Translate to all 4 languages at once
        try {
            codePython.textContent = ShunyaTranspiler.transpile(code, 'python');
            codeC.textContent = ShunyaTranspiler.transpile(code, 'c');
            codeCpp.textContent = ShunyaTranspiler.transpile(code, 'cpp');
            codeJava.textContent = ShunyaTranspiler.transpile(code, 'java');
        } catch (e) {
            codePython.innerHTML = '<span class="out-err">Error: ' + e.message + '</span>';
        }

        // Smooth scroll to the grid
        translateGrid.scrollIntoView({ behavior: 'smooth', block: 'start' });
    });

    // Copy buttons for each card
    document.querySelectorAll('.copy-btn[data-copy]').forEach(function(btn) {
        btn.addEventListener('click', function() {
            var lang = this.getAttribute('data-copy');
            var codeEl = document.getElementById('code-' + lang);
            if (!codeEl) return;
            var text = codeEl.textContent;
            if (!text || text.startsWith('Click')) return;

            var self = this;
            navigator.clipboard.writeText(text).then(function() {
                self.textContent = '✅ Copied!';
                self.classList.add('copied');
                setTimeout(function() {
                    self.textContent = '📋 Copy';
                    self.classList.remove('copied');
                }, 2000);
            });
        });
    });

    // Line numbers
    function updateLines() {
        var n = editor.value.split('\n').length;
        var h = '';
        for (var i = 1; i <= n; i++) h += '<span>' + i + '</span>';
        lineNums.innerHTML = h;
    }
    editor.addEventListener('input', updateLines);
    editor.addEventListener('scroll', function() { lineNums.scrollTop = editor.scrollTop; });

    // Tab & Ctrl+Enter
    editor.addEventListener('keydown', function(e) {
        if (e.key === 'Tab') {
            e.preventDefault();
            var s = this.selectionStart, end = this.selectionEnd;
            this.value = this.value.substring(0, s) + '    ' + this.value.substring(end);
            this.selectionStart = this.selectionEnd = s + 4;
            updateLines();
        }
        if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
            e.preventDefault();
            if (!btnRun.disabled) btnRun.click();
        }
    });

    // Examples
    var EX = {
hello: 'show("Hello from Shunya!")\n\nlet name = "World"\nshow("Hello, {name}!")\n\nlet x = 10\nlet y = 32\nshow("The answer: " + to_string(x + y))',

fibonacci: 'func fib(n) -> number:\n    if n == 0:\n        return 0\n    end\n    if n == 1:\n        return 1\n    end\n    return fib(n - 1) + fib(n - 2)\nend\n\nloop i from 0 to 10:\n    show("fib(" + to_string(i) + ") = " + to_string(fib(i)))\nend',

fizzbuzz: 'loop i from 1 to 30:\n    let out = ""\n    if i - (i / 3) * 3 == 0:\n        let out = out + "Fizz"\n    end\n    if i - (i / 5) * 5 == 0:\n        let out = out + "Buzz"\n    end\n    if out == "":\n        show(to_string(i))\n    else:\n        show(out)\n    end\nend',

functions: 'func add(a, b) -> number:\n    return a + b\nend\n\nfunc greet(name) -> string:\n    return "Hello, " + name + "!"\nend\n\nshow(to_string(add(10, 32)))\nshow(greet("ShunyaOS"))\n\n-- Pipeline operator\n42 |> to_string() |> show()',

match: 'func day_name(n) -> string:\n    match n:\n        1 -> return "Monday"\n        2 -> return "Tuesday"\n        3 -> return "Wednesday"\n        4 -> return "Thursday"\n        5 -> return "Friday"\n        6 -> return "Saturday"\n        7 -> return "Sunday"\n        _ -> return "Unknown"\n    end\n    return ""\nend\n\nloop i from 1 to 7:\n    show(to_string(i) + " = " + day_name(i))\nend',

desi_hello: 'aarambh\n\n-- Namaste Duniya!\nmahavir naam = "Jayesh"\ndikha("Namaste, {naam}!")\n\nmahavir x = 10\nmahavir y = 32\ndikha("Jawab hai: " + badal(x + y))\n\nkhatam',

desi_func: 'aarambh\n\nkaam jodo(a, b) -> number:\n    wapas a + b\nbas\n\nkaam namaste(naam) -> string:\n    wapas "Namaste, " + naam + " ji!"\nbas\n\ndikha(badal(jodo(100, 200)))\ndikha(namaste("Duniya"))\n\n-- Pipeline bhi chalega!\n42 |> badal() |> dikha()\n\nkhatam',

desi_loop: 'aarambh\n\nghuma i se 1 tak 10:\n    agar i - (i / 2) * 2 == 0:\n        dikha(badal(i) + " - Even hai")\n    warna:\n        dikha(badal(i) + " - Odd hai")\n    bas\nbas\n\nmahavir mood = sahi\nagar mood == sahi:\n    dikha("Aaj ka din accha hai!")\nwarna:\n    dikha("Koi baat nahi")\nbas\n\nkhatam',

desi_match: 'aarambh\n\nkaam din(n) -> string:\n    mila n:\n        1 -> wapas "Somvaar"\n        2 -> wapas "Mangalvaar"\n        3 -> wapas "Budhvaar"\n        4 -> wapas "Guruvaar"\n        5 -> wapas "Shukravaar"\n        6 -> wapas "Shanivaar"\n        7 -> wapas "Ravivaar"\n        _ -> wapas "Pata nahi"\n    bas\n    wapas ""\nbas\n\nghuma i se 1 tak 7:\n    dikha(badal(i) + " = " + din(i))\nbas\n\nkhatam'
    };

    dropdown.addEventListener('change', function() {
        if (this.value && EX[this.value]) {
            editor.value = EX[this.value];
            updateLines();
        }
        this.value = '';
    });

    // ==========================================
    // GAMIFICATION (LEARNING LEVELS)
    // ==========================================
    var currentMission = null;
    var MISSIONS = [
        {
            id: 1,
            title: "Write your first code!",
            desc: "Use <code>dikha(\"Namaste Duniya\")</code> to print the text.",
            code: "aarambh\n\n-- Your code here\n\n\nkhatam",
            expected: "Namaste Duniya\n"
        },
        {
            id: 2,
            title: "Variables",
            desc: "Create a variable: <code>mahavir score = 100</code> and print it.",
            code: "aarambh\n\n-- Create score and use dikha(badal(score))\n\n\nkhatam",
            expected: "100\n"
        },
        {
            id: 3,
            title: "Math",
            desc: "Print the result of <code>50 + 50</code>.",
            code: "aarambh\n\ndikha(badal(  ))\n\nkhatam",
            expected: "100\n"
        },
        {
            id: 4,
            title: "If / Else",
            desc: "Write an <code>agar 10 > 5:</code> statement that prints 'Pass'.",
            code: "aarambh\n\nagar 10 > 5:\n    -- print Pass here\nbas\n\nkhatam",
            expected: "Pass\n"
        },
        {
            id: 5,
            title: "Loops",
            desc: "Print 1 to 3 using <code>ghuma i se 1 tak 3:</code>",
            code: "aarambh\n\nghuma i se 1 tak 3:\n    -- print i here\nbas\n\nkhatam",
            expected: "1\n2\n3\n"
        }
    ];

    var missionDesc = document.getElementById('mission-desc');
    var missionBar = document.getElementById('mission-bar');
    var lvlBtns = document.querySelectorAll('.lvl-btn');

    lvlBtns.forEach(function(btn) {
        btn.addEventListener('click', function() {
            var lvlId = parseInt(this.getAttribute('data-level'));
            loadMission(lvlId);
        });
    });

    function loadMission(id) {
        currentMission = MISSIONS.find(m => m.id === id);
        if (!currentMission) return;
        
        // Update UI
        lvlBtns.forEach(b => b.classList.remove('active'));
        document.querySelector('.lvl-btn[data-level="'+id+'"]').classList.add('active');
        
        missionDesc.innerHTML = currentMission.desc;
        editor.value = currentMission.code;
        updateLines();
        
        outputBuffer = '';
        output.innerHTML = '<span class="dim">-- Level ' + id + ' Loaded. Complete the mission!</span>';
    }

    // Wrap the original Run logic to check for mission success
    var originalRunClick = btnRun.onclick; // Remove inline if any
    
    // We already added a listener for btnRun earlier, let's override the behavior
    // by removing the old one or just adding a check in the same file.
    // Actually, we can just replace the btnRun listener or patch it.
    // To avoid duplication, I will just redefine the listener after cloning the button.
    var newBtnRun = btnRun.cloneNode(true);
    btnRun.parentNode.replaceChild(newBtnRun, btnRun);
    btnRun = newBtnRun;
    
    btnRun.addEventListener('click', function() {
        var code = editor.value;
        if (!code.trim()) return;
        outputBuffer = '';
        output.innerHTML = '';
        try {
            Module.ccall('run_vel_code', null, ['string'], [code]);
        } catch (e) {
            outputBuffer += '[System Error] ' + e.message + '\n';
            renderOutput();
        }
        
        // Gamification Check
        if (currentMission) {
            // strip ansi/colors if any, just check raw outputBuffer
            if (outputBuffer === currentMission.expected || outputBuffer === currentMission.expected.trim() + '\n') {
                // SUCCESS!
                var currentBtn = document.querySelector('.lvl-btn[data-level="'+currentMission.id+'"]');
                currentBtn.classList.add('completed');
                
                missionDesc.innerHTML = "<strong>🎉 SUCCESS!</strong> Mission completed!";
                missionBar.classList.add('mission-success');
                setTimeout(() => missionBar.classList.remove('mission-success'), 1000);
                
                outputBuffer += '\n[MISSION ACCOMPLISHED] Great job!\n';
                renderOutput();
            }
        }
        
        if (outputBuffer.trim() === '') {
            output.innerHTML = '<span class="out-ok">✓ Script executed (no output)</span>';
        }
    });

    updateLines();

    // ==========================================
    // TYPEWRITER ANIMATION
    // ==========================================
    const headlineText = "Write code.<br><span class=\"gradient-text\">Run it instantly.</span>";
    const typeTarget = document.getElementById("typewriter-headline");
    
    if (typeTarget) {
        let isTag = false;
        let textToType = headlineText;
        let index = 0;
        let currentHTML = "";

        typeTarget.innerHTML = "<span class=\"typewriter-cursor\"></span>";

        function typeChar() {
            if (index < textToType.length) {
                let char = textToType.charAt(index);
                if (char === '<') isTag = true;
                
                currentHTML += char;
                index++;

                if (isTag) {
                    if (char === '>') isTag = false;
                    typeChar(); // Type tag immediately
                } else {
                    typeTarget.innerHTML = currentHTML + "<span class=\"typewriter-cursor\"></span>";
                    setTimeout(typeChar, 60 + Math.random() * 40);
                }
            } else {
                typeTarget.innerHTML = currentHTML + "<span class=\"typewriter-cursor\"></span>";
            }
        }
        setTimeout(typeChar, 500); // Start after short delay
    }
});
