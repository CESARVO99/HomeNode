#!/usr/bin/env python3
"""
HomeNode QA Automation Script
================================
Runs unit tests, static analysis (cppcheck) and generates code metrics.
All results are saved in reports/<timestamp>/ inside the project.

Usage:
    python smrt_qa.py              # Run all (tests + analysis + metrics)
    python smrt_qa.py --tests      # Run only unit tests
    python smrt_qa.py --analysis   # Run only static analysis
    python smrt_qa.py --metrics    # Run only code metrics
    python smrt_qa.py --summary    # Show last report summary

Requirements:
    - PlatformIO CLI (pio) in PATH or installed as VS Code extension
    - cppcheck in PATH or installed via WinLibs/winget
    - GCC/G++ in PATH (for native test compilation)
    - Python 3.6+
"""

import os
import sys
import re
import subprocess
import shutil
from datetime import datetime
from pathlib import Path

# =============================================================================
# Configuration
# =============================================================================

# Project root (where this script lives)
PROJECT_ROOT = Path(__file__).parent.resolve()
SRC_DIR = PROJECT_ROOT / "src"
SRC_CORE_DIR = PROJECT_ROOT / "src" / "core"
SRC_MODULES_DIR = PROJECT_ROOT / "src" / "modules"
INCLUDE_DIR = PROJECT_ROOT / "include"
INCLUDE_CORE_DIR = PROJECT_ROOT / "include" / "core"
INCLUDE_MODULES_DIR = PROJECT_ROOT / "include" / "modules"
TEST_DIR = PROJECT_ROOT / "test"
REPORTS_DIR = PROJECT_ROOT / "reports"

# Tool paths - auto-detect or use PATH
PIO_PATHS = [
    shutil.which("pio"),
    str(Path.home() / ".platformio" / "penv" / "Scripts" / "pio.exe"),
    str(Path.home() / ".platformio" / "penv" / "bin" / "pio"),
]

CPPCHECK_PATHS = [
    shutil.which("cppcheck"),
]

# WinLibs common install path (winget)
_winget_pkg = Path.home() / "AppData" / "Local" / "Microsoft" / "WinGet" / "Packages"
if _winget_pkg.exists():
    for pkg_dir in _winget_pkg.iterdir():
        if "WinLibs" in pkg_dir.name:
            winlibs_bin = pkg_dir / "mingw64" / "bin"
            if (winlibs_bin / "gcc.exe").exists():
                CPPCHECK_PATHS.append(str(winlibs_bin / "cppcheck.exe"))
                # Also add GCC to the search
                os.environ["PATH"] = str(winlibs_bin) + os.pathsep + os.environ["PATH"]
                break


def find_tool(paths, name):
    """Find the first existing tool from a list of candidate paths."""
    for p in paths:
        if p and Path(p).exists():
            return str(p)
    print(f"[WARNING] {name} not found. Install it or add it to PATH.")
    return None


PIO_CMD = find_tool(PIO_PATHS, "PlatformIO (pio)")
CPPCHECK_CMD = find_tool(CPPCHECK_PATHS, "cppcheck")

# Source file extensions to analyze
SOURCE_EXTS = {".cpp", ".c"}
HEADER_EXTS = {".h", ".hpp"}
TEST_EXTS = {".cpp", ".c"}


# =============================================================================
# Utilities
# =============================================================================

def create_report_dir():
    """Create a timestamped report directory and return its path."""
    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    report_dir = REPORTS_DIR / timestamp
    report_dir.mkdir(parents=True, exist_ok=True)
    return report_dir, timestamp


def run_command(cmd, cwd=None, timeout=120):
    """Run a command and return (returncode, stdout+stderr)."""
    try:
        result = subprocess.run(
            cmd,
            cwd=cwd or str(PROJECT_ROOT),
            capture_output=True,
            text=True,
            timeout=timeout,
            shell=isinstance(cmd, str),
        )
        output = result.stdout + result.stderr
        return result.returncode, output
    except subprocess.TimeoutExpired:
        return -1, f"[ERROR] Command timed out after {timeout}s: {cmd}"
    except FileNotFoundError:
        return -1, f"[ERROR] Command not found: {cmd}"


def count_pattern(filepath, pattern):
    """Count regex matches in a file."""
    try:
        with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()
        return len(re.findall(pattern, content, re.MULTILINE))
    except Exception:
        return 0


