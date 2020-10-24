#!/bin/bash
# The password will be XORed with each last 5 bits of all characters of the local MAC-address.
# Without physical connection to the network you can't easily determine the password.
XORCRYPT(){ # $1:XORed password
    local DataIn=$1
    local mac ptr DataOut
    mac=$(echo $(echo $(ip link | grep ether) | cut -d" " -f2 | tr -d :))
    for (( ptr=0; ptr < ${#DataIn}; ptr++ )); do
        DataOut+=$(echo -e \\x$(printf %x $(($(printf %d "'${DataIn:$ptr:1}")^
            $(($(printf %d "'${mac:$((ptr % ${#mac})):1}") & 31))))))
    done
    echo $DataOut
}
