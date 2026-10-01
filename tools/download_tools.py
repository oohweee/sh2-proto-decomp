#!/usr/bin/env python3
"""Fetch the pinned toolchain into tools/: the Metrowerks PS2 compiler, MIPS binutils, and wibo.

The build uses one compiler, mwcps2-2.4-001213 (docs/toolchain.md). Binutils are
platform-specific and go in tools/binutils/<platform>/. On Linux and macOS the build runs MWCC
under wibo, as decomp.me does.

Every download is pinned by its SHA-256 (SHA256 below) and checked before it is unpacked or
written: a download that differs stops the script, and nothing is installed from it.

    python3 tools/download_tools.py              # the build's tools
    python3 tools/download_tools.py --sweep      # also the other compiler builds, for the compiler
                                                 # sweep of docs/toolchain.md
                                                 # (tools/compiler_test/sweep.py) and version
                                                 # experiments; the build never uses them
    python3 tools/download_tools.py --permuter   # also decomp-permuter (tools/permute.py)
    python3 tools/download_tools.py --objdiff    # also objdiff-cli (objdiff.json reports)
"""
import hashlib
import io
import platform
import stat
import sys
import tarfile
import urllib.request
import zipfile
from pathlib import Path

TOOLS = Path(__file__).resolve().parent

MWCC_URL = "https://github.com/decompme/compilers/releases/download/compilers/{}.tar.gz"
MWCC_VERSIONS = ["mwcps2-2.4-001213"]  # the build's compiler (configure.py's MWCC)
MWCC_SWEEP = [
    # The other builds the compiler sweep compares (docs/toolchain.md; the game used 2.4.1.01,
    # which isn't available).
    "mwcps2-2.3.3-000906",
    "mwcps2-3.0-011126",
    # The nearest later builds, for compiler-version experiments.
    "mwcps2-3.0.1-020123",
    "mwcps2-3.0.3-020716",
]

BINUTILS_TAG = "v0.10"
BINUTILS_URL = "https://github.com/decompals/binutils-mips-ps2-decompals/releases/download/{}/{}"
BINUTILS_ASSETS = {
    "Windows": "binutils-mips-ps2-decompals-windows-x86-64.zip",
    "Linux": "binutils-mips-ps2-decompals-linux-x86-64.tar.gz",
    "Darwin": "binutils-mips-ps2-decompals-macos-arm64.tar.gz",
}

PERMUTER_COMMIT = "059609d4aec73eb0650726772954e1ad575825f8"
PERMUTER_URL = "https://github.com/simonlindholm/decomp-permuter/archive/{}.tar.gz"

OBJDIFF_TAG = "v3.8.1"
OBJDIFF_URL = "https://github.com/encounter/objdiff/releases/download/{}/{}"
OBJDIFF_ASSETS = {
    "Windows": "objdiff-cli-windows-x86_64.exe",
    "Linux": "objdiff-cli-linux-x86_64",
    "Darwin": "objdiff-cli-macos-arm64",
}

WIBO_TAG = "1.2.0"
WIBO_URL = "https://github.com/decompals/wibo/releases/download/{}/{}"
WIBO_ASSETS = {
    "Linux": "wibo-x86_64",
    "Darwin": "wibo-macos",
}

