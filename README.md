# picámpinos
 DVP CAMERA IF(8-bit parallel IF) using RP2040.
This project is for connecting a DVP camera, such as the OV5642, to Raspberry Pi Pico; the DVP camera is primarily an 8-bit parallel one produced by OmniVision, but it may be applicable to other image sensors with 8-bit parallel interfaces with I2C Control.
## Getting Started
### For Raspberry Pi 3/4
Use <code>[get_sdk.sh](get_sdk.sh)</code> to clone all Projects with Raspberry Pi Pico SDKs and Pico Examples.

```
wget https://raw.githubusercontent.com/panda5mt/picampinos/main/get_sdk.sh

chmod +x get_sdk.sh
./get_sdk.sh
```
To compile this project, just run
```
cd ~/
cd picampinos/
./pico_build.sh
```

### For Docker
Use <code>[Dockerfile](Dockerfile)</code>.
When you'd built an image, run and attach the image, type on terminal,
```
cd /root/picampinos/
./pico_build.sh
```
The docker image contains this Project, Pico SDKs, and Pico Examples.


### On your local CLI
Get ideas to see <code>[get_sdk.sh](get_sdk.sh)</code>.

## Hardware settings