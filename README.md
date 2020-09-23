# PS2PCPAD
<b> A socket based ps2 program that allows your dualshock 2 controller to be used with a computer </b>

## Usage
#### Prerequisites
- `PS2SDK` & `PS2DEV` environment variables are set
- Your ps2 has `ps2link` running
#### Building
* Clone this repository
* Optional (Configure globalincludes.h)

* Compile 
    * `make`
* Execute on your ps2
    * `make run`
    
## Features
- UDP :) 
- Port configuration (using a #define in globalincludes.h)

## TODO
- TCP hearbeat?
- Properly implement threading (My old implementation didn't work)
- Config file support
- (optionally) Configure the ethernet device via DHCP. 
    - Currently we rely on ps2link to configure that for us
    - We could also add static ip configuration
- Support more than DS2 controllers
- Detect when the controller is disconnected
- Support more than one client
