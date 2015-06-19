local Util = require("lua.util")

local router = {
	handler = {}
}


--[[/getToken?
sign=a2e5cb9674631820e7881f06b47adc0d
&extension=&
sid=sst1game803225ed07ef42448640b05006fcbb5f127783
&appID=1
&channelID=10
]]--

local function parse(req)
	local method = req:GetMethod()
	local tmp1 = Util.SplitString(req:GetUrl(),"?")
	if #tmp1 == 0 then
		return
	end
	local url    = tmp1[1]
	if method == "POST" then
		return url,req:GetBody()
	elseif method == "GET" then
		local param = {}
		if tmp1[2] then
			tmp1 = SplitString(tmp1[2],"&")
			for k,v in pairs(tmp1) do
				local tmp2 = SplitString(v,"=")
				if #tmp2 == 2 then
					param[tmp2[1]] = tmp2[2]
				end
			end
		end
		return url,param
	else
		return
	end
end


function router.Dispatch(req,res)
	local url,param = parse(req)
	if url then
		local handler = router.handler[url]
		if handler then
			return not handler(req,res,param)
		else
			return false
		end
	else
		return false
	end
end

function router.RegHandler(url,handler)
	router.handler[url] = handler
end

return router