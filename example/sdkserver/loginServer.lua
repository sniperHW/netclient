local Router = require("lua.router")
local cjson  = require("cjson")
local Util = require("lua.util")

local function parse(param)
	local tmp1 = Util.SplitString(param,"&")
	param = {}
	for k,v in pairs(tmp1) do
		local tmp2 = Util.SplitString(v,"=")
		if #tmp2 == 2 then
			param[tmp2[1]] = tmp2[2]
		end
	end
	return param
end

Router.RegHandler("/loginServer",function (req,res,param)
	param = parse(param)
	local result = {
		state = 0,
		data = {
			accountName = "pvp1",
			accountID = "12345",
		}
	}
	res:WriteHead(200,"OK", {"Content-Type: text/plain"})
  	res:End(cjson.encode(result))

end)