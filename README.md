# ESP32 Neopixel Tetris
Tetris playable on a WS2812B LED matrices via an ESP-Now remote.

#### Tetris Game
The Tetris game code became involved enough that I moved it to its own repository at [0xjmux/tetris](https://github.com/0xjmux/tetris). Since I designed it as a single-header library, this project only needs to import `tetris.h/tetris.c`, but I wanted to minimize the presence of all the other code for the x86 Linux driver, Unit tests, helpers, and CI in this repo.

#### Remote
I decided to use the [Wizmote](https://www.wizconnected.com/en-us/p/accessory-wizmote/046677603595) as a game controller because of its low cost and use of Espressif's ESP-NOW protocol, which I thought would be easy to receive via the ESP32's builtin antenna. This ended up being harder than anticipated, but does work relatively well.


#### Hardware
One of my personal goals was to complete a project of moderate complexity in which I designed the entire stack myself - Mechanical, Hardware, and Software.

As such, this project runs on [my iot_leddriver PCB](https://github.com/0xjmux/iot_leddriver_hw), and the final version will incorporate v3 of that board with a custom 3D printed enclosure for the 8x32 LED Matrix.


### Project Goals
I designed this project to target areas of my skillset which I felt could use additional development. I wanted a better understanding of the entire embedded development toolchain and process, and needed a way to fill in the gaps between what school teaches and the skills that are required to be a competent embedded software engineer.

These goals included:
* Write Tetris game code from scratch with minimal dependencies for embedded platforms using cross-platform development techniques. Code must be testable without hardware to expedite iteration and testing.
    * Set up CI from scratch for an embedded project using SIL techniques ([tetris repo CI](https://github.com/0xjmux/tetris/actions))
    * Learn CMake and set up the build system from scratch
    * Game needs to be runnable on low-powered devices using interrupts. Memory and compute requirements should be as minimal as possible.
    * Driver code needed to make a given display or controller work should be kept as minimal as possible, and supporting new hardware should be as easy as possible.
* Teach myself embedded test-driven development and embedded unit testing using techniques from James Greening's "Test-Driven Development for Embedded C" (link)
* Make extensive use of an RTOS and interrupts to make program more performant.
Improve my skills using JTAG and OpenOCD/GDB to debug RTOS task-based programs.


#### Controls
| Wizmote Button | Action        |
|----------------|---------------|
| ON             | None          |
| OFF            | Quit Game     |
| NIGHT          | Pause/Unpause |
| 1              | Left          |
| 2              | Up            |
| 3              | Down          |
| 4              | Right         |
| Bright Up/Down | None          |

### Libraries
```
.
├── neopixel                - zorxx/neopixel library, uses ESP32 I2S
├── neopixel_display        - my driver for displaying tetris boards on the LED matrix
└── tetris                  - my tetris game logic, 0xjmux/tetris
```

#### Library Modifications
Libraries are added as submodules for depedencies to be correctly tracked. However, some modifications might be needed.

For Zorxx's [`neopixel` library](https://github.com/zorxx/neopixel) to work with my hardware, I had to change apply the fix mentioned in [this issue](https://github.com/zorxx/neopixel/issues/2). Without this fix, the first LED would be stuck as Green and unable to be changed.
