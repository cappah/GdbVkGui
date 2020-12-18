-- Module that communicates w/ Gdb

local Json = require "JSON"

local GdbData = {}

function GdbData.UpdateFile(data, title, file, line, column)
	if file == data.open_file.full and data.open_file.is_open then
		-- set line number
		SetEditorFileLineNum(tonumber(line), column)
	else
		-- open new file
		if SetEditorFile(file, tonumber(line), column) then
			data.open_file.short = title
			data.open_file.full = file
			data.open_file.is_open = true
		else
			print("Failed to open: "..file)
		end
	end
end

function GdbData.Next(data, input)
	data.output_txt = tostring(input)
	if type(input) == "string" then
		local _, _, reply = input:find("*stopped,reason=(.*)%(")
		if reply then 
			local _, _, short = reply:find("file=\"([%w%s_/%.]*)\"")
			local _, _, full = reply:find("fullname=\"([%w%s_/%.]*)\"")
			local _, _, line = reply:find("line=\"(%d*)\"")
			--print(string.format("Frame : %s, %s, %d", short, full, line))

			GdbData.UpdateFile(data, short, full, line, 0)
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
			--print(string.format("Frame : %s, %s, %s", short, full, tostring(line)))
			if short and full and line then
				GdbData.UpdateFile(data, short, full, line, 0)
			end
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

		data.local_vars_txt = Json:encode_pretty(new_data)

		if data.get_local_types == false then return end

		-- get type of each top level variable
		for _, var in ipairs(new_data.locals) do
			print("Var name : "..var.name)
			if SendToGdb("whatis "..var.name) then
				local t = ReadFromGdb()
				local _, _, vtype = t:find("type = ([%w%s_%*]*)\\")
				var.vtype = vtype
			end
		end
	end
end

return GdbData
