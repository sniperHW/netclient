local Http = require("lua.http")

Http.HttpServer("127.0.0.1",8010,function(req,res)
	for k,v in pairs(req:GetHeaders()) do
		print(k,v)
	end
	res:WriteHead(200,"OK", {"Content-Type: text/plain"})
  	res:End("Hello World\n")
end)

while true do
	C.Run(50)
	collectgarbage("collect")
end
