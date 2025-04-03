# FFmpeg with Subpixel Zoompan

This is a fork of FFmpeg that adds **subpixel accuracy** to the `zoompan` filter, eliminating the jittery/shaky effect when applying smooth zoom animations.

## What's New

This fork introduces a new option to the `zoompan` filter:

`subpixel=1`

This enables **bilinear interpolation**, allowing smoother subpixel zooming and panning that isn't possible with upstream FFmpeg.

---

## The Problem

By default, FFmpeg's `zoompan` filter does not support subpixel precision. This causes **noticeable jitter** when zooming or panning, especially when values like `x` or `y` are set.

This bug is [well-known and still open (#4298)](https://trac.ffmpeg.org/ticket/4298).

### ❌ Without Subpixel (Default FFmpeg)

`ffmpeg -i in.png -loop 1 -vf "zoompan=z='zoom+0.05':x=50:d=150" -r 30 -t 6 -s 640x380 out_janky.mp4 -y`

### ✅ With Subpixel (This Fork)

`ffmpeg -i in.png -loop 1 -vf "zoompan=z='zoom+0.05':x=50:d=150:subpixel=1" -r 30 -t 6 -s 640x380 out_smooth.mp4 -y`

---

## Workaround Without This Patch

If you're using upstream FFmpeg, the only known workaround is to **scale before** applying `zoompan`, like so:

`ffmpeg -i in.png -vf "scale=hd720,zoompan=z='min(zoom+0.0015,1.4)':x=50:d=150:s=640x360" -t 6 out_workaround.mp4`

This helps a bit, but it's:

- Slower (especially on high-res inputs)
- Still not truly smooth
- More complex to chain in workflows

---

## ⚠️ Disclaimer

This patch was mostly generated using **DeepSeek V3** and **ChatGPT-4o**, and then reviewed manually by someone with close to zero c experience. Expect bugs and undefined behaviour.

---

## References

- [Bug Report: trac.ffmpeg.org/ticket/4298](https://trac.ffmpeg.org/ticket/4298)
- Related SuperUser threads:
  - https://superuser.com/q/1112617
  - https://superuser.com/q/776452

---

## License

This fork uses the same license as upstream FFmpeg. See the `LICENSE` file for details.

---

## Contributing

This fork is focused on solving one issue: **subpixel zooming with `zoompan`**. PRs and issues are welcome if they directly relate to improving this functionality.

For general FFmpeg contributions, see [ffmpeg.org](https://ffmpeg.org).
