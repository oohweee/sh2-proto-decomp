# Progress on decomp.dev

[decomp.dev](https://decomp.dev) tracks decompilation projects from an
[objdiff](https://github.com/encounter/objdiff) progress report that a GitHub Actions run uploads
on the default branch (the [integration guide](https://decomp.wiki/tools/decomp-dev)).

## The report

`configure.py` writes `objdiff.json`: every object linked into the 13 targets, each with its
target (the original, assembled) and, for C units, its base (built from our C; for a unit with
functions linked from the original code, the C alone, from `build/objdiff/`, so those functions
don't count as matched). Units carry a progress category (`game`, `libraries`, `data`) and a
`complete` flag, which decomp.dev shows as "fully linked": a unit is complete when no function
is linked from the original code and none is a fake match (`fake_units()` in `tools/progress.py`).

```sh
.venv/bin/python tools/download_tools.py --objdiff   # objdiff-cli, pinned
ninja report                                         # build/report.json
```

The game category of the report agrees with `tools/progress.py`. Across all code the figures are
lower, because the Sony libraries, the Metrowerks runtime and the sound driver stay assembly.

## Why the report is committed

Building needs the game disc, which a public CI runner can't have. So the report is generated
locally and committed as `progress/report.json`, and CI only uploads it. The file can't go stale
unnoticed: `tools/check_all.sh` regenerates it and fails if the committed copy differs (it is
deterministic). After a change: `ninja report && cp build/report.json progress/report.json`.

## Publishing it

The workflow is `.github/workflows/report.yml` (it needs no secrets and builds nothing):

```yaml
name: decomp.dev report
on:
  push:
    branches: [main]
  workflow_dispatch:
permissions:
  contents: read
jobs:
  report:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/upload-artifact@v4
        with:
          name: proto-2001-07-13_report
          path: progress/report.json
          if-no-files-found: error
```

The artifact name is `<version>_report`; the version is a label shown in decomp.dev's version
selector.

Then, as a repository admin: add the project at <https://decomp.dev/manage/new> (platform PS2),
and set its default category to `game`, so that the headline shows the game code rather than all
code. The [GitHub app](https://github.com/apps/decomp-dev) is optional: it replaces polling and
comments on pull requests.
