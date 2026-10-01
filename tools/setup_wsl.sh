#!/bin/sh
# One-time setup of the Linux (WSL Ubuntu 24.04, or any recent Debian/Ubuntu) build environment.
# Run from the repo root: sh tools/setup_wsl.sh
set -e

sudo apt-get update
sudo apt-get install -y python3 python3-venv python3-pip ninja-build git curl

python3 -m venv .venv
.venv/bin/pip install --upgrade pip
.venv/bin/pip install -r requirements.txt

.venv/bin/python tools/download_tools.py --objdiff

tools/wibo tools/mwcc/mwcps2-2.4-001213/mwccps2.exe -version
