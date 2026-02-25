# SixEyes CI/CD Pipeline Documentation

Complete reference for the automated build, test, and release pipeline for SixEyes firmware.

---

## Table of Contents

1. [Pipeline Overview](#pipeline-overview)
2. [Build Workflow](#build-workflow)
3. [Code Quality Workflow](#code-quality-workflow)
4. [Release Workflow](#release-workflow)
5. [Configuration & Customization](#configuration--customization)
6. [Troubleshooting](#troubleshooting)

---

## Pipeline Overview

### Architecture Diagram

```
GitHub Push/PR
    ↓
├─→ PlatformIO Build (platformio-build.yml)
│   ├─→ Compile firmware
│   ├─→ Check memory constraints
│   ├─→ Verify no warnings
│   ├─→ Generate artifacts
│   └─→ Comment on PR
│
├─→ Code Quality (code-quality.yml)
│   ├─→ Static analysis (cppcheck)
│   ├─→ Format checking (clang-format)
│   ├─→ Dependency validation
│   ├─→ Compiler warnings
│   └─→ Include guard checks
│
└─→ [If tagged: v*.*.* ] Release (release.yml)
    ├─→ Build release firmware
    ├─→ Generate checksums (SHA256)
    ├─→ Create GitHub release
    └─→ Upload artifacts
```

### Triggers

| Event | Workflow | When |
|:------|:---------|:-----|
| **Push to main/develop** | All 3 workflows | Any change to firmware or CI files |
| **Pull Request** | All 3 workflows | When PR targets main/develop |
| **Git Tag (v*.*.*) ** | Release only | When pushing a version tag |
| **Manual Dispatch** | All 3 workflows | Trigger from GitHub UI |

---

## Build Workflow

### File: `.github/workflows/platformio-build.yml`

Comprehensive firmware compilation with quality gates.

#### What It Does

```
1. Checkout repository
2. Set up Python 3.11
3. Install PlatformIO
4. Build firmware for ESP32-S3
5. Verify memory constraints (< 80%)
6. Check for compiler warnings
7. Generate build artifacts
8. Comment results on PR
```

#### Execution Time

- **Fast path** (cache hit): ~2-3 minutes
- **Full rebuild**: ~5-7 minutes

#### Memory Constraints

```yaml
Fails if:
  - RAM usage > 80% (currently ~6%)
  - Flash usage > 80% (currently ~10%)
  - Any compiler warnings found
```

#### Artifacts Generated

```
firmware.bin    → 335 KB (main application binary)
firmware.elf    → 1.2 MB (debug symbols)
firmware.map    → 50 KB (symbol map)
build_report.md → HTML summary for PR comment
```

#### PR Comment Example

The workflow automatically comments on PRs with:

```markdown
# Build Report

**Environment**: follower_esp32
**Date**: 2026-02-25 15:30:45 UTC
**Git Commit**: abc123def456...

## Build Status
✅ **PASSED**

## Memory Summary
```
RAM:   [=         ]   6.1% (used 20064 bytes from 327680 bytes)
Flash: [=         ]  10.0% (used 335257 bytes from 3342336 bytes)
```

## Artifacts
- `firmware.bin`: 335257 bytes
- `firmware.elf`: 1268543 bytes
```

---

## Code Quality Workflow

### File: `.github/workflows/code-quality.yml`

Multi-stage code quality checks (non-blocking).

#### Stages

##### 1. Static Analysis (cppcheck)

```bash
cppcheck --enable=all \
  --suppress=missingIncludeSystem \
  --std=c++17 \
  src/ include/
```

**Checks**:
- Uninitialized variables
- Memory leaks
- Buffer overflows
- Logic errors
- Performance issues

**Status**: Non-blocking (warnings only)

##### 2. Code Formatting (clang-format)

```bash
clang-format --dry-run --Werror <file>
```

**Checks**:
- Line length (max 120 chars)
- Indentation (2 spaces)
- Brace style (K&R)
- Spacing conventions

**Status**: Blocks on failure

##### 3. Dependency Validation

```bash
pio boards esp32-s3-devkitc-1  # Verify board
grep lib_deps platformio.ini   # List libraries
```

**Verifies**:
- Board configuration valid
- Dependencies available
- Version compatibility

##### 4. Header Guard Checks

```bash
find . -name "*.h" -exec grep -L "#pragma once" {} \;
```

**Requirement**: All headers must have `#pragma once`

##### 5. Compiler Warnings

```bash
pio run --verbose 2>&1 | grep "warning:"
```

**Policy**: Zero warnings tolerance

**Status**: Blocks build if warnings found

---

## Release Workflow

### File: `.github/workflows/release.yml`

Automated release builds triggered by version tags.

### Trigger

```bash
git tag -a v1.0.0 -m "Release: ROS2 heartbeat integration"
git push origin v1.0.0
```

When you push a tag matching `v*.*.*`, the release workflow:

1. **Builds** the firmware
2. **Generates checksums** (SHA256)
3. **Creates GitHub release** with artifacts
4. **Uploads** all documentation

### What Gets Released

```
firmware-v1.0.0.bin          (335 KB)
firmware-v1.0.0.elf          (1.2 MB)
firmware-v1.0.0.map          (50 KB)
firmware.bin.sha256          (64 byte checksum)
firmware.elf.sha256
firmware.map.sha256
RELEASE_NOTES.md             (auto-generated)
JSON_MESSAGE_PROTOCOL.md
FLASHING_AND_DEPLOYMENT.md
IMPLEMENTATION_SUMMARY.md
VISUAL_ARCHITECTURE_GUIDE.md
WIRING_AND_ASSEMBLY.md
```

### Release Notes

Automatically generated with:
- Version number
- Build date & time
- Commit hash
- Features list
- Hardware requirements
- Build instructions
- Memory usage

### Checksum Verification

After downloading release artifacts:

```bash
sha256sum -c firmware.bin.sha256
# Output: firmware.bin: OK
```

### Example Release Page

```
Release: SixEyes Firmware v1.0.0
Published Feb 25, 2026

Features
✅ 400 Hz deterministic control loop
✅ ROS2 heartbeat integration
✅ 4× stepper motor control
✅ 3× servo motor control
✅ JSON message protocol

Downloads
- firmware-v1.0.0.bin      (335 KB)
- firmware-v1.0.0.elf      (1.2 MB)
- Complete documentation

Memory Usage: 6.1% RAM, 10.0% Flash ✅
```

---

## Configuration & Customization

### Modifying Build Triggers

**File**: `.github/workflows/platformio-build.yml`

Current configuration:
```yaml
on:
  push:
    branches: [main, develop]
    paths:
      - 'sixeyes/firmware/follower_esp32/src/**'
      - 'sixeyes/firmware/follower_esp32/platformio.ini'
  pull_request:
    branches: [main, develop]
```

**To also trigger on doc changes**:
```yaml
paths:
  - 'sixeyes/firmware/follower_esp32/**'
  - 'JSON_MESSAGE_PROTOCOL.md'
```

### Custom Build Flags

**File**: `sixeyes/firmware/follower_esp32/platformio.ini`

```ini
[env:follower_esp32]
build_flags =
  -DCORE_MHZ=240
  -DDEBUG_LEVEL=2          # Add debug output
  -DENABLE_TELEMETRY=1     # Compile telemetry
  -DHEARTBEAT_TIMEOUT_MS=500
```

Add to `platformio.ini`, then rebuild:
```bash
pio run --define="DEBUG_LEVEL=2"
```

### Memory Threshold Adjustment

In `platformio-build.yml`, change the 80% threshold:

```yaml
- name: Check Memory Usage
  run: |
    MAX_RAM=$((320 * 70 / 100))  # 70% threshold
    MAX_FLASH=$((3342336 * 60 / 100))  # 60% threshold
    ...
```

### Adding Static Analysis Tools

**Example**: Add sanitizers to compiler flags

```bash
build_flags =
  -fsanitize=address
  -fsanitize=undefined
```

**Note**: May increase binary size

---

## Status Badges

Add to `README.md` to show build status:

```markdown
[![Build Status](https://github.com/SixEyes-Open-Source/sixeyes/actions/workflows/platformio-build.yml/badge.svg)](https://github.com/SixEyes-Open-Source/sixeyes/actions)
[![Code Quality](https://github.com/SixEyes-Open-Source/sixeyes/actions/workflows/code-quality.yml/badge.svg)](https://github.com/SixEyes-Open-Source/sixeyes/actions)
```

---

## Troubleshooting

### Build Fails with "Module not found"

**Error**: `UNRESOLVED_LIBRARY: ArduinoJson`

**Solution**:
```bash
pio lib install "ArduinoJson"
# Or rebuild cache:
rm -rf ~/.platformio/
pio run
```

### Memory Constraint Failure

**Error**: `WARNING: RAM usage exceeds 80%`

**Debug**:
```bash
pio run --target size
# Shows detailed memory map

# Check largest functions
objdump -S firmware.elf | head -100
```

**Solutions**:
- Remove debug features
- Optimize loops to remove allocations
- Increase threshold (not recommended)

### Warnings Not Failing Build

**Check**: Warnings are supposed to fail the build

**Verify**:
```bash
grep "exit 1" .github/workflows/code-quality.yml
```

**If missing**, add to build step:
```yaml
- run: pio run --verbose 2>&1 | grep -q "warning:" && exit 1
```

### Release Not Creating GitHub Release

**Check**: Did you use correct tag format?

```bash
git tag v1.0.0    # ✅ Correct format (triggers release)
git tag release-1 # ❌ Wrong format (skipped)
git tag 1.0.0     # ❌ Missing 'v' prefix
```

**Verify tag exists**:
```bash
git tag -l
# Output: v1.0.0
```

### Artifacts Not Uploaded

**Check workflow logs**:
1. Go to GitHub → Actions
2. Click failing workflow
3. Expand "Build Release Firmware" job
4. Look for error messages

**Common issues**:
- Firmware build failed (check compiler output)
- Path wrong in copy command
- Insufficient disk space

### Slow Builds

**Cache Benefits**:
- First build: ~7 minutes (downloads ESP-IDF)
- Cached builds: ~2-3 minutes

**To clear cache** (manual):
1. GitHub → Settings → Actions → General
2. "Remove all caches"
3. Next push rebuilds cache

### PR Comment Not Appearing

**Check**:
1. Did build complete successfully?
2. Is it a pull request (not a direct push)?

**Permissions issue**?
Edit workflow:
```yaml
permissions:
  contents: read
  pull-requests: write  # Required for comments
```

---

## Performance Metrics

### Build Times

```
Matrix: 1 environment × 3 workflows
├─ platformio-build.yml:    ~5 min
├─ code-quality.yml:        ~4 min
└─ release.yml:             ~6 min
────────────────────────────
Total per commit: ~15 minutes (worst case)
                  ~5 minutes (with caching)
```

### Resource Usage

```
CPU: 2 cores (GitHub Actions standard)
Memory: 7 GB available
Storage: 14 GB temporary workspace
Artifacts: 30-day retention (configurable)
```

---

## Best Practices

### ✅ Do

- **Always** create PR for changes
- **Use** descriptive commit messages
- **Test** locally before push:
  ```bash
  pio run
  cd sixeyes/firmware/follower_esp32
  pio run --verbose
  ```
- **Tag** releases with semantic versioning
- **Keep** workflows simple and focused

### 🚫 Don't

- **Force push** to main (breaks CI consistency)
- **Ignore** CI failures (they're blocking for a reason)
- **Commit** generated artifacts
- **Disable** code quality gates
- **Merge** PRs without passing checks

---

## Advanced Topics

### Custom Notifications

Send Slack message on build failure:

```yaml
- name: Notify Slack on Failure
  if: failure()
  uses: 8398a7/action-slack@v3
  with:
    status: ${{ job.status }}
    webhook_url: ${{ secrets.SLACK_WEBHOOK }}
```

### Hardware Testing Integration

For future: add hardware smoke tests
```yaml
- name: Flash and Test Hardware
  run: |
    pio run --target upload
    # Run serial port tests
    python test_hardware.py /dev/ttyUSB0
```

### Performance Benchmarking

Track build size over time:
```yaml
- name: Track Firmware Size
  run: |
    SIZE=$(stat -c%s firmware.bin)
    echo "firmware_size_bytes{build=\"$GITHUB_RUN_ID\"} $SIZE" >> metrics.txt
```

---

## GitHub Actions Insights

**To view CI metrics**:
1. GitHub → Actions → Workflows
2. Select `platformio-build.yml`
3. See execution times, pass/fail trends
4. Download logs for failed runs

**To troubleshoot failed job**:
1. Click failed run
2. Expand failed job name
3. Read step output (search for "Error" or "FAILED")
4. Copy error message
5. Reproduce locally if possible

---

**CI/CD Pipeline Complete! 🎯**

For questions or issues, refer to GitHub Actions logs or create an issue on GitHub.
