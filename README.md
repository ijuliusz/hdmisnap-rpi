# hdmisnap-rpi

A minimal HDMI screenshot tool for Raspberry Pi, using the legacy
dispmanx API. Captures whatever is currently being displayed (e.g. by
omxplayer or any other dispmanx-based output) and saves it as a small
BMP file.

## Why this exists

Standard screenshot tools (`fbgrab`, X11 screenshot utilities) do not
capture content rendered through dispmanx/OpenMAX (e.g. video played
by `omxplayer`), because that content bypasses the Linux framebuffer
and is composited directly by the GPU. This tool reads the GPU output
layer directly via `vc_dispmanx_snapshot`, so it captures exactly what
is shown on the HDMI output, including video.

## Output

- Format: BMP, 24-bit, no compression.
- Resolution: automatically scaled so the longer edge is at most
  854px, preserving the screen's aspect ratio (works for both
  landscape and portrait orientation).
- Typical file size: ~1.2 MB.
- Purpose: confirm whether *some* content is being displayed (vs. a
  black screen or a stray console prompt) - not for reading fine
  detail.

## Requirements

- Raspberry Pi OS with the legacy VideoCore libraries
  (`/opt/vc/include`, `/opt/vc/lib` - present on wheezy through
  bullseye by default; on bookworm/trixie these may need to be
  installed/retained separately).
- `gcc` and the `libraspberrypi-dev` package (provides `bcm_host.h`
  and `vc_dispmanx.h`).

## Building

```bash
gcc -std=gnu99 -o hdmisnap hdmisnap.c \
    -I/opt/vc/include \
    -I/opt/vc/include/interface/vcos/pthreads \
    -I/opt/vc/include/interface/vmcs_host \
    -I/opt/vc/include/interface/vmcs_host/linux \
    -L/opt/vc/lib \
    -lbcm_host -lvchiq_arm -lvcos
```

If `vcos_platform_types.h` is not found, locate it first and adjust
the `-I/opt/vc/include/interface/vcos/pthreads` path accordingly:

```bash
find /opt/vc/include -iname "vcos_platform_types.h"
```

A precompiled `hdmisnap` binary (built on Raspberry Pi OS wheezy,
armhf) is included in this repo for convenience. It should run on
later releases too (glibc backward compatibility), but has not been
verified on bookworm/trixie, where the legacy VideoCore stack may
need to be set up separately (see the `omx-wrapper` repo for the same
kind of issue with `omxplayer`).

## Usage

```bash
sudo ./hdmisnap output.bmp
```

If no filename is given, it defaults to `screen.bmp` in the current
directory.

## Known limitations

- Requires `sudo` (or appropriate permissions) to access the
  VideoCore interface.
- Single HDMI output only - not tested with dual-HDMI setups
  (e.g. Raspberry Pi 4).
- Nearest-neighbor scaling only (no interpolation) - sufficient to
  confirm there is content on screen, not for detailed inspection.
