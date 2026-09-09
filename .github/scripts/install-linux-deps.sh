#!/usr/bin/env bash
set -euo pipefail

# These dependencies come from Ubuntu; unrelated runner repositories previously broke apt update.
sources=/etc/apt/sources.list.d/ubuntu.sources
if [[ ! -s "$sources" ]]; then
  echo "Missing Ubuntu APT sources: $sources" >&2
  exit 1
fi
apt_opts=(-o "Dir::Etc::sourcelist=$sources" -o Dir::Etc::sourceparts=-)
for attempt in 1 2 3 4 5; do
  echo "Ubuntu apt attempt $attempt"
  if timeout 180 sudo apt-get "${apt_opts[@]}" update && \
     sudo DEBIAN_FRONTEND=noninteractive apt-get "${apt_opts[@]}" install -y \
       libgles2-mesa-dev libsdl2-dev libjpeg-dev; then
    exit 0
  fi
  sleep 15
done
exit 1
