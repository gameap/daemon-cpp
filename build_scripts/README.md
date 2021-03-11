# GameAP Daemon Building scripts

Scripts to build GameAP Daemon at various Linux distributions

> Note. Some scripts is abandoned

## Distributions

| Operating System      | Version           | Notes                   |
|-----------------------|-------------------|-------------------------|
| **Debian**            | sid               | 
|                       | 10 / buster       | 
|                       | 9 / stretch       | 
|                       | 8 / jessie        | 
|                       | 7 / wheezy        |
| **Ubuntu**            | 18.04             |
|                       | 16.04             |
|                       | 14.04             |
|                       | 12.04             |
| **CentOS**            | 8                 |
|                       | 7                 |
|                       | 6                 |

## Example

Building GameAP Daemon on Debian Buster

```bash
cd ~
git clone --recurse-submodules https://github.com/gameap/daemon
mkdir daemon/build
export ROOTDIR=$(pwd)/daemon/build
cd daemon/build
../build_scripts/build-debian-buster.sh
```