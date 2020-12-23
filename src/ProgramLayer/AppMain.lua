-- Main app module. Should be skeleton that stores data and calls through to
-- modules that save no state

ImGui     = ImGuiLib
GuiRender = require "GuiRender"
GdbData   = require "GdbData"

local Json = require "JSON"

GdbApp = {
	exe_filename = "",
	open_file = { short = "", full = "", func = "", is_open = false },

	force_reload = false,

	local_vars = {},
	asm = {},
	bktrace = {},
	registers = nil,
	memory = {},

	curr_stack_frame = 1,

	user_args = {},

	output_txt = "",
	local_vars_txt = "",
	frame_info_txt = "",

	open_dialog = false,
	open_dialog_exe = false,
}

print("Starting GdbApp")

function GdbApp:Init(args)
	-- Increase aggressiveness GC (wait for memory to grow to 1.5x then collect)
	collectgarbage("setpause", 150)
end

function GdbApp:OnExit(args)
	local session = Json:encode_pretty(self.user_args)

	local out_file = ROOT_DIR.."/bin/session.json"
	local file = io.open(out_file, "w")
	if file then
		file:write(session)
		file:close()
	end
end

function GdbApp:Update(args)
	if ImGui.BeginMainMenuBar() then
		if ImGui.BeginMenu("File") then
			if ImGui.MenuItem("Load Last Session", false) then
				local in_file = ROOT_DIR.."/bin/session.json"
				local file = io.open(in_file, "r")
				if file then
					local file_contents = {}
					for line in file:lines() do
						file_contents[#file_contents + 1] = line
					end

					file:close()
					
					self.user_args = Json:decode(table.concat(file_contents, "\n"))
					if self.user_args.ExeStart and self.user_args.ExeStart.exe then
						GdbData.LoadExe(self)
					end
					ReadFromGdb()
					GdbData.LoadSettings(self)
				end
			end
			if ImGui.MenuItem("Exe Open", false) then
				self.open_dialog_exe = true
			end
			if ImGui.MenuItem("File Open", false) then
				self.open_dialog = true
			end
			if ImGui.BeginMenu("Text Editor Theme") then
				if ImGui.MenuItem("Dark", false) then
					SetEditorTheme(1);
				end
				if ImGui.MenuItem("Light", false) then
					SetEditorTheme(2);
				end
				if ImGui.MenuItem("RetroBlue", false) then
					SetEditorTheme(3);
				end
				ImGui.EndMenu()
			end

			ImGui.Separator()
			if ImGui.MenuItem("Reload Modules", false) then
				print("Reloading all scripts...")
				package.loaded["GdbData"] = nil
				package.loaded["GuiRender"] = nil

				GdbData = require("GdbData")
				GuiRender = require("GuiRender")
			end
			ImGui.EndMenu()
		end
		ImGui.EndMainMenuBar()
	end

	GuiRender.Present(GdbApp, args.win_width, args.win_height)
end
