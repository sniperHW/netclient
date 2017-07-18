local Http  = require("lua.http")
local cjson = require("cjson")
local net = require("net")

--test getToken

local client = Http.HttpClient("192.168.1.117",8010)
local http_request = Http.HttpRequest("/loginServer")
http_request:WriteHead({
	"User-Agent: Dalvik/1.6.0 (Linux; U; Android 4.2.2; HM NOTE 1TD MIUI/JHECNBL41.0)",
	"Connection: Keep-Alive",
	"Accept-Encoding: gzip",
	"Accept-language: en-us",
	"Accept: text/xml,application/xml,application/xhtml+xml,text/html,text/plain,image/png,image/jpeg,image/gif,*/*",	
})

http_request:End([[sign=00661e98514100347e01e724d2db9cd9&token=333333&userID=11111param]])

if not client:Post(http_request,function (result)
	if result then
		print(result:GetBody())
	else
		print("get request error")
	end
end) then
	print("get failed")
end


while true do
	net.Run(50)
	collectgarbage("collect")
end
