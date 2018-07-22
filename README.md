# Mood Blender Component

I wrote Mood Blender component for easy blending between "mood states" of the world during game. Visual setting of day-night cycle is example of such "mood state".
Sequencer is used as WYSIWYG editor for these states. Every world state is represented as a single frame. 

The problem: in our game we often jump between non-consecutive frames, so we can't simply play the sequence. Player would see transitions through many states.
Solution? Let's use custom code for reading a new mood state from Sequencer and nicely blend world objects to this new state.

## Notes
* Currently component reads and blends: float, color/vector, transform. There was no need for more types.
* Blending could work with any kind of actor. Although I didn't make it working with any actor out of the box, it wouldn't be used in a my project.
* It's easy to support other actors. 
* My current implementation is limited to few actors commonly used for creating mood. 
  * Atmospheric Fog
  * Exponential Fog
  * Directional Light
  * Sky Light
* You only need copy MoodBlenderComponent class to project. Oh, and the few includes from Build.cs.
* I'm not a full-time programmers, just a technical designer. If you see how I could improve this code, let me know!