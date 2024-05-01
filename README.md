## Interactive physarum

Interactive physarum/organic simulations, for an installation with gamepad control.

It is implemented with openFrameworks and the simulations run on GPU with compute shaders.

Please check out 36 Points by Sage Jenson first: https://www.sagejenson.com/36points/

### License:
License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

#### Crediting:
Work derived from mxsage's 36 points (https://www.sagejenson.com/36points/) but with a different implementation.
Before studying mxsage's code I already had a physarum simulation implementation. I modified my implementation to use the most important features of their algorithm.
I'm using atomic counters on pixels and this is different. No fragment or vertex shader, only compute shader. I'm using the set of parameters from 36 Points. Some work well, some don't, I had to tune stuff. Currently, 17 points that seem to work quite well are used.

The license from 36 points is reproduced here.

In the current version, some text for crediting is displayed all the time on screen.

### Screenshots

Some screenshots to give a little taste of the project, as of May 1, 2024.

![Screenshot 1 May, 2024, 1](/doc/screenshot-1-may-2024-1.png)
![Screenshot 1 May, 2024, 2](/doc/screenshot-1-may-2024-2.png)
![Screenshot 1 May, 2024, 3](/doc/screenshot-1-may-2024-3.png)
![Screenshot 1 May, 2024, 5](/doc/screenshot-1-may-2024-5.png)
![Screenshot 1 May, 2024, 4bis](/doc/screenshot-1-may-2024-4-bis.png)

### How to compile

[ofxGamepad](https://github.com/Bleuje/ofxGamepad) addon is used (it seems that you should use my fork because of a fix, or maybe just to have same version as me).
I'm only testing on Linux, and I bet that if there's a problem on other platforms it would be because of this gamepad code. Maybe a version with keyboard interaction should be done to have this project without dependency on this addon.

### Some context to understand the technique

The simulation algorithm is a more advanced version of the physarum simulation algorithm described by Jeff Jones in [this paper](https://uwe-repository.worktribe.com/output/980579/characteristics-of-pattern-formation-and-evolution-in-approximations-of-physarum-transport-networks).

Sage Jenson's approach is to control the original physarum simulation parameters of a particle based on some current sensed value. The original physarum simulation parameters are sensor distance, sensing angle, rotation angle and move distance. Particles also respawn at a random place after some time.

### Interaction with gamepad

It's designed for xbox gamepad.

- Have a "*pen*" with location controlled by a joystick. The area near the pen has different simulation parameters compared to *background*: linear interpolation of parameters is done between pen and background. A 2D gaussian function is used to set the interpolation parameter (1 on pen center, almost 0 far from the pen). The size of the pen is the sigma of the gaussian function, and the user can control it.
- The other joystick can displace particles near the pen area. Some noise is used for better looking displacement.
- simulation parameters of pen and background areas can be chosen with pad controls, there are currently 17 default points.
- button to set random points choice
- button to swap the points of pen and background
- button for spawning of particles in few locations near pen
- button for circular particles spawning
- button to decrease the pen size
- button to increase the pen size
- joystick click to display or not the pen
- joystick click to create wave (speed + sensing intensity)
- button for color style change
- button for points parameters tuning (advanced settings)
- left trigger: control the use of inertia in movement (speed updates instead of position updates)
- right trigger: information display

![screenshot with information display](/doc/information-screenshot.png)

Advanced settings to be able to tune points and have different patterns than default ones:
![screenshot with advanced settings display](/doc/advanced-settings.png)
