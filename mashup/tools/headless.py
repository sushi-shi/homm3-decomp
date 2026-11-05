#!/usr/bin/env python3
"""Run the mashup runtime headless under Xvfb and script it.

    headless.py [--exe NAME] [--args ...] STEP...

Steps run in order:
    sleep:SECONDS      wait
    wait:SECONDS       wait until the game exits (at most SECONDS)
    key:KEYSYM         send a key press/release (hex keysym, e.g. 0x6e for 'n')
    click:X,Y          left click at window-relative coordinates
    shot:NAME          screenshot the root window to build/mashup-shots/NAME.png

The game is killed after the last step. With --record FILE.mp4 the whole
run is captured from the virtual display with ffmpeg, without the mouse
pointer, and with the game's sound: Wine plays into a private PulseAudio
null sink whose monitor ffmpeg records (nothing reaches the user's own
sound server). Run inside `nix develop .#build`.
"""
import argparse
import ctypes as c
import glob
import os
import subprocess
import shutil
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
RUNTIME = ROOT / "build/mashup-runtime"
SHOTS = ROOT / "build/mashup-shots"


def first(pattern):
    hits = sorted(glob.glob(pattern))
    if not hits:
        sys.exit(f"not found: {pattern}")
    return hits[0]


def x11():
    lib = c.CDLL(first("/nix/store/*-libx11-*/lib/libX11.so.6"))
    lib.XOpenDisplay.restype = c.c_void_p
    lib.XOpenDisplay.argtypes = [c.c_char_p]
    lib.XDefaultRootWindow.argtypes = [c.c_void_p]
    lib.XDefaultRootWindow.restype = c.c_ulong
    lib.XKeysymToKeycode.argtypes = [c.c_void_p, c.c_ulong]
    lib.XKeysymToKeycode.restype = c.c_uint
    lib.XFlush.argtypes = [c.c_void_p]
    lib.XSync.argtypes = [c.c_void_p, c.c_int]
    lib.XWarpPointer.argtypes = [c.c_void_p, c.c_ulong, c.c_ulong, c.c_int, c.c_int,
                                 c.c_uint, c.c_uint, c.c_int, c.c_int]
    return lib


