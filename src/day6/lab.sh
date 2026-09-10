#!/usr/bin/env bash
# All network mutations are confined to named, isolated lab namespaces.
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
lab_dir="$PWD"
state="$lab_dir/.state"
names=(iot25-d6-iot iot25-d6-srv iot25-d6-admin iot25-d6-router)

if [[ $EUID != 0 ]]; then
    echo 'Run with sudo in a Linux lab environment'
    exit 1
fi

for tool in ip nft python3 sysctl; do
    if ! command -v "$tool" >/dev/null; then
        echo "Missing: $tool"
        exit 1
    fi
done

inside() {
    ip netns exec iot25-d6-router "$@"
}

owned() {
    if [[ ! -f "$state/owner" ]]; then
        echo 'No locally owned lab; run up first'
        exit 1
    fi
}

down() {
    owned
    for name in "${names[@]}"; do
        if ip netns list | awk '{print $1}' | grep -qx "$name"; then
            for pid in $(ip netns pids "$name"); do
                kill "$pid" 2>/dev/null || true
            done
            ip netns del "$name"
        fi
    done
    rm -- "$state/owner"
    echo 'Lab namespaces removed; backup retained in .state'
}

cleanup_failed_setup() {
    echo 'Setup failed; removing only this lab'
    down
}

up() {
    for name in "${names[@]}"; do
        if ip netns list | awk '{print $1}' | grep -qx "$name"; then
            echo "Name already exists: $name; refusing to overwrite"
            exit 1
        fi
    done
    if [[ -e "$state/owner" ]]; then
        echo 'Previous owner marker exists; inspect then run down'
        exit 1
    fi

    mkdir -p "$state"
    printf '%s\n' 'iot25 day6 isolated lab' > "$state/owner"
    trap cleanup_failed_setup ERR
    for name in "${names[@]}"; do
        ip netns add "$name"
        ip -n "$name" link set lo up
    done
    for item in iot:10 srv:20 admin:30; do
        leaf="${item%:*}"
        subnet="${item#*:}"
        # Both endpoints are created inside namespaces, never on the host LAN.
        ip -n iot25-d6-router link add "r-$leaf" type veth \
            peer name eth0 netns "iot25-d6-$leaf"
        ip -n iot25-d6-router addr add "10.60.$subnet.1/24" dev "r-$leaf"
        ip -n iot25-d6-router link set "r-$leaf" up
        ip -n "iot25-d6-$leaf" addr add "10.60.$subnet.10/24" dev eth0
        ip -n "iot25-d6-$leaf" link set eth0 up
        ip -n "iot25-d6-$leaf" route add default via "10.60.$subnet.1"
    done
    inside sysctl -qw net.ipv4.ip_forward=1
    inside nft -c -f "$lab_dir/rules.nft"
    inside nft -f "$lab_dir/rules.nft"
    ip netns exec iot25-d6-srv python3 "$lab_dir/probe_service.py" \
        > "$state/service.log" 2>&1 &

    # Bounded readiness check, not a fixed sleep assumption.
    for attempt in {1..30}; do
        if ip netns exec iot25-d6-srv python3 - 2>/dev/null <<'PYTHON'
import socket

connection = socket.create_connection(("10.60.20.10", 8080), timeout=0.1)
connection.close()
PYTHON
        then
            trap - ERR
            echo 'Lab ready: run test, backup, break, test-broken, restore, test, down'
            return
        fi
        sleep .1
    done
    false
}

case "${1:-help}" in
    up)
        up
        ;;
    down)
        down
        ;;
    test)
        owned
        python3 check.py
        ;;
    test-broken)
        owned
        python3 check.py broken
        ;;
    status)
        owned
        ip netns list
        inside ip route
        inside nft list ruleset
        ;;
    backup)
        owned
        {
            printf 'flush ruleset\n'
            inside nft list ruleset
        } > "$state/backup.nft"
        inside nft -c -f "$state/backup.nft"
        echo "Backup: $state/backup.nft"
        ;;
    break)
        owned
        if [[ ! -s "$state/backup.nft" ]]; then
            echo 'Run backup first'
            exit 1
        fi

        cat > "$state/broken.nft" <<'NFTABLES'
flush ruleset
table inet iot25 {
    chain forward {
        type filter hook forward priority 0;
        policy drop;
    }
}
NFTABLES
        inside nft -f "$state/broken.nft"
        ;;
    restore)
        owned
        if [[ ! -s "$state/backup.nft" ]]; then
            echo 'No backup'
            exit 1
        fi

        inside nft -c -f "$state/backup.nft"
        inside nft -f "$state/backup.nft"
        ;;
    *)
        echo 'Usage: sudo bash lab.sh up|status|test|backup|break|test-broken|restore|down'
        ;;
esac