# SHA-256 of every file this script downloads; a new version or asset needs its hash here.
# The permuter comes from GitHub's source archive of a commit: should GitHub ever produce that
# archive differently, the check fails loudly rather than installing something unverified.
SHA256 = {
    MWCC_URL.format("mwcps2-2.4-001213"):
        "0ebe09436b4163c3a109b0431517281501d2de1bd5a0b049d33057710a943e3e",
    MWCC_URL.format("mwcps2-2.3.3-000906"):
        "c2a4e463f8f7b324350d6bfb26d6daed83f4d5cb46df8b20dcfec8617b22ad5d",
    MWCC_URL.format("mwcps2-3.0-011126"):
        "b83fef398fd765aaeb48576dae0fa0751bf9c1496036d0d878e7a2a0e71c2a7f",
    MWCC_URL.format("mwcps2-3.0.1-020123"):
        "72d823d85943c92ca13378e93d56ac1870298ccbfda0ef8a2efb9650692e968c",
    MWCC_URL.format("mwcps2-3.0.3-020716"):
        "63f89baae691403294724178efbcfc9e1b9ac48151536b6a321b17eb8f9970e5",
    BINUTILS_URL.format(BINUTILS_TAG, BINUTILS_ASSETS["Windows"]):
        "2bb5904ee174e04f1600cc70ffdb3dc36b0efa4d1892ea446be588ba8dc6198c",
    BINUTILS_URL.format(BINUTILS_TAG, BINUTILS_ASSETS["Linux"]):
        "9fe31ea3ee1a37536f9f0e2e12c668e7c3cb99e59f5154bd2f1fc4762473094c",
    BINUTILS_URL.format(BINUTILS_TAG, BINUTILS_ASSETS["Darwin"]):
        "948cc736e01b2c135168caabfb616f6ed8efe89f1dad1ed8ef1ca74acb1b742c",
    PERMUTER_URL.format(PERMUTER_COMMIT):
        "2e4e0d991e4258df332c19d8dcf372212fe27f1b18e777d9e1eb25fc2c64a404",
    OBJDIFF_URL.format(OBJDIFF_TAG, OBJDIFF_ASSETS["Windows"]):
        "f2a8865f3b928dc412e4f633626fe138edc79077d6fcf23e63b7cfe649c154b4",
    OBJDIFF_URL.format(OBJDIFF_TAG, OBJDIFF_ASSETS["Linux"]):
        "c8290281e82114bcc1a06ff73061110d3902a177822e750337de2537188e358f",
    OBJDIFF_URL.format(OBJDIFF_TAG, OBJDIFF_ASSETS["Darwin"]):
        "98f8275c27900c4fe2248fce3af37617658be49648fa7dbb5b376371f046dfdb",
    WIBO_URL.format(WIBO_TAG, WIBO_ASSETS["Linux"]):
        "13f86a2d618f0dbe67179d349625345eabf9b46450295cb4c904e49f6aff85af",
    WIBO_URL.format(WIBO_TAG, WIBO_ASSETS["Darwin"]):
        "2b3000ef6a7a490c24ccd71967735ae0005e218922e51806cca1b8d77fd3cf7c",
}


def fetch(url):
    """The file at `url`, once its SHA-256 matches the pinned one (SHA256)."""
    print(f"fetching {url}")
    with urllib.request.urlopen(url) as r:
        data = r.read()
    digest = hashlib.sha256(data).hexdigest()
    if url not in SHA256:
        sys.exit(f"{url}: no pinned SHA-256 in tools/download_tools.py (the download's is {digest})")
    if digest != SHA256[url]:
        sys.exit(f"{url}: SHA-256 {digest}, expected {SHA256[url]}: not installed")
    return data


def unpack(data, name, dest):
    dest.mkdir(parents=True, exist_ok=True)
    if name.endswith(".zip"):
        zipfile.ZipFile(io.BytesIO(data)).extractall(dest)
    else:
        tarfile.open(fileobj=io.BytesIO(data)).extractall(dest, filter="data")


def main():
    system = platform.system()
    args = sys.argv[1:]

    for ver in MWCC_VERSIONS + (MWCC_SWEEP if "--sweep" in args else []):
        dest = TOOLS / "mwcc" / ver
        if not dest.exists():
            unpack(fetch(MWCC_URL.format(ver)), ".tar.gz", dest)

    dest = TOOLS / "binutils" / system.lower()
    if not dest.exists():
        asset = BINUTILS_ASSETS[system]
        unpack(fetch(BINUTILS_URL.format(BINUTILS_TAG, asset)), asset, dest)

    if system in WIBO_ASSETS:
        dest = TOOLS / "wibo"
        if not dest.exists():
            dest.write_bytes(fetch(WIBO_URL.format(WIBO_TAG, WIBO_ASSETS[system])))
            dest.chmod(dest.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    dest = TOOLS / "decomp-permuter"
    if "--permuter" in args and not dest.exists():
        tmp = TOOLS / "decomp-permuter.tmp"
        unpack(fetch(PERMUTER_URL.format(PERMUTER_COMMIT)), ".tar.gz", tmp)
        (tmp / f"decomp-permuter-{PERMUTER_COMMIT}").rename(dest)
        tmp.rmdir()

    dest = TOOLS / ("objdiff-cli.exe" if system == "Windows" else "objdiff-cli")
    if "--objdiff" in args and not dest.exists():
        dest.write_bytes(fetch(OBJDIFF_URL.format(OBJDIFF_TAG, OBJDIFF_ASSETS[system])))
        dest.chmod(dest.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
    return 0


if __name__ == "__main__":
    sys.exit(main())
