riftty
============

This repo is now read-only and archived, it uses an old Oculus SDK from DK1 era and is unlikely to work on newer HMDs. 

Terminal emulator meant for use with the Oculus Rift headseat.

Built with Oculus SDK 0.4.1 for DK2 support.

![](https://raw.github.com/hyperlogic/riftty/master/docs/screenshot.png)

## Download

[riftty.zip](http://www.hyperlogic.org/riftty.zip) - For MacOSX 10.9

## Configuration

To customize the configuraiton create a `.riffty` file in your home directory.
Each line in the file should be in the form Key=Value.

Here is an example config:

    SwapAltAndMetaKeys=true
    Columns=80
    Rows=25
    WinPosX=0
    WinPosY=1.7
    WinPosZ=-1.0
    WinFullscreen=false
    WinWidth=2.0
    Font=font/SourceCodePro-Regular.ttf
    FontSize=64

### Font

* **Font** (string) file location of a FreeType2 compatible font.
* **FontSize** (int) This won't change the size of the letters, but only how crisp they are when viewed up close.
  Don't use larger then 64 or so. 32 is the default.

### Terminal settings

* **Columns** (int) number of columns in the terminal. 80 columns is the default.
* **Rows** (int) number of rows in the terminal. 24 columns is the default.
* **Term** (string) reported in the TERM enviornment variable.

### Window positioning

* **WinPosX** (float) Used to move terminal window in world space.
  Negative values will move the terminal to the left, positive to the right.  0 is the default.
* **WinPosY** (float) Used to move the terminal window in world space.
  Positive values will move the terminal up, negative will move it down, Possibly below the floor.
  1 is the default.
* **WinPosZ** (float) Used to move the position of the terminal window in world space.
  The view is facing down the negative Z axis, so -1 will be 1 meter from your face, -10 will be 10 meters from your face.
  -1 is the default.
  Avoid positive values as they will be behind your head in the starting orientation.
* **WinWidth** (float) The width of the terminal in the world. 2 meters is the default
* **WinRotX** (float) Tilt the terminal around the x axis in degrees. 0 is the default.
* **WinRotY** (float) Tilt the terminal around the y axis in degrees. 0 is the default.
* **WinRotZ** (float) Tilt the terminal around the z axis in degrees. 0 is the default.

### Misc

* **WinFullscreen** (bool) when false the application will run in a window instead of fullscreen. true is the default.
* **SwapAltAndMetaKeys** (bool) when true the option and command keys are swapped. The default value is false.

