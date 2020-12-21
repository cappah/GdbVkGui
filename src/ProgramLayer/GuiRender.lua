-- Gui render module

local GuiRender = {}

function GuiRender.Present(data, width, height)
	local ImGui   = ImGuiLib
	local GdbData = GdbData

	local ExecuteCmd = function(cmd) if SendToGdb(cmd) then return ReadFromGdb() end end
	local PrintFrame = function(a,b) print(b) end

	local buttons = {
		{ id        = "Refresh", 
		  args      = { "-stack-info-frame" },
		  parse     = GdbData.UpdateFramePos, 
		  upd_frame = false,
		  invisible = false,
		  mod_args  = nil,
		  auto_upd  = false,
	    },
		{ id        = "Next",
		  args      = { "-exec-next" },
		  parse     = GdbData.Next, 
		  upd_frame = true,
		  invisible = false,
		  mod_args  = nil,
		  auto_upd  = false,
	    },
		{ id        = "Step Into",
		  args      = { "-exec-step" },
		  parse     = GdbData.UpdateFramePos, 
		  upd_frame = true,
		  invisible = false,
		  mod_args  = nil,
		  auto_upd  = false,
	    },
		{ id	    = "Finish",
		  args      = { "-exec-finish" },
		  parse     = GdbData.UpdateFramePos, 
		  upd_frame = true, 
		  invisible = false,
		  mod_args  = nil,
		  auto_upd  = false,
	    },
		{ id        = "Continue",
		  args      = { "-exec-continue" },
		  parse     = GdbData.UpdateFramePos, 
		  upd_frame = true, 
		  invisible = false,
		  mod_args  = nil,
		  auto_upd  = false,
	    },
		{ id        = "Locals",
		  args      = { "-stack-list-locals 1" },
		  parse     = GdbData.UpdateLocals, 
		  upd_frame = false, 
		  invisible = true,
		  mod_args  = nil,
		  auto_upd  = true,
	    },
		{ id        = "Disassembly",
		  args      = { "-data-disassemble -s \"$pc - ", "@before", "\" -e \"$pc + ", "@after", "\" -- 0" },
		  parse     = GdbData.UpdateAsm, 
		  upd_frame = false, 
		  invisible = true,
		  mod_args  = GdbData.ParseDataInput,
		  auto_upd  = true,
		  defaults  = { 30, 30 }
	    },
		{ id        = "Backtrace",
		  args      = { "-stack-list-frames" },
		  parse     = GdbData.UpdateBacktrace, 
		  upd_frame = false, 
		  invisible = true,
		  mod_args  = nil,
		  auto_upd  = true,
	    },
		{ id        = "Registers",
		  args      = { "-data-list-register-names" },
		  parse     = GdbData.SetTrackedRegisters, 
		  upd_frame = false, 
		  invisible = true,
		  mod_args  = nil,
		  auto_upd  = false,
	    },
		{ id        = "Register Values",
		  args      = { "-data-list-register-values r 1" },
		  parse     = GdbData.UpdateRegisters, 
		  upd_frame = true, 
		  invisible = true,
		  mod_args  = GdbData.SetupRegisterDataCmd,
		  auto_upd  = true,
	    },
		{ id        = "Memory",
		  args      = { "-data-read-memory-bytes", " @address", " @bytes" },
		  parse     = GdbData.UpdateMemory, 
		  upd_frame = false, 
		  invisible = true,
		  mod_args  = GdbData.ParseDataInput,
		  auto_upd  = false,
		  defaults  = { "&main", 100 }
	    },
		{ id        = "Watch",
		  args      = { "-data-evaluate-expression", " @expression" },
		  parse     = PrintFrame, 
		  upd_frame = false, 
		  invisible = true,
		  mod_args  = GdbData.ParseDataInput,
		  auto_upd  = false,
	    },
	}

	local trigger_updates = false

	local old_stack_frame = data.curr_stack_frame

	if #data.user_args == 0 then
		-- start tracking
		for _, cmd in ipairs(buttons) do
			if data.user_args[cmd.id] == nil then
				data.user_args[cmd.id] = {}

				local d_idx = 1
				for _, val in ipairs(cmd.args) do
					if val:find("@") then 
						data.user_args[cmd.id][#data.user_args[cmd.id] + 1] = {
							id = val:gsub("@", ""),
							val = cmd.defaults and tostring(cmd.defaults[d_idx]) or ""
						}
						d_idx = d_idx + 1
					end
				end
			end
		end
	end

	-- if watch args does not exist, create default
	if data.user_args and (data.user_args.WatchExpr == nil) then
		data.user_args.WatchExpr = { { expr = "", value = "" } }
	end

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

				ReadFromGdb() -- clear stdout

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

	for _, val in ipairs(buttons) do
		if (val.invisible == false) then
			ImGui.SameLine()
			if ImGui.Button(val.id) then
				if val.mod_args then
					val.parse(data, ExecuteCmd(table.concat(val.mod_args(data, val), "")))
				else
					val.parse(data, ExecuteCmd(table.concat(val.args, "")))
				end

				-- this command resets to stack trace to lowest frame
				if val.upd_frame then data.curr_stack_frame = 1 end

				trigger_updates = true
			end
			-- user input
			if data.user_args[val.id] then
				for user_i, user_v in ipairs(data.user_args[val.id]) do
					ImGui.Text(" - "); ImGui.SameLine()

					_, user_v.val = ImGui.InputTextWithHint(
						"##"..val.id..user_i, user_v.id, user_v.val)
				end
			end
		end
	end

	if data.open_file.is_open then
		ShowTextEditor()
	end

	ImGui.End()

	------------------------------------------------------------------------
	
	ImGui.Begin("CallStack")

	ImGui.NewLine()

	local tbl_sz = ImGui.GetWindowSize()
	tbl_sz[2] = tbl_sz[2] - 60
	if #data.asm > 0 then
		if ImGui.BeginTable("##bktrace", 5, tbl_sz) then
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
				or val.id == "Refresh"
				or val.id == "Register Values"

			if upd_flag then
				if val.mod_args then
					val.parse(data, ExecuteCmd(table.concat(val.mod_args(data, val), "")))
				else
					val.parse(data, ExecuteCmd(table.concat(val.args, "")))
				end
			end
		end
	end

	ImGui.End()

	------------------------------------------------------------------------
	
	if trigger_updates then
		for _, val in ipairs(buttons) do
			if val.auto_upd then
				if val.mod_args then
					val.parse(data, ExecuteCmd(table.concat(val.mod_args(data, val), "")))
				else
					val.parse(data, ExecuteCmd(table.concat(val.args, "")))
				end
			end
		end

		-- watch window
		for i, watch_data in ipairs(data.user_args.WatchExpr) do
			if watch_data.expr ~= "" then
				local val = GdbData.UpdateWatchExpr(
					data, ExecuteCmd("-data-evaluate-expression "..watch_data.expr))
				watch_data.value = val == "" and watch_data.value or val
			end
		end
	end

	------------------------------------------------------------------------

	ImGui.Begin("Ouput")

	ImGui.TextWrapped(data.output_txt)
	ImGui.Separator()

	ImGui.TextWrapped(data.frame_info_txt)

	ImGui.End()

	------------------------------------------------------------------------
	
	-- force size on columns
	-- kinda hacky way to get what I want here
	local spacing20 = "                    "

	ImGui.Begin("Locals")

	local txt = "Retrieve type info (requires round trip thru GDB)"
	clicked, data.get_local_types = ImGui.CheckBox(txt, data.get_local_types)
	if clicked then
		GdbData.GetVCard(data.local_vars)
	end

	tbl_sz = ImGui.GetWindowSize()
	tbl_sz[2] = tbl_sz[2] - 60
	if ImGui.BeginTable("##local_vars", 3, tbl_sz) then
		ImGui.TableNextRow()
		ImGui.TableSetColumnIndex(0)
		ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "name")
		ImGui.TableSetColumnIndex(1)
		ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "type")
		ImGui.TableSetColumnIndex(2)
		ImGui.TextColored({ 1.0, 1, 1, 0.5 }, string.format("data%s%s%s", spacing20, spacing20, spacing20))

		for i, var in ipairs(data.local_vars) do
			ImGui.TableNextRow()

			-- name
			ImGui.TableSetColumnIndex(0)
			ImGui.Text(var.name)

			-- type
			if var.vtype then
				ImGui.TableSetColumnIndex(1)
				ImGui.Text(var.vtype)
			end

			-- value
			if var.value then
				ImGui.TableSetColumnIndex(2)
				ImGui.PushItemWidth(-1)
				ImGui.InputText("##lv_item"..i, var.value, imgui.enums.text.ReadOnly)
				ImGui.PopItemWidth()
			end
		end
		ImGui.EndTable()
	end

	--ImGui.TextWrapped(data.local_vars_txt)

	ImGui.End()

	------------------------------------------------------------------------
	
	ImGui.Begin("ASM")

	if #data.asm > 0 then
		-- must exist
		local curr_pc = data.bktrace[data.curr_stack_frame].addr

		tbl_sz = ImGui.GetWindowSize()
		tbl_sz[2] = tbl_sz[2] - 30
		if ImGui.BeginTable("##asm", 5, tbl_sz) then

			for _, val in ipairs(buttons) do
				if val.id == "Disassembly" then
					for i, user_v in ipairs(data.user_args[val.id]) do
						ImGui.TableNextRow()

						ImGui.TableSetColumnIndex(1)
						ImGui.Text(" - bytes "..user_v.id..": ")
						ImGui.TableSetColumnIndex(2)

						ImGui.PushItemWidth(-1)
						_, user_v.val = ImGui.InputText("##"..val.id..i, user_v.val)
						ImGui.PopItemWidth()

						ImGui.TableSetColumnIndex(3)
						ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "offset $PC")
					end
					break
				end
			end

			ImGui.TableNextRow()
			ImGui.TableSetColumnIndex(1)
			ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "address")
			ImGui.TableSetColumnIndex(2)
			ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "offset")
			ImGui.TableSetColumnIndex(3)
			ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "instruction")
			ImGui.TableSetColumnIndex(4)
			ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "function")

			for _, asm in ipairs(data.asm) do
				ImGui.TableNextRow()

				-- location marker
				if asm.address == curr_pc then
					ImGui.TableSetColumnIndex(0)
					ImGui.Text(">")
				end

				-- address
				ImGui.TableSetColumnIndex(1)
				ImGui.Text(asm.address)

				-- offset
				if asm.offset then
					ImGui.TableSetColumnIndex(2)
					ImGui.Text(asm.offset)
				end

				-- instruction
				ImGui.TableSetColumnIndex(3)
				ImGui.Text(asm.inst)

				-- function
				if asm.func then
					ImGui.TableSetColumnIndex(4)
					ImGui.Text(asm.func)
				end
			end
			ImGui.EndTable()
		end
	end

	ImGui.End()

	------------------------------------------------------------------------
	
	ImGui.Begin("Registers")

	if ImGui.Button("Populate") then
		GdbData.SetTrackedRegisters(data, ExecuteCmd("-data-list-register-names"))
		GdbData.UpdateRegisters(data, ExecuteCmd(table.concat(GdbData.SetupRegisterDataCmd(data, val), "")))
	end

	tbl_sz = ImGui.GetWindowSize()
	if ImGui.BeginTable("##registers", 2, tbl_sz) then
		ImGui.TableNextRow()
		ImGui.TableSetColumnIndex(0)
		ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "name")
		ImGui.TableSetColumnIndex(1)
		ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "data"..spacing20..spacing20)

		if data.registers then
			for _, reg in pairs(data.registers) do
				ImGui.TableNextRow()

				-- name
				ImGui.TableSetColumnIndex(0)
				ImGui.Text(reg.id)

				-- value
				ImGui.TableSetColumnIndex(1)
				ImGui.PushItemWidth(-1)
				ImGui.InputText("##reg_item"..reg.number, reg.value, imgui.enums.text.ReadOnly)
				ImGui.PopItemWidth()
			end
		end
		ImGui.EndTable()
	end

	ImGui.End()

	------------------------------------------------------------------------
	
	ImGui.Begin("Watch")
	

	if ImGui.Button("Add watch expression") then
		data.user_args.WatchExpr[#data.user_args.WatchExpr + 1] = { expr = "", value = "" }
	end

	-- get longest expression
	local longest_expr = 15
	for i, watch_data in ipairs(data.user_args.WatchExpr) do
		local e_len = watch_data.expr:len()
		longest_expr = e_len > longest_expr and e_len or longest_expr
	end
	local expr_space = {}
	for i = 1, longest_expr, 1 do expr_space[#expr_space + 1] = " " end

	tbl_sz = ImGui.GetWindowSize()
	tbl_sz[2] = tbl_sz[2] - 60 -- shrink in y-axis
	if ImGui.BeginTable("##watch", 2, tbl_sz) then
		ImGui.TableNextRow()
		ImGui.TableSetColumnIndex(0)
		ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "expr      "..table.concat(expr_space))
		ImGui.TableSetColumnIndex(1)
		ImGui.TextColored({ 1.0, 1, 1, 0.5 }, string.format("data%s%s%s", spacing20, spacing20, spacing20))

		for i, watch_data in ipairs(data.user_args.WatchExpr) do
			ImGui.TableNextRow()

			ImGui.TableSetColumnIndex(0)
			local in_expr = watch_data.expr

			ImGui.PushItemWidth(-1)
			clicked, in_expr = ImGui.InputText("##watche"..i, in_expr, imgui.enums.text.EnterReturnsTrue)
			ImGui.PopItemWidth()
			if clicked then
				-- update specific watch value
				watch_data.expr = in_expr
				local val = GdbData.UpdateWatchExpr(
					data, ExecuteCmd("-data-evaluate-expression "..watch_data.expr))
				watch_data.value = val == "" and watch_data.value or val
			end

			ImGui.TableSetColumnIndex(1)
			ImGui.PushItemWidth(-1)
			ImGui.InputText("##watchv"..i, watch_data.value, imgui.enums.text.ReadOnly)
			ImGui.PopItemWidth()
		end

		ImGui.EndTable()
	end

	ImGui.End()

	------------------------------------------------------------------------
	
	ImGui.Begin("Memory")

	-- if settings does not exist, create default
	if data.user_args and (data.user_args.MemView == nil) then
		data.user_args.MemView = { active = false, bPerColumn = 2, nColumns = 5 }
	end
	local mem_settings = data.user_args.MemView

	-- Memory view setting UI
	local mem_cmd = nil
	for _, val in ipairs(buttons) do
		if val.id == "Memory" then
			mem_cmd = val

			clicked, mem_settings.active = ImGui.CheckBox("Track", mem_settings.active)
			if mem_settings.active then
				val.parse(data, ExecuteCmd(table.concat(val.mod_args(data, val), "")))
			end

			ImGui.SameLine()

			-- TODO : Maybe fix imgui functions so that I don't need to keep transforming
			-- ReadFBufferFromLua is greedy and is currently hardcoded to 4

			local v4 = { mem_settings.bPerColumn, mem_settings.nColumns }

			ImGui.PushItemWidth(-1)
			_, v4 = ImGui.SliderFloat2("##mem_input", v4, 1.0, 20.0, "%g")
			ImGui.PopItemWidth()

			mem_settings.bPerColumn = math.floor(v4[1])
			mem_settings.nColumns = math.floor(v4[2])
		end
	end

	tbl_sz = ImGui.GetWindowSize()
	tbl_sz[1] = tbl_sz[1] - 3 -- shrink in x-axis
	tbl_sz[2] = tbl_sz[2] - 60 -- shrink in y-axis
	if mem_cmd and ImGui.BeginTable("##memory", mem_settings.nColumns + 2, tbl_sz) then
		-- Each memory unit in gdb is 8 bits
		-- bPerColumn are the # of hex values per column
		-- Each hex value represents up to 16 bits (i.e. 2 bytes)
		-- so request size per column is (bPerColumn * 8 * 2)
		local mem_unit = mem_settings.bPerColumn * 8 * 2

		for i, user_v in ipairs(data.user_args[mem_cmd.id]) do
			ImGui.TableNextRow()

			ImGui.TableSetColumnIndex(1)
			ImGui.PushItemWidth(-1)
			_, user_v.val = ImGui.InputTextWithHint(
				"##"..mem_cmd.id..i, user_v.id, user_v.val)
			ImGui.PopItemWidth()
		end

		ImGui.TableNextRow()
		ImGui.TableSetColumnIndex(1)
		ImGui.TextColored({ 1.0, 1, 1, 0.5 }, "address          ")
		for i = 1, mem_settings.nColumns, 1 do
			ImGui.TableSetColumnIndex(i + 1)
			ImGui.TextColored({ 1.0, 1, 1, 0.5 }, " "..(i - 1).." ")
		end

		if data.memory.contents then
			local total_digits = data.memory.contents:len()
			local max_rows = math.ceil(
				total_digits / (mem_settings.bPerColumn * mem_settings.nColumns))

			local h_cntr = 1
			for irow = 1, max_rows, 1 do
				ImGui.TableNextRow()
				ImGui.TableSetColumnIndex(0)
				ImGui.TextColored({ 1.0, 1, 1, 0.5 }, irow - 1)

				if irow == 1 then
					ImGui.TableSetColumnIndex(1)
					ImGui.Text(data.memory.begin)
				end

				-- write backwards because of big endian formatting
				for iclmn = mem_settings.nColumns, 1, -1 do
					ImGui.TableSetColumnIndex(iclmn + 1)

					local out_hex = { "0x" }
					for ihex = mem_settings.bPerColumn, 1, -1 do
						if total_digits >= h_cntr then
							out_hex[ihex + 1] = tostring(
								data.memory.contents:sub(h_cntr, h_cntr))
						else
							out_hex[ihex + 1] = "0"
						end

						h_cntr = h_cntr + 1
					end
					ImGui.Text(table.concat(out_hex))
				end
			end
			ImGui.TableNextRow()
			ImGui.TableSetColumnIndex(1)
			ImGui.Text(data.memory.last)
		end
		ImGui.EndTable()
	end

	ImGui.End()

	------------------------------------------------------------------------
end

return GuiRender
