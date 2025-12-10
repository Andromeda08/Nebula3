### Nebula : Resource Tools

A simple cli tool for compiling shader files and moving resources to the binary directory.

❗`copy-resources` **will** delete the directory specified by `-r` but does a safety check first whether or not the specified directory is indeed a `Resources` directory under a `cmake-build-[debug, release]` directory. Note that `build-shaders` will just overwrite existing compiled shader files in the binary directory.

pre-launch commands:
```shell
nrt build-shaders -b ../../cmake-build-debug/Resources/Shaders/bin -p ../../Resources/Shaders -d
nrt copy-resources -b ../../cmake-build-debug/Resources -r ../../Resources -f -t
```
