#!/usr/bin/env bash
set -euo pipefail

# If COMPUTE_NAME isn’t provided, generate one
if [[ -z "${COMPUTE_NAME:-}" || "${COMPUTE_NAME}" == "auto" ]]; then
  # /proc/sys/kernel/random/uuid exists by default and doesn’t need uuidgen
  RAND_ID="$(cat /proc/sys/kernel/random/uuid)"
  # Optional: prefix to recognize the node in AWS console
  COMPUTE_NAME="odin-$(echo "$RAND_ID" | tr '[:upper:]' '[:lower:]')"
fi
: "${REGION:?Set REGION}"
export AWS_REGION="${AWS_REGION:-$REGION}"
: "${FLEET_ID:?Set FLEET_ID}"
: "${LOCATION:?Set LOCATION}"
: "${REGION:?Set REGION}"
: "${PUBLIC_IP:?Set PUBLIC_IP}"

exec java -jar /gamelift/agent.jar -c "${COMPUTE_NAME}" -f "${FLEET_ID}" -loc "${LOCATION}" -r "${REGION}" -ip-address "${PUBLIC_IP}"