local net=require("net")

local recvcount = 0

net.Listen("127.0.0.1",9010,function (s)
	print("new client",s)
	net.Bind(s,net.PacketDecoder(),function (s,rpk)
		print("recv packet",rpk:ReadStr(),recvcount)
		recvcount = recvcount + 1;
		net.Send(s,net.NewWPacket(rpk))
	end)
end)

while true do
	net.Run(50)
end
