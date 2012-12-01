N64iOS
======
An N64 Emulator without the need to jailbreak.
----------------------------------------------

### ATTENTION

This version of N64iOS is NOT complete. While it compiles and you can select a ROM, the emulator will crash, and even if it didn't crash there is no UI to actually play the ROM yet. Here's an Q&A to help:

**So why put it on Github now?**

I would love help trying to get this finished. While [SNES4iOS](http://github.com/iSkythe/SNES4iOS) and [GBA4iOS](http://github.com/iSkythe/GBA4iOS) were far from easy to put together, the overall structure of the apps weren't _too_ hard. However, to emulate an N64 much more is required in the development process, such as the addition of plugins.

**How can I help?**

You can help by cloning the project to your Mac, and then running it on your iOS Device. It will crash somewhat shortly after selecting a ROM, and then you can use your debugging magic to help figure out what is causing the crash. The vast majority of the code is working fine.

**What about the UI for playing games?**

I will implement the UI once the backend of the emulator is working. It shouldn't take too long to do.

**Anything else I should know?**

The plugins aren't perfect yet, such as gles2n64. Help would be appreciated with this as well. However, if you wish to modify the plugins, make sure under Xcode>Preferences>Locations>Advanced… you've selected "Legacy". This builds the plugins in the same folder as the source, allowing them to be included by N64iOS.