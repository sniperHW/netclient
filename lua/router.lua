local router = {
	handler = {}
}

local function parse(req)
	local method = req:GetMethod()
	local urlstr = req:GetUrl()
	local url
	for w in string.gmatch(urlstr, "/%a+") do
		url = w
	end
	if method == "POST" then
		return url,req:GetBody()
	elseif method == "GET" then
		local param = {}
		for k, v in string.gmatch(urlstr, "(%w+)=(%w+)") do
  			param[k] = v
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