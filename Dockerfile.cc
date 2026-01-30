FROM python:3.11-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work

COPY python/requirements.txt python/requirements.txt
RUN pip install --no-cache-dir -r python/requirements.txt

COPY . .

ENV PYTHONPATH=/work/python

CMD ["python", "/work/python/main_cc.py", "--config", "/work/configs/main_cc.yaml"]
