#!/bin/bash

set -ex

CLIENT_NS=dhcp-client
CLIENT_VETH=veth-dhcpcl
SERVER_NS=dhcp-server
SERVER_VETH=veth-dhcpsrv
BRIDGE=dhcp-bridge
N_CLIENT=${1:-3}

sudo ip netns add "$SERVER_NS"
sudo ip link add "$BRIDGE" type bridge
sudo ip link add name "$SERVER_VETH" type veth peer name "$CLIENT_VETH"

sudo ip link set dev "$SERVER_VETH" netns "$SERVER_NS"
sudo ip link set dev "$CLIENT_VETH" master "$BRIDGE"

sudo ip link set dev "$BRIDGE" up
sudo ip netns exec "$SERVER_NS" ip link set dev "$SERVER_VETH" up
sudo ip link set "$CLIENT_VETH" up

sudo ip netns exec "$SERVER_NS" ip route add default dev "$SERVER_VETH"

for i in $(seq 1 $N_CLIENT); do
	sudo ip netns add "${CLIENT_NS}${i}"
	sudo ip link add name "${CLIENT_VETH}${i}" type veth peer name "${SERVER_VETH}${i}"

	sudo ip link set dev "${CLIENT_VETH}${i}" netns "${CLIENT_NS}${i}"
	sudo ip link set dev "${SERVER_VETH}${i}" master "${BRIDGE}"

	sudo ip netns exec "${CLIENT_NS}${i}" ip link set dev "${CLIENT_VETH}${i}" up
	sudo ip link set dev "${SERVER_VETH}${i}" up
done

sudo iptables -A FORWARD -m physdev --physdev-is-bridged -j ACCEPT
