# GdbVkGui
GDB Frontend using X, Vulkan, and ImGui

![App Image](/images/screen_shot.png)

Wanted an easily configurable frontend for gdb for the few things I use it for. TUI interface for gdb just doesn't cut it. Also, I use this project to play around w/ low-level C programming. 

Making it public for others in case you'd like to use it to. There's a [.deb](https://github.com/maadeagbo/GdbVkGui/releases) to download it on the releases page if you don't want to download and build from source. I'll update this when I have the free time.

##### Shortcuts :
- Ctrl-e : open executable (after selection, another window will open and you can provide arguments)
- Ctrl-o : open file

The app will remember the last session (breakpoints, executable, arguments, etc...). It's saved automically on exit. The file menu has an option for loading the last session.

All config and settings files are written to `~/.gdbvkgui` in the user's home directory

Any program output generated is written to `~/.gdbvkgui/gdbmi_output.txt`. It's wriiten as the program generates it.

#### Credits:
- [ImGui](https://github.com/ocornut/imgui)
- [ImGui File Browser](https://github.com/gallickgunner/ImGui-Addons)
- [ImGui Text Editor](https://github.com/BalazsJako/ImGuiColorTextEdit)
- [Lua 5.4](https://github.com/lua/lua/tree/v5.4.0)
- [JSON Parser for Lua](http://regex.info/blog/lua/json)
- [TLSF memory allocator](https://github.com/mattconte/tlsf)
