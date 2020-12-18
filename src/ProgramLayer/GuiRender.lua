-- Gui render module

local GuiRender = {}

function GuiRender.Present(data)
	local ImGui   = ImGuiLib
	local GdbData = GdbData

	local ExecuteCmd = function(cmd) if SendToGdb(cmd) then return ReadFromGdb() end end

	local buttons = {
		{ id    = "Update Stack Frame", 
		  args  = { "-stack-info-frame" }, 
		  parse = GdbData.UpdateFramePos },
		{ id = "Next",
		  args = { "-exec-next" },
		  parse = GdbData.Next },
		{ id = "Step Into",
		  args = { "-exec-step" },
		  parse = GdbData.UpdateFramePos },
		{ id = "Finish",
		  args = { "-exec-finish" },
		  parse = GdbData.UpdateFramePos },
		{ id = "Continue",
		  args = { "-exec-continue" },
		  parse = GdbData.UpdateFramePos },
		{ id = "Locals",
		  args = { "-stack-list-locals", "1" },
		  parse = GdbData.UpdateLocals },
	}

	local trigger_updates = false

	---------------------------------------------------------------------------
	-- Shortcut keys
	if ImGui.IsKeyPressed("o") and ImGui.IsKeyPressed("ctrl") then
		data.open_dialog = true
	end
	if ImGui.IsKeyPressed("e") and ImGui.IsKeyPressed("ctrl") then
		data.open_dialog_exe = true
	end
	if ImGui.IsKeyPressed("r") and ImGui.IsKeyPressed("ctrl") then
		data.reload_module = true
	end

	---------------------------------------------------------------------------

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

	------------------------------------------------------------------------
	-- load exe to gdb and set temp breakpoint in main

	local ename <const> = FileDialog("Open Executable to Debug", { 600, 300 } )
	if ename then
		data.exe_filename = ename

		local cmd_exe = string.format("-file-exec-and-symbols %s", ename)
		if SendToGdb(cmd_exe) then
			_ = ReadFromGdb()
			if SendToGdb("-exec-run --start") then
				local response <const> = ReadFromGdb()
				data.output_txt = response
				GdbData.UpdateFramePos(data, ExecuteCmd("-stack-info-frame"))

				trigger_updates = true
			end
		end
	end

	local fname <const> = FileDialog("Open File", { 600, 300 } )
	if fname then
		data.new_file = fname
	end

	------------------------------------------------------------------------
	-- UI for reloading modules
	
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
				trigger_updates = true

				ImGui.CloseCurrentPopup()
			end
		end
		
		if ImGui.Button("Cancel##reload_mod0") then
			ImGui.CloseCurrentPopup()
		end

		ImGui.EndPopup()
	end

	------------------------------------------------------------------------
	
	ImGui.Begin(string.format("%s###CodeWnd", data.open_file.short))

	if data.open_file.is_open then
		ShowTextEditor()
	end

	ImGui.End()

	------------------------------------------------------------------------
	
	ImGui.Begin("Watch1")

	for _, val in ipairs(buttons) do
		if ImGui.Button(val.id) then
			val.parse(data, ExecuteCmd(table.concat(val.args, " ")))
			trigger_updates = true
		end
	end

	ImGui.End()

	------------------------------------------------------------------------
	
	if trigger_updates then
		GdbData.UpdateLocals(data, ExecuteCmd("-stack-list-locals 1"))
	end

	------------------------------------------------------------------------

	ImGui.Begin("Watch2")

	ImGui.TextWrapped(data.output_txt)
	ImGui.Separator()

	ImGui.TextWrapped(data.frame_info_txt)

	ImGui.End()

	------------------------------------------------------------------------
	
	ImGui.Begin("Locals")

	ImGui.TextWrapped(data.local_vars_txt)
	ImGui.Separator()

	ImGui.End()
	
	------------------------------------------------------------------------
end

return GuiRender
