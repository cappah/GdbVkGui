#!/usr/bin/env lua

--[[
  Clang-format script
]]
local UtilFuncs = require "scripts.UtilFuncs"

local style = {
"BasedOnStyle: mozilla",
"IndentWidth: 4", 
"AlignConsecutiveAssignments: true", 
"AlignConsecutiveDeclarations: true",
}
style = "\"{"..table.concat( style, ", " ).."}\""

local folder_list = {
  "src",
}

if UtilFuncs:GetOS() == UtilFuncs.OS_WINDOWS then
  for idx,folder in ipairs(folder_list) do
    folder_list[idx] = folder:gsub("/", "\\")
  end
end

local file_list = {}
for idx,folder in ipairs(folder_list) do
  file_list[#file_list + 1] = UtilFuncs:ListFilesInDir(folder)
end

local files = {}
for idx,folder in ipairs(file_list) do
  for _,file in ipairs(folder) do
    if file:find("%.[hc].*") and not file:find("%.config") then
      files[#files + 1] = file
    end
  end
end

for _,file in ipairs(files) do
  os.execute( string.format( "clang-format \"%s\" -i -style=%s", file, style ) )
end
