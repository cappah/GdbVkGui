-- Main app module. Should be skeleton that stores data and calls through to
-- modules that save no state

ImGui     = ImGuiLib
GuiRender = require "GuiRender"
GdbData   = require "GdbData"

GdbApp = {
	exe_filename = "",
	open_file = { short = "", full = "", func = "", is_open = false },

	force_reload = nil,
	module_list = {
		{ 1, "GuiRender" },
		{ 2, "GdbData" },
	},

	local_vars = {},
	asm = {},
	bktrace = {},
	registers = nil,
	memory = {},

	curr_stack_frame = 1,
	get_local_types = false,

	user_args = {},

	output_txt = "",
	local_vars_txt = "",
	frame_info_txt = "",

	open_dialog = false,
	open_dialog_exe = false,
	reload_module = false,
}

print("Starting GdbApp")

function GdbApp:Init(args)
	-- Increase aggressiveness GC (wait for memory to grow to 1.5x then collect)
	collectgarbage("setpause", 150)
end

function GdbApp:Update(args)
	if ImGui.BeginMainMenuBar() then
		if ImGui.BeginMenu("File") then
			if ImGui.MenuItem("ShowDemo", false) then
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
					SetEditorTheme(1);
				end
				if ImGui.MenuItem("RetroBlue", false) then
					SetEditorTheme(1);
				end
				ImGui.EndMenu()
			end

			ImGui.Separator()
			if ImGui.MenuItem("Reload Module", false) then
				self.reload_module = true
			end
			ImGui.EndMenu()
		end
		ImGui.EndMainMenuBar()
	end

	GuiRender.Present(GdbApp, args.win_width, args.win_height)

	-- module reloading
	
	if self.force_reload then
		local idx <const> = self.force_reload
		self.force_reload = nil

		print("Reloading: \""..self.module_list[idx][2].."\"...")
		package.loaded[self.module_list[idx][2]] = nil

		if self.module_list[idx][1] == 1 then 
			GuiRender = require(self.module_list[idx][2])
		elseif self.module_list[idx][1] == 2 then
			GdbData = require(self.module_list[idx][2])
		end
	end
end
