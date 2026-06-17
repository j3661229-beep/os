#!/bin/bash
cd /mnt/d/os/velOS

echo "=== 1. HELLO ==="
cat > /tmp/t.vel << 'EOF'
show("Hello from Vel!")
let name = "World"
show("Hello, {name}!")
EOF
./vel /tmp/t.vel

echo ""
echo "=== 2. FIBONACCI ==="
cat > /tmp/t.vel << 'EOF'
func fib(n) -> number:
    if n == 0:
        return 0
    end
    if n == 1:
        return 1
    end
    return fib(n - 1) + fib(n - 2)
end

loop i from 0 to 10:
    show("fib(" + to_string(i) + ") = " + to_string(fib(i)))
end
EOF
./vel /tmp/t.vel

echo ""
echo "=== 3. FUNCTIONS ==="
cat > /tmp/t.vel << 'EOF'
func add(a, b) -> number:
    return a + b
end

func greet(name) -> string:
    return "Hello, " + name + "!"
end

show(to_string(add(10, 32)))
show(greet("VelOS"))
EOF
./vel /tmp/t.vel

echo ""
echo "=== 4. DESI HELLO ==="
cat > /tmp/t.vel << 'EOF'
aarambh

mahavir naam = "Jayesh"
dikha("Namaste, {naam}!")

mahavir x = 10
mahavir y = 32
dikha("Jawab hai: " + badal(x + y))

khatam
EOF
./vel /tmp/t.vel

echo ""
echo "=== 5. DESI FUNCTIONS ==="
cat > /tmp/t.vel << 'EOF'
aarambh

kaam jodo(a, b) -> number:
    wapas a + b
bas

kaam namaste(naam) -> string:
    wapas "Namaste, " + naam + " ji!"
bas

dikha(badal(jodo(100, 200)))
dikha(namaste("Duniya"))

khatam
EOF
./vel /tmp/t.vel

echo ""
echo "=== 6. DESI LOOPS ==="
cat > /tmp/t.vel << 'EOF'
aarambh

ghuma i se 1 tak 5:
    dikha("Ginti: " + badal(i))
bas

mahavir mood = sahi
agar mood == sahi:
    dikha("Aaj ka din accha hai!")
warna:
    dikha("Koi baat nahi")
bas

khatam
EOF
./vel /tmp/t.vel

echo ""
echo "=== 7. DESI MATCH ==="
cat > /tmp/t.vel << 'EOF'
aarambh

kaam din_ka_naam(n) -> string:
    mila n:
        1 -> wapas "Somvaar"
        2 -> wapas "Mangalvaar"
        3 -> wapas "Budhvaar"
        4 -> wapas "Guruvaar"
        5 -> wapas "Shukravaar"
        6 -> wapas "Shanivaar"
        7 -> wapas "Ravivaar"
        _ -> wapas "Pata nahi"
    bas
    wapas ""
bas

ghuma i se 1 tak 7:
    dikha(badal(i) + " = " + din_ka_naam(i))
bas

khatam
EOF
./vel /tmp/t.vel

echo ""
echo "=== ALL TESTS DONE ==="
