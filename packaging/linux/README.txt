TrickLogic2 for Linux (x86-64)
==============================

TrickLogic2 plays Trick x Logic Season 2 (PSP, UCJS-10105) natively from your
own disc image. None of the game's files are included.

Needs: a 64-bit x86 PC or a Steam Deck with Vulkan, and an image of your
Season 2 disc: the Japanese release or its Simplified Chinese translation,
as an .iso or a CHD (chdman createdvd).


Start
-----

Unpack the archive anywhere and run ./tricklogic2 in it. On the Steam Deck,
do that in desktop mode, or add tricklogic2 to Steam as a non-Steam game
and start it from game mode.

Setup then asks for your Season 2 disc image, checks it and prepares the
game. It also offers to add Season 1's four cases from an image of your
Season 1 disc (UCJS-10097); you can skip that and add them later with

    ./tricklogic2 --add-disc season1.iso


Playing
-------

Z or Space confirms, X or Backspace cancels, the arrows move, Enter skips a
movie, and Esc opens TrickLogic2's menu with the settings and the controls.
A gamepad, including the Steam Deck's own controls, works as a PSP's
controls; L3+R3 opens the menu.

Game data, saves and settings are kept in ~/.local/share/TrickLogic2/UCJS10105.


Report a problem: https://github.com/Hintay/TrickLogic2Native/issues (never attach
game files). Third-party software and its licenses: licenses/
