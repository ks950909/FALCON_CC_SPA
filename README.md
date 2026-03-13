# Secret Key Recovery of FALCON using Simple Power Analysis in Conditional Calculator

This repository contains code, scripts, and data needed to reproduce the main results of our FALCON side-channel analysis using simple power analysis on a conditional calculator implementation.

## Scope
- Purpose: Reproduce key-recovery results and analysis workflow from the paper.
- Artifact target: CHES 2026 Functional.

## Quickstart
1) Clone the repository with submodules:
```bash
git clone --recurse-submodules https://github.com/ks950909/FALCON_CC_SPA.git
cd FALCON_CC_SPA
```
If you already cloned without submodules, run:
```bash
git submodule update --init --recursive
```
2) Install dependencies (toolchain + Python). See Requirements below.
3) (Optional) Select a prebuilt firmware image for ChipWhisperer capture.
4) Prepare the required CC trace files under `trace/full/`.
These large `.npy` files are not included in a fresh Git clone and must be obtained separately from the dataset release / Zenodo before running the CC pipeline.
5) Run the analysis script on the provided trace subset.

Example (adjust paths/commands for your environment):
```bash
# prebuilt firmware paths (examples)
# CC variants:   fw/SPA_CC/FALCON_SPA_O{0,1,2,3,s}-CW308_STM32F4.hex
# Falcon target: fw/SPA_FALCON/FALCON_fpr_SPA-CW308_STM32F4.hex

# run analysis on trace subset
python ./python/main_cc.py --config ./configs/main_cc.yaml
python ./python/main_falcon.py --config ./configs/main_falcon.yaml
```

Docker (CC pipeline):
```bash
docker build -f Dockerfile.cc -t falcon-cc .
docker run --rm -it \
  -v "$PWD/trace:/work/trace" \
  -v "$PWD/outputs:/work/outputs" \
  -v "$PWD/configs:/work/configs" \
  falcon-cc
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
- `trace/` trace data (Falcon subset + full CC experiment traces)
- `outputs/` example outputs and result artifacts
- `csrc/` C sources (target/crypto), including firmware-related sources under `csrc/SPA/`
- `csrc/pqclean/` PQClean source tree (git submodule; initialize with `git submodule update --init --recursive`)
- `fw/` prebuilt firmware images for capture

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
- Note: on a fresh clone, `trace/` may be empty depending on transfer/package settings; ensure the required `.npy` files are present under `trace/full/` before running Docker pipelines.
- Not included: full Falcon trace set from the paper (too large for artifact package).
- Full Falcon trace request: Zenodo record DOI: 10.5281/zenodo.18430573
- The CC pipeline requires the following files under `trace/full/`:
  - `trace_CC_O0.npy`, `pt_CC_O0.npy`, `ct_CC_O0.npy`
  - `trace_CC_m1_O0.npy`, `pt_CC_m1_O0.npy`, `ct_CC_m1_O0.npy`
  - `trace_CC_m2_O0.npy`, `pt_CC_m2_O0.npy`, `ct_CC_m2_O0.npy`
- These large `.npy` files may not be present in a fresh Git clone and should be prepared separately before running the CC pipeline.
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
- Use the provided prebuilt firmware images under `fw/` for capture.
- If `csrc/pqclean` appears empty, initialize submodules with `git submodule update --init --recursive`.
- If `python/main_cc.py` or `falcon-cc` reports missing files under `trace/full/`, ensure the required CC trace `.npy` files have been placed there.
- If traces are not detected, check ChipWhisperer connection and target configuration.
- If results differ, ensure you are using the provided seed and trace subset.

## License and Citation
- License: see `LICENSE` and `THIRD_PARTY_NOTICES.md`.
