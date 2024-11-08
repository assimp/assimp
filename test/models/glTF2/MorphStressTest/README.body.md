## Screenshot

![screenshot](screenshot/screenshot_large.png)

## Description

This model has a base mesh, plus eight morph targets.  The base and each of the morph targets has
both a POSITION and a NORMAL accessor, resulting in no less than 18 vertex attributes being requested
by a typical implementation.  This number can be too high for many realtime graphics systems, and
the client runtime may take steps to limit the overall number of morph targets and/or limit the
number of active morph targets.

As such, this model is not expected to render correctly everywhere.  Instead, it pushes the limits
to see how many morph targets can move at once before problems become apparent.

## Animations

Three animations are included:

---
### "Individuals"

![Individuals animation](screenshot/Anim_Individuals.gif)

Each morph target is exercised one at a time, and returns to zero strength before the next one
starts to move.  This offers runtimes the best chance of success, as vertex attributes need only
be allocated for one morph target at a time.  Even so, systems that pre-allocate morph target
attributes may place an upper limit here, allowing only some of the test blocks to move.

---
### "TheWave"

![Wave animation](screenshot/Anim_TheWave.gif)

This animation tests a wave of morph targets being activated simultaneously.  This does require
a high number of vertex attributes to be available.  If glitches in movement are observed,
continue on to the "Pulse" animation for closer inspection.

---
### "Pulse"

![Pulse animation](screenshot/Anim_Pulse.gif)

This is the most stressful test, with all 8 morph targets reaching full strength before any
begin to subside.  Some runtimes may place limits on how many morph targets can be active at
once, resulting in a distinctive visual pattern here:  Only the first N test blocks will
appear to move, where N is the number of simultaneous active morph targets allowed.  Test
blocks on the right will remain frozen in the starting position until the first few blocks
on the left have returned to rest.  After the blocks on the left have returned to rest, they
could relinquish their vertex attributes to blocks on the right, allowing those blocks to
"catch up" to their assigned positions in the animation.

It is also possible that there could be a hard limit on the number of morph targets, regardless
of whether they are active or not.  In this case, only the first N blocks will move at all,
and the remainder will be frozen the entire time.

---
## Implementation Notes

BabylonJS has shared some technical details of their "Infinite Morph Targets" implementation
in [this YouTube video](https://www.youtube.com/watch?v=LBPRmGgU0PE).

