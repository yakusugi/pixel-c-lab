# pixel-c-lab

Companion C sample programs for my Pixel rooting + C labs book.

## Layout
All labs are under `labs/`:

- `labs/01_uname_demo/`
- `labs/02_proc_uptime/`
- …
- `labs/13_droidstat/`

## Build (Ubuntu / Linux)
Example:

```bash
cd labs/01_uname_demo
clang -O2 -Wall uname_demo.c -o uname_demo
./uname_demo

