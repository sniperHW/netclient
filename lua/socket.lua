local net=require("net")
local socket = {}

function socket:new(s)
  local o = {}
  o.__index = socket
  o.__gc    = function (self)
  		self:Close()
  		net.SocketRelease(self.s)
  end      
  setmetatable(o,o)
  o.s = s
  net.SocketRetain(s)
  return o
end

function socket:Send(packet,on_finish)
	if on_finish then
		return net.Send(self.s,packet,function ()
			on_finish(self)
		end)
	else
		return net.Send(self.s,packet)
	end
end

function socket:Close()
	net.Close(self.s)
end


return {
	New = function (s) return socket:new(s) end
}