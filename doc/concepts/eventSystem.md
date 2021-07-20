\page eventSystem [deprecated] Event System
\todo check and update
[TOC]


# Notes for event handling the radium engine

 * Basic events
   * UI (ui clicks, keyboard, mouse)
   * Object moved
   * New plugin registered
 * Every plugin should be able to register his own events

## Events identification
Events could be identified by a name (string) and a unique ID

## Events registering
When registering a new plugin, a function could be called by the engine to register new events, associating event names and ID and giving back IDs to the plugin.

Any plugin could also ask the engine the ID of other plugins events, to subscribe to.

## Events queue
When an event occurs, the sender adds it to engine's event queue, with associated data (_any_ ftw !).
Then at each frame, events could be handled this way :
 * Send all events / data to each system
 * Send events to systems that have subscribed for it
 * Call system functions bound to an event (given at subscribe time)
