local http_response = {}

function http_response:new()
  local o = {}
  self.__index = self      
  setmetatable(o,self)
  return o
end

function http_response:Send(connection)
	local s  = string.format("HTTP/1.1 %d %s\r\nConnection: Keep-Alive\r\n",self.status,self.phase)	
	table.insert(self.headers,"Date: Tue, 26 Aug 2014 11:40:38 GMT")	
	for k,v in pairs(self.headers) do
		s = s .. string.format("%s\r\n",v)
	end	
	if self.body then
		s = s ..  "Transfer-Encoding: chunked\r\n\r\nc\r\n"
		s = s .. string.format("%s\r\n0\r\n",self.body)
	end
	s = s .. "\r\n"
	C.Send(connection,C.NewRawPacket(s))	
end

function http_response:WriteHead(status,phase,contents)
	self.status = status
	self.phase = phase
	self.headers = self.headers or {}
	if contents then
		for k,v in pairs(contents) do
			table.insert(self.headers,v)
		end
	end
end

function http_response:End(body)
	self.body = body
end

local http_server = {}

function http_server:new()
  local o = {}
  self.__index = self      
  setmetatable(o,self)
  return o
end

function http_server:CreateServer(ip,port,on_request)
	self.socket = C.Listen(ip,port,function (s)
		C.Bind(s,C.HttpDecoder(4096),function (s,rpk)
			local response = http_response:new()
			on_request(rpk,response)
			response:Send(s)
		end)
	end)
	if self.socket then
		return self
	else
		return nil
	end
end

local function HttpServer(ip,port,on_request)
	return http_server:new():CreateServer(ip,port,on_request)
end

return {
	HttpServer = HttpServer
}
