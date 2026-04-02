#!/bin/sh
# Test suite for axt

PASS=0
FAIL=0
AXT=./axt

expect_fail() {
    desc="$1"
    shift
    if "$@" > /dev/null 2>&1; then
        FAIL=$((FAIL + 1))
        printf "FAIL: %s\n" "$desc"
    else
        PASS=$((PASS + 1))
        printf "ok:   %s\n" "$desc"
    fi
}

expect_pass() {
    desc="$1"
    shift
    if "$@" > /dev/null 2>&1; then
        PASS=$((PASS + 1))
        printf "ok:   %s\n" "$desc"
    else
        FAIL=$((FAIL + 1))
        printf "FAIL: %s\n" "$desc"
    fi
}

# Test 1: no arguments -> non-zero exit
expect_fail "no arguments exits with error" $AXT

# Test 2: nonexistent file -> non-zero exit
expect_fail "nonexistent file exits with error" $AXT nonexistent_file.at

# Test 3: valid AT_INIT input -> success (no test cases, just init)
TMPFILE=$(mktemp /tmp/axt_test_XXXXXX.at)
cat > "$TMPFILE" <<'EOF'
AT_INIT([Test Suite])
EOF
expect_pass "parse valid AT_INIT" $AXT "$TMPFILE"
rm -f "$TMPFILE"

# Test 4: AT_SETUP/AT_CHECK/AT_CLEANUP with actual command execution
TMPFILE=$(mktemp /tmp/axt_test_XXXXXX.at)
cat > "$TMPFILE" <<'EOF'
AT_INIT([Test Suite])
AT_SETUP([echo test])
AT_CHECK([echo hello], [0], [hello
])
AT_CLEANUP
EOF
expect_pass "execute AT_CHECK with echo command" $AXT "$TMPFILE"
rm -f "$TMPFILE"

# Test 5: AT_CHECK with wrong exit code should fail
TMPFILE=$(mktemp /tmp/axt_test_XXXXXX.at)
cat > "$TMPFILE" <<'EOF'
AT_INIT([Test Suite])
AT_SETUP([failing test])
AT_CHECK([false], [0])
AT_CLEANUP
EOF
expect_fail "AT_CHECK detects wrong exit code" $AXT "$TMPFILE"
rm -f "$TMPFILE"

# Test 6: AT_DATA creates file and AT_CHECK reads it
TMPFILE=$(mktemp /tmp/axt_test_XXXXXX.at)
cat > "$TMPFILE" <<'EOF'
AT_INIT([Test Suite])
AT_SETUP([data file test])
AT_DATA([input.txt], [hello world])
AT_CHECK([cat input.txt], [0], [hello world])
AT_CLEANUP
EOF
expect_pass "AT_DATA creates file for AT_CHECK" $AXT "$TMPFILE"
rm -f "$TMPFILE"

# Test 7: AT_CHECK with only command (defaults: exit=0, stdout=, stderr=)
TMPFILE=$(mktemp /tmp/axt_test_XXXXXX.at)
cat > "$TMPFILE" <<'EOF'
AT_INIT([Test Suite])
AT_SETUP([default args test])
AT_CHECK([true])
AT_CLEANUP
EOF
expect_pass "AT_CHECK with default arguments" $AXT "$TMPFILE"
rm -f "$TMPFILE"

# Test 8: Multiple test cases
TMPFILE=$(mktemp /tmp/axt_test_XXXXXX.at)
cat > "$TMPFILE" <<'EOF'
AT_INIT([Multi Test])
AT_SETUP([test one])
AT_CHECK([echo one], [0], [one
])
AT_CLEANUP
AT_SETUP([test two])
AT_CHECK([echo two], [0], [two
])
AT_CLEANUP
EOF
expect_pass "multiple test cases" $AXT "$TMPFILE"
rm -f "$TMPFILE"

# Test 9: Escape sequences @<:@ and @:>@
TMPFILE=$(mktemp /tmp/axt_test_XXXXXX.at)
cat > "$TMPFILE" <<'EOF'
AT_INIT([Test Suite])
AT_SETUP([escape test])
AT_CHECK([echo @<:@hello@:>@], [0], [[hello]
])
AT_CLEANUP
EOF
expect_pass "escape sequences @<:@ and @:>@" $AXT "$TMPFILE"
rm -f "$TMPFILE"

# Test 10: my_include with multiple files
TMPDIR_INC=$(mktemp -d /tmp/axt_inc_XXXXXX)
cat > "$TMPDIR_INC/suite.at" <<'EOF'
AT_INIT([Include Suite])
my_include([a.at])
my_include([b.at])
EOF
mkdir -p "$TMPDIR_INC/suite.src"
cat > "$TMPDIR_INC/suite.src/a.at" <<'EOF'
AT_SETUP([Test A])
AT_CHECK([echo a], [0], [a
])
AT_CLEANUP
EOF
cat > "$TMPDIR_INC/suite.src/b.at" <<'EOF'
AT_SETUP([Test B])
AT_CHECK([echo b], [0], [b
])
AT_CLEANUP
EOF
expect_pass "my_include with multiple files" $AXT "$TMPDIR_INC/suite.at"
rm -rf "$TMPDIR_INC"

# Summary
TOTAL=$((PASS + FAIL))
printf "\n---\nTests: %d  Passed: %d  Failed: %d\n" "$TOTAL" "$PASS" "$FAIL"

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
