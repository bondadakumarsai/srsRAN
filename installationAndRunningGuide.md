# Installation and Configuration Commands

## Core Network Installation - Open5GS

##### Update package lists and install dependencies

    sudo apt update
    sudo apt-get install gnupg curl

##### Import MongoDB GPG Key

    curl -fsSL https://pgp.mongodb.com/server-6.0.asc |
    sudo gpg -o /usr/share/keyrings/mongodb-server-6.0.gpg
    --dearmor

##### Add this MongoDB repository if using Ubuntu 22.04 (Jammy)

    echo "deb [ arch=amd64,arm64 signed-by=/usr/share/keyrings/mongodb-server-6.0.gpg ] https://repo.mongodb.org/apt/ubuntu jammy/mongodb-org/6.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-6.0.list

##### Add this MongoDB repository if using Ubuntu 20.04 (Focal)

    echo "deb [ arch=amd64,arm64 signed-by=/usr/share/keyrings/mongodb-server-6.0.gpg ] https://repo.mongodb.org/apt/ubuntu focal/mongodb-org/6.0 multiverse" | sudo tee /etc/apt/sources.list.d/mongodb-org-6.0.list

##### Update package lists and install MongoDB

    sudo apt-get update
    sudo apt-get install -y mongodb-org

##### Start and enable MongoDB service

    sudo systemctl start mongod
    sudo systemctl status mongod
    sudo systemctl enable mongod

##### Install Open5GS

    sudo add-apt-repository ppa:open5gs/latest
    sudo apt update
    sudo apt install open5gs

##### Install Node.js and Open5GS Web UI

    sudo apt install curl
    curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
    sudo apt install nodejs
    curl -fsSL https://open5gs.org/open5gs/assets/webui/install | sudo -E bash -

##### Register Subscriber Information
Connect to [http://localhost:3000](http://localhost:3000) and login with admin account.
- Username: admin
- Password: 1423

Configure Open5GS

    Mode: soft
    Algorithm: milenage
    OPC: 63BFA50EE6523365FF14C1F45F88737D
    K: 00112233445566778899aabbccddeeff
    IMSI: 901700123456780
    IMEI: 353490069873319
    Type: IPv4
    DNN/APN: srsapn
    UE IPv4 Address: 10.45.0.3

##### Enable IP forwarding and configure iptables rules

    sudo sysctl -w net.ipv4.ip_forward=1
    sudo iptables -t nat -A POSTROUTING -s 10.45.0.0/16 ! -o ogstun -j MASQUERADE
    sudo iptables -I INPUT -i ogstun -j ACCEPT
    sudo iptables -I INPUT -s 10.45.0.0/16 -j DROP
    sudo iptables -I FORWARD -s 10.45.0.0/16 -d x.x.x.x/y -j DROP

##### Restart Open5GS services

    sudo systemctl restart open5gs-*

#### This completes the installation of Open5GS

## srsRAN 5G SA Installation 

##### Clone and checkout srsRAN repository
    git clone https://github.com/bondadakumarsai/srsRAN.git
    cd srsRAN/
    git checkout underlayAddition

##### Create a network namespace
    sudo ip netns add ue1

##### Open four terminals and run the following commands separately:

##### Terminal 1:
    sudo srsenb

##### Terminal 2:
    sudo srsenb

##### Terminal 3:
    sudo ip netns exec ue1 iperf3 -s  # at UE

##### Terminal 4:
    iperf3 -i 1 -c 10.45.0.3 -t 6000 -b 14M  # at gNB
