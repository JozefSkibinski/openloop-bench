set -e

COUNT=$1
MSG=$2
STALL_MS=${3:-0}
STALL_EVERY=${4:-0}
MODE=${5:-closed}
RATE=${6:-1000}

./echo_server --stall-ms "$STALL_MS" --stall-every-sec "$STALL_EVERY" &
SERVER_PID=$!

sleep 0.5

./loadgen "$COUNT" "$MSG" --mode "$MODE" --rate "$RATE" --stall-ms "$STALL_MS" --stall-every-sec "$STALL_EVERY"

kill $SERVER_PID
