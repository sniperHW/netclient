local router = {
	handler = {}
}

local function SplitString(s,separator)
	local ret = {}
	local initidx = 1
	local spidx
	while true do
		spidx = string.find(s,separator,initidx)
		if not spidx then
			break
		end
		table.insert(ret,string. sub(s,initidx,spidx-1))
		initidx = spidx + 1
	end
	if initidx <= string.len(s) then
		table.insert(ret,string. sub(s,initidx))
	end
	return ret
end

local function parse(req)
	local method = req:GetMethod()
	local urlstr = req:GetUrl()
	local url    = "/"
	local query_str  = urlstr
	
	local idx1 = string.find(urlstr,"/",1)
	if idx1 then
		local idx2 = string.find(urlstr,"?",idx1)
		if idx2 then 
			query_str = string.sub(urlstr,idx2 + 1) 
			idx2 = idx2 - 1
		end
		url = string.sub(urlstr,idx1,idx2)
	end	

	if method == "POST" then
		return url,req:GetBody()
	elseif method == "GET" then
		local param_strs = SplitString(query_str,"&")
		local param = {}
		for k1,v1 in pairs(param_strs) do
			for k2, v2 in string.gmatch(v1, "(%g+)=(%g+)") do
  				param[k2] = v2
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
			return handler(req,res,param)
		else
			return "error"
		end
	else
		return "error"
	end
end

function router.RegHandler(url,handler)
	router.handler[url] = handler
end

return router
