# OdysseyWebBrowser

Notes about branches:

* webkit  - branch that only contains original WebKit codes at certain revisions/releases. Source changes between versions/releases are squashed into in (most cases) one commit.
* master  - branch that contains commits from webkit branch combined with Odyssey Web Browser source codes which don't conflict with codes from webkit branch. Check master-editable file for details.
* odyssey - branch that contains remaining Odyssey Web Browser source changes rebased onto certain version of master branch.

In order to build/develop the Odyssey Web Browser, checkout branch **odyssey**.


## Build Odyssey for x86_64

Please note that building Odyssey requires considerable resources. Build machine of 12 cores, 32 GB RAM and NVME disk builds Odyssey in 12 minutes with 'm -12'. Build machine of 3 cores, 24 GB RAM and SATA SSD disk builds Odyssey in 90 minutes with 'm -3'.


### Building cross compiler and SDK

Before building Odyssey you need to have a working AROS cross compiler and SDK built and installed on your machine. Please follow this turorial to build them:

https://arosnews.github.io/how-to-cross-compile-aros-hosted-wsl/

Notes:

* If you are building directly under linux, you can skip first chapter about WSL2. Start with chapter about installing required development packages.
* Skip the last chapter about installing i386 version, this is not needed for this build.
* Instead of checking out master, make sure you checkout branch 'release-20241102' in both AROS and contrib

As an effect of the whole process you should have two 'commands' x86_64-aros-gcc and x86_64-aros-g++ available in path. The compilers should be in version 6.5.0.

### Building Odyssey

Run the following script in the root of Odyssey source tree:

```
$ ./rebuild.sh
```

Select option 11) x86_64-aros (Release). Once the script completes:

```
$ cd cross-build-x86_64-aros
$ make
```

After build completes, your executable will be available in 'cross-build-x86_64-aros/bin' directory.
