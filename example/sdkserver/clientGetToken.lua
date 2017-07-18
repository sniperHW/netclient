local Http  = require("lua.http")
local cjson = require("cjson")
local net = require("net")

--test getToken

local client = Http.HttpClient("192.168.1.117",8010)
local http_request = Http.HttpRequest([[/getToken?sign=a2e5cb9674631820e7881f06b47adc0d&extension=&sid=sst1game803225ed07ef42448640b05006fcbb5f127783&appID=1&channelID=10]])
http_request:WriteHead({
	"User-Agent: Dalvik/1.6.0 (Linux; U; Android 4.2.2; HM NOTE 1TD MIUI/JHECNBL41.0)",
	"Connection: Keep-Alive",
	"Accept-Encoding: gzip",
	"Accept-language: en-us",
	"Accept: text/xml,application/xml,application/xhtml+xml,text/html,text/plain,image/png,image/jpeg,image/gif,*/*",	
})

http_request:End(nil)

if not client:Get(http_request,function (result)
	if result then
		--local data = cjson.decode(C.Utf82Gbk(result:GetBody()))
		--print(data.id,data.state.msg)
		print(result:GetBody())
		--print("get response")
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
