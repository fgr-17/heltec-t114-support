# zephyr-app-template

Template repo for creating Zephyr apps using T2 topology

## Usage

Create the container and get into it:

~~~bash
docker compose up -d
docker exec -it zephyr-app-template bash
~~~

The container is based on [Zephyr developer Docker image](ghcr.io/zephyrproject-rtos/zephyr-build:latest), so, it has all the tools (like west) already installed.

Once inside the container, initialize the project running the following lines:

~~~bash
cd workspace
west init -l app 
~~~

You should see an output similar to this:

~~~bash
root@fee7bb268f0d:/workspace# west init -l app
=== Initializing from existing manifest repository app
--- Creating /workspace/.west and local configuration file
=== Initialized. Now run "west update" inside /workspace.
~~~

Then, you need to pull all the Zephyr modules by running `west update`. First time you run this will take several minutes (and GB!). You should see the terminal showing all the downloaded packages and their revision.

**Note:** The file [`west.yml`](./app/west.yml) contains the list of modules that the app will need. In this version of the template, there's only one module defined:

~~~yml
projects:
    # Zephyr repository (specific tag or branch)
    - name: zephyr
      path: zephyr
      revision: v4.3.0
      import: true
      west-commands: scripts/west-commands.yml 
~~~

The line `import: true` is the responsible for downloading **all** the modules. Listing individual moduels would be much better than downloading everything, but I couldn't find a reliable way to make it work that way so far


## Building the project

Once you go through the previous steps, you should be ready to compile the project. Run the following:

~~~bash
cd workspace
west build -b promicro_nrf52840 -s app
~~~

If everything runs smoothly, you should see somethign like this:

~~~bash
west build -b promicro_nrf52840 -s app
-- west build: generating a build system
Loading Zephyr default modules (Zephyr base).
-- Application: /workspace/app
-- CMake version: 3.28.3
-- Found Python3: /opt/python/venv/bin/python3 (found suitable version "3.12.3", minimum required is "3.10") found components: Interpreter 
-- Cache files will be written to: /root/.cache/zephyr
-- Zephyr version: 4.3.0 (/workspace/zephyr)
-- Found west (found suitable version "1.5.0", minimum required is "0.14.0")
-- Board: promicro_nrf52840, qualifiers: nrf52840
-- Found host-tools: zephyr 0.17.4 (/opt/toolchains/zephyr-sdk-0.17.4)
-- Found toolchain: zephyr 0.17.4 (/opt/toolchains/zephyr-sdk-0.17.4)
-- Found Dtc: /opt/toolchains/zephyr-sdk-0.17.4/sysroots/x86_64-pokysdk-linux/usr/bin/dtc (found suitable version "1.7.0", minimum required is "1.4.6") 
-- Found BOARD.dts: /workspace/zephyr/boards/others/promicro_nrf52840/promicro_nrf52840_nrf52840.dts
-- Generated zephyr.dts: /workspace/build/zephyr/zephyr.dts
-- Generated pickled edt: /workspace/build/zephyr/edt.pickle
-- Generated devicetree_generated.h: /workspace/build/zephyr/include/generated/zephyr/devicetree_generated.h
Parsing /workspace/zephyr/Kconfig
Loaded configuration '/workspace/zephyr/boards/others/promicro_nrf52840/promicro_nrf52840_nrf52840_defconfig'
Merged configuration '/workspace/app/prj.conf'
Configuration saved to '/workspace/build/zephyr/.config'
Kconfig header saved to '/workspace/build/zephyr/include/generated/zephyr/autoconf.h'
-- Found GnuLd: /opt/toolchains/zephyr-sdk-0.17.4/arm-zephyr-eabi/arm-zephyr-eabi/bin/ld.bfd (found version "2.38") 
-- The C compiler identification is GNU 12.2.0
-- The CXX compiler identification is GNU 12.2.0
-- The ASM compiler identification is GNU
-- Found assembler: /opt/toolchains/zephyr-sdk-0.17.4/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc
-- Using ccache: /usr/local/bin/ccache
-- Found gen_kobject_list: /workspace/zephyr/scripts/build/gen_kobject_list.py
-- Configuring done (6.0s)
-- Generating done (0.1s)
-- Build files have been written to: /workspace/build
-- west build: building application
[1/186] Preparing syscall dependency handling

[2/186] Generating include/generated/zephyr/version.h
-- Zephyr version: 4.3.0 (/workspace/zephyr), build: v4.3.0
[186/186] Linking CXX executable zephyr/zephyr.elf
Memory region         Used Size  Region Size  %age Used
           FLASH:       54828 B      1020 KB      5.25%
             RAM:       15736 B       256 KB      6.00%
        IDT_LIST:          0 GB        32 KB      0.00%
Generating files from /workspace/build/zephyr/zephyr.elf for board: promicro_nrf52840
Converted to uf2, output size: 110080, start address: 0x1000
Wrote 110080 bytes to zephyr.uf2
~~~

Your binaries should be delivered in the `build/` dir.

## Project Renaming

Before you start working, probably you want to rename some stuff in the repo:

* **Project name:** go to [`CMakeLists.txt`](./app/CMakeLists.txt) and change the content of variable called `APP` with the name of your project

* **Container name:** go to [`compose.yml`](./compose.yml) and change the `service's name` and `container_name` 

## Flashing the boards

I didn't find a way to flash the boards *from the container*, normally because of issues with `udev`. So you should move the binaries to the computer or host where you want to flash and install your stuff there

## Contributing

Pull requests are welcome. For major changes, please open an issue first
to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License

[MIT](https://choosealicense.com/licenses/mit/)