Mood Blender component is used for easy blending between "mood states" of the world during game. Setting of day-night cycle or weather state are examples of such "mood state".

Sequencer is used as WYSIWYG editor for these states. Every world state is represented as a single frame. 

Problem to solve: what if we need to often jump between non-consecutive frames? We can't simply play the sequence. Player would see transitions through many states.

Solution? Let's use custom code for reading a new state from Sequencer and nicely blend world objects to this state!

![Imgur](https://i.imgur.com/7wlymOY.gif)

You're free to fork this code and use it in your project. You're not allowed to sell this code.

## Getting started
* Content project serves as a simple demo. You don't need to copy any assets from the demo.
* What you need is to copy Mood Blender Component class and the few includes from Build.cs to your C++ project.
* Let me know, if you see how I could improve this code!

## Notes
Most of the code is universal and it's easy customize component for a specific game. Although it didn't make sense to make it 100% universal.
* Blending supports these object properties: float, color/vector, transform. 
* Component also blends Material Parameter Collections.
