local Router = require("lua.router")
local cjson  = require("cjson")

Router.RegHandler("/loginServer",function (req,res,param)
	local params = {}
	for k, v in string.gmatch(param, "(%w+)=(%w+)") do
  		params[k] = v
	end	

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