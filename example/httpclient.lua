local net=require("net")
local Http = require("lua.http")

local client = Http.HttpClient("sdk.test4.g.uc.cn")
local http_request = Http.HttpRequest("/ss")
http_request:WriteHead({
	"Content-Type: application/x-www-form-urlencoded",
	"Accept-language: en-us",
	"Accept: text/xml,application/xml,application/xhtml+xml,text/html,text/plain,image/png,image/jpeg,image/gif,*/*",
})
http_request:End([[{"id":1434594091614,"service":"ucid.bind.create","game":{"gameId":123456},"data":{"gameUser":"user123456"},"encrypt":"md5","sign":"2b2b75609346de77362cb5005eaa7a11"}]])

if not client:Post(http_request,function (result)
	if result then
		for k,v in pairs(result:GetHeaders()) do
			print(k,v)
		end
		print("recv result",result:GetBody())
	else
		print("post request error")
	end
end) then
	print("post failed")
end


while true do
	net.Run(50)
	collectgarbage("collect")
end
