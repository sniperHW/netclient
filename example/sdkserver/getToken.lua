local Router = require("lua.router")
local cjson  = require("cjson")

Router.RegHandler("/getToken",function (req,res,param)
	local result = {
		state = 1,
		data = {
			userID = 11111,
			sdkUserID = 222222,
			token = 333333,
			extension = 444444,
		}
	}
	res:WriteHead(200,"OK", {"Content-Type: text/plain"})
  	res:End(cjson.encode(result))
end)