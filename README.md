# GameAP Daemon
The server management daemon

Branches        | Build         |
----------------|-------------- | 
develop         |[![Build Status](https://travis-ci.org/gameap/GDaemon2.svg?branch=develop)](https://travis-ci.org/gameap/GDaemon2)       |
master          |[![Build Status](https://travis-ci.org/gameap/GDaemon2.svg?branch=master)](https://travis-ci.org/gameap/GDaemon2)        |

- [Compiling](#compiling)
  - [Dependencies](#dependencies)
  - [Linux](#compiling-on-linux)
  - [Windows](#compiling-on-windows)
- [Configuration](#configuration)
- [Creating certificates](#creating-certificates)
- [Clients API](#clients-api)

## Compiling

### Dependencies

* [Binn Library](https://github.com/liteserver/binn)
* [Rest Client C++](https://github.com/mrtazz/restclient-cpp)
* CURL Library
* [Json C++ Library](https://github.com/open-source-parsers/jsoncpp)
* Boost Libraries (>= 1.66.0): Boost System, Boost Filesystem, Boost Iostreams, Boost Thread, Boost Process
* [PLog](https://github.com/SergiusTheBest/plog) >= 1.1.5

#### Tests
* [Google Tests](https://github.com/google/googletest)
* [GMock Global](https://github.com/apriorit/gmock-global) >= 1.0.2

### Compiling on Linux

```
git clone https://github.com/gameap/GDaemon2
cd GDaemon2
cmake .
make gameap-daemon
```

### Compiling on Windows

Install or compile [Dependencies](#dependencies). I recommend using [Vcpkg](https://github.com/et-nik/vcpkg):

```
vcpkg install binn restclient-cpp curl jsoncpp boost-system boost-filesystem boost-iostreams boost-thread boost-process boost-property-tree
```

Add -DCMAKE_TOOLCHAIN_FILE to cmake arguments:
```
-DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

Open GDaemon folder in your Visual Studio, run cmake and run build.

### CMake options

| Option                        | Info
|-------------------------------|-----------------------
| -DBUILD_STATIC_BOOST          | Build with static boost libraries
| -DBUILD_STATIC                | Build with static libraries
| -DSYSCTL_DAEMON               | 
| -DNON_DAEMON                  |


## Configuration

Configuration file: daemon.cfg

### Base parameters

| Parameter                 | Required              | Type      | Info
|---------------------------|-----------------------|-----------|------------
| ds_id                     | yes                   | integer   | Dedicated Server ID
| listen_ip                 | no (default "0.0.0.0")| string    | Listen IP
| listen_port               | no (default 31717)    | integer   | Listen port
| api_host                  | yes                   | string    | API Host
| api_key                   | yes                   | string    | API Key


### SSL/TLS

| Parameter                 | Required              | Type      | Info
|---------------------------|-----------------------|-----------|------------
| ca_certificate_file       | yes                   | string    | CA Certificate
| certificate_chain_file    | yes                   | string    | Server Certificate
| private_key_file          | yes                   | string    | Server Private Key
| private_key_password      | no                    | string    | Server Private Key Password
| dh_file                   | yes                   | string    | Diffie-Hellman Certificate

### Base Authentification

| Parameter                 | Required              | Type      | Info
|---------------------------|-----------------------|-----------|------------
| password_authentication   | no                    | boolean   | Login+password authentification
| daemon_login              | no                    | string    | Login. On Linux if empty or not set will be used Linux PAM
| daemon_password           | no                    | string    | Password. On Linux if empty or not set will be used Linux PAM

### Stats

| Parameter                 | Required              | Type      | Info
|---------------------------|-----------------------|-----------|------------
| if_list                   | no                    | string    | Network interfaces list
| drives_list               | no                    | string    | Disk drivers list
| stats_update_period       | no                    | integer   | Stats update period
| stats_db_update_period    | no                    | integer   | Update database period

### Other

#### Only on Windows

| Parameter                 | Required              | Type      | Info
|---------------------------|-----------------------|-----------|------------
| 7zip_path                 | no                    | string    | Path to 7zip file archiver. Example: "C:\Program Files\7-Zip\7z.exe"
| starter_path              | no                    | string    | Path to GameAP Starter. Example: "C:\gameap\gameap-starter.exe"

### Example daemon.cfg

```ini
; Dedicated server list
ds_id=1337
; listen_port=31717

api_host=localhost
api_key=h1KhwImgjTsvaOIgQxxQkbKxzOMfybI1U1p0ALq1NB54hNw0d1X3vVWFXpkZZPDH

; Disable login password authentication
password_authentication=false

; SSL/TLS

ca_certificate_file=/path/to/client.crt
certificate_chain_file=/path/to/server.crt
private_key_file=//path/to/server.key
private_key_password=abracadabra
dh_file=/path/to/dh2048.pem

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

## Creating certificates

Generate root certificate:

```bash
openssl genrsa -out rootca.key 2048
openssl req -x509 -new -nodes -key rootca.key -days 3650 -out rootca.crt
```

Generate server certificate:
```bash
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr

openssl x509 -req -in server.csr -CA rootca.crt -CAkey rootca.key -CAcreateserial -out server.crt -days 3650
```

Generate client certificate:

```bash
openssl genrsa -out client.key 2048
openssl req -new -key client.key -out client.csr

openssl x509 -req -in client.csr -CA rootca.crt -CAkey rootca.key -CAcreateserial -out client.crt -days 3650
```

Generate Diffie-Hellman Certificate:
```bash
openssl dhparam -out dh2048.pem 2048
```

Configuration:
```ini
ca_certificate_file=rootca.crt
certificate_chain_file=server.crt
private_key_file=server.key
dh_file=/home/path/to/dh2048.pem
```

client.csr and client.key is using by client side

## Clients API

* PHP: https://github.com/et-nik/gameap-daemon-client