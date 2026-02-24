#!/usr/bin/env python3
"""Replace the LLVM test-results block in README.md.

Called by the daily-llvm-test workflow.  All inputs come from
environment variables set by the workflow step.
"""

import os
import re
from datetime import datetime, timezone
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
README = REPO_ROOT / "README.md"
TEST_LINES = Path("test-lines.txt")

START_MARKER = "<!-- LLVM_TEST_RESULTS_START -->"
END_MARKER = "<!-- LLVM_TEST_RESULTS_END -->"


def env(name: str, default: str = "") -> str:
    return os.environ.get(name, default)


def parse_test_lines() -> str:
    if not TEST_LINES.exists():
        return ""

    rows: list[str] = []
    for line in TEST_LINES.read_text().splitlines():
        m = re.search(r"Test\s+#\d+:\s+(.+?)\s+\.+", line)
        if not m:
            continue
        name = m.group(1)
        if "Passed" in line:
            rows.append(f"| {name} | Passed |")
        elif "Not Run" in line:
            rows.append(f"| {name} | **Not Run** |")
        else:
            rows.append(f"| {name} | **FAILED** |")

    return "\n".join(rows)


def build_block() -> str:
    llvm_sha = env("LLVM_SHA", "unknown")
    llvm_full = env("LLVM_FULL_SHA", "")
    llvm_date = env("LLVM_DATE", "unknown")
    passed = env("TEST_PASSED", "?")
    total = env("TEST_TOTAL", "?")
    exit_code = env("TEST_EXIT", "1")
    repo = env("GH_REPOSITORY", "studyztp/Nugget-LLVM-passes")
    run_id = env("GH_RUN_ID", "")
    run_date = datetime.now(timezone.utc).strftime("%Y-%m-%d")

    if exit_code == "0":
        status = "\u2705 passing"
    elif passed == "?" or total == "?":
        status = "\u26a0\ufe0f build failed"
    else:
        status = "\u274c failing"

    commit_link = (
        f"[`{llvm_sha}`](https://github.com/llvm/llvm-project/commit/{llvm_full})"
        if llvm_full
        else f"`{llvm_sha}`"
    )
    run_link = (
        f"[View Run](https://github.com/{repo}/actions/runs/{run_id})"
        if run_id
        else "N/A"
    )

    table_rows = parse_test_lines()

    lines = [
        START_MARKER,
        "### Latest LLVM Tip-of-Tree Test Results",
        "",
        "| | |",
        "|---|---|",
        f"| **Status** | {status} |",
        f"| **LLVM Commit** | {commit_link} |",
        f"| **LLVM Date** | {llvm_date} |",
        f"| **Tests Passed** | {passed} / {total} |",
        f"| **Run Date** | {run_date} |",
        f"| **Workflow** | {run_link} |",
        "",
        "<details>",
        "<summary>Per-test results (click to expand)</summary>",
        "",
        "| Test | Result |",
        "|------|--------|",
        table_rows,
        "",
        "</details>",
        END_MARKER,
    ]
    return "\n".join(lines)


def main() -> None:
    content = README.read_text()
    replacement = build_block()

    pattern = re.compile(
        rf"{re.escape(START_MARKER)}.*?{re.escape(END_MARKER)}", re.DOTALL
    )

    if pattern.search(content):
        content = pattern.sub(replacement, content)
    else:
        split = content.split("\n", 1)
        content = split[0] + "\n\n" + replacement + "\n\n" + split[1]

    README.write_text(content)
    print(f"README updated ({env('TEST_PASSED', '?')}/{env('TEST_TOTAL', '?')} passed)")


if __name__ == "__main__":
    main()
