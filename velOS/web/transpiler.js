/* ==============================================================================
 * transpiler.js — Shunya → C / C++ / Java / Python Transpiler
 * A learning-oriented line-by-line transpiler for the Shunya playground.
 * ============================================================================== */

var ShunyaTranspiler = (function() {

    // Normalize Desi keywords to English equivalents
    function normalize(line) {
        var t = line;
        // Keywords (whole word replacements using regex word boundaries)
        var map = [
            [/\bmahavir\b/g, 'let'],
            [/\brakho\b/g, 'const'],
            [/\bkaam\b/g, 'func'],
            [/\bwapas\b/g, 'return'],
            [/\bagar\b/g, 'if'],
            [/\bwarna\b/g, 'else'],
            [/\bghuma\b/g, 'loop'],
            [/\bjab_tak\b/g, 'while'],
            [/\bmila\b/g, 'match'],
            [/\bbas\b/g, 'end'],
            [/\baarambh\b/g, '__START__'],
            [/\bkhatam\b/g, '__END__'],
            [/\bdikha\b/g, 'show'],
            [/\bbadal\b/g, 'to_string'],
            [/\bsahi\b/g, 'true'],
            [/\bgalat\b/g, 'false'],
            [/\bse\b/g, 'from'],
            [/\btak\b/g, 'to']
        ];
        for (var i = 0; i < map.length; i++) {
            t = t.replace(map[i][0], map[i][1]);
        }
        return t;
    }

    // Get indentation level of a line
    function getIndent(line) {
        var m = line.match(/^(\s*)/);
        return m ? m[1].length : 0;
    }

    // Make indent string
    function indent(level, lang) {
        var unit = '    ';
        var s = '';
        for (var i = 0; i < level; i++) s += unit;
        return s;
    }

    // Detect type from value
    function inferType(val) {
        val = val.trim();
        if (val.match(/^".*"$/)) return 'string';
        if (val.match(/^\d+\.\d+$/)) return 'float';
        if (val.match(/^\d+$/)) return 'int';
        if (val === 'true' || val === 'false') return 'bool';
        return 'auto';
    }

    // Convert inferred type to C type
    function cType(t) {
        if (t === 'string') return 'char*';
        if (t === 'float') return 'float';
        if (t === 'int') return 'int';
        if (t === 'bool') return 'int';
        return 'int';
    }
    function cppType(t) {
        if (t === 'string') return 'string';
        if (t === 'float') return 'float';
        if (t === 'int') return 'int';
        if (t === 'bool') return 'bool';
        return 'auto';
    }
    function javaType(t) {
        if (t === 'string') return 'String';
        if (t === 'float') return 'double';
        if (t === 'int') return 'int';
        if (t === 'bool') return 'boolean';
        return 'int';
    }

    // Convert Shunya return type annotation to language type
    function mapReturnType(rt, lang) {
        rt = rt.trim();
        if (rt === 'number') {
            if (lang === 'python') return '';
            return 'int';
        }
        if (rt === 'string') {
            if (lang === 'c') return 'char*';
            if (lang === 'cpp') return 'string';
            if (lang === 'java') return 'String';
            return '';
        }
        if (lang === 'python') return '';
        return 'void';
    }

    // Replace show(...) calls — handles nested parens
    function convertShow(expr, lang) {
        // Extract the argument inside show(...) by counting brackets
        if (!expr.match(/^show\(/)) return null;
        var inner = expr.slice(5); // remove "show("
        // remove trailing ")"
        var depth = 0;
        var end = -1;
        for (var i = 0; i < inner.length; i++) {
            if (inner[i] === '(') depth++;
            if (inner[i] === ')') {
                if (depth === 0) { end = i; break; }
                depth--;
            }
        }
        var arg = end >= 0 ? inner.slice(0, end) : inner.replace(/\)$/, '');

        // Replace to_string(x) in the argument
        arg = convertToString(arg, lang);

        if (lang === 'c') return 'printf("%s\\n", ' + arg + ');';
        if (lang === 'cpp') return 'cout << ' + arg + ' << endl;';
        if (lang === 'java') return 'System.out.println(' + arg + ');';
        if (lang === 'python') return 'print(' + arg + ')';
        return expr;
    }

    // Replace to_string(x) calls
    function convertToString(expr, lang) {
        // Recursively replace to_string(...)
        var re = /to_string\(([^()]*(?:\([^()]*\))*[^()]*)\)/g;
        var maxIter = 10;
        while (re.test(expr) && maxIter-- > 0) {
            re.lastIndex = 0;
            expr = expr.replace(re, function(full, inner) {
                if (lang === 'c') return inner; // simplified for printf
                if (lang === 'cpp') return 'to_string(' + inner + ')';
                if (lang === 'java') return 'String.valueOf(' + inner + ')';
                if (lang === 'python') return 'str(' + inner + ')';
                return inner;
            });
        }
        return expr;
    }

    // Replace string concat "+" for C (simplified)
    function convertExpr(expr, lang) {
        expr = convertToString(expr, lang);
        return expr;
    }

    // Main transpile function
    function transpile(code, lang) {
        var lines = code.split('\n');
        var out = [];
        var depthStack = [];
        var depth = 0;
        var hasMain = false;
        var hasFunc = false;

        // Headers
        if (lang === 'c') {
            out.push('#include <stdio.h>');
            out.push('#include <stdlib.h>');
            out.push('#include <string.h>');
            out.push('');
        } else if (lang === 'cpp') {
            out.push('#include <iostream>');
            out.push('#include <string>');
            out.push('using namespace std;');
            out.push('');
        } else if (lang === 'java') {
            out.push('public class Main {');
            out.push('');
        } else if (lang === 'python') {
            out.push('# Translated from Shunya');
            out.push('');
        }

        for (var i = 0; i < lines.length; i++) {
            var raw = lines[i];
            var trimmed = raw.trim();

            // Skip empty lines
            if (trimmed === '') {
                out.push('');
                continue;
            }

            // Normalize Desi → English
            var norm = normalize(trimmed);

            // ---- Comments ----
            if (norm.match(/^--/)) {
                var comment = norm.replace(/^--\s*/, '');
                if (lang === 'python') {
                    out.push(indent(depth) + '# ' + comment);
                } else {
                    out.push(indent(depth) + '// ' + comment);
                }
                continue;
            }

            // ---- aarambh (program start) ----
            if (norm === '__START__') {
                hasMain = true;
                if (lang === 'c' || lang === 'cpp') {
                    out.push('int main() {');
                } else if (lang === 'java') {
                    out.push('    public static void main(String[] args) {');
                    depth = 1;
                }
                // Python: no main needed
                depth++;
                continue;
            }

            // ---- khatam (program end) ----
            if (norm === '__END__') {
                if (depth > 0) depth--;
                if (lang === 'c' || lang === 'cpp') {
                    out.push(indent(depth) + 'return 0;');
                    out.push('}');
                } else if (lang === 'java') {
                    out.push('    }');
                    out.push('}');
                    depth = 0;
                }
                // Python: nothing
                continue;
            }

            // ---- Variable declaration ----
            var letMatch = norm.match(/^(let|const)\s+(\w+)\s*=\s*(.+)$/);
            if (letMatch) {
                var isConst = letMatch[1] === 'const';
                var varName = letMatch[2];
                var varVal = convertExpr(letMatch[3], lang);
                var vType = inferType(letMatch[3]);

                if (lang === 'c') {
                    out.push(indent(depth) + (isConst ? 'const ' : '') + cType(vType) + ' ' + varName + ' = ' + varVal + ';');
                } else if (lang === 'cpp') {
                    out.push(indent(depth) + (isConst ? 'const ' : '') + cppType(vType) + ' ' + varName + ' = ' + varVal + ';');
                } else if (lang === 'java') {
                    out.push(indent(depth) + (isConst ? 'final ' : '') + javaType(vType) + ' ' + varName + ' = ' + varVal + ';');
                } else if (lang === 'python') {
                    out.push(indent(depth) + varName + ' = ' + varVal);
                }
                continue;
            }

            // ---- Function definition ----
            var funcMatch = norm.match(/^func\s+(\w+)\(([^)]*)\)\s*->\s*(\w+)\s*:$/);
            if (funcMatch) {
                hasFunc = true;
                var fName = funcMatch[1];
                var fArgs = funcMatch[2];
                var fRetType = funcMatch[3];
                var retT = mapReturnType(fRetType, lang);

                if (lang === 'c') {
                    var cArgs = fArgs.split(',').map(function(a) {
                        return 'int ' + a.trim();
                    }).join(', ');
                    if (retT === 'char*') {
                        cArgs = fArgs.split(',').map(function(a) { return 'int ' + a.trim(); }).join(', ');
                    }
                    out.push(retT + ' ' + fName + '(' + cArgs + ') {');
                } else if (lang === 'cpp') {
                    var cppArgs = fArgs.split(',').map(function(a) {
                        return 'int ' + a.trim();
                    }).join(', ');
                    out.push(retT + ' ' + fName + '(' + cppArgs + ') {');
                } else if (lang === 'java') {
                    var jArgs = fArgs.split(',').map(function(a) {
                        return 'int ' + a.trim();
                    }).join(', ');
                    out.push(indent(depth) + 'static ' + retT + ' ' + fName + '(' + jArgs + ') {');
                } else if (lang === 'python') {
                    out.push(indent(depth) + 'def ' + fName + '(' + fArgs.trim() + '):');
                }
                depth++;
                continue;
            }

            // ---- Return ----
            var retMatch = norm.match(/^return\s+(.+)$/);
            if (retMatch) {
                var retVal = convertExpr(retMatch[1], lang);
                if (lang === 'python') {
                    out.push(indent(depth) + 'return ' + retVal);
                } else {
                    out.push(indent(depth) + 'return ' + retVal + ';');
                }
                continue;
            }

            // ---- Loop (for) ----
            var loopMatch = norm.match(/^loop\s+(\w+)\s+from\s+(\w+)\s+to\s+(\w+)\s*:$/);
            if (loopMatch) {
                var lVar = loopMatch[1];
                var lFrom = loopMatch[2];
                var lTo = loopMatch[3];

                if (lang === 'c' || lang === 'cpp') {
                    out.push(indent(depth) + 'for (int ' + lVar + ' = ' + lFrom + '; ' + lVar + ' <= ' + lTo + '; ' + lVar + '++) {');
                } else if (lang === 'java') {
                    out.push(indent(depth) + 'for (int ' + lVar + ' = ' + lFrom + '; ' + lVar + ' <= ' + lTo + '; ' + lVar + '++) {');
                } else if (lang === 'python') {
                    out.push(indent(depth) + 'for ' + lVar + ' in range(' + lFrom + ', ' + (parseInt(lTo) ? parseInt(lTo) + 1 : lTo + ' + 1') + '):');
                }
                depth++;
                continue;
            }

            // ---- While ----
            var whileMatch = norm.match(/^while\s+(.+)\s*:$/);
            if (whileMatch) {
                var cond = whileMatch[1];
                if (lang === 'python') {
                    out.push(indent(depth) + 'while ' + cond + ':');
                } else {
                    out.push(indent(depth) + 'while (' + cond + ') {');
                }
                depth++;
                continue;
            }

            // ---- If ----
            var ifMatch = norm.match(/^if\s+(.+)\s*:$/);
            if (ifMatch) {
                var ifCond = ifMatch[1]
                    .replace(/\b(true|sahi)\b/g, lang === 'python' ? 'True' : (lang === 'java' ? 'true' : '1'))
                    .replace(/\b(false|galat)\b/g, lang === 'python' ? 'False' : (lang === 'java' ? 'false' : '0'));
                if (lang === 'python') {
                    out.push(indent(depth) + 'if ' + ifCond + ':');
                } else {
                    out.push(indent(depth) + 'if (' + ifCond + ') {');
                }
                depth++;
                continue;
            }

            // ---- Else ----
            if (norm === 'else:' || norm === 'else') {
                if (depth > 0) depth--;
                if (lang === 'python') {
                    out.push(indent(depth) + 'else:');
                } else {
                    out.push(indent(depth) + '} else {');
                }
                depth++;
                continue;
            }

            // ---- Match ----
            var matchMatch = norm.match(/^match\s+(\w+)\s*:$/);
            if (matchMatch) {
                var mVar = matchMatch[1];
                if (lang === 'c' || lang === 'cpp') {
                    out.push(indent(depth) + 'switch (' + mVar + ') {');
                } else if (lang === 'java') {
                    out.push(indent(depth) + 'switch (' + mVar + ') {');
                } else if (lang === 'python') {
                    out.push(indent(depth) + 'match ' + mVar + ':');
                }
                depth++;
                continue;
            }

            // ---- Match arm: value -> action ----
            var armMatch = norm.match(/^(\w+|_)\s*->\s*(.+)$/);
            if (armMatch && depth > 0) {
                var armVal = armMatch[1];
                var armAction = normalize(armMatch[2]);

                // Convert return in arm
                var armRetMatch = armAction.match(/^return\s+(.+)$/);

                if (lang === 'c' || lang === 'cpp' || lang === 'java') {
                    if (armVal === '_') {
                        out.push(indent(depth) + 'default:');
                    } else {
                        out.push(indent(depth) + 'case ' + armVal + ':');
                    }
                    if (armRetMatch) {
                        out.push(indent(depth + 1) + 'return ' + convertExpr(armRetMatch[1], lang) + ';');
                    } else {
                        out.push(indent(depth + 1) + convertShow(armAction, lang) || armAction + ';');
                    }
                    out.push(indent(depth + 1) + 'break;');
                } else if (lang === 'python') {
                    if (armVal === '_') {
                        out.push(indent(depth) + 'case _:');
                    } else {
                        out.push(indent(depth) + 'case ' + armVal + ':');
                    }
                    if (armRetMatch) {
                        out.push(indent(depth + 1) + 'return ' + convertExpr(armRetMatch[1], lang));
                    } else {
                        var pyArm = armAction.replace(/^show\(/, 'print(');
                        out.push(indent(depth + 1) + pyArm);
                    }
                }
                continue;
            }

            // ---- End block ----
            if (norm === 'end') {
                if (depth > 0) depth--;
                if (lang === 'python') {
                    // Python uses indentation, no closing brace
                } else {
                    out.push(indent(depth) + '}');
                }
                continue;
            }

            // ---- Show ----
            if (norm.match(/^show\(/)) {
                var showResult = convertShow(norm, lang);
                if (showResult) {
                    out.push(indent(depth) + showResult);
                    continue;
                }
            }

            // ---- Pipeline operator ----
            var pipeMatch = norm.match(/^(.+?)\s*\|>\s*(.+)$/);
            if (pipeMatch) {
                // Simplified pipeline: just comment it
                if (lang === 'python') {
                    out.push(indent(depth) + '# Pipeline: ' + norm);
                    // try to unwind: val |> to_string() |> show()
                    var parts = norm.split('|>').map(function(p) { return p.trim(); });
                    if (parts.length >= 1) {
                        var result = parts[0];
                        for (var p = 1; p < parts.length; p++) {
                            var fn = parts[p].replace('()', '').trim();
                            if (fn === 'show') result = 'print(' + result + ')';
                            else if (fn === 'to_string') result = 'str(' + result + ')';
                            else result = fn + '(' + result + ')';
                        }
                        out.push(indent(depth) + result);
                    }
                } else if (lang === 'c') {
                    out.push(indent(depth) + '// Pipeline: ' + norm);
                    out.push(indent(depth) + 'printf("%d\\n", ' + norm.split('|>')[0].trim() + ');');
                } else if (lang === 'cpp') {
                    out.push(indent(depth) + '// Pipeline: ' + norm);
                    out.push(indent(depth) + 'cout << ' + norm.split('|>')[0].trim() + ' << endl;');
                } else if (lang === 'java') {
                    out.push(indent(depth) + '// Pipeline: ' + norm);
                    out.push(indent(depth) + 'System.out.println(' + norm.split('|>')[0].trim() + ');');
                }
                continue;
            }

            // ---- Generic function call: foo(args) ----
            var funcCallMatch = norm.match(/^(\w+)\((.*)?\)$/);
            if (funcCallMatch) {
                var fnName = funcCallMatch[1];
                var fnArgs = funcCallMatch[2] || '';
                fnArgs = convertExpr(fnArgs, lang);
                if (lang === 'python') {
                    out.push(indent(depth) + fnName + '(' + fnArgs + ')');
                } else {
                    out.push(indent(depth) + fnName + '(' + fnArgs + ');');
                }
                continue;
            }

            // ---- Variable reassignment: x = expr ----
            var assignMatch = norm.match(/^(\w+)\s*=\s*(.+)$/);
            if (assignMatch) {
                var aName = assignMatch[1];
                var aVal = convertExpr(assignMatch[2], lang);
                if (lang === 'python') {
                    out.push(indent(depth) + aName + ' = ' + aVal);
                } else {
                    out.push(indent(depth) + aName + ' = ' + aVal + ';');
                }
                continue;
            }

            // ---- Any expression with function call inside: foo(bar(x)) ----
            var exprCallMatch = norm.match(/\w+\s*\(/);
            if (exprCallMatch) {
                var converted = convertExpr(norm, lang);
                if (lang === 'python') {
                    // Replace show() with print()
                    converted = converted.replace(/\bshow\(/g, 'print(');
                    out.push(indent(depth) + converted);
                } else if (lang === 'c') {
                    converted = converted.replace(/\bshow\(/g, 'printf("%s\\n", ');
                    out.push(indent(depth) + converted + ';');
                } else if (lang === 'cpp') {
                    converted = converted.replace(/\bshow\(([^)]*)\)/g, 'cout << $1 << endl');
                    out.push(indent(depth) + converted + ';');
                } else if (lang === 'java') {
                    converted = converted.replace(/\bshow\(/g, 'System.out.println(');
                    out.push(indent(depth) + converted + ';');
                }
                continue;
            }

            // ---- Absolute fallback: output as-is (not as comment!) ----
            if (lang === 'python') {
                out.push(indent(depth) + norm);
            } else {
                out.push(indent(depth) + norm + ';');
            }
        }

        // Close Java class if needed
        // Already handled in __END__

        return out.join('\n');
    }

    // Public API
    return {
        transpile: transpile,
        toC: function(code) { return transpile(code, 'c'); },
        toCpp: function(code) { return transpile(code, 'cpp'); },
        toJava: function(code) { return transpile(code, 'java'); },
        toPython: function(code) { return transpile(code, 'python'); }
    };

})();
