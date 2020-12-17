-- Main app module. Should be skeleton that stores data and calls through to
-- modules that save no state

local ImGui = ImGuiLib

GdbApp = {}

print("Starting GdbApp")

function GdbApp:Init(args)
	-- Increase aggressiveness of GC (will wait for memory to grow to 1.5x then collect)
	print("Old Garbage pause rate: ", collectgarbage("setpause", 150), " (new = 150)")
end

function GdbApp:Update(args)
	if ImGui.BeginMainMenuBar() then
		if ImGui.BeginMenu("File") then
			if ImGui.MenuItem("ShowDemo", false) then
			end
			if ImGui.MenuItem("Exe Open", false) then
			end
			if ImGui.MenuItem("File Open", false) then
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
			ImGui.EndMenu()
		end
		ImGui.EndMainMenuBar()
	end
end
