-- Gui render module

local ImGui = ImGuiLib

local GuiRender = {}

function GuiRender:Present(data)
	if data.open_dialog then
		ImGui.OpenPopup("Open File")
		data.open_dialog = false
	elseif data.open_dialog_exe then
		ImGui.OpenPopup("Open Executable to Debug")
		data.open_dialog_exe = false
	elseif data.reload_module then
		ImGui.OpenPopup("Reload Module")
		data.reload_module = false
	end

	local ename <close> = FileDialog("Open Executable to Debug", { 600, 300 } )
	if fname then
		data.exe_filename = fname
	end

	local fname <close> = FileDialog("Open File", { 600, 300 } )
	if fname then
		data.new_file = fname
	end

	local clicked, _ = ImGui.BeginPopupModal("Reload Module")
	if clicked then
		ImGui.Text("Select module to reload :")
		ImGui.NewLine()

		for i, val in ipairs(data.module_list) do
			ImGui.Text("> "..val[2])
			ImGui.SameLine()
			ImGui.Dummy({5, 2})
			ImGui.SameLine()

			if ImGui.SmallButton("reload##reload_mod"..i) then
				data.force_reload = i
				ImGui.CloseCurrentPopup()
			end
		end
		
		ImGui.EndPopup()
	end

	------------------------------------------------------------------------
	
	ImGui.Begin(string.format("%s##CodeWnd", data.open_file.short))
	ImGui.End()

	------------------------------------------------------------------------
	
	ImGui.Begin("Watch1")
	ImGui.End()

	------------------------------------------------------------------------

	ImGui.Begin("Watch2")

	ImGui.TextWrapped(data.output_txt)
	ImGui.Separator()

	ImGui.TextWrapped(data.local_vars_txt)
	ImGui.Separator()

	ImGui.TextWrapped(data.frame_info_txt)

	ImGui.End()
end

return GuiRender
