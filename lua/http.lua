local net=require("net")
local socket = require "lua.socket"

local http_response = {}

function http_response:new()
  local o = {}
  o.__index = http_response
  o.headers = {}      
  setmetatable(o,o)
  return o
end

function http_response:buildResponse()
	local strResponse  = string.format("HTTP/1.1 %d %s\r\n",self.status,self.phase)
	for k,v in pairs(self.headers) do
		strResponse = strResponse .. string.format("%s\r\n",v)
	end	
	if self.body then
		strResponse = strResponse .. string.format("Content-Length: %d \r\n\r\n%s",#self.body,self.body)
	else
		strResponse = strResponse .. "\r\n\r\n"
	end
	return strResponse
end

function http_response:WriteHead(status,phase,heads)
	self.status = status
	self.phase = phase
	self.headers = self.headers or {}
	if heads then
		for k,v in pairs(heads) do
			table.insert(self.headers,v)
		end
	end
end

function http_response:End(body)
	self.body = body
	self.connection:Send(net.NewRawPacket(self:buildResponse()))
end


local http_request = {}

function http_request:new(path)
  local o = {}
  o.__index = http_request      
  setmetatable(o,o)
  o.path = path
  o.headers = {} 
  return o
end

function http_request:WriteHead(heads)
	self.headers = self.headers or {}
	if heads then
		for k,v in pairs(heads) do
			table.insert(self.headers,v)
		end
	end
end

function http_request:End(body)
	self.body = body
end


local http_server = {}

function http_server:new()
  local o = {}
  o.__index = http_server      
  setmetatable(o,o)
  return o
end

function http_server:CreateServer(ip,port,on_request)
	self.socket = net.Listen(ip,port,function (s)
		local connection = socket.New(s)
		net.Bind(s,net.HttpDecoder(65535),function (_,rpk)
			local response = http_response:new()
			response.connection = connection
			if on_request(rpk,response) then
				connection:Close()
			end
		end,
		function (_)
			connection = nil
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


local httpclient = {}

function httpclient:new(host,port)
  local o = {}
  o.__index = httpclient      
  setmetatable(o,o)
  o.host    = host
  o.port    = port or 80
  return o
end

function httpclient:buildRequest(request)
	local strRequest = string.format("%s %s HTTP/1.1\r\n",request.method,request.path)
	strRequest = strRequest .. string.format("Host: %s \r\n",self.host)
	for k,v in pairs(request.headers) do
		strRequest = strRequest .. string.format("%s\r\n",v)
	end	
	if request.body then
		strRequest = strRequest .. string.format("Content-Length: %d \r\n\r\n%s",#request.body,request.body)
	else
		strRequest = strRequest .. "\r\n"
	end
	return strRequest
end


function httpclient:request(method,request,on_result)
	print(self.host,self.port)
	if net.Connect(self.host,self.port,function (s,success)
			if success then
				print("connect success") 
				local connection = socket.New(s)
				net.Bind(s,net.HttpDecoder(65535*2),function (_,rpk)
					on_result(rpk)
					on_result = nil
					if connection then
						connection:Close()
					end
				end,
				function (_)
					if on_result then
						on_result(nil)
					end
					connection = nil
				end)
				request.method = method
				connection:Send(net.NewRawPacket(self:buildRequest(request)))
			else
				on_result(nil)
				print("connect failed")
			end
		end) then
		return true
	else
		return false
	end	
end

function httpclient:Post(request,on_result)
	return self:request("POST",request,on_result)
end

function httpclient:Get(request,on_result)
	return self:request("GET",request,on_result)
end

local function HttpClient(host,port)
	return httpclient:new(host,port)
end

local function HttpRequest(path)
	return http_request:new(path)
end 


return {
	HttpServer  = HttpServer,
	HttpClient  = HttpClient,
	HttpRequest = HttpRequest,
}
