# RenderDoos

Framework library for building compute shaders in OpenGL and Metal.
RenderDoos provides a uniform API for setting up the compute shaders.
However, the compute shaders themselves need to be written for the target
graphical language, i.e., glsl or Metal Shading Language. There is no uniform
shader language, so if you want to run the compute shader on both Windows and MacOs,
you'll need to provide two compute shaders for both target shader languages.

See https://github.com/janm31415/RenderDoosDemo for examples.

Building should be easy with CMake. All necessary dependencies are delivered with the code.
