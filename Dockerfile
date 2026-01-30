FROM python:3.11-slim

# 1) 시스템 패키지 (C 빌드용)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git \
    && rm -rf /var/lib/apt/lists/*

# 2) 작업 디렉토리
WORKDIR /work

# 3) Python 의존성 설치 (캐시 효율 위해 먼저 복사)
COPY python/requirements.txt python/requirements.txt
RUN pip install --no-cache-dir -r python/requirements.txt

# 4) 프로젝트 전체 복사
COPY . .

# 5) Python 코드 경로 지정 (패키지 설치가 아니라 경로로 실행)
ENV PYTHONPATH=/work/python

# 6) C 빌드 (여러 실행파일이면 전부 빌드됨)
RUN cmake -S csrc -B csrc/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build csrc/build -j \
    && mkdir -p bin \
    && find csrc/build -maxdepth 1 -type f -executable -exec cp -f {} bin/ \; || true

# 7) 기본 실행: 스모크 테스트 (분석/재현 전용)
CMD ["bash", "scripts/01_smoketest.sh"]
