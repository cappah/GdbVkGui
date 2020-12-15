-- Helper lua functions

local UtilFuncs =
{
	OS_WINDOWS = 1,
	OS_OTHERS  = 2
}

function UtilFuncs:GetOS()
	-- Returns OS ID

	local os_type = package.config:sub(1,1)

	if os_type == '\\' then
		return 1
	else
		return 2
	end
end

function UtilFuncs:ListFilesInDir( root_dir, no_recurse )
		-- Lamba for checking argument
		local SetRootDir = function ( OS )
			if root_dir == "" then
				root_dir = OS == UtilFuncs.OS_WINDOWS and "." or "$PWD"
			end
		end

		local curr_os = self:GetOS()
		SetRootDir( curr_os )

		local files = {}
		if curr_os == self.OS_WINDOWS then
			if no_recurse == true then
				files = io.popen( string.format( "dir /b %s", root_dir ) )
			else
				files = io.popen( string.format( "dir /s /b %s", root_dir ) )
			end
		else
			if no_recurse == true then
				files = io.popen( string.format( "find -maxdepth 0 %s", root_dir) )
			else
				files = io.popen( string.format( "find %s", root_dir) )
			end
		end

		local file_entries = {}
		for file in files:lines() do
			file_entries[#file_entries + 1] = file
		end

		return file_entries
end

function UtilFuncs:ParseArgs()
	local parsed_args = {}

	local last_arg			= " "
	local unclaimed_idx = 0

	for idx = 1, #arg do
		if string.find( arg[idx], "-" ) then
			-- Save argument flags
			last_arg							= arg[idx]
			parsed_args[last_arg] = ""
		else
			-- Save argument parameter
			if not string.match( last_arg, " " ) then
				parsed_args[last_arg] = arg[idx]
			else
				unclaimed_idx							 = unclaimed_idx + 1
				parsed_args[unclaimed_idx] = arg[idx]
			end
		end
	end

	return parsed_args
end

function UtilFuncs:DumpFileToArray( file_path )
	local file = assert(io.open( file_path ), "UtilFuncs:DumpFileContents :: Failed to open " .. file_path )

	local file_contents = {}
	for line in file:lines() do
		file_contents[#file_contents + 1] = line
	end

	file:close()

	return file_contents
end

function UtilFuncs:DumpFileContents( file_path )
	local file = assert(io.open( file_path ), "UtilFuncs:DumpFileContents :: Failed to open " .. file_path )

	local file_contents = {}
	for line in file:lines() do
		file_contents[#file_contents + 1] = line
	end

	file:close()

	return table.concat( file_contents, "\n" )
end

function UtilFuncs:DumpContentsToFile(file_path, contents)
	if self.GetOS() == self.OS_WINDOWS then
		file_path = file_path:gsub("/", "\\")
	else
		file_path = file_path:gsub("\\", "/")
	end

	local file = assert(io.open(file_path, "w"), "UtilFuncs:DumpContentsToFile :: Failed to open "..file_path)
	assert(contents, "UtilFuncs:DumpContentsToFile :: Invalid file contents")

	file:write(contents)

	file:close()
end

function UtilFuncs:ExtractExtension( file )
	return file:match("[^%.]+$")
end

function UtilFuncs:ExtractNameFromPath( file_path )
	if self:GetOS() == self.OS_WINDOWS then
		return file_path:match( "[^\\]+$" ):match( "[^%.]+" )
	else
		return file_path:match( "[^/]+$" ):match( "[^%.]+" )
	end
end

-- From : https://stackoverflow.com/questions/132397/get-back-the-output-of-os-execute-in-lua
-- by : Norman Ramsey
function UtilFuncs:OSCapture(cmd, raw)
  local f = assert(io.popen(cmd, 'r'))
  local s = assert(f:read('*a'))
  f:close()

  if raw then return s end

  s = string.gsub(s, '^%s+', '')
  s = string.gsub(s, '%s+$', '')
  s = string.gsub(s, '[\n\r\t]+', '\n')
  return s
end

function UtilFuncs:SplitString(data, delims)
	local out = {}
	local keep_parsing = true

	local rgx = string.format("[^%s]+", delims)

	repeat
		local var_begin, var_end = data:find(rgx)
		keep_parsing = var_begin ~= nil

		if var_begin then
			local var = data:sub(var_begin, var_end - var_begin + 2)
			data = data:sub(var_end + 2)

			out[#out + 1] = var
		end
	until(keep_parsing == false)

	return out
end

return UtilFuncs

