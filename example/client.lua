local net=require("net")
net.Connect("127.0.0.1",9010,function (s,success)
	if success then 
		net.Bind(s,net.PacketDecoder(),function (s,rpk)
			print("recv packet",rpk:ReadStr())
			net.Send(s,net.NewWPacket(rpk))
		end)
		local wpk = net.NewWPacket()
		wpk:WriteStr("hello")
		net.Send(s,wpk)
	end
end)

while true do
	net.Run(50)
	collectgarbage("collect")
end