# GameAP Daemon
The server management daemon

Branches        | Build         |
----------------|-------------- | 
develop         |[![Build Status](https://travis-ci.org/gameap/GDaemon2.svg?branch=develop)](https://travis-ci.org/GameAP/GDaemon2)       |
master          |[![Build Status](https://travis-ci.org/gameap/GDaemon2.svg?branch=master)](https://travis-ci.org/GameAP/GDaemon2)        |

- [Compiling](#compiling)
  - [Dependencies](#dependencies)
  - [Linux](#compiling-on-linux)
  - [Windows](#compiling-on-windows)
- [Configuration](#configuration)
- [Creating certificates](#creating-certificates)

## Compiling

### Dependencies

* [Binn Library](https://github.com/liteserver/binn)
* [Rest Client C++](https://github.com/mrtazz/restclient-cpp)
* CURL Library
* [Json C++ Library](https://github.com/open-source-parsers/jsoncpp)
* Boost Libraries: Boost System, Boost Filesystem, Boost Iostreams, Boost Thread, Boost Process

### Compiling on Linux

```
git clone https://github.com/gameap/GDaemon2
cd GDaemon2
cmake .
make gameap-daemon
```

### Compiling on Windows
Coming soon

## Configuration

### daemon.cfg

#### Base parameters

| Parameter                 | Required              | Type      | Info
|---------------------------|-----------------------|-----------|------------
| ds_id                     | yes                   | integer   | Dedicated Server ID
| listen_port               | no (default 31717)    | integer   | Server port
| api_host                  | yes                   | string    | API Host
| api_key                   | yes                   | string    | API Key


#### SSL/TLS

| Parameter                 | Required              | Type      | Info
|---------------------------|-----------------------|-----------|------------
| certificate_chain_file    | yes                   | string    | Certificate
| private_key_file          | yes                   | string    | Private Key
| private_key_password      | no                    | string    | Private Key Password
| dh_file                   | yes                   | string    | Diffie-Hellman Certificate

#### Stats

| Parameter                 | Required              | Type      | Info
|---------------------------|-----------------------|-----------|------------
| if_list                   | no                    | string    | Network interfaces list
| drives_list               | no                    | string    | Disk drivers list
| stats_update_period       | no                    | integer   | Stats update period
| stats_db_update_period    | no                    | integer   | Update database period

#### Example daemon.cfg

```ini
; Dedicated server list
ds_id=1337
; listen_port=31717

api_host=localhost
api_key=h1KhwImgjTsvaOIgQxxQkbKxzOMfybI1U1p0ALq1NB54hNw0d1X3vVWFXpkZZPDH

; SSL/TLS

certificate_chain_file=/path/to/server_certificate.pem
private_key_file=/home/path/to/server_certificate.key
private_key_password=abracadabra
dh_file=/home/path/to/dh2048.pem

; Interfase list

; Space is delimiter
; Example:
; if_list=lo eth0 eth1
; Default Value: eth0 eth1
if_list=eth0 eth1

; Drives list

; No Spaces in path. Space is delimiter
; Example:
; drives_list=/ /home/servers
drives_list=/ /home/servers

stats_update_period=60
stats_db_update_period=300
```

### Creating certificates

Generate:

```bash
openssl genrsa -out rootca.key 2048
openssl req -x509 -new -nodes -key rootca.key -days 3650 -out rootca.crt

openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr

openssl x509 -req -in server.csr -CA rootca.crt -CAkey rootca.key -CAcreateserial -out server.crt -days 3650

openssl dhparam -out dh2048.pem 2048
```

Configuration:
```ini
certificate_chain_file=server.csr
private_key_file=server.key
dh_file=/home/path/to/dh2048.pem
```

