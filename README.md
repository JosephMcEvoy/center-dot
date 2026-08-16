# Center Dot

Center Dot is a Windows application that renders a dot in the center of the screen. It is useful for PC games that do not provide a crosshair.

Note that this only works, when using the "borderless window" mode of games, as CenterDot is only a "Windows window" that places itself over the game window and stays on top. Adding a crosshair to a game in fullscreen mode would require more DirectX hacking and may be suspicous to anti-cheat engines.

![grafik](https://user-images.githubusercontent.com/5735788/216760639-e13a3e86-af15-4e8e-8cda-eaf984ee96d0.png)

## Usage

Start CenterDot.exe.
A little crosshair should appear on the screen.
The crosshair should stay visible in games using borderless window mode.

When the program is running it does not appear in the task bar but is accessible as a notification icon via the systray (notification bar):

![grafik](https://user-images.githubusercontent.com/5735788/216760715-c025ff34-980a-4338-934d-8a190f226fc3.png)

From there you can access a context menu using right click on the icon. 

The dot can be shown/hidden using the menu entry. 
There is also a shortcut/hotkey to show/hide the crosshair in-game: <kbd>Ctrl</kbd> + <kbd>H</kbd>

Using the option "Adjust dot" you can move the crosshair/dot to another position using the arrow keys on your keyboard. When done, click the menu entry "Adjust dot" again to disable the adjustment mode.

Using the option "Reset dot" the dot is re-aligned to the center of your primary screen and all of its settings go back to their defaults.

The context menu entry "Exit" quits the application and removes the crosshair.

## Crosshair styles

The context menu entry "Settings..." opens a dialog where the shape of the crosshair is picked. A preview next to the controls shows what the choice looks like before it is applied.

| Style | Description |
| --- | --- |
| Dot | A filled dot, the original Center Dot look and the default. |
| Circle | A hollow ring. |
| Dot in circle | A ring with a small dot in its middle. |
| Cross | Four arms meeting in the middle, a plain plus sign. |
| Cross with gap | Four arms with the middle left open, the classic shooter crosshair. |
| Cross with gap and dot | The same, plus a dot on the aiming point. |
| T-shape | Left, right and lower arm only, keeps the view above the aiming point clear. |
| X-shape | Four diagonal arms with the middle left open. |
| Chevron | An arrow head with its tip on the aiming point. |
| Corner brackets | Four corner brackets framing the aiming point. |

Every style is drawn in a fill color with a contour around it, both of which are free to choose, transparency included. Setting the contour color to fully transparent leaves the crosshair without an outline.

Four measurements shape the crosshair. Only the ones the selected style actually uses stay enabled in the dialog:

| Setting | Range | Used for |
| --- | --- | --- |
| Dot size | 1-30 px | Diameter of the dot resp. of the surrounding circle. |
| Line thickness | 1-10 px | Stroke width of the lines a crosshair is built from. |
| Center gap | 0-30 px | Distance between the aiming point and where the lines start. |
| Arm length | 1-40 px | Length of a single line of the crosshair. |

Settings are stored in `centerdot.ini` below your Windows application data directory and are written when the program exits.
