# Secret Key Recovery of FALCON using Simple Power Analysis in Conditional Calculator

This repository contains code, scripts, and data needed to reproduce the main results of our FALCON side-channel analysis using simple power analysis on a conditional calculator implementation.

## Scope
- Purpose: Reproduce key-recovery results and analysis workflow from the paper.
- Artifact target: CHES 2026 Functional.

## Quickstart
1) Install dependencies (toolchain + Python). See Requirements below.
2) (Optional) Build target firmware for ChipWhisperer.
3) Run the analysis script on the provided trace subset.

Example (adjust paths/commands for your environment):
```bash
# build firmware (optional)
./scripts/build_firmware.sh

# run analysis on trace subset
python ./python/main_cc.py --config ./configs/main_cc.yaml
python ./python/main_falcon.py --config ./configs/main_falcon.yaml
```

Docker (Falcon pipeline, using prebuilt C binaries on host):
```bash
cmake -S csrc -B csrc/build -DCMAKE_BUILD_TYPE=Release
cmake --build csrc/build -j

docker build -f Dockerfile.falcon -t falcon-falcon .
docker run --rm -it \
  -v "$PWD/bin:/work/bin" \
  -v "$PWD/trace:/work/trace" \
  -v "$PWD/outputs:/work/outputs" \
  -v "$PWD/configs:/work/configs" \
  falcon-falcon
```

Docker (Figure generation: FPR add region plot):
```bash
docker build -f Dockerfile.fpr_add_region_plot -t fpr-add-region .
docker run --rm -v "$PWD/outputs:/work/outputs" fpr-add-region
```

## Requirements
- OS: Linux recommended (tested on: WSL2 Ubuntu kernel 5.15.167.4-microsoft-standard-WSL2 on x86_64)
- Toolchain: gcc-arm-none-eabi 9.2.1 (15:9-2019-q4-0ubuntu1)
- Python: 3.11 (Docker base: python:3.11-slim), see `python/requirements.txt`
- Hardware: ChipWhisperer Husky, target board CW308 (MCU: STM32F405)

## Repository Layout
- `python/` analysis and post-processing scripts (entrypoints under consolidation)
- `scripts/` helper scripts for building/capturing/running
- `trace/` trace data (Falcon subset + full CC experiment traces)
- `outputs/` example outputs and result artifacts
- `csrc/` C sources (target/crypto)
- `fw/` firmware-related files

## Reproducibility Notes
- Paper results were obtained with randomized seeds; the artifact uses a fixed seed for deterministic reproduction.
- The trace data included here is a subset due to size constraints. See the Data section.
- Minor differences from the paper may occur due to capture/firmware changes (e.g., NOP insertion shifting trace length and SNR peak locations).
- Scatter plots may appear inverted (upper/lower swapped) depending on capture conditions; the key criterion is whether the conditional variable separates into two groups.
- Capture is performed in a separate hardware environment; input generation and capture are run as distinct steps.

## Additional CC Experiments (Optional)
### Countermeasure Overhead
- Summary:
  - Cycles (local): CC = 13, CM1 = 17, CM2 = 35
  - Overhead (local): CM1 = 30.8%, CM2 = 169.2%
  - Overhead (full): CM1 = 2.15%, CM2 = 11.83%
- Details: see ARTIFACT.md (instruction table and cycle calculation).

### Compiler Flags
- Provided: prebuilt hex files compiled with -O0/-O1/-O2/-O3/-Os.
- Location: `fw/SPA_CC/`
- Notes: each hex contains CC/CM1/CM2. To reproduce traces, swap the hex file used by `python/capture/cc_gettrace.py`.

## Data / Traces
- Included: Falcon trace subset (artifact release) and full CC experiment traces.
- Not included: full Falcon trace set from the paper (too large for artifact package).
- Full Falcon trace request: Zenodo record DOI: 10.5281/zenodo.18430573
- Subset description:
  - Paper dataset: N=1000 keys.
  - Artifact subset: first 100 keys only, due to trace size constraints.
  - `of`: 100 keys × 512 traces/key = 51,200 traces.
  - `add`: 100 keys × 6,144 traces/key = 614,400 traces.
  - Subset sampling: prefix slice of keys (0..99).
- CC experiment data:
  - Paper dataset: N=1,000,000 traces (baseline + m1/m2).
  - Artifact release: full CC traces provided.
  - Location: `trace/full/`
  - Files: `trace_CC_O0.npy`, `pt_CC_O0.npy`, `ct_CC_O0.npy`, `trace_CC_m1_O0.npy`, `pt_CC_m1_O0.npy`, `ct_CC_m1_O0.npy`, `trace_CC_m2_O0.npy`, `pt_CC_m2_O0.npy`, `ct_CC_m2_O0.npy`

## Outputs
- Analysis outputs are written under `outputs/` by default.
- CC: `outputs/cc/cc_snr.pdf`, `outputs/cc/cc_trace.pdf`, `outputs/cc/cc_scatter.png`, `outputs/cc/recovery_bit_CC.txt`
- Falcon: `outputs/falcon/fpr_snr.pdf`, `outputs/falcon/fpr_scatter.png`, `outputs/falcon/recovery_bit_fpr.txt`
- Falcon (postproc): `outputs/falcon/postproc/*.log`
- Falcon (sidechannel): `outputs/falcon/sidechannel.txt`, `outputs/falcon/sidechannel_pr.txt`

## Troubleshooting
- If firmware build fails, verify `gcc-arm-none-eabi` is on PATH and matches the required version.
- If traces are not detected, check ChipWhisperer connection and target configuration.
- If results differ, ensure you are using the provided seed and trace subset.

## License and Citation
- License: see `LICENSE` and `THIRD_PARTY_NOTICES.md`.
