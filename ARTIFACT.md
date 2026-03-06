# CHES 2026 Functional Artifact Guide

## Overview
This artifact supports functional evaluation of our FALCON key-recovery workflow using simple power analysis on a conditional calculator implementation.

## Artifact Badging Target
- CHES 2026: Functional

## System Requirements
- OS: WSL2 Ubuntu kernel 5.15.167.4-microsoft-standard-WSL2 on x86_64
- CPU/RAM: 12th Gen Intel(R) Core(TM) i7-12700K (20 CPUs), 31 GiB RAM
- Python: 3.11 (Docker image: `python:3.11-slim`)
- Toolchain: gcc-arm-none-eabi [FILL version]
- Hardware: ChipWhisperer Husky, target board CW308 (MCU: STM32F405)

## Installation
1) Clone the repository and enter it.
2) Install Python dependencies (if applicable).
3) Install toolchain and ChipWhisperer dependencies.

Example:
```bash
# Python deps (if requirements file exists)
python -m pip install -r python/requirements.txt

# Toolchain (example for Debian/Ubuntu)
sudo apt-get install gcc-arm-none-eabi
```

## Firmware for Capture
This artifact provides prebuilt firmware images; no firmware build step is required.

- CC variants: `fw/SPA_CC/FALCON_SPA_O{0,1,2,3,s}-CW308_STM32F4.hex`
- Falcon target: `fw/SPA_FALCON/FALCON_fpr_SPA-CW308_STM32F4.hex`

Firmware source files are provided under:
- `csrc/SPA/cc_spa.c`
- `csrc/SPA/fpr_spa.c`

## Trace Capture (Hardware Required)
1) Connect ChipWhisperer and target.
2) Configure the capture script.
3) Capture traces.

Example:
```bash
python ./python/capture/cc_gettrace.py
python ./python/capture/falcon_gettrace.py
```

## Run Analysis
Before running analysis, ensure required trace `.npy` files exist under `trace/full/` (fresh Git clones may not include large trace files).

Run the analysis on the provided trace subset:
```bash
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

## Verify Results
- Expected outputs:
  - CC: `outputs/cc/cc_snr.pdf`, `outputs/cc/cc_trace.pdf`, `outputs/cc/cc_scatter.png`, `outputs/cc/recovery_bit_CC.txt`
  - Falcon: `outputs/falcon/fpr_snr.pdf`, `outputs/falcon/fpr_scatter.png`, `outputs/falcon/recovery_bit_fpr.txt`
  - Falcon (postproc): `outputs/falcon/postproc/*.log`
  - Falcon (sidechannel): `outputs/falcon/sidechannel.txt`, `outputs/falcon/sidechannel_pr.txt`
- Expected success criteria: [FILL metric or key recovery target]
- Expected runtime (Docker, on the above system): CC main run ~4m21s; Falcon optional postproc runs vary by mode: postproc(mode 0) ~2m29s, postproc(mode 1) ~3s, rank ~16m02s, pr_rank ~18m23s, maxrank ~9s, maxrank_pr ~9s, maxrank_layer ~19s.

## Data / Traces
- Included: subset of traces sufficient to demonstrate the workflow.
- Note: if `trace/full/` is missing after clone/transfer, place the required trace `.npy` files there before executing Docker runs.
- Subset size:
  - `of`: 100 keys × 512 traces/key = 51,200 traces.
  - `add`: 100 keys × 6,144 traces/key = 614,400 traces.
- Subset sampling criteria:
  - Paper dataset uses N=1000 keys.
  - Artifact subset uses the first 100 keys only (prefix slice 0..99) due to size constraints.
- Full dataset: Zenodo record DOI: 10.5281/zenodo.18430573

## Reproducibility Notes
- Fixed seed is used in artifact runs for determinism.
- Paper results used randomized seeds; minor numerical differences may occur.
- Minor differences from the paper may occur due to capture/firmware changes (e.g., NOP insertion shifting trace length and SNR peak locations).
- Scatter plots may appear inverted (upper/lower swapped) depending on capture conditions; the key criterion is whether the conditional variable separates into two groups.
- Capture is performed in a separate hardware environment; input generation and capture are run as distinct steps.

## Optional CC Experiments
### Countermeasure Overhead
- Result:
  - Cycles (local): CC = 13, CM1 = 17, CM2 = 35
  - Overhead (local): CM1 = 30.8%, CM2 = 169.2%
  - Overhead (full): CM1 = 2.15%, CM2 = 11.83%
- Instruction summary (from paper):
  - Instruction | Cycle | CC | CM1 | CM2
  - LDR | 2 | 4 | 5 | 11
  - STR | 2 | 1 | 1 | 1
  - EOR | 1 | 2 | 0 | 4
  - AND | 1 | 1 | 0 | 2
  - LSL | 1 | 0 | 2 | 2
  - ADD | 1 | 0 | 1 | 1
  - SUB | 1 | 0 | 2 | 2
- Cycle estimate:
  - Total signature cycles (PQClean): 49,522,949
  - CC occurrences: 512 (floating-point conversion calls) × 8 (CC per conversion) + 6,144 (FFT fpr_add calls) × 8 (CC per fpr_add) = 266,240
  - CM1 overhead cycles: 266,240 × (17-13) = 1,064,960 → 2.15% of total
  - CM2 overhead cycles: 266,240 × (35-13) = 5,857,280 → 11.83% of total

### Compiler Flags
- Provided: prebuilt hex files compiled with -O0/-O1/-O2/-O3/-Os.
- Location: `fw/SPA_CC/`
- Notes: each hex contains CC/CM1/CM2. To reproduce traces, swap the hex file used by `python/capture/cc_gettrace.py`.

## Reusability
- Scripts are modular and can be extended to new targets or trace sets.
- Key parameters live in: `configs/main_cc.yaml`, `configs/main_falcon.yaml`, `configs/capture.yaml`

## Troubleshooting
- Toolchain not found: confirm `gcc-arm-none-eabi` is installed and on PATH.
- ChipWhisperer connection issues: verify firmware, USB permissions, and cable.
- Unexpected results: ensure the provided trace subset is used.
