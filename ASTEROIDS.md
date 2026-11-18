# Asteroids

Open **Apps → Games → Asteroids**. Move a finger left or right of screen centre
to turn, or above centre to thrust. The ship fires automatically;
rocks split when hit. Ship, shots and rocks wrap at the screen edge. The game
starts with three lives and briefly protects the ship after a collision. Clear a
wave to advance: later levels add rocks and increase their speed.

Movement uses Q4 subpixel coordinates, a fixed 20 ms update, bounded velocity
and gentle drag for smoother motion on the 240×240 display. It draws directly
with LVGL vector lines and allocates no framebuffer. The game is an original
implementation inspired by the arcade genre; it includes no ROM, copied art or
sound.
