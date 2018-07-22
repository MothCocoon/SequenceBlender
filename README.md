# Mood Blender Component

I wrote Mood Blender component for easy blending between "mood states" of the world during game. Visual setting of day-night cycle is example of such "mood state".
Sequencer is used as WYSIWYG editor for these states. Every world state is represented as a single frame. 

The problem: in our game we often jump between non-consecutive frames, so we can't simply play the sequence. Player would see transitions through many states.
Solution? Let's use custom code for reading a new mood state from Sequencer and nicely blend world objects to this new state.

![Alt Text](https://imgur.com/a/xzsRHpb.gif)

You're free to fork this code and use it in your project. You're not allowed to sell this code to other developers.

## Demo project
Most of this project is used as simple demo. Simply move the code to your project, you don't need any assets or blueprints.

## For programmers  
* You only need to copy MoodBlenderComponent class and the few includes from Build.cs to your project.
* I'm not a full-time programmers, just a technical designer. If you see how I could improve this code, let me know!

## Notes
Most of the code is universal and it's easy customize component for a specific game. Although it didn't make sense to make it 100% universal.
* Currently component reads and blends: float, color/vector, transform. 
* My implementation is limited to few actors commonly used for creating mood - for the convencience only.
  * Atmospheric Fog
  * Exponential Fog
  * Directional Light
  * Sky Light
* It's easy to support other actors.