def count_lines(filepath):
    """Count total lines, blank lines, comment lines, and code lines."""
    try:
        with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
            lines = f.readlines()
    except Exception:
        return 0, 0, 0, 0

    total = len(lines)
    blank = sum(1 for l in lines if l.strip() == "")
    comment = sum(
        1 for l in lines if re.match(r"^\s*(//|/\*|\*[^/]|\*/)", l)
    )
    code = total - blank - comment
    return total, code, blank, comment


def extract_functions_cc(filepath):
    """Extract functions and compute cyclomatic complexity for each.

    Returns a list of dicts: [{name, start_line, end_line, lines, cc}, ...]

    Cyclomatic Complexity (CC) = 1 + decision_points where:
      decision_points = if + else if + for + while + case + && + || + ? + catch
    """
    try:
        with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()
            f.seek(0)
            all_lines = f.readlines()
    except Exception:
        return []

    functions = []
    # Find function definitions: type name(params) {
    func_pattern = re.compile(
        r"^(\w[\w\s\*]+?)\s+(\w+)\s*\(([^)]*)\)\s*\{",
        re.MULTILINE,
    )

    for match in func_pattern.finditer(content):
        func_name = match.group(2)
        start_pos = match.start()
        brace_pos = match.end() - 1  # Position of opening {

        # Find the matching closing brace
        depth = 1
        pos = brace_pos + 1
        while pos < len(content) and depth > 0:
            ch = content[pos]
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
            pos += 1

        if depth != 0:
            continue

        # Extract function body
        func_body = content[brace_pos:pos]
        start_line = content[:start_pos].count("\n") + 1
        end_line = content[:pos].count("\n") + 1
        func_lines = end_line - start_line + 1

        # Calculate cyclomatic complexity
        cc = 1  # Base complexity
        # Remove strings and comments to avoid false positives
        clean_body = re.sub(r'"[^"\\]*(?:\\.[^"\\]*)*"', '""', func_body)
        clean_body = re.sub(r"'[^'\\]*(?:\\.[^'\\]*)*'", "''", clean_body)
        clean_body = re.sub(r"/\*.*?\*/", "", clean_body, flags=re.DOTALL)
        clean_body = re.sub(r"//[^\n]*", "", clean_body)

        cc += len(re.findall(r"\bif\s*\(", clean_body))
        cc += len(re.findall(r"\belse\s+if\b", clean_body))
        cc += len(re.findall(r"\bfor\s*\(", clean_body))
        cc += len(re.findall(r"\bwhile\s*\(", clean_body))
        cc += len(re.findall(r"\bcase\s+", clean_body))
        cc += len(re.findall(r"&&", clean_body))
        cc += len(re.findall(r"\|\|", clean_body))
        cc += len(re.findall(r"\?", clean_body))
        cc += len(re.findall(r"\bcatch\s*\(", clean_body))
        # Subtract double-counted else if (already counted in 'if')
        cc -= len(re.findall(r"\belse\s+if\b", clean_body))

        functions.append({
            "name": func_name,
            "start_line": start_line,
            "end_line": end_line,
            "lines": func_lines,
            "cc": cc,
        })

    return functions


# =============================================================================
# Changelog Generator
# =============================================================================

def generate_changelog(report_dir, timestamp, description=""):
    """Generate a changelog.md from recent git changes.

    Reads git log and diff to populate the changelog automatically.
    """
    lines = []
    lines.append(f"# Changelog — {timestamp.replace('_', ' ')}")
    lines.append("")

    # Get recent git log (last 5 commits)
    retcode, git_log = run_command(
        ["git", "log", "--oneline", "-10", "--no-decorate"],
        timeout=10,
    )

    # Get list of recently modified files
    retcode2, git_status = run_command(
        ["git", "diff", "--name-only", "HEAD~1", "HEAD"],
        timeout=10,
    )

    lines.append("## Cambios Realizados")
    if description:
        lines.append(f"- {description}")
    else:
        lines.append("- _(Descripcion generada automaticamente)_")
    lines.append("")

    lines.append("## Commits Recientes")
    if retcode == 0 and git_log.strip():
        for log_line in git_log.strip().splitlines()[:5]:
            lines.append(f"- `{log_line.strip()}`")
    else:
        lines.append("- _(No se pudo obtener el historial de git)_")
    lines.append("")

    lines.append("## Archivos Modificados")
    if retcode2 == 0 and git_status.strip():
        for fname in git_status.strip().splitlines():
            lines.append(f"- `{fname.strip()}`")
    else:
        lines.append("- _(No se detectaron cambios respecto al commit anterior)_")
    lines.append("")

    lines.append("## Resultados QA")
    lines.append("- _(Ver summary.txt para resultados detallados)_")
    lines.append("")

    changelog_text = "\n".join(lines) + "\n"

    (report_dir / "changelog.md").write_text(changelog_text, encoding="utf-8")
    print("\n" + "=" * 60)
    print("  CHANGELOG GENERATED")
    print("=" * 60)
    print(changelog_text)

    return changelog_text


