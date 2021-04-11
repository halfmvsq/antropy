# ANTROPY

*A cross-platform tool for interactively visualizing, comparing, segmenting, and annotating 3D medical images.*

Copyright 2021 Penn Image Computing and Science Lab, Department of Radiology, University of Pennsylvania.

## Building

Antropy requires C++17 and build generation uses CMake. It has been built on the following platforms:

* Ubuntu 20.04 (with gcc 9.3.0)
* macOS 10.14.6 (with Apple clang 11.0.0)
* Windows 10 (with Microsoft Visual Studio 19)

Please download and install these libraries:

* ITK 5.1.0
* Boost 1.66 (header libraries only)

More recent versions of ITK and Boost should also work with potentially minor modification to Antropy code. Please note that from Boost, only the header-only libraries (none of the compiled libraries) are required.

The following libraries and depdencies are brought in as Git submodules to the Antropy repository:

* argparse (https://github.com/p-ranav/argparse.git)
* CMake Modules Collection (https://github.com/rpavlik/cmake-modules)
* CMakeRC (https://github.com/vector-of-bool/cmrc.git)
* "Dear ImGui" (https://github.com/ocornut/imgui.git)
* ghc::filesystem (https://github.com/gulrak/filesystem.git)
* GLFW (https://github.com/glfw/glfw.git)
* GLM (https://github.com/g-truc/glm.git)
* imgui-filebrowser (https://github.com/AirGuanZ/imgui-filebrowser.git)
* imGuIZMO.quat (https://github.com/BrutPitt/imGuIZMO.quat.git)
* "JSON for Modern C++" (https://github.com/nlohmann/json.git)
* NanoVG (https://github.com/memononen/nanovg.git)
* spdlog (https://github.com/gabime/spdlog.git)
* stduuid (https://github.com/mariusbancila/stduuid.git)

Please run `git submodule update --init --recursive` after cloning to get these submodules.

The following external sources and libraries have been committed directly to the Antropy repository:

* GLAD OpenGL loaders (generated from https://github.com/Dav1dde/glad.git)
* GridCut: fast max-flow/min-cut graph-cuts optimized for grid graphs (https://gridcut.com)
* Local modifications to the ImGui bindings for GLFW and OpenGL 3 (see originals in externals/imgui/backends)
* Local modifications to the ImGui file browser (https://github.com/AirGuanZ/imgui-filebrowser)

The following external resources have been committed directly to the Antropy repository:

* Cousine font (https://fonts.google.com/specimen/Cousine)
* Roboto fonts (https://fonts.google.com/specimen/Roboto)
* "Library of Perceptually Uniform Colour Maps" by Peter Kovesi (https://colorcet.com)
* matplotlib color maps (https://matplotlib.org/3.1.1/gallery/color/colormap_reference.html)

Original attributions and licenses have been preserved and committed for all external sources and resources.


## Running

Antropy is run from the terminal. Images can be specified directly as command line arguments or from a JSON project file.

1. A list of images can be provided as positional arguments. If an image has an accompanying segmentation, then it is separated from the image filename using a comma (,) and no space. e.g.:

`./antropy reference_image.nii.gz,reference_seg.nii.gz additional_image1.nii.gz additional_image2.nii.gz,additional_image2_seg.nii.gz`

With this input format, each image may have only one segmentation.

2. Images can be specified in a JSON project file that is loaded using the `-p` argument. A sample current project file is given below.

```json
{
  "reference":
  {
    "image": "reference_image.nii.gz",
    "segmentations": [
      {
        "path": "reference_seg.nii.gz"
      }
    ],
    "landmarks": [
      {
        "path": "reference_landmarks.csv",
        "inVoxelSpace": false
      }
    ]
  },
  "additional":
  [
    {
      "image": "additional_image1.nii.gz",
      "affine": "additional_image1_affine.txt",
      "segmentations": [
        {
          "path": "additional_image1_seg1.nii.gz"
        },
        {
          "path": "additional_image1_seg2.nii.gz"
        }
      ],
      "landmarks": [
        {
          "path": "additional_image1_landmarks1.csv",
          "inVoxelSpace": false
        },
        {
          "path": "additional_image1_landmarks2.csv",
          "inVoxelSpace": false
        }
      ]
    },
    {
      "image": "additional_image2.nii.gz",
      "affine": "additional_image2_affine.txt"
    }
  ]
}
```

> The project must specify a reference image with the "reference" entry. Any number of additional (overlay) images are provided in the "additional" vector. An image can have an optional affine transformation matrix file, any number of segmentation layers, and any number of landmark files.

> Note: The project file format is subject to change!

Logs are output to the console and to files saved in the `logs` folder. Log level can be set using the `-l` argument. See help (`-h`) for more details.


## To be done

- [x] Vector annotations (in progress)
- [ ] Saving modifications to project files
- [ ] 3D rendering of landmark points
- [ ] A list of deformation field images can also be provided ("deformation" field in JSON project) for each image. The deformation fields are loaded, but code has not been implemented to apply them to warp the image.
