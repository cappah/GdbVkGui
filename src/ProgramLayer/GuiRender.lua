-- Gui render module

local GuiRender = {}

function GuiRender.Present(data, width, height)
	local ImGui   = ImGuiLib
	local GdbData = GdbData

	local ExecuteCmd = function(cmd) if SendToGdb(cmd) then return ReadFromGdb() end end
	local PrintFrame = function(a,b) a.frame_info_txt = b; print(b) end

	local buttons = {
		{ id        = "Update Stack Frame", 
		  args      = { "-stack-info-frame" },
		  parse     = GdbData.UpdateFramePos, 
		  upd_frame = false,
	    },
		{ id        = "Next",
		  args      = { "-exec-next" },
		  parse     = GdbData.Next, 
		  upd_frame = true,
	    },
		{ id        = "Step Into",
		  args      = { "-exec-step" },
		  parse     = GdbData.UpdateFramePos, 
		  upd_frame = true,
	    },
		{ id	    = "Finish",
		  args      = { "-exec-finish" },
		  parse     = GdbData.UpdateFramePos, 
		  upd_frame = true, 
	    },
		{ id        = "Continue",
		  args      = { "-exec-continue" },
		  parse     = GdbData.UpdateFramePos, 
		  upd_frame = true, 
	    },
		{ id        = "Locals",
		  args      = { "-stack-list-locals", "1" },
		  parse     = GdbData.UpdateLocals, 
		  upd_frame = false, 
	    },
		{ id        = "Disassembly",
		  args      = { "-data-disassemble", "-s \"$pc", "- 30\"", "-e \"$pc", "+ 30\"", "-- 0" },
		  parse     = GdbData.UpdateAsm, 
		  upd_frame = false, 
	    },
		{ id        = "Backtrace",
		  args      = { "-stack-list-frames" },
		  parse     = GdbData.UpdateBacktrace, 
		  upd_frame = false, 
	    },
	}

	local trigger_updates = false

	local old_stack_frame = data.curr_stack_frame

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

	local ename <const> = FileDialog("Open Executable to Debug", { width * 0.7, height * 0.5 } )
	if ename then
		data.exe_filename = ename

		local cmd_exe = string.format("-file-exec-and-symbols %s", ename)
		if SendToGdb(cmd_exe) then
			_ = ReadFromGdb()
			if SendToGdb("-exec-run --start") then
				local response <const> = ReadFromGdb()
				data.output_txt = response

				GdbData.UpdateBreakpoint(data, ExecuteCmd("-stack-info-frame"))
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
	
	ImGui.Begin("Watch1")

	for _, val in ipairs(buttons) do
		if ImGui.Button(val.id) then
			val.parse(data, ExecuteCmd(table.concat(val.args, " ")))

			-- this command resets to stack trace to lowest frame
			if val.upd_frame then data.curr_stack_frame = 1 end

			trigger_updates = true
		end
	end

	ImGui.End()

	------------------------------------------------------------------------
	
	ImGui.Begin("CallStack")

	ImGui.NewLine()

	if #data.asm > 0 then
		if ImGui.BeginTable("##bktrace", 5) then
			ImGui.TableNextRow()
			ImGui.TableSetColumnIndex(1)
			ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "address")
			ImGui.TableSetColumnIndex(2)
			ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "function")
			ImGui.TableSetColumnIndex(3)
			ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "line")
			ImGui.TableSetColumnIndex(4)
			ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "file")

			local bk_frame = {}
			for i in ipairs(data.bktrace) do 
				bk_frame[#bk_frame + 1] = data.curr_stack_frame == i
			end

			for i, stack_frame in ipairs(data.bktrace) do
				ImGui.TableNextRow()

				ImGui.TableSetColumnIndex(0)
				clicked, _ = ImGui.CheckBox("##bktr_box"..i, bk_frame[i])
				if clicked and (bk_frame[i] == false) then
					data.curr_stack_frame = i
				end

				-- address
				ImGui.TableSetColumnIndex(1)
				ImGui.Text(stack_frame.addr)

				-- function
				ImGui.TableSetColumnIndex(2)
				ImGui.Text(stack_frame.func)

				-- line
				ImGui.TableSetColumnIndex(3)
				ImGui.Text(stack_frame.line)

				-- file
				ImGui.TableSetColumnIndex(4)
				ImGui.Text(stack_frame.file)
			end
			ImGui.EndTable()
		end
	end

	if old_stack_frame ~= data.curr_stack_frame then
		-- change frames
		ExecuteCmd("-stack-select-frame "..(data.curr_stack_frame - 1))

		-- update relevant data views
		for _, val in ipairs(buttons) do
			local upd_flag = val.id == "Locals" 
				or val.id == "Disassembly"
				or val.id == "Update Stack Frame"

			if upd_flag then
				val.parse(data, ExecuteCmd(table.concat(val.args, " ")))
			end
		end
	end

	ImGui.End()

	------------------------------------------------------------------------
	
	if trigger_updates then
		---GdbData.UpdateLocals(data, ExecuteCmd("-stack-list-locals 1"))
		for _, val in ipairs(buttons) do
			local upd_flag = val.id == "Locals" 
				or val.id == "Disassembly" 
				or val.id == "Backtrace"

			if upd_flag then
				val.parse(data, ExecuteCmd(table.concat(val.args, " ")))
			end
		end
	end

	------------------------------------------------------------------------

	ImGui.Begin("Watch2")

	ImGui.TextWrapped(data.output_txt)
	ImGui.Separator()

	ImGui.TextWrapped(data.frame_info_txt)

	ImGui.End()

	------------------------------------------------------------------------
	
	ImGui.Begin("Locals")

	local txt = "Retrieve type info (requires round trip thru GDB)"
	clicked, data.get_local_types = ImGui.CheckBox(txt, data.get_local_types)
	if clicked then
		GdbData.GetVCard(data.local_vars)
	end

	if ImGui.BeginTable("##local_vars", 3) then
		-- force size on columns
		-- kinda hacky way to get what I want here
		local spacing10 = "          "
		ImGui.TableNextRow()
		ImGui.TableSetColumnIndex(0)
		ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "name")
		ImGui.TableSetColumnIndex(1)
		ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "type")
		ImGui.TableSetColumnIndex(2)
		ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "data                                                      ")

		for i, var in ipairs(data.local_vars) do
			ImGui.TableNextRow()

			-- name
			ImGui.TableSetColumnIndex(0)
			ImGui.Text(var.name)
			--ImGui.InputText("##lv_name"..i,var.name, imgui.enums.text.ReadOnly)

			-- type
			ImGui.TableSetColumnIndex(1)
			ImGui.Text(var.vtype)
			--ImGui.InputText("##lv_type"..i,var.vtype, imgui.enums.text.ReadOnly)

			-- value
			ImGui.TableSetColumnIndex(2)
			ImGui.InputText("##lv_item"..i, var.value, imgui.enums.text.ReadOnly)
		end
		ImGui.EndTable()
	end

	--ImGui.TextWrapped(data.local_vars_txt)

	ImGui.End()

	------------------------------------------------------------------------
	
	ImGui.Begin("ASM")

	ImGui.Text("Program Counter +- 30 bytes")
	ImGui.NewLine()

	if #data.asm > 0 then
		if ImGui.BeginTable("##asm", 4) then
			ImGui.TableNextRow()
			ImGui.TableSetColumnIndex(0)
			ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "address")
			ImGui.TableSetColumnIndex(1)
			ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "offset")
			ImGui.TableSetColumnIndex(2)
			ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "instruction")
			ImGui.TableSetColumnIndex(3)
			ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "function")

			for _, asm in ipairs(data.asm) do
				ImGui.TableNextRow()

				-- address
				ImGui.TableSetColumnIndex(0)
				ImGui.Text(asm.address)

				-- offset
				if asm.offset then
					ImGui.TableSetColumnIndex(1)
					ImGui.Text(asm.offset)
				end

				-- instruction
				ImGui.TableSetColumnIndex(2)
				ImGui.Text(asm.inst)

				-- function
				if asm.func then
					ImGui.TableSetColumnIndex(3)
					ImGui.Text(asm.func)
				end
			end
			ImGui.EndTable()
		end
	end

	ImGui.End()

	------------------------------------------------------------------------
	
	ImGui.Begin(string.format("%s###CodeWnd", data.open_file.short))

	if data.open_file.is_open then
		ShowTextEditor()
	end

	ImGui.End()

	------------------------------------------------------------------------
end

return GuiRender