# =============================================================================
# Test Runner
# =============================================================================

def run_tests(report_dir):
    """Run PlatformIO native tests and save results."""
    print("\n" + "=" * 60)
    print("  RUNNING UNIT TESTS (PlatformIO Unity)")
    print("=" * 60)

    if not PIO_CMD:
        msg = "[ERROR] PlatformIO not found. Cannot run tests.\n"
        print(msg)
        (report_dir / "test_results.txt").write_text(msg)
        return False, 0, 0, 0

    cmd = [PIO_CMD, "test", "-e", "native", "-v"]
    retcode, output = run_command(cmd, timeout=180)

    # Save raw output
    (report_dir / "test_results.txt").write_text(output)

    # Parse results
    total = passed = failed = 0
    match = re.search(r"(\d+) Tests (\d+) Failures (\d+) Ignored", output)
    if match:
        total = int(match.group(1))
        failed = int(match.group(2))
        passed = total - failed

    status = "PASS" if retcode == 0 and failed == 0 else "FAIL"
    print(output)
    print(f"\nResult: {status} ({passed}/{total} passed, {failed} failed)")

    return retcode == 0, total, passed, failed


# =============================================================================
# Static Analysis
# =============================================================================

def run_static_analysis(report_dir):
    """Run cppcheck static analysis and save results."""
    print("\n" + "=" * 60)
    print("  RUNNING STATIC ANALYSIS (cppcheck)")
    print("=" * 60)

    if not CPPCHECK_CMD:
        msg = "[ERROR] cppcheck not found. Cannot run static analysis.\n"
        print(msg)
        (report_dir / "static_analysis.txt").write_text(msg)
        return False, {}

    cmd = [
        CPPCHECK_CMD,
        "--enable=all",
        "--suppress=missingInclude",
        "--suppress=unknownMacro",
        "--std=c++11",
        "-I", str(INCLUDE_DIR),
        "-I", str(INCLUDE_CORE_DIR),
        "-I", str(INCLUDE_MODULES_DIR),
        str(SRC_DIR),
        str(INCLUDE_DIR),
    ]

    retcode, output = run_command(cmd, timeout=120)

    # Save raw output
    (report_dir / "static_analysis.txt").write_text(output)

    # Parse findings by severity
    findings = {"error": 0, "warning": 0, "style": 0, "performance": 0, "information": 0}
    for line in output.splitlines():
        for severity in findings:
            if f": {severity}:" in line.lower() or f"[{severity}]" in line.lower():
                # Avoid counting suppression info and checkers report
                if "unmatchedSuppression" not in line and "checkersReport" not in line:
                    findings[severity] += 1
                break

    print(output)
    print(f"\nFindings: {findings}")

    return True, findings


# =============================================================================
# Code Metrics
# =============================================================================

