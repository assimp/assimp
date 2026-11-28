# © 2025-2026 Kevin G. Schlosser <kevin.g.schlosser@gmail.com>
#
# ============================================================================
# HANDS OFF: this is the one subprocess-running mechanism for the whole
# builder/ package. Every child process this codebase runs (cmake, ninja —
# anything) goes through spawn() below. Do not add a parallel
# subprocess.run()/subprocess.Popen()/Popen.communicate() call anywhere in
# builder/, and do not change spawn()'s core mechanism (shell relay + the
# per-pipe draining loop) — only extend what callers do with the
# (returncode, error_lines) it returns. Ported from harness_designer's
# builder/spawn.py, where this exact approach was already tried, reverted,
# and re-derived twice; see the reasoning below before considering changing
# it again.
#
# WHAT IT DOES
# Launches a shell (cmd.exe on Windows, bash on POSIX), writes `cmd` into its
# stdin as a single line, then closes stdin so the shell runs that one line
# and exits. Meanwhile it drains stdout (printing each line live, so a long
# build shows real progress instead of going silent) and stderr (collected,
# not printed, so callers can decide whether/when to surface it) until the
# process exits.
#
# WHY NOT subprocess.run(capture_output=True) / Popen.communicate()
# communicate() looks like the obvious "correct" replacement, but it solves
# a different, narrower problem than the one that actually matters here:
#
#   1. The classic dual-pipe deadlock (child fills one pipe's OS buffer
#      while you're blocked reading the other) — communicate() does solve
#      this, via select() on POSIX and a dedicated reader thread per stream
#      on Windows (Windows' select() doesn't work on arbitrary pipe
#      handles).
#   2. Windows pipe-handle inheritance by grandchild processes — this is
#      what communicate() does NOT solve, and it's the real culprit behind
#      "even the threaded fix hangs" reports. Redirecting stdout/stderr to a
#      pipe means CreateProcess's default handle inheritance can hand that
#      pipe's write handle down to a GRANDCHILD too — exactly what happens
#      when cl.exe spawns mspdbsrv.exe, or when a shell relays to whatever
#      program it's running (cmake spawning ninja spawning cl.exe, here).
#      Windows will not signal EOF on a pipe until every write handle is
#      closed, so if that grandchild holds one open — even after the process
#      you actually launched has exited — any blocking read on that pipe
#      hangs forever. This is true whether the read happens on the main
#      thread, a dedicated reader thread, or via select(); none of those
#      change what has to happen at the OS level for the read to return. A
#      daemon reader thread only means the *process* can still exit — the
#      hang itself is never actually resolved. (See CPython bpo-23213 and
#      Microsoft's own "Pipe Handle Inheritance" docs.)
#   3. Observability for long-running commands — communicate() is
#      all-or-nothing: it blocks until the process fully exits and only then
#      hands back everything it collected. For a multi-minute build (this
#      project's CMake+Ninja assimp build), that means total silence in the
#      log for the whole duration — indistinguishable from a genuine hang
#      until it either finishes or times out.
#
# spawn()'s own returncode is NOT reliable for detecting failure — it pipes
# `cmd` into a bare shell's stdin rather than running it directly
# (`cmd.exe`/`bash` with no `/c`/`-c`), and on Windows a bare `cmd.exe` fed a
# command this way exits 0 on natural EOF regardless of that command's own
# exit status. Callers must decide success/failure themselves from what
# spawn() returns (e.g. builder/assimp_build.py treats any non-empty
# error_lines that don't look like warnings, or a missing expected output
# file, as a failure).
# ============================================================================

import sys
import os
import subprocess
import threading


if sys.platform.startswith('win'):
    SHELL = 'cmd'
else:
    SHELL = 'bash'


DUMMY_RETURN = b''

print_lock = threading.Lock()


def spawn(cmd):
    # Print the command itself before doing anything else — cmd.exe happens
    # to echo piped-in commands back to stdout by default, but bash does not
    # (verified: a bare bash fed a command via stdin prints only the
    # command's own output, nothing else), so relying on the shell to show
    # what's running would be silent on macOS/Linux. Printing it ourselves
    # here is consistent on every platform regardless.
    with print_lock:
        sys.stdout.write(cmd + '\n')
        sys.stdout.flush()

    # `cmd` is a single line of shell text (may itself be a `cd X && Y && Z`
    # chain), not an argv list — it gets fed to the shell's stdin below, not
    # passed as Popen's own command-line argument.
    cmd += '\n'
    cmd = cmd.encode('utf-8')

    # Launch the shell itself with no arguments (not `cmd /c "..."` or
    # `bash -c "..."`) — an interactive-style session that reads and
    # executes whatever we write to its stdin, line by line, until stdin
    # closes. This is what the whole design hangs off: the shell (and
    # whatever it runs) is a single child process we fully control, so
    # there's no separate "build the argv" step to get subtly wrong per
    # platform.
    p = subprocess.Popen(
        SHELL,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.PIPE,
        env=os.environ
    )

    # Write the command, then close stdin — that EOF is what tells the
    # shell "no more input, run what you have and exit" once it finishes.
    p.stdin.write(cmd)
    p.stdin.close()

    error_lines = []

    # Drain both pipes ourselves instead of calling communicate() — see the
    # module-level comment at the top of this file for why. p.poll() being
    # None just means "still running"; the actual draining happens in the
    # two inner loops below, one pass over whatever's currently available
    # on each pipe per iteration of this outer loop, until the process exits
    # and both pipes hit EOF.
    while p.poll() is None:
        for line in iter(p.stdout.readline, DUMMY_RETURN):
            line = line.strip()
            if line:
                with print_lock:
                    sys.stdout.write(line.decode('utf-8') + '\n')
                    sys.stdout.flush()

        # Error output is collected, not printed here — callers decide
        # whether/when to surface it (e.g. only after a whole batch of
        # spawned commands has finished, to avoid interleaved output).
        for line in iter(p.stderr.readline, DUMMY_RETURN):
            line = line.strip()
            if line:
                error_lines.append(line.decode('utf-8'))

    if not p.stdout.closed:
        p.stdout.close()

    if not p.stderr.closed:
        p.stderr.close()

    sys.stdout.flush()

    # returncode here is the SHELL's own exit code, not reliably the piped
    # command's — see the module-level comment. Callers must apply their own
    # success/failure policy using this return value and error_text. Joined
    # into one string (rather than returned as a list) so callers can
    # classify a whole invocation's stderr at once — e.g. distinguishing a
    # genuine error from warning-only output — without re-splitting it
    # themselves.
    return p.returncode, '\n'.join(error_lines)
