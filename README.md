## Interactive physarum

Interactive physarum/organic simulations, for an installation with gamepad control.

It is implemented with openFrameworks and the simulations run on GPU with compute shaders.

Please check out 36 Points by Sage Jenson (mxsage) first: https://www.sagejenson.com/36points/
Press 0-9, A-Z for different Points (different simulation parameters).

It runs at 60 fps on a RTX 2060 GPU, with more than 5M particles.

### License:
License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

#### Crediting:
Work derived from mxsage's 36 points (https://www.sagejenson.com/36points/) but with a different implementation.
Before studying mxsage's code I already had a physarum simulation implementation (already inspired by their work). I modified my implementation to use the most important features of their algorithm.
I am using atomic counters on pixels and this is different. No fragment or vertex shader, only compute shader. I am using the set of parameters from 36 Points. Some work well, some don't, I had to tune stuff. Currently, 17 points that seem to work quite well are used.

The license from 36 points is reproduced here.

Some text for crediting is displayed all the time on screen (bottom left corner). More crediting is shown when pressing the trigger of the gamepad that shows more information.

### Screenshots

Some screenshots to give a little taste of the project, as of May 1, 2024.

![Screenshot 1 May, 2024, 1](/doc/screenshot-1-may-2024-1.png)
![Screenshot 1 May, 2024, 2](/doc/screenshot-1-may-2024-2.png)
![Screenshot 1 May, 2024, 3](/doc/screenshot-1-may-2024-3.png)
![Screenshot 1 May, 2024, 5](/doc/screenshot-1-may-2024-5.png)
![Screenshot 1 May, 2024, 4bis](/doc/screenshot-1-may-2024-4-bis.png)

### Video

▶️ [**Demo video on YouTube**](https://www.youtube.com/watch?v=4hLtjlaVzsk) ✨

### Some context to understand the technique

The simulation algorithm is a more advanced version of the physarum simulation algorithm described by Jeff Jones in [this paper](https://uwe-repository.worktribe.com/output/980579/characteristics-of-pattern-formation-and-evolution-in-approximations-of-physarum-transport-networks).

Sage Jenson's approach is to control the original physarum simulation parameters of a particle based on some current sensed value. The original physarum simulation parameters are sensor distance, sensing angle, rotation angle and move distance. Particles also respawn at a random place after some time.

______________

I'm introducing many things on top of this simulation algorithm:

- Mainly spatial interpolation between the parameters of 2 Points. That introduces a lot of possibilities of interaction, and new patterns.
- interaction to move particles
- interaction to spawn particles
- particles keeping their inertia
- possibility to tune the parameters of each point
- color experiments

The way particles add trail is different compared to 36 points and it can result in different patterns.
______________

The compute shader code of my implementation is in bin/data/

**computeshader_move.glsl** : Main shader: updates particles, increases a counter on the particle's pixel position.

**computeshader_deposit.glsl** : Add trail from particle counter on pixel. Also gives the displayed image, from trail image and count on pixel.

**computeshader_blur.glsl** : Blur step on trail map (diffusion).

**computeshader_setter.glsl** : Just used to reset the counters of the pixels to 0 at each iteration.

### How to compile

[ofxGamepad](https://github.com/Bleuje/ofxGamepad) addon is used (it seems that you should use my fork because of a fix, or maybe just to have same version as me).
I'm only testing on Linux, and I bet that if there's a problem on other platforms it would be because of this gamepad code.

Here is a branch without gamepad code:
https://github.com/Bleuje/interactive-physarum/tree/no-gamepad
It's not perfect. Right now it keeps using some amount of "inertia" (something that can be triggered with gamepad). I'm keeping it like this because I think it looks quite cool. Mouse and keyboard interaction is explained later below.

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

Different color modes (many hues and black and white are available):
![Screenshot 1 May, 2024, 6](/doc/screenshot-1-may-2024-6.png)
![Screenshot 8 May, 2024, 1](/doc/screenshot-8-may-2024-1.png)
![Screenshot 8 May, 2024, 2](/doc/screenshot-8-may-2024-2.png)

### Interaction with keyboard and mouse

It also has interaction without connected gamepad

- Pen location with mouse position
- particles spawning with mouse clicks, waves triggered with middle click
- keyboard **arrows**: points choice navigation
- **R** or **Spacebar**: set random points
- **S**: swap pen/background points
- **D**/**F**: paricles spawning
- **X**/**C**: change pen size
- **P** or **H**: hide/show pen
- **W**: trigger wave
- **A**: change color mode

### Versions with less computation

I made a branch that uses less particles and has smaller simualtion canvas, for lighter computation:
https://github.com/Bleuje/interactive-physarum/tree/light-compute

It seems that the number of particles could be reduced even more while keeping a similar look by changing the simulation parameters: increasing a factor on the sensed value to compensate for having less particles.

I really haven't put much thought on making optimal choices for number of particles, simulation canvas size and parameters of compute shader parallelization.
