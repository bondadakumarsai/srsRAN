# Installation, Configuration and Running Commands

srsRAN 4G 22.04 brings 5G SA support to both srsUE and srsENB. 5G SA features can be enabled via the configuration files of both srsUE and srsENB. This guide provides instructions for installing and configuring srsRAN 4G 22.04 with 5G SA support added with underlay transmission and reception, along with the required hardware and software components.

## Hardware and Software Requirements

- A fast computing machine (min 16 GB RAM) with Ubuntu 20.04 or Ubuntu 22.04
- srsRAN 4G 22.04 (or later)
- ZeroMQ
- Open5GS 5G Core

#### It is recommended to run all commands as the sudo user
	sudo -i

## srsRAN 5G SA Installation 

#### Required libraries
    sudo apt-get install build-essential cmake libfftw3-dev libmbedtls-dev libboost-program-options-dev libconfig++-dev libsctp-dev libzmq3-dev iperf3

##### Clone and checkout srsRAN repository
    git clone https://github.com/bondadakumarsai/srsRAN.git
    cd srsRAN/
    git checkout underlayAddition

#### Before building, set the underlay bin files folder path in line 252 of file gnb_dl.c. The path to file is srsran/lib/src/phy/gnb. The host_name should be your user name.
    Ex: sprintf(fullfilename, "/home/host_name/Desktop/txFolderBin/underlay_grid%d.bin",slotId);  

#### Build srsRAN
	mkdir build
	cd build
	cmake ../
	make -j`nproc`
	sudo make install
	sudo ldconfig
	./srsran_install_configs.sh user --force
	
#### Updating srsRAN config files to set 5G SA to work
	cd ../
	cp -r config_files/srsran /root/.config 
 
### This completes the installation of srsRAN 5G setup
 
## Core Network Installation - Open5GS

##### Update package lists and install dependencies

    sudo apt update
    sudo apt-get install gnupg curl

##### Import MongoDB GPG Key

    curl -fsSL https://pgp.mongodb.com/server-6.0.asc | sudo gpg -o /usr/share/keyrings/mongodb-server-6.0.gpg --dearmor

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

Add a subscriber and configure the below fields in the subscriber info.

    OPC: 63BFA50EE6523365FF14C1F45F88737D
    K: 00112233445566778899aabbccddeeff
    IMSI: 901700123456780
    Type: IPv4
    DNN/APN: srsapn
    UE IPv4 Address: 10.45.0.3

#### Updating open5GS config files to connect to 5G SA setup
	cp -r config_files/open5gs /etc/ 

##### Enable IP forwarding and configure iptables rules

    sudo sysctl -w net.ipv4.ip_forward=1
    sudo iptables -t nat -A POSTROUTING -s 10.45.0.0/16 ! -o ogstun -j MASQUERADE
    sudo iptables -I INPUT -i ogstun -j ACCEPT
    sudo iptables -I INPUT -s 10.45.0.0/16 -j DROP
   
##### Restart Open5GS services

    sudo systemctl restart open5gs-*

### This completes the installation of Open5GS


#### Clone the repository containing MATLAB scripts for pre-processing and post-processing of underlay signal

	git clone https://github.com/bondadakumarsai/underlay.git
 	sudo mkdir /home/ch_folder_bin
  	sudo mkdir /home/underlay_rx_folder_bin
	
### Run underlayTxFileGen.mlx in underlay folder

## Running srsRAN 5G SA setup with underlay transmission and reception

##### Create a network namespace
    sudo ip netns add ue1

##### Open four terminals and run the following commands separately:

##### Terminal 1:
    sudo srsenb

##### Terminal 2:
    sudo srsue

##### Terminal 3:
    sudo ip netns exec ue1 iperf3 -s  # at UE

##### Terminal 4:
    iperf3 -i 1 -c 10.45.0.3 -t 6000 -b 14M  # at gNB
	
### Wait for some time, i.e., 15 secs, stop all processes in the terminals, and run the underlayRxProc.mlx in underlay folder
	
