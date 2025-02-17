# MoNA-VTK examples

This repo shows how to implement the MonaController and use it for Paraview Catalyst to do the in-situ data analytics. The `src` folder contains the implementation details of the MonaController based on the MonaCommunicator which is implemented based on [mochi-mona](https://github.com/mochi-hpc/mochi-mona).

There are several examples in the `example` folder:

- basic: This example shows that how the MonaController can be used to execute the basic vtk parallel operations such as send and recv vtk object.


- icetExample: This exmaple shows that how the mochi-mona can be used to execute the iceT test cases based on the iceT wrapper for the mochi-mona.


- MandelbulbCatalystExample: This example shows how the MonaController can be used to execute the tightly coupled in-situ analytics in distributed way.


- MandelbulbColza: This example shows how the MonaController can be used to execute the loosely coupled in-situ analytics in distributed way, the [mochi-colza](https://github.com/mochi-hpc/mochi-colza) is used as the data staging service for this example.


- GrayScottColza: This example is similar with the MandelbulbColza case but the simulation data is generated by Gray-Scott simulation.

## Installing

We assume there is a new account on cori system, and we need following operations to install necessary depedencies

**Spack configuration**

There are two ways to use the Spack to install the software packages, the first one is to init the package.yaml file and the second one is to use the spack env.

For example, we use `spack arch -p` to check the current architecture. If the architecture is the cray, the `package.yaml` file should locate at the `~/.spack/cray/`. And we update the `package.yaml` file as needed for installing the mochi-software stacks. One sample `package.yaml` for cori system is located in `./config/cori/packages.yaml`.

The repo of the spack used by the mochi project: https://xgitlab.cels.anl.gov/sds/sds-repo.git, we need to add this repo into the spack system by executing `spack repo add sds-repo` at the current direactly.

**Building ParaView patch version**

The source code of ParaView patch is located at this repo: https://gitlab.kitware.com/mdorier/paraview/-/tree/dev-icet-integration.

```
git clone https://gitlab.kitware.com/mdorier/paraview.git
cd paraview
git checkout ecb0a075f459c9db78bdd57bf83d715a99f0fe55
git submodule update --init --recursive
```

The ParaView needs the osmesa to support the capability of in-situ rendering. We use the osmesa installed by the spack on the cori system:

```
module load spack
spack load -r mesa/qozjngg
PATH="/global/common/cori/software/altd/2.0/bin:$PATH"
```

We also need to set the compiler on the cori before building the ParaView

```
# for compiling vtk on cori
export CRAYPE_LINK_TYPE=dynamic

# let cc and CC to be the gnu compier
module swap PrgEnv-intel PrgEnv-gnu

module swap gcc/8.3.0 gcc/9.3.0
```

At the build direactory of the ParaView, we use cmake commands as follows (if we assume the source direactory is `~/cworkspace/src/ParaView_patch/paraview`):

```
cmake ~/cworkspace/src/ParaView_patch/paraview -DPARAVIEW_USE_QT=OFF -DPARAVIEW_USE_PYTHON=ON -DPARAVIEW_USE_MPI=ON -DVTK_OPENGL_HAS_OSMESA:BOOL=TRUE -DVTK_USE_X:BOOL=FALSE -DCMAKE_CXX_COMPILER=CC -DCMAKE_C_COMPILER=cc -DVTK_PYTHON_OPTIONAL_LINK=OFF -DCMAKE_BUILD_TYPE=Release
```

**Building and installing Colza**

This command will install the mochi-colza and other related mochi softwares

```
spack install mochi-colza@main+drc+examples%gcc@9.3.0
```

**Building all examples**

We can load these depedencies if all packages are installed successfully. The sample commands are located in `config/cori/monavtkEnv.sh`. We execute these commands before building the mona-vtk examples. 

Then we can build the mona-vtk the cmake command like this:

```
cmake ~/cworkspace/src/mona-vtk/ -DCMAKE_CXX_COMPILER=CC -DCMAKE_C_COMPILER=cc -DVTK_DIR=$SCRATCH/build_paraview_patch_release/ -DENABLE_EXAMPLE=ON -DParaView_DIR=$SCRATCH/build_paraview_patch_release/ -DBUILD_SHARED_LIBS=ON 
```

## Running

The scripts for scale evaluation are located at the `example/MandelbulbColza/testScripts` and `./example/GrayScottColza/testScripts` separately.

For example, we can set the build and src dir properly at the beginning of the scripts, such as

```
BUILDDIR=/global/cscratch1/sd/zw241/build_monavtk
SRCDIR=/global/homes/z/zw241/cworkspace/src/mona-vtk
``` 
 
and then use sbatch to submit jobs with specific node configurations as needed:

```
sbatch ~/cworkspace/src/mona-vtk/example/MandelbulbColza/testScripts/strongscale/cori_strongscale_mona_4.scripts
```
or

```
sbatch ~/cworkspace/src/mona-vtk/example/GrayScottColza/testScripts/strongscale/cori_gsstrongscale_mona_128_512.scripts
```

We can check the corresponding server and log file to get the particular data put and analysing time.

For example, the `mbclient_mona_4_512.log` records the client information when there are 4 staging processes and 512 client pracesses.

For the `MandelbulbColza` example, we can set the size of the data block by updating the `BLOCKLENW`, `BLOCKLENH` and `BLOCKLEND` in the associated script.

For the `GrayScottColza` example, we can set the size of the data block by updating the `L` value at the client configuration file. For example, at the `client_settings_monaback_408.json`, we set the `L` as 408, which means there are `408*408*408` cells for each data block.

## Other potential issues

We could also try to install osmesa by spack manaully:

```
spack install mesa+osmesa~llvm swr=none
```

https://discourse.paraview.org/t/undefined-symbol-pyexc-valueerror/5494/5

```
/usr/bin/ld: /global/common/sw/cray/sles15/x86_64/mesa/18.3.6/gcc/8.2.0/qozjngg/lib/libOSMesa.so: undefined reference to `del_curterm@NCURSES6_TINFO_5.0.19991023'
```
try this:

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ltinfo")

refer to

https://github.com/halide/Halide/issues/1112

if the MPICH_GNI_NDREG_ENTRIES is not set properly
https://github.com/mercury-hpc/mercury/issues/426

some osmesa warning from paraview if it is built in the Debug mode for building paraview (it is ok when we use the Release mode to build the paraview)

(  44.958s) [pvbatch.3       ]vtkOpenGLFramebufferObj:356    ERR| vtkOpenGLFramebufferObject (0x10005dc58e0): failed at glGenFramebuffers 1 OpenGL errors detected
1:   0 : (1280) Invalid enum

 vtkOpenGLState.cxx:505   WARN| Error glBindFramebuffer1 OpenGL errors detected
2:   0 : (1280) Invalid enum

Try to build the paraview with the Release mode, otherwise, there are mosa related warnings

For the python on cori, refer to this (https://docs.nersc.gov/development/languages/python/nersc-python/)
If you only use the module option, but the python is not the default one, there are some issues

One issue is "unnamed python module encoding", or other issues that have different gcc version which may cause the byte code issue
It is prefered to use the conda activate then the python virtual env if you not use the default python3 system on cori 