def generate_metrics(report_dir):
    """Generate code metrics and save report."""
    print("\n" + "=" * 60)
    print("  GENERATING CODE METRICS")
    print("=" * 60)

    lines = []
    lines.append("=" * 65)
    lines.append("  HOMENODE - CODE METRICS REPORT")
    lines.append(f"  Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append("=" * 65)
    lines.append("")

    # --- Source files (recursive scan) ---
    lines.append("--- SOURCE FILES (src/) ---")
    src_files = sorted(f for f in SRC_DIR.rglob("*") if f.suffix in SOURCE_EXTS)

    total_src_lines = 0
    total_src_code = 0
    total_functions = 0

    for f in src_files:
        total, code, blank, comment = count_lines(f)
        funcs = count_pattern(f, r"^\w.+\(.*\)\s*\{")
        total_src_lines += total
        total_src_code += code
        total_functions += funcs
        rel_path = f.relative_to(SRC_DIR)
        lines.append(
            f"  {str(rel_path):<35s} {total:4d} lines | {code:4d} code | "
            f"{blank:3d} blank | {comment:3d} comment | {funcs:2d} functions"
        )

    lines.append("")

    # --- Header files (recursive scan) ---
    lines.append("--- HEADER FILES (include/) ---")
    hdr_files = sorted(f for f in INCLUDE_DIR.rglob("*") if f.suffix in HEADER_EXTS)

    total_hdr_lines = 0
    for f in hdr_files:
        total, _, _, _ = count_lines(f)
        total_hdr_lines += total
        defines = count_pattern(f, r"^\s*#define")
        prototypes = count_pattern(f, r"^\w.+\(.*\);")
        rel_path = f.relative_to(INCLUDE_DIR)
        lines.append(
            f"  {str(rel_path):<35s} {total:4d} lines | {defines:3d} defines | "
            f"{prototypes:2d} prototypes"
        )

    lines.append("")

    # --- Test files ---
    lines.append("--- TEST FILES (test/) ---")
    total_test_lines = 0
    total_test_cases = 0

    for test_dir in sorted(TEST_DIR.iterdir()):
        if test_dir.is_dir():
            for f in sorted(test_dir.rglob("*")):
                if f.suffix in TEST_EXTS:
                    total, _, _, _ = count_lines(f)
                    total_test_lines += total
                    run_tests_count = count_pattern(f, r"RUN_TEST\(")
                    test_funcs = count_pattern(f, r"^void test_")
                    total_test_cases += run_tests_count
                    rel_path = f.relative_to(TEST_DIR)
                    lines.append(
                        f"  {str(rel_path):<35s} {total:4d} lines | "
                        f"{run_tests_count:2d} RUN_TEST | {test_funcs:2d} test functions"
                    )

    lines.append("")

    # --- Totals ---
    total_all = total_src_lines + total_hdr_lines + total_test_lines
    lines.append("--- TOTALS ---")
    lines.append(f"  Source code (src/):      {total_src_lines:5d} lines")
    lines.append(f"  Headers (include/):      {total_hdr_lines:5d} lines")
    lines.append(f"  Tests (test/):           {total_test_lines:5d} lines")
    lines.append(f"  Total project:           {total_all:5d} lines")
    lines.append("")

    # --- Complexity ---
    total_ifs = 0
    total_loops = 0
    total_switch = 0
    for f in src_files:
        total_ifs += count_pattern(f, r"\bif\s*\(")
        total_loops += count_pattern(f, r"\b(for|while)\s*\(")
        total_switch += count_pattern(f, r"\bswitch\s*\(")

    lines.append("--- COMPLEXITY INDICATORS (GLOBAL) ---")
    lines.append(f"  Total functions:         {total_functions:5d}")
    lines.append(f"  if statements:           {total_ifs:5d}")
    lines.append(f"  for/while loops:         {total_loops:5d}")
    lines.append(f"  switch statements:       {total_switch:5d}")
    lines.append("")

    # --- Cyclomatic Complexity per function ---
    lines.append("--- CYCLOMATIC COMPLEXITY PER FUNCTION ---")
    lines.append(f"  {'Function':<40s} {'Lines':>5s}  {'CC':>3s}  {'Level':<12s}")
    lines.append("  " + "-" * 65)

    all_func_cc = []
    for f in src_files:
        func_list = extract_functions_cc(f)
        for func in func_list:
            all_func_cc.append(func)
            cc = func["cc"]
            if cc <= 5:
                level = "simple"
            elif cc <= 10:
                level = "moderate"
            elif cc <= 20:
                level = "high"
            else:
                level = "REFACTOR!"
            lines.append(
                f"  {func['name']:<40s} {func['lines']:5d}  {cc:3d}  {level:<12s}"
            )

    if all_func_cc:
        avg_cc = sum(fn["cc"] for fn in all_func_cc) / len(all_func_cc)
        max_cc_func = max(all_func_cc, key=lambda fn: fn["cc"])
        lines.append("")
        lines.append(f"  Average CC:              {avg_cc:5.1f}")
        lines.append(f"  Max CC:                  {max_cc_func['cc']:5d}  ({max_cc_func['name']})")
        high_cc = [fn for fn in all_func_cc if fn["cc"] > 10]
        lines.append(f"  Functions CC > 10:       {len(high_cc):5d}")
    lines.append("")

    # --- Documentation ---
    doxygen = 0
    inline = 0
    for f in list(src_files) + list(hdr_files):
        doxygen += count_pattern(f, r"/\*\*")
    for f in src_files:
        inline += count_pattern(f, r"//[^/]")

    lines.append("--- DOCUMENTATION ---")
    lines.append(f"  Doxygen comment blocks:  {doxygen:5d}")
    lines.append(f"  Inline comments (src/):  {inline:5d}")
    lines.append("")

    # --- Ratios ---
    lines.append("--- CODE QUALITY RATIOS ---")
    if total_src_code > 0:
        comment_ratio = (doxygen + inline) / total_src_code * 100
        test_ratio = total_test_cases / total_functions * 100 if total_functions > 0 else 0
        lines.append(f"  Comment density:         {comment_ratio:5.1f}%")
        lines.append(f"  Test/function ratio:     {test_ratio:5.1f}%")
        lines.append(f"  Avg lines/function:      {total_src_code / total_functions:5.1f}" if total_functions > 0 else "  Avg lines/function:        N/A")

    report_text = "\n".join(lines) + "\n"

    # Save
    (report_dir / "metrics.txt").write_text(report_text)
    print(report_text)

    return {
        "src_lines": total_src_lines,
        "hdr_lines": total_hdr_lines,
        "test_lines": total_test_lines,
        "total_lines": total_all,
        "functions": total_functions,
        "test_cases": total_test_cases,
        "ifs": total_ifs,
        "loops": total_loops,
    }


# =============================================================================
# Summary
# =============================================================================

def generate_summary(report_dir, timestamp, test_ok, test_total, test_passed,
                     test_failed, analysis_ok, findings, metrics):
    """Generate a combined summary report."""
    lines = []
    lines.append("=" * 65)
    lines.append("  HOMENODE - QA SUMMARY REPORT")
    lines.append(f"  Timestamp: {timestamp}")
    lines.append(f"  Report:    {report_dir}")
    lines.append("=" * 65)
    lines.append("")

    # Tests
    test_status = "PASS" if test_ok else "FAIL"
    lines.append(f"  [TESTS]     {test_status}  {test_passed}/{test_total} passed, {test_failed} failed")

    # Analysis
    if analysis_ok:
        errors = findings.get("error", 0)
        warnings = findings.get("warning", 0)
        style = findings.get("style", 0)
        analysis_status = "PASS" if errors == 0 else "FAIL"
        lines.append(f"  [ANALYSIS]  {analysis_status}  {errors} errors, {warnings} warnings, {style} style")
    else:
        lines.append("  [ANALYSIS]  SKIP  cppcheck not available")

    # Metrics
    lines.append(f"  [METRICS]   {metrics.get('total_lines', 0)} total lines, "
                 f"{metrics.get('functions', 0)} functions, "
                 f"{metrics.get('test_cases', 0)} tests")
    lines.append("")

    # Files generated
    lines.append("  Files generated:")
    for f in sorted(report_dir.iterdir()):
        size = f.stat().st_size
        lines.append(f"    - {f.name} ({size:,} bytes)")
    lines.append("")

    summary_text = "\n".join(lines) + "\n"

    (report_dir / "summary.txt").write_text(summary_text)
    print(summary_text)

    return summary_text


def show_last_report():
    """Display the most recent report summary."""
    if not REPORTS_DIR.exists():
        print("No reports found.")
        return

    report_dirs = sorted(REPORTS_DIR.iterdir(), reverse=True)
    if not report_dirs:
        print("No reports found.")
        return

    last = report_dirs[0]
    summary_file = last / "summary.txt"
    if summary_file.exists():
        print(summary_file.read_text())
    else:
        print(f"Last report: {last}")
        for f in sorted(last.iterdir()):
            print(f"  - {f.name}")


# =============================================================================
# Main
# =============================================================================

def main():
    args = sys.argv[1:]

    if "--summary" in args:
        show_last_report()
        return 0

    run_all = not any(a in args for a in ["--tests", "--analysis", "--metrics"])

    report_dir, timestamp = create_report_dir()
    print(f"\nReport directory: {report_dir}\n")

    # Tests
    test_ok, test_total, test_passed, test_failed = False, 0, 0, 0
    if run_all or "--tests" in args:
        test_ok, test_total, test_passed, test_failed = run_tests(report_dir)

    # Static analysis
    analysis_ok, findings = False, {}
    if run_all or "--analysis" in args:
        analysis_ok, findings = run_static_analysis(report_dir)

    # Metrics
    metrics = {}
    if run_all or "--metrics" in args:
        metrics = generate_metrics(report_dir)

    # Changelog
    if run_all:
        generate_changelog(report_dir, timestamp)

    # Summary (only in full run)
    if run_all:
        generate_summary(
            report_dir, timestamp,
            test_ok, test_total, test_passed, test_failed,
            analysis_ok, findings, metrics,
        )

    # Exit code: 0 if tests pass (or not run), non-zero if tests fail
    if run_all or "--tests" in args:
        return 0 if test_ok else 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