def xtest():
    lib = c.CDLL(first("/nix/store/*-libxtst-*/lib/libXtst.so.6"))
    lib.XTestFakeKeyEvent.argtypes = [c.c_void_p, c.c_uint, c.c_int, c.c_ulong]
    lib.XTestFakeButtonEvent.argtypes = [c.c_void_p, c.c_uint, c.c_int, c.c_ulong]
    lib.XTestFakeMotionEvent.argtypes = [c.c_void_p, c.c_int, c.c_int, c.c_int, c.c_ulong]
    return lib


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default="Heroes3.exe")
    ap.add_argument("--size", default="800x600")
    ap.add_argument("--gameargs", default="/i0 /s0")
    ap.add_argument("--log", default=str(ROOT / "build/mashup-run.log"))
    ap.add_argument("--winedebug", default="-all")
    ap.add_argument("--record", help="capture the run to this video file")
    ap.add_argument("steps", nargs="*")
    args = ap.parse_args()

    SHOTS.mkdir(parents=True, exist_ok=True)
    r, w = os.pipe()
    xvfb = subprocess.Popen(
        [first("/nix/store/*-xvfb-*/bin/Xvfb"), "-displayfd", str(w), "-screen", "0",
         f"{args.size}x24", "-nolisten", "tcp", "+extension", "XTEST"],
        pass_fds=(w,), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    os.close(w)
    display = ":" + os.read(r, 32).decode().strip()
    os.close(r)
    env = dict(os.environ, DISPLAY=display,
               WINEPREFIX=str(ROOT / "build/mashup-wine"),
               WINEDEBUG=args.winedebug,
               WINEDLLOVERRIDES="mscoree,mshtml,winepulse.drv,winealsa.drv=",
               LIBGL_ALWAYS_SOFTWARE="1")
    # Wine prefers the Wayland driver when it can see a compositor, which
    # would put the game on the user's real desktop instead of Xvfb.
    env.pop("WAYLAND_DISPLAY", None)
    # A private PulseAudio with one null sink: the game's sound goes there and
    # the recorder takes it from the sink's monitor.
    pulse = None
    pulse_dir = None
    if args.record:
        pulse_dir = Path(tempfile.mkdtemp(prefix="mashup-pulse-"))
        socket = pulse_dir / "native"
        pulse = subprocess.Popen(
            [first("/nix/store/*-pulseaudio-[0-9]*/bin/pulseaudio"), "-n", "--daemonize=no",
             "--exit-idle-time=-1", "--use-pid-file=no", "--disable-shm=yes",
             "-L", f"module-native-protocol-unix auth-anonymous=1 socket={socket}",
             "-L", "module-null-sink sink_name=mashup rate=44100"],
            env=dict(os.environ, HOME=str(pulse_dir), XDG_RUNTIME_DIR=str(pulse_dir),
                     PULSE_RUNTIME_PATH=str(pulse_dir)),
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        for _ in range(100):
            if socket.exists():
                break
            time.sleep(0.1)
        env["PULSE_SERVER"] = f"unix:{socket}"
        env["PULSE_SINK"] = "mashup"
        env["WINEDLLOVERRIDES"] = "mscoree,mshtml,winealsa.drv="
    log = open(args.log, "w")
    game = subprocess.Popen(
        ["wine", args.exe] + args.gameargs.split(),
        cwd=RUNTIME, env=env, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT)
    os.environ["DISPLAY"] = display
    X, T = x11(), xtest()
    # Wait until the server takes connections; recording too early fails
    # with "Cannot open display".
    d = None
    for _ in range(100):
        d = X.XOpenDisplay(display.encode())
        if d:
            break
        time.sleep(0.1)
    recorder = None
    if args.record:
        for attempt in range(5):
            recorder = subprocess.Popen(
                # Both inputs stamp packets with the wall clock (x11grab and
                # pulse do so themselves); -copyts keeps those stamps and
                # shifts the whole file once. By default
                # ffmpeg starts each input at zero separately, and the sound
                # input opens a second after the picture, which put the sound
                # that much early.
                [first("/nix/store/*-ffmpeg-full-[0-9]*-bin/bin/ffmpeg"), "-loglevel", "error", "-y",
                 "-thread_queue_size", "1024",
                 "-f", "x11grab", "-draw_mouse", "0",
                 "-video_size", args.size, "-framerate", "25", "-i", display,
                 "-thread_queue_size", "1024",
                 "-f", "pulse", "-i", "mashup.monitor",
                 "-copyts", "-avoid_negative_ts", "make_zero",
                 "-c:v", "libx264", "-preset", "veryfast", "-crf", "20", "-pix_fmt", "yuv420p",
                 "-c:a", "aac", "-b:a", "160k", args.record],
                stdin=subprocess.PIPE, env=dict(env, XAUTHORITY=""))
            time.sleep(1)
            if recorder.poll() is None:
                break
            print("recorder failed to start, retrying", flush=True)
    try:
        for step in args.steps:
            kind, _, val = step.partition(":")
            if kind == "sleep":
                time.sleep(float(val))
            elif kind == "wait":
                deadline = time.time() + float(val)
                while game.poll() is None and time.time() < deadline:
                    time.sleep(1)
            elif kind == "key":
                code = X.XKeysymToKeycode(d, int(val, 0))
                T.XTestFakeKeyEvent(d, code, 1, 0)
                T.XTestFakeKeyEvent(d, code, 0, 30)
                X.XFlush(d)
                time.sleep(0.3)
            elif kind == "click":
                x, y = (int(v) for v in val.split(","))
                T.XTestFakeMotionEvent(d, -1, x, y, 0)
                X.XFlush(d)
                time.sleep(0.2)
                T.XTestFakeButtonEvent(d, 1, 1, 0)
                T.XTestFakeButtonEvent(d, 1, 0, 60)
                X.XFlush(d)
                time.sleep(0.3)
            elif kind == "shot":
                out = SHOTS / f"{val}.png"
                subprocess.run([first("/nix/store/*-imagemagick-*/bin/import"), "-display",
                                display, "-window", "root", str(out)], env=env, check=False)
                print("shot", out, flush=True)
            else:
                sys.exit(f"bad step {step}")
            if game.poll() is not None:
                print("game exited", game.returncode, flush=True)
                break
    finally:
        if recorder:
            recorder.communicate(b"q", timeout=60)
            print("recorded", args.record, flush=True)
        subprocess.run(["wineserver", "-k"], env=env, check=False)
        xvfb.terminate()
        xvfb.wait(timeout=5)
        if pulse:
            pulse.terminate()
            pulse.wait(timeout=5)
            shutil.rmtree(pulse_dir, ignore_errors=True)


if __name__ == "__main__":
    main()
