-- Module that communicates w/ Gdb

local Json = require "JSON"

local GdbData = {}

local ExecuteCmd = function(cmd) if SendToGdb(cmd) then return ReadFromGdb() end end

-- TODO : maybe create parser function to load() that appends '_' to lua lang keywords

function GdbData.LoadExe(data)
	ExecuteCmd("-file-exec-and-symbols "..data.user_args.ExeStart.exe)

	if data.user_args.ExeStart.args ~= "" then
		ExecuteCmd("-exec-arguments "..data.user_args.ExeStart.args)
	end

	ExecuteCmd("-break-insert -t main")
end

function GdbData.LoadSettings(data)
	if data.user_args.Breaks then
		for _, brk_pt in ipairs(data.user_args.Breaks) do
			ExecuteCmd(string.format("-break-insert%s --source %s --line %s",
				brk_pt.disp == "del" and " -t" or "",
				brk_pt.fullname,
				brk_pt.line
			))
		end
	end
	if data.user_args.Watch then
		for i, watch_data in ipairs(data.user_args.Watch) do
			watch_data.value = ""
		end
	end
	GdbData.ParseBreakpoints(data, ExecuteCmd("-break-list"))
end

function GdbData.ShowBreaks(data)
	if data.user_args.Breaks then
		local markers = {}
		for _, brk_pt in ipairs(data.user_args.Breaks) do
			if data.open_file.full == brk_pt.fullname and brk_pt.line then
				markers[#markers + 1] = tonumber(brk_pt.line)
			end
		end
		SetEditorBkPts(#markers, markers)
	end
end

function GdbData.UpdateFile(data, title, file, line, column, func)
	if file == data.open_file.full and data.open_file.is_open then
		-- set line number
		SetEditorFileLineNum(tonumber(line), column)

		GdbData.ShowBreaks(data)
	elseif file then
		-- open new file
		if SetEditorFile(file, tonumber(line), column) then
			data.open_file.short = title
			data.open_file.full = file
			data.open_file.is_open = true
			
			GdbData.ShowBreaks(data)
		else
			print("Failed to open: "..file)
		end
	end
	data.open_file.func = func
end

function GdbData.Next(data, input)
	data.output_txt = tostring(input)
	if type(input) == "string" then
		local _, _, reply = input:find("*stopped,reason=(.*)%(")
		if reply then 
			local _, _, short = reply:find("file=\"([%w%s_/%.]*)\"")
			local _, _, full = reply:find("fullname=\"([%w%s_/%.]*)\"")
			local _, _, line = reply:find("line=\"(%d*)\"")
			local _, _, func = reply:find("func=\"([%w%s_]*)\"")
			--print(string.format("Frame : %s, %s, %d", short, full, line))

			GdbData.UpdateFile(data, short, full, line, 0, func)
		end
	end
end

function GdbData.ParseBreakpoints(data, input)
	if type(input) == "string" then
		data.user_args.Breaks = {}

		local _, _, bkpts = input:find("body=(.*)")
		if bkpts then
			for match in bkpts:gmatch("({[^{}]*})") do
				match = match:gsub("type=", "btype=")
				match = match:gsub("thread%-groups=%[([%w\"]*)%]", "thread_g={%1}")
				match = match:gsub("original%-location=", "org_loc=")

				local bkpt_chunk = load("return "..match)
				if bkpt_chunk then
					local new_idx = #data.user_args.Breaks + 1
					data.user_args.Breaks[new_idx] = bkpt_chunk()

					if data.user_args.Breaks[new_idx].cond == nil then
						data.user_args.Breaks[new_idx].cond = ""
					end
				end
				--print(match)
			end
		end
	end
	GdbData.ShowBreaks(data)
end

function GdbData.UpdateBreakpoint(data, input)
	if type(input) == "string" then
		local _, _, bkpt = input:find("bkpt={(.*)}")
		if bkpt then 
			local _, _, short = bkpt:find("file=\"([%w%s_/%.]*)\"")
			local _, _, full = bkpt:find("fullname=\"([%w%s_/%.,]*)\"")
			local _, _, line = bkpt:find("line=\"(%d*)\"")
			local _, _, func = bkpt:find("func=\"([%w%s_]*)\"")
			if short and full and line then
				GdbData.UpdateFile(data, short, full, line, 0, func)
			end
		end
	end
end

function GdbData.UpdateFramePos(data, input)
	if type(input) == "string" then
		local _, _, frame = input:find("frame=(.*)")
		if frame then 
			data.frame_info_txt = tostring(frame)

			local _, _, short = frame:find("file=\"([%w%s_/%.]*)\"")
			local _, _, full = frame:find("fullname=\"([%w%s_/%.,]*)\"")
			local _, _, line = frame:find("line=\"(%d*)\"")
			local _, _, func = frame:find("func=\"([%w%s_]*)\"")
			--print(string.format("Frame : %s, %s, %s", short, full, tostring(line)))
			if short and full and line then
				GdbData.UpdateFile(data, short, full, line, 0, func)
			end
		end
	end
end

function GdbData.UpdateWatchExpr(data, input)
-- ^done,value="\"string_daslkda\", '\\000' <repeats 206 times>"

	local _, _, watch_val = input:find("value=\"(.*)\"")
	return watch_val and watch_val or ""
end

function GdbData.UpdateMemory(data, input)
-- memory=[{begin="0x0000555555648981",offset="0x0000000000000000",end="0x000055555564898b",contents="f30f1efa554889e55348"}]

	local _, _, mem = input:find("memory=%[(.*)%]")
	if mem then 
		mem = mem:gsub("end", "last")
		data.memory = {}
		local mem_chunk = load("return "..mem)
		if mem_chunk then
			data.memory = mem_chunk()
		end
	end
end

function GdbData.UpdateAsm(data, input)
--{address="0x0000555555648963",func-name="ImVector<ImGuiTabBar>::_grow_capacity(int) const",offset="49",inst="add    %edx,%eax"},

	local _, _, asm_sns = input:find("asm_insns=%[(.*)%]")
	if asm_sns then 
		data.asm = {}
		for match in asm_sns:gmatch("({[^{}]*})") do
			match = match:gsub("func%-name", "func")
			local asm_chunk = load("return "..match)
			if asm_chunk then
				data.asm[#data.asm + 1] = asm_chunk()
			end
		end
	end
end

function GdbData.UpdateBacktrace(data, input)
--{level="0",addr="0x000055555564932a",func="CommonStartupInit",file="src/System/main.cpp",fullname="/home/maadeagbo/Code/VulkanDemoScene/src/System/main.cpp",line="287",arch="i386:x86-64"}
	local _, _, stack = input:find("stack=%[(.*)%]")
	if stack then 
		data.bktrace = {}
		for match in stack:gmatch("frame=({[^{}]*})") do
			--print(match)
			local bktr_chunk = load("return "..match)
			if bktr_chunk then
				data.bktrace[#data.bktrace + 1] = bktr_chunk()
			end
		end
	end
end

function GdbData.GetTrackedRegisters(data)
	-- currently hardcode most used
	-- TODO : allow naming/modifying list
	data.registers = {
		rax = { number = -1, id = "rax", value = "" },
		rbx = { number = -1, id = "rbx", value = "" },
		rcx = { number = -1, id = "rcx", value = "" },
		rdx = { number = -1, id = "rdx", value = "" },
		rsi = { number = -1, id = "rsi", value = "" },
		rdi = { number = -1, id = "rdi", value = "" },
		rbp = { number = -1, id = "rbp", value = "" },
		rsp = { number = -1, id = "rsp", value = "" },
		eax = { number = -1, id = "eax", value = "" },
		ebx = { number = -1, id = "ebx", value = "" },
		ecx = { number = -1, id = "ecx", value = "" },
		edx = { number = -1, id = "edx", value = "" },
		esi = { number = -1, id = "esi", value = "" },
		edi = { number = -1, id = "edi", value = "" },
		ebp = { number = -1, id = "ebp", value = "" },
		esp = { number = -1, id = "esp", value = "" },
	}
end

function GdbData.SetupRegisterDataCmd(data, cmd_data)
	local cmd = { "-data-list-register-values x", }

	if data.registers then
		for name, val in pairs(data.registers) do
			if val.number >= 0 then cmd[#cmd + 1] = " "..tostring(val.number) end
		end
	end

	return cmd
end

function GdbData.SetTrackedRegisters(data, input)
	GdbData.GetTrackedRegisters(data)

	local _, _, regs = input:find("register%-names=%[(.*)%]")
	if regs then
		local regs_chunk = load("return {"..regs.."}")
		if regs_chunk then
			local found_regs = regs_chunk()
			for idx, val in ipairs(found_regs) do
				if data.registers[val] then 
					data.registers[val].number = idx - 1;
					--print(string.format("Reg: %s = %d", val, idx - 1))
				end
			end
		end
	end
end

function GdbData.UpdateRegisters(data, input)
	-- register-values=[{number="195",value="0xffffde68"},{number="198",value="0xffffdd78"},
	local _, _, regs_vals = input:find("register%-values=%[(.*)%]")
	if regs_vals then
		local regv_chunk = load("return {"..regs_vals.."}")
		if regv_chunk then
			local regv = regv_chunk()
			for _, reg in pairs(data.registers) do
				for _, regv_i in ipairs(regv) do
					reg.value = reg.number == tonumber(regv_i.number) and regv_i.value or reg.value 
					--if reg.value ~= "" then print(reg.id.." : "..reg.value) end
				end
			end
		end
	end
end

function GdbData.GetVCard(local_vars)
	for _, var in ipairs(local_vars) do
		if SendToGdb("whatis "..var.name) then
			local t = ReadFromGdb()
			local _, _, vtype = t:find("type = ([^\\]*)\\")
			var.vtype = vtype
		end
	end
end

function GdbData.UpdateLocals(data, input)
	for locals in input:gmatch("locals=%[(.*)%]") do

		-- interpret as json array and transform into lua table
		locals = locals:gsub("{name=", "{\"name\":")
		locals = locals:gsub(",value=", ",\"value\":")
		local new_data = "{\"locals\":["..locals.."]}"
		new_data = Json:decode(new_data)
		data.local_vars = new_data.locals

		for _, var in ipairs(new_data.locals) do
			if var.value:find("^{") then
				-- sanitize long junk strings
				var.value = var.value:gsub("\", [\\'%d]* <[%w%s]*>, \"", "")
				--var.value = var.value:gsub("(\"[\\%w]*\")..., ", "%1, ")
			end
			var.vtype = ""
		end

		--data.local_vars_txt = Json:encode_pretty(new_data.locals)

		if data.user_args.FetchTypes then GdbData.GetVCard(data.local_vars) end
	end
end

function GdbData.ParseDataInput(data, cmd_data)
	local complete_cmd = {}

	local user_idx = 1
	for i, val in ipairs(cmd_data.args) do
		complete_cmd[i] = val

		if val:find("@") then 
			complete_cmd[i] = val:gsub("@%w*", data.user_args[cmd_data.id][user_idx].val)
			user_idx = user_idx + 1
		end
	end

	return complete_cmd
end

return GdbData
