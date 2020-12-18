-- Module that communicates w/ Gdb

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
	data.frame_info_txt = tostring(input)
	if type(input) == "string" then
		local _, _, short = input:find("file=\"([%w%s_/%.]*)\"")
		local _, _, full = input:find("fullname=\"([%w%s_/%.]*)\"")
		local _, _, line = input:find("line=\"(%d*)\"")
		--print(string.format("Frame : %s, %s, %s", short, full, tostring(line)))
		if short and full and line then
			GdbData.UpdateFile(data, short, full, line, 0)
		end
	end
end

return GdbData
