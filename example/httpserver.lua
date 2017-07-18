local net=require("net")
local Http = require("lua.http")

Http.HttpServer("127.0.0.1",9010,function(req,res)
	for k,v in pairs(req:GetHeaders()) do
		print(k,v)
	end
	res:WriteHead(200,"OK", {"Content-Type: text/plain"})
  	res:End("Hello World\n")
end)

while true do
	net.Run(50)
	collectgarbage("collect")
end
