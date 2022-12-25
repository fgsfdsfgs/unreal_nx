### UnrealNX
This is a Switch port/loader for Unreal and Unreal Gold.
It loads the aarch64-linux binaries supplied with the 227j patch for Unreal/Unreal Gold, patches them and runs them,
similar in function to the [Max Payne Mobile loader](https://github.com/fgsfdsfgs/max_nx).
Currently pretty unstable as exception handling is broken, meaning any non-fatal error will cause a crash.

### How to install

You're going to need an Unreal or Unreal Gold installation patched with the [OldUnreal 227j patch](https://www.oldunreal.com/phpBB3/viewtopic.php?f=51&t=10395).
Unfortunately oldunreal.com seems to disallow direct linking to files, so click the link at the bottom of that post to get it.

Ensure that the patch is installed correctly: when installing **make sure to tick every box, but only tick "Return to Na Pali support" if you have Unreal Gold**.
If you don't have a folder called `SystemARM` in the Unreal folder, you need to reinstall the patch. Simply run the installer again to do that.

To install:
1. Create a folder called `unreal` in the `switch` folder on your SD card.
2. Copy the contents of your `Unreal` or `UnrealGold` folder into `/switch/unreal/`.
3. Extract the contents of the `.zip` file from the latest release into `/switch/unreal/`. Replace everything.

### Notes

This will only work with Unreal/UnrealGold 227j right now. If 227k is released, the loader will most likely have to be updated to support it.

This will likely not work in applet/album mode. Use a proper game override.

The performance sucks, mostly because of the dynamic lights.
You can disable dynamic lighjts to get a performance increase at the cost of all the pretty lighting effects in the game.
To do that, in `SystemARM/UnrealLinux.ini` look for `NoDynamicLights` and set it to `True`.

You can also disable volumetric lights to get a small performance increase in some areas with less of an impact on visuals.
To do that, in `SystemARM/UnrealLinux.ini` look for all instances of `VolumetricLighting` and set them to `True`.
There might be other config parameters that affect performance, but I haven't explored that much yet.

Trying to join a multiplayer game or opening the server browser will likely cause the game to crash.
Hosting a multiplayer game or playing Botmatch seems to work, though.

Exiting and relaunching the game will cause it to crash. Quit out of hbmenu after exiting the game to avoid that.

If you see any `appThrowf` errors, first try to re-extract the `.zip` file again, replacing everything.

Return to Na Pali may crash on new game with an error about `UPakFonts`.
To fix that get the original 3 MB `UPakFonts.utx` file from the `Textures` folder on your Unreal Gold CD
or unpatched Unreal Gold install and replace the one in `/switch/unreal/Textures`.

Joystick sensitivity can be adjusted for each axis individually in the `SystemARM/User.ini` file.
Search for for `SpeedBase` and edit the number for each axis to your liking.

You can bring up the on-screen keyboard by pressing `MINUS` at any time.

If the game crashes, usually there will be a message on the screen telling you the reason.
It will also create logs at `SystemARM/fatal.log` and `SystemARM/UnrealLinux.log`.
Please attach these to issue reports when possible, along with the last Atmosphere crash report.

### How to build

You're going to need devkitA64, libnx and the following libraries:
* switch-mesa
* switch-libdrm_nouveau
* switch-sdl2
* switch-libogg
* switch-libvorbis
* switch-flac
* switch-opus
* switch-libsodium
* switch-openal
* devkitpro-pkgbuild-helpers
* [libsolder](https://github.com/fgsfdsfgs/libsolder) (use `make install` to build and install)
* [libiconv-switch](https://github.com/snaiperskaya96/libiconv-switch) (use `./build-switch.sh` to build and install)
* libsndfile
* libxmp
* libalure

See [here](https://gist.github.com/fgsfdsfgs/dfb38bb86188e54f362c450353c8c448) on how to install the last three libraries.
The rest can be obtained using `(dkp-)pacman`.

After you've obtained all the dependencies and ensured devkitA64 is properly installed and the `DEVKITPRO` environment variable is set,
build this repository using the commands:
```
git clone https://github.com/fgsfdsfgs/unreal_nx.git && cd unreal_nx
source $DEVKITPRO/switchvars.sh
make
```

### Credits
* OldUnreal for the 227j patch;
* Switchbrew for libnx;
* devkitPro for devkitA64.
