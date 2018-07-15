# Mood Blender Component

I wrote Mood Blender component for easy blending between "mood states" of the world during game. Visual setting of day-night cycle is example of such "mood state".
We use Sequencer as WYSIWYG editor for such states. Every world state is represented as a single frame. 

The problem is that in our game we often jump between non-consecutive frames, so we can't simply play the sequence. Player would see transitions through many states. 

Solution? Let's use custom code for reading a new mood state from Sequencer and nicely blend world objects to it.


Notes
* Blending could work with any kind of actor/object. Currently component reads and blends: float, color/vector, transform.
* My implementation is limited to few actors commonly used to create a mood. 
 * Atmospheric Fog
 * Exponential Fog
 * Directional Light
 * Sky Light
* It's easy to support other actors. I didn't make code universal because I wouldn't find use for it.
* You only need copy MoodBlenderComponent class to project. Oh, and the few includes from Build.cs.
* I'm not a full-time programmers, just a technical designer. If you see how I could improve this code, let me know!