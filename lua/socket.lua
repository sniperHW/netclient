local socket = {}

function socket:new(s)
  local o = {}
  o.__index = socket
  o.__gc    = function (self)
  		self:Close()
  		C.SocketRelease(self.s)
  end      
  setmetatable(o,o)
  o.s = s
  C.SocketRetain(s)
  return o
end

function socket:Send(packet,on_finish)
	if on_finish then
		return C.Send(self.s,packet,function ()
			on_finish(self)
		end)
	else
		return C.Send(self.s,packet)
	end
end

function socket:Close()
	C.Close(self.s)
end


return {
	New = function (s) return socket:new(s) end
}