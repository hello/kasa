#!/bin/sh

P2P_SCRIPT_PATH=/home/atheros/Atheros-P2P/scripts

IFNAME=$1
CMD=$2

if [ "$CMD" = "P2P-GROUP-STARTED" ]; then
    GIFNAME=$3
    echo $3
    if [ "$4" = "GO" ]; then
        killall udhcpd
        killall udhcpc 
        /sbin/ifconfig $GIFNAME 192.168.42.1 up
        touch /var/run/udhcpd-p2p.leases
        sed -i '/^interface/d' $P2P_SCRIPT_PATH/udhcpd-p2p.conf
        echo "interface $GIFNAME" >> $P2P_SCRIPT_PATH/udhcpd-p2p.conf
        udhcpd $P2P_SCRIPT_PATH/udhcpd-p2p.conf	&
    fi
    if [ "$4" = "client" ]; then
        killall udhcpd                                                                                                                                             
        killall udhcpc
        # more one retry immediately if P2P-GO's DHCP server not yet ready.
        DHCP_RESULT=`udhcpc -n -q -i $GIFNAME -s $P2P_SCRIPT_PATH/udhcpc-p2p.script | grep "No lease"`
        if [ "$DHCP_RESULT" != "" ]; then
            udhcpc -i $GIFNAME -s $P2P_SCRIPT_PATH/udhcpc-p2p.script &
        fi
    fi
fi                          

if [ "$CMD" = "P2P-GROUP-REMOVED" ]; then
    GIFNAME=$3
    if [ "$4" = "GO" ]; then
        killall udhcpd
        ifconfig $GIFNAME 0.0.0.0
    fi
    if [ "$4" = "client" ]; then
        killall udhcpc
        ifconfig $GIFNAME 0.0.0.0
    fi
fi

if [ "$CMD" = "P2P-CROSS-CONNECT-ENABLE" ]; then
    GIFNAME=$3
    UPLINK=$4
    # enable NAT/masquarade $GIFNAME -> $UPLINK
    iptables -P FORWARD DROP
    iptables -t nat -A POSTROUTING -o $UPLINK -j MASQUERADE
    iptables -A FORWARD -i $UPLINK -o $GIFNAME -m state --state RELATED,ESTABLISHED -j ACCEPT
    iptables -A FORWARD -i $GIFNAME -o $UPLINK -j ACCEPT
    echo 1 > /proc/sys/net/ipv4/ip_forward
fi

if [ "$CMD" = "P2P-CROSS-CONNECT-DISABLE" ]; then
    GIFNAME=$3
    UPLINK=$4
    # disable NAT/masquarade $GIFNAME -> $UPLINK
    echo 0 > /proc/sys/net/ipv4/ip_forward
    iptables -t nat -D POSTROUTING -o $UPLINK -j MASQUERADE
    iptables -D FORWARD -i $UPLINK -o $GIFNAME -m state --state RELATED,ESTABLISHED -j ACCEPT
    iptables -D FORWARD -i $GIFNAME -o $UPLINK -j ACCEPT
fi
