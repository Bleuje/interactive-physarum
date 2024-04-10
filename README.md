## Interactive physarum

Interactive physarum/organic simulations, for an installation with gamepad control.

It is implemented with openFrameworks and the simulations run on GPU with compute shaders.

Please check out 36 Points by Sage Jenson first: https://www.sagejenson.com/36points/

### License:
License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

#### Crediting:
Work derived from mxsage's 36 points (https://www.sagejenson.com/36points/) but with a different implementation.
Before studying mxsage's code I already had a physarum simulation implementation. I modified my implementation to use the most important features of their algorithm.
I'm using atomic counters on pixels and this is different. No fragment or vertex shader, only compute shader. I'm using the set of parameters from 36 Points. Some work well, some don't, I had to tune stuff. Currently, I'm using 19 points that seem to work quite well.
This is also introducing interaction with gamepad.

The license from 36 points is reproduced here.

In the current version, some text for crediting is displayed all the time on screen.

### Screenshots

Some screenshots to give a little taste of the project, as of April 10.

![Screenshot 10 April 2024, 1](/doc/screenshot-10-april-2024-1.png)
![Screenshot 10 April 2024, 2](/doc/screenshot-10-april-2024-2.png)

### Some context to understand the technique

The simulation algorithm is a more advanced version of the physarum simulation algorithm described by Jeff Jones in [this paper](https://uwe-repository.worktribe.com/output/980579/characteristics-of-pattern-formation-and-evolution-in-approximations-of-physarum-transport-networks).

Sage Jenson's approach is to control the original physarum simulation parameters of a particle based on some current sensed value. The original physarum simulation parameters are sensor distance, sensing angle, rotation angle and "jump distance" (or speed). Particles also respawn randomly after some time.

### Interaction with gamepad

- Have a "*pen*" with location controlled by a joystick. The area near the pen has different simulation parameters comapred to *background*, and linear interpolation (with a gaussian function) is done between pen and background.
- The other joystick can displace particles near the pen area. Some noise is used for better looking displacement.
- simulation parameters of pen and background areas can be chosen with pad controls, there are currently 19 kinds of settings.
- button to set random settings
- button to swap the settings of pen and background
- button to trigger a wave of increased sensed value, from the pen location
- button to display or not the pen
- button to decrease the pen size
- button to increase the pen size
