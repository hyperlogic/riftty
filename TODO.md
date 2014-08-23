# TODO

## Platforms
  * Linux (pending SDK udpate)
  * Windows... I suppose there would be value here?

## Configuration
Configurable placement. add to .riftty config file (mintty/config.c)
  * position, width, (in meters)
  * actually use more of the config variables: such as rows, cols color. etc.
  * transparency
  * font

Document config variables and what values they expect.

## Performance Improvements
Already on DK2 with a full screen of text, my laptop can't keep 75htz.

VBO's per line is probably the way to go, this way they don't have to be rebuilt
when scrolling, the floor shader is per-pixel and dynamic, probably a static light map would
be better.

## Keyboard Shortcuts
Keybindings for riftty specific things such as:
   * view recenter
   * standing vs sitting
   * display / hide fps or other data.
   * move screen in/out
   * increase/decrease screen size
   * move screen left/right/up/down
   * increase decrease terminal geometry (rows, cols)

The problem here is that we need some special way to escape keybindings so they don't go to the
terminal, but instead are interpreted by the system. Perhaps hooking up mouse would help here...

## Mouse Support
   * scroll back (middle mouse wheel), might be useful, even without actual cursor support.
   * click to place cursor
   * select for cut and paste.
   * fade out when typing.
   * eventually would be useful for switching keyboard focus between terminals
   * configurable cursor

## Future Experiments
   * multiple panels! mintty would need some serious re-factoring for this, so many globals.
     unless we use seperate processes and ipc, also a serious refactor.
   * curved screen! cylindrical, spherical.
   * avatar!
   * sky skinning - colors, textures
   * floor skinning - tile size, textures
   * dynamic monitor glow on your avatar and the environment!
   * pluggable 3d scenes (would probably want to switch to a real render for this)
   * would like to experiment with history extending past the extent of the
     terminal bounds, extending words upwards and fading out.
   * tmux integration? iTerm2 does some cool stuff here..

