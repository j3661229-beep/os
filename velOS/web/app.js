/* ==============================================================================
 * app.js — Vel Playground Controller
 * MUST load before vel.js
 * ============================================================================== */

var outputBuffer = '';
var isReady = false;

var Module = {
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
hello: 'show("Hello from Vel!")\n\nlet name = "World"\nshow("Hello, {name}!")\n\nlet x = 10\nlet y = 32\nshow("The answer: " + to_string(x + y))',

fibonacci: 'func fib(n) -> number:\n    if n == 0:\n        return 0\n    end\n    if n == 1:\n        return 1\n    end\n    return fib(n - 1) + fib(n - 2)\nend\n\nloop i from 0 to 10:\n    show("fib(" + to_string(i) + ") = " + to_string(fib(i)))\nend',

fizzbuzz: 'loop i from 1 to 30:\n    let out = ""\n    if i - (i / 3) * 3 == 0:\n        let out = out + "Fizz"\n    end\n    if i - (i / 5) * 5 == 0:\n        let out = out + "Buzz"\n    end\n    if out == "":\n        show(to_string(i))\n    else:\n        show(out)\n    end\nend',

functions: 'func add(a, b) -> number:\n    return a + b\nend\n\nfunc greet(name) -> string:\n    return "Hello, " + name + "!"\nend\n\nshow(to_string(add(10, 32)))\nshow(greet("VelOS"))\n\n-- Pipeline operator\n42 |> to_string() |> show()',

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

    updateLines();
});
