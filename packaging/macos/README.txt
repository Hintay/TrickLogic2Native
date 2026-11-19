TrickLogic2 for macOS (Apple Silicon)
=====================================

TrickLogic2 plays Trick x Logic Season 2 (PSP, UCJS-10105) natively from your
own disc image. None of the game's files are included.

Needs: a Mac with Apple Silicon (M1 or newer) and macOS @MINIMUM_SYSTEM_VERSION@ or newer,
and an image of your Season 2 disc: the Japanese release or its Simplified
Chinese translation, as an .iso or a CHD (chdman createdvd).


Install
-------

Drag TrickLogic2 into Applications or any folder you can write to.


First start
-----------

This build is not notarized by Apple, so the first time you open it macOS
says that it cannot verify the developer and offers only Done or Move to
Trash. Choose Done, then:

  1. Open System Settings > Privacy & Security.
  2. Scroll down to the message about TrickLogic2 and click "Open Anyway".
  3. Confirm with your password or Touch ID, then "Open Anyway" once more.

Or, from Terminal:

    xattr -dr com.apple.quarantine /Applications/TrickLogic2.app

Setup then asks for your Season 2 disc image, checks it and prepares the
game. It also offers to add Season 1's four cases from an image of your
Season 1 disc (UCJS-10097); you can skip that and add them later with

    /Applications/TrickLogic2.app/Contents/MacOS/TrickLogic2 --add-disc season1.iso


Playing
-------

Z or Space confirms, X or Backspace cancels, the arrows move, Enter skips a
movie, and Esc opens TrickLogic2's menu with the settings and the controls.
A gamepad works as a PSP's controls; L3+R3 opens the menu.

Game data, saves and settings are kept in
~/Library/Application Support/TrickLogic2/UCJS10105.


Report a problem: https://github.com/Hintay/TrickLogic2Native/issues (never attach
game files). Third-party software and its licenses:
TrickLogic2.app/Contents/Resources/licenses/
