local cjson  = require("cjson")
local Http   = require("lua.http")
local Router = require("lua.router")
local net = require("net")
require("sdkserver.getToken")
require("sdkserver.loginServer")

Http.HttpServer("192.168.1.117",8010,function(req,res)
	return Router.Dispatch(req,res)
end)

while true do
	net.Run(50)
	collectgarbage("collect")
end