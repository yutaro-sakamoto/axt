#!/bin/sh
# Test suite for axt

PASS=0
FAIL=0
AXT=./axt

run_test() {
    desc="$1"
    shift
    if "$@" > /dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

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

# Test 3: valid AT_INIT input -> success
TMPFILE=$(mktemp /tmp/axt_test_XXXXXX.at)
cat > "$TMPFILE" <<'EOF'
AT_INIT([Test Suite])
EOF
expect_pass "parse valid AT_INIT" $AXT "$TMPFILE"
rm -f "$TMPFILE"

# Test 4: AT_SETUP/AT_CLEANUP block
TMPFILE=$(mktemp /tmp/axt_test_XXXXXX.at)
cat > "$TMPFILE" <<'EOF'
AT_INIT([Test Suite])
AT_SETUP([Test Case 1])
AT_CHECK([echo hello], [0], [hello])
AT_CLEANUP
EOF
expect_pass "parse AT_SETUP/AT_CHECK/AT_CLEANUP block" $AXT "$TMPFILE"
rm -f "$TMPFILE"

# Summary
TOTAL=$((PASS + FAIL))
printf "\n---\nTests: %d  Passed: %d  Failed: %d\n" "$TOTAL" "$PASS" "$FAIL"

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi
exit 0
