# Raymarching SDF Creation Interface

Written in C with [Raylib](https://www.raylib.com), this experiment provides a GUI for creating a hierarchy of 3d object descriptions.

These are dynamically interpreted as SDFs and combined into an OpenGL fragment shader which displays them via raymarching.
This is rendered with a hybrid approach so it's compatible with "normal" 3d rendering, such as the guides.

Original raymarching shader and SDFs are by [Inigo Quilez](https://iquilezles.org/articles/distfunctions2d/), with some modifications.

Supported primitives are:

- box
- sphere
- ellipsoid
- cylinder
- capsule
- plane

These can be modified by smoothing, translation or rotation, and combined with other primitives by union, intersection or subtraction
(which can also be smooth).

Only one "object" with a single colour is supported at the moment, and the lighting calculations are not editable by the interface.

The GUI is written with the help of [raygui](https://github.com/raysan5/raygui), and shows some custom controls:

- Expandable tree with drag and drop support
- Vector3 editor with support for editing in-place (eg `"0.5, 1, -1"`) and shortcuts (`"X"` for `<1, 0, 0>`) or expanding to components.

Sadly these are quite integrated with the system and are not in a position to be placed in a standalone library, although it could be done with some work.

## Screenshots

### A simple box

![Just a box](/screenshots/box.png)

### Hard union of box and sphere

![Hard union](/screenshots/hard-union.png)

### Smooth union of box and sphere

![Smooth union](/screenshots/smooth-union.png)

### Smooth union of a rounded box and a sphere

![Super smooth](/screenshots/smooth-union-smooth-box.png)