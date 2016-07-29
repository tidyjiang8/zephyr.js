# The Zephyr.js Project

This purpose of this project is to provide a JavaScript API and environment
for IoT development on low-memory systems, using JerryScript and the Zephyr
operating system.

This code requires a local copy of JerryScript and Zephyr source, and we
will upstream code to those projects as appropriate, but this repo is for
everything else.

## File Descriptions
```zjs-env.sh``` - Source this file to set environment variables and path to be
able to use tools from ```scripts/``` anywhere.

## Subdirectories
- ```arc/``` - Contains sensor subsystem code for ARC side of the Arduino 101.
- ```deps/``` - Contains dependency repos and scripts for working with them.
- ```outdir/``` - Directory generated by build, can be safely removed.
- ```samples/``` - Sample JavaScript files that can be built with make JS=<path>.
- ```scripts/``` - Subdirectory containing tools useful during development.
- ```src/``` - JS API bindings for JerryScript written directly on top of Zephyr.
- ```test/``` - Zephyr applications that test various components.

## Setting up your environment
Download the [latest Zephyr SDK] (https://nexus.zephyrproject.org/content/repositories/releases/org/zephyrproject/zephyr-sdk/), then:
```
$ chmod +x zephyr-sdk-0.*.*-i686-setup.run
$ sudo ./zephyr-sdk-0.*.*-i686-setup.run
```

Next, set up the Zephyr SDK environment variables. It is recommended that you
add the following two lines to your ```.bashrc``` so that you don't have to type
them again. If you installed your Zephyr SDK elsewhere, adjust as needed.
```
$ export ZEPHYR_GCC_VARIANT=zephyr
$ export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk
```

To check out the other repos Zephyr.js is dependent on:

```
$ cd deps
$ ./getrepos
```

Later, to update them to the latest compatible code:
```
$ cd deps
$ ./update
```

Next, set up the Zephyr OS environment variables:
```
$ source deps/zephyr/zephyr-env.sh
```

If you wish to use a Zephyr tree elsewhere on your system, source its
zephyr-env.sh file instead.

Next, set up the Zephyr.js environment variables:
```
$ source zjs-env.sh
```

Note: If you are using Zephyr.js with an Arduino101 that you have converted to
a 256KB X86 partition (see below), you should pass 256 to this command:
```
$ source zjs-env.sh 256
```

The command will display the current expected partition size to remind you of
the expected target.

### JS Minifier

To save space it is recommended to use a minifier. In convert.sh, the script used
to encode your JS into a source file, we use uglifyjs. This can be downloaded by
doing:
```
sudo apt-get install node-uglify
```

## Getting more space on your Arduino 101
By default, Arduino 101 comes with a **144K** X86 partition, but we're able to
pretty safely increase it to **256K**. You should only use the ```dfu-util```
method of flashing after this.

The easiest way to do this is to use a flashpack.zip file that I can provide
you (geoff@linux.intel.com). I need to make sure of the license details before
I add it to the repo.

The default build settings currently expect you to be using the 256KB partition.

## Build the *Hello World* sample
```
$ make JS=samples/HelloWorld.js
```

This will build the system image, resulting in outdir/zephyr.bin as the final
output. To flash this to your device with dfu-util, first press the Master Reset
button on your Arduino 101, and about three seconds later type:

```
$ make dfu
```

There is a window of about five seconds where the DFU server is available,
starting a second or two after the device resets.

## Build other samples
The other samples may require some hardware to be set up and connected; read
the top of each JS file, but then simply pass in the path to the JS file to make
as with ```HelloWorld.js``` above.

## Running JerryScript Unit Tests
```
$ cd apps/jerryscript_test/tests/
$ ./rununittest.sh
```

This will iterate through the unit tests one by one. After each test completes,
press Ctrl-x to exit QEMU and continue with the next test.

## FRDM-K64F Platform

See the [Zephyr Wiki] (https://wiki.zephyrproject.org/view/NXP_FRDM-K64F) for general information about Zephyr on the FRDM-K64F.

The instructions below are assuming Ubuntu 14.04 on the host PC.

Connect a micro-USB cable from the device to your PC.

If you hit the Reset switch and wait about five seconds, you should be able to
start up a serial console. Either:

...
$ screen /dev/ttyACM0 115200
...
or
...
$ minicom -D /dev/ttyACM0
...

worked for me, but I typically had to try either command several times before it
would work. The benefit of minicom is it will keep running even if you unplug
the cable and then plug it back in later.

(Check your dmesg output or watch your /dev directory to know what device it
shows up as.)

Then, follow [these instructions] (https://developer.mbed.org/handbook/Firmware-FRDM-K64F) to update your firmware.

Next, you can try to build Zephyr.js for the platform:
...
$ make BOARD=frdm_k64f JS=samples/HelloWorld.js
$ cp outdir/zephyr.bin /media/<USERNAME>/MBED/

After you copy the new .bin file to that directory, the device will reboot,
blink an LED quickly as it writes the image somewhere, and then you should see
the device reconnect as a USB storage device to your PC. Then you can press the
Reset button to run the Zephyr image. You should see "Hello, Zephyr.js world!"
output on the serial console in less than a second.

If something doesn't work, you may want to establish that you're able to
upload the K64F [hello world application] (https://developer.mbed.org/platforms/FRDM-K64F/#flash-a-project-binary).

Then, you could try the Zephyr hello_world sample to narrow down the problem:
...
$ cd deps/zephyr/samples/hello_world/nanokernel
$ make pristine && make BOARD=frdm_k64f
$ cp outdir/zephyr.bin /media/<USERNAME>/MBED/
...

Using the same procedure as above, once you hit Reset you should see
"Hello World!" within a second on your serial console.
