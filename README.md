dwmstatus
===

My [dwmstatus](http://dwm.suckless.org/dwmstatus/) config files...

### Installation ###

Before following those steps, you need to have [dwm](http://git.suckless.org/dwm) installed.

  * Clone the repo: `git clone git@github.com:Afnarel/dwmstatus.git`.
  * cd into it (`cd dwmstatus`) and run `make`. A dwmstatus executable is created.
  * Copy this file to /usr/bin (or any directory that is in your $PATH): `cp dwmstatus /usr/bin` or `sudo cp dwmstatus /usr/local/bin/` (this won't work if you are currently using dwmstatus!).
  * Put 'exec dwmstatus & dwm' in your .xinitrc file: `echo "exec dwmstatus & dwm" > ~/.xinitrc`.
  * Run `startx`
