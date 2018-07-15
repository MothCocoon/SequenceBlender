# Mood Blender Component

I wrote Mood Blender component for easy blending between "mood states" of the world during game. Visual setting of day-night cycle is example of such "mood state".
We use Sequencer as WYSIWYG editor for such states. Every world state is represented as a single frame. 

The problem is that in our game we often jump between non-consecutive frames, so we can't simply play the sequence. Player would see transitions through many states. 

Solution? Let's use custom code for reading a new mood state from Sequencer and nicely blend world objects to it.