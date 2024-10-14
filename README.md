# NP-GODOT

This is a modified version of the [Godot Engine](https://godotengine.org) built specifically for my game, *At the Ends of Eras*.

## What are the differences?

It fixes various bugs, adds some small features that would be infeasible to implement otherwise, and replaces the renderer with a custom deferred renderer (not implemented yet lol).

## Why are these not ported upstream to the main Godot repository?

- These changes make the engine less general-purpose. Some features are removed, and those added are specific to my game.
- They are not tested in as many environments.  Some platforms, such as mobile and web, have been removed entirely.

## Should I use this for my own projects?

No. I made this because it was more fun than porting the game to Godot 4, which would be a much more adviseable solution.