Mood Blender component is used for easy blending between "mood states" of the world during the game. Setting of a day-night cycle or weather state are examples of such a "mood state".

Sequencer is used as a WYSIWYG editor for these states. Every world state is represented as a single keyframe. 

Problem to solve: what if we need to jump between non-consecutive keyframes? We can't simply play the sequence. Player would see transitions through many states.

Solution? Let's use custom code for reading a new state from Sequencer and nicely blend world objects to this state!

![Imgur](https://i.imgur.com/7wlymOY.gif)

You're free to fork this code and use it in your project. You're not allowed to sell this code.

## Getting started
* Content project serves as a simple demo. You don't need to copy any assets from the demo.
* What you need is to copy Mood Blender Component class and the few includes from Build.cs to your C++ project.

## Notes
Most of the code is universal and it's easy to customize component for a specific game.
* Blending supports object properties: float, color/vector, transform. 
* Component also blends Material Parameter Collections